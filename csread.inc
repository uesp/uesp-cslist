<?php
require_once 'common.inc';

// No longer requires csdata.inc!!

/**
 * Objective is to have a base class for reading any ES file
 * Should be able to handle a full spectrum of complexness:
 * * At simplest, should be able (on its own) to scan an unknown file and provide functions to start
 *   dissecting the file format
 * * Functions to create a compressed version of a .esm file by removing unwanted records
 * * With full or partial data definitions (i.e., via OBRead) do more thorough reading of file contents
 * * Be able to read a single esm/esp file, or to read a set of esm/esp files
 * * In context such as OBSaveRead be able to read files that don't comply with the standard ES record
 *   format
 * * If CSData is also loaded, put the data that has been read into the CSData structure
 * * Be able to interact with a database (transfer file contents into DB) depending upon the datamode

 * Note that if csdata.inc is not loaded, then it's assumed that code is operating in DATAMODE_READ
 
 * Reality is that this class was originally written to only function in coordination with CSData,
 * and since then has been tweaked to add non-CSData features.  Lots of features haven't been tested
 * using latest code modifications.

 * Database notes
 * Originally code was setup to wipe out entire database every time any data was re-read.
 * Have now added the capability to just re-read one record type, so that rectypes can be updated as
 *   Skyrim decoding is improved.
 * Nevertheless, first time any file is read, want to always fill the AllItems table with all entries in the
 * file -- so that all formid lookups can be done -> and that includes even records without a datadef entry
 * When re-reading one rectype, steps taken are:
 *  * wipe out all the database tables specific to that rectype -- but not the rest of the tables
 *  * check the AllItems entries have not changed -- update as necessary
 *  * refill the re-created database tables
**/

// Usage notes ... things I didn't realize at first after coming back to the code after 
// a long period (and trying to adapt the code to reading save files instead of mod files)
// * _parentlist is a csdata object, which is used to handle all of the data that's read
// * any data that is read is saved to _parentlist's active csitem
// * the various set_active calls don't try to add the info to the database ... 
//   writing to database only happens when active->write_db() is called, which only
//   can happen if datamode is DATAMODE_WRITE
// * inactive() triggers write_db() (but again, only in DATAMODE_WRITE)
// * any valname can be saved to the active item -- and individual array values can also be saved
//   (but only one-level deep with arrays ... anything more complex has to be passed as an entire
//    array)
// * when not in DATAMODE_WRITE, _parentlist will store in memory all items that are created via
//   csread

// Database issues:
// When reading REFR/ACHR/ACRE records (including during initial read of entire file), need to at
//   least be reading NAME subrecord, too -> without base_id REFRs are essentially useless
// Have opted to not create tables such as TREE_Record
// -> If I'm skipping any meaningful entries about the items, I'll have to rescan the items to use them anyway
// -> Also, new reading options allow a single record to be re-scanned as necessary

// Need to add capability to index certain entries in tables
// But should this be done as an after-the-fact tweak?  (faster to index large table such as REFR_Record all at once
// instead of incrementally while reading)
// (Note that indexing ordid during initial read is necessary for sake of ordid-lookups also being done
//  during read)

// Fix formid lookup to not create 0x00000000 entry -> just set corresponding item to NULL

// Should I have a function to take care of adding mod_status to existing tables?

// Current tasks:
// add mod_status -> set up flag based on mod
//  need to run create_mod_status on content3
//  need to test what happens when reading data

// make sure new patches can be read in
//   whenever a new file is in list, don't delete existing DB tables
//   error/die/ignore if requesting a specific record be re-processed
//   only read new file instead of re-reading all the old files


// SR stats:
// 0/0 = 242
// 0/7 = 839038
// 1/0 = 33
// 2/6 = 242
// 2/7 = 18

// when re-reading -- double check whether bad entries in AllItems are being recreated (formid=NULL, rectype=NULL)

// get cslist.php fileid to display filename
// get mod_status to display meaningful info
// get other instances of a formid listed on page
// add asterisk or some other symbol anytime a formid listed on a cslist page has modified versions
// get out-of-date versions of formid to display comparison of modified data
// get some of way of listing what specific records have been altered in each patch

class CSRead {
	public $keep = array();
	
	protected $_params = array(
		'doprint' => FALSE, // can also be set as 0/1/2/3/4 to control extent of printing
		'keepall' => FALSE,
		'scanrecs' => FALSE,
		'skip_compressed' => FALSE,
		'calc_levlists' => NULL, // NULL means to auto-set value
		'redo_cells' => FALSE,
		'setup_cells' => FALSE,
		'inputdir' => '../cs_input/',
		'redo_db' => FALSE,
	);
	protected $_parentlist;
	protected $_datadef = NULL;
	protected $_datadef_alias = array();
	protected $_keep_default;
	protected $_fpin;
	protected $_fpsize;
	protected $_datachunk = NULL;
	protected $_stringindex = NULL;
	
	protected $_datatype = array(
		'raw' => array('len' => 1), // unprocessed raw data (may be a string)
		'short' => array( 'code' => "s", 'len' => 2, 'sql' => 'smallint'),
		'ushort' => array( 'code' => "S", 'len' => 2, 'sql' => 'smallint UNSIGNED' ),
		'long' => array( 'code' => "l", 'len' => 4, 'sql' => 'int'),
		'ulong' => array( 'code' => "L", 'len' => 4, 'sql' => 'int UNSIGNED' ),
		'byte' => array( 'code' => "c", 'len' => 1, 'sql' => 'tinyint'),
		'ubyte' => array( 'code' => "C", 'len' => 1, 'sql' => 'tinyint UNSIGNED' ),
		'float' => array( 'sub' => "process_float", 'len' => 4, 'sql' => 'float' ),
		'double' => array( 'code' => 'd', 'len' => 8 ),
		'string' => array( 'sub' => "process_string", 'sql' => 'varchar(255)' ),
	   // string prefixed with 1-byte length
		'bstring' => array( 'sub' => "process_string", 'sql' => 'varchar(255)' ),
	   // string prefixed with 2-byte length
		'wstring' => array( 'sub' => "process_string", 'sql' => 'varchar(255)' ),
	   // string terminated with \0
		'zstring' => array( 'sub' => "process_string", 'sql' => 'varchar(255)' ),
		// bytes takes arbitrary number of bytes and creates a hex'd string
		'bytes' => array( 'sub' => "process_bytes" ),
		'blob' => array( 'sub' => "process_string", 'sql' => 'blob' ),
		'edid' => array('sub' => "process_edid", 'sql' => 'int' ),
		'infoid' => array('sub' => "process_infoid", 'sql' => 'varchar(255)' ),
		'dialid' => array('sub' => "process_dialid", 'sql' => 'varchar(255)' ),
		'formid' => array( 'sub' => "process_formid", 'len' => 4, 'sql' => 'int'),
		'raw_formid' => array( 'sub' => "process_raw_formid", 'len' => 4),
		'mgefid' => array( 'sub' => "process_mgefid", 'len' => 2, 'sql' => 'int' ),
		'mgefidl' => array( 'sub' => "process_mgefid", 'len' => 4, 'sql' => 'int' ),
		'mgefstr' => array( 'sub' => "process_mgefstr", 'len' => 4, 'sql' => 'int'),
		'boolean' => array( 'sub' => "process_boolean", 'len' => 1, 'sql' => 'boolean' ),
		'boolean2' => array( 'sub' => "process_boolean", 'len' => 2, 'sql' => 'boolean' ),
		'boolean4' => array( 'sub' => "process_boolean", 'len' => 4, 'sql' => 'boolean' ),
		'rev_boolean' => array( 'sub' => "process_rev_boolean", 'len' => 1, 'sql' => 'boolean' ),
		'rev_boolean2' => array( 'sub' => "process_rev_boolean", 'len' => 2, 'sql' => 'boolean' ),
		'rev_boolean4' => array( 'sub' => "process_rev_boolean", 'len' => 4, 'sql' => 'boolean' ),
		'gmst' => array( 'sub' => 'process_gmst', 'sql' => 'varchar(255)'),
		'gmst4' => array( 'sub' => 'process_gmst4', 'len' => 4, 'sql' => 'varchar(255)'),
		'glob' => array( 'sub' => "process_glob", 'len' => 4, 'sql' => 'float'),
		'lvld' => array( 'sub' => "process_lvld", 'len' => 1, 'sql' => 'tinyint UNSIGNED' ),
		'systemtime' => array( 'sub' => "process_systemtime", 'len' => 16 ),
		// special processing for PLDT/PTDT subrecord contents in PACK records (can be formids OR numbers)
		// save as ordid or *negative* of number
		'packid' => array( 'sub' => 'process_packid', 'len' => 4, 'sql' => 'int'),
		'packid_PLDT' => array( 'sub' => 'process_packid_PLDT', 'len' => 8, 'sql' => 'int'),
		'packid_PTDT' => array( 'sub' => 'process_packid_PTDT', 'len' => 8, 'sql' => 'int'),
		'packid_PTDA' => array( 'sub' => 'process_packid_PTDA', 'len' => 8, 'sql' => 'int'),
		'packid_PDTO' => array( 'sub' => 'process_packid_PDTO', 'len' => 8, 'sql' => 'char(4)'),
		'PACK_UNAM' => array( 'sub' => 'process_PACK_UNAM', 'sql' => 'tinyint'),
		'PACK_CITC' => array( 'sub' => 'process_PACK_CITC', 'sql' => 'long'),
		'PACK_CNAM' => array( 'sub' => 'process_PACK_CNAM', 'sql' => 'float'), // variable length: 1-4 bytes; not setting len says to just grab everything
		// Skyrim string handling
		'str_index' => array('sub' => 'process_str_index', 'len' => 4, 'sql' => 'varchar(255)'),
		'blob_index' => array('sub' => 'process_str_index', 'len' => 4, 'sql' => 'blob'),
		// VMAD's int storage is actually used to save the version of the VMAD entry
		// All other values are stored in artificially created extra fields
		'vmad' => array('sub' => 'process_vmad', 'sql' => 'int'),
	);
	protected $_header_group = array(
		'type' => 4,
		'size' => 'ulong',
		'label' => 4, // L, SS, formid
		'grtypenum' => 'ulong',
		'stamp' => 4, // date stamp?
		// records
	);
	// MW-specific values for header_rec and header_sub are in mwread.inc
	protected $_header_rec = array(
		'type' => 4,
		'size' => 'ulong',
		'flags' => 'ulong',
		'formid' => 'raw_formid',
		'flags_b' => 4,
		// subrecords
	);
	protected $_header_sub = array(
		'type' => 4,
		'datasize' => 'ushort',
		'data' => NULL,
	);
	
	protected $_filemodnum = NULL;
	protected $_fileid = NULL;
	protected $_files = array();
	protected $_filenamespace = array();
	protected $_filemodname = array();
	protected $_filemodabbrev = array();
	protected $_fileofficial = array();
	protected $_modids;
	
	protected $_subset;
	protected $_recgroup = array();
	protected $_reclist = array();
	protected $_reclens = array();
	protected $_reccount = array();
	protected $_reccurr = array();
	protected $_recstats = array();
	
	protected $_xxxxsize = NULL;
	protected $_grouptype = NULL;
	protected $_ngroup = 0;
	protected $_groupid = NULL;
	protected $_groupordid = NULL;
	
	protected $_allitemvals;
	
	const KEEPOPT_ALL = 1;
	const KEEPOPT_ENCH = 2;
	
	const SRCHOPT_GUARANTEE = 1;
	
	// __construct originally did datadef_setup, datadef_check
	function __construct($parentlist=NULL) {
		$this->keep = array();
		$this->_keep_default = array();
		
		$this->_general_setup();
		
		$this->init_csdata($parentlist, FALSE);
	}	
	
	function init_csdata($parentlist=NULL, $autoset=TRUE) {
		if (is_null($parentlist)) {
			if (empty($this->_parentlist)) {
				if ($autoset) {
					$classname = substr(get_class($this),0,2)."Data";
					$this->_parentlist = new $classname();
				}
				else {
					$this->_parentlist = NULL;
				}
			}
			else {
				return;
			}
		}
		elseif (is_object($parentlist)) {
			$this->_parentlist = $parentlist;
		}
		else
			$this->_parentlist = new $parentlist();
		
		$this->_parentlist->set_connection('csread_conn.inc');
		// Want to setup datadef as soon as parentlist is known -- so that all parentlist-ish parameters are available
		if (!empty($this->_parentlist) && empty($this->_datadef))
			$this->_datadef_setup();
	}
	
	/* 
	   User-set preferences
	*/
	
	function set_param($value, $param) {
		// Don't allow external code to alter internal params
		if (substr($param,0,1)!='_')
			$this->_params[$param] = $value;
	}
	
	// Commented out because completely incompatible with how files are now being selected/read
	/*	// improve this to provide more functionality: request files by ID (MW, BM, etc). or override standard files (originals instead of modified, or prepatch instead of postpatch)
	function set_filestoread($toread) {
		if ($toread=='all') {
			$this->_nfilesread = count($this->_files);
		}
		elseif (is_numeric($toread) && $toread<=count($this->_files)) {
			$this->_nfilesread = $toread;
		}
		return $this->_nfilesread;
	}*/
	
	// Originally this function called _keep_init to do a full set up of $this->keep
	// Now want to do just do preliminary setup here -- enough setup so that _datadef_check can figure out
	// which values to process
	// Then rely on final call to _keep_init to properly fill in all subrecs within each rectype
	function request_data($valname, $option=0) {
		if ($valname=='all') {
			$this->_params['keepall'] = TRUE;
			if (isset($this->_datadef)) {
				foreach ($this->_datadef as $rectype => $data) {
					if ($rectype!='all')
						$this->keep[$rectype] = TRUE;
				}
			}
		}
		else {
			if (!is_array($valname))
				$valname = array($valname);
			foreach ($valname as $valalt) {
				$valalt = substr(strtoupper($valalt),0,4);
				$this->keep[$valalt] = $option|self::KEEPOPT_ALL;
			}
			
			if ($option&self::KEEPOPT_ENCH) {
				$this->keep['MGEF'] = $option|self::KEEPOPT_ALL;
				$this->keep['ENCH'] = $option|self::KEEPOPT_ALL;
			}	
		}
	}
	
	/* 
	   Read 
	*/
	// This was originally just function read()
	function read_all_files() {
		$this->_params['_multi_file'] = TRUE;

		$new_files = array();
		foreach ($this->_files as $filenum => $filename) {
			$this->set_fileinfo($filename, $filenum, TRUE);
			if (empty($this->_params['_first_db']) && !empty($this->_params['_first_read']))
				$new_files[$filenum] = $filename;
		}

		// If some of the files are new, read *only* those files, and force entire file to be read
		// -> reading is done differently on first_read (don't need to drop any existing tables)
		if (!empty($new_files)) {
			// Originally, I couldn't simply call request_data('all') at this point, because _datadef_check had already been called, and had cleared out
			// un-requested datadefs
			// However, even now that I can... I'm not sure I should: exiting makes it really clear that all that's being done
			// right now is reading the new files -- any other original request is being ignored
			if (empty($this->_params['keepall'])) {
				print "ERROR: Whenever a file is being read for the first time, all data records must be read\n";
				exit;
			}
			foreach ($new_files as $filenum => $filename) {
				$this->read_file($filename, $filenum);
			}
		}
		else {
			foreach ($this->_files as $filenum => $filename) {
				$this->read_file($filename, $filenum);
			}
		}
		
		$this->_parentlist->set_curr_nfile(0);
		$this->_finish_read();
		
		return $this->_parentlist;
	}
	
	function open_file($file) {
		if (!($this->_fpin = fopen($this->_params['inputdir'].$file, "rb"))) {
			print "Unable to open file ".$this->_params['inputdir'].$file."\n";
			exit();
		}
		$this->_fpsize = filesize($this->_params['inputdir'].$file);
	}
	function close_file() {
		fclose($this->_fpin);
		$this->_fpin = NULL;
	}
	
	function read_file($file, $filenum=NULL) {
		$this->_init_read();
		$this->_init_read_db();
		$this->_keep_init();
		$this->_grouptype = NULL;
		$this->_ngroup = 0;
		$this->_groupordid = array();
		$this->_groupid = array();
		
		$this->set_fileinfo($file, $filenum);
		$this->open_file($file);
		while ($this->read_next()) {
		}
		$this->close_file();
		
		if (empty($this->_params['_multi_file']))
			$this->_finish_read();
	}
	
	function recstats_sort($a, $b) {
		if (substr($a,0,4)!=substr($b,0,4))
			return (substr($a,0,4) < substr($b,0,4)) ? -1 : 1;
		if (strlen($a)<=4)
			$anum = -1;
		else
			$anum = (int) substr($a,5);
		if (strlen($b)<=4)
			$bnum = -1;
		else
			$bnum = (int) substr($b,5);
		if ($anum==$bnum)
			return 0;
		return ($anum < $bnum) ? -1 : 1;
	}
	
	function _init_read() {
		// Only go through DB initialization once when a set of files is being read
		if (!empty($this->_params['_init_read_done']))
			return;
		$this->_params['_init_read_done'] = TRUE;
		
		if (!isset($this->_params['_use_db'])) {
			if (!isset($this->_parentlist))
				$this->_params['_use_db'] = FALSE;
			elseif ($this->_parentlist->get_datamode()==CSData::DATAMODE_READ)
				$this->_params['_use_db'] = FALSE;
			else
				$this->_params['_use_db'] = TRUE;
		}
		if (empty($this->_params['_use_db'])) {
			// Do whatever remaining setup might be needed for non-DB-type reads
			// Can't do this yet for DB-type reads, because need to figure out whether I'm reloading DB
			$this->_datadef_check();
			return;
		}
		
		// Likely to be a redundant call, but best to be safe and make sure connection has been opened
		$this->_parentlist->init_db();
		
		if (!empty($this->_params['redo_db'])) {
			$this->_params['_first_db'] = TRUE;
		}
		elseif ($this->_parentlist->hastables()) {
			$this->_params['_first_db'] = FALSE;
		}
		else {
			$this->_params['_first_db'] = TRUE;
		}
	}
	
	function _init_read_db() {
		// Only go through DB initialization once when a set of files is being read
		if (!empty($this->_params['_init_read_db_done']))
			return;
		$this->_params['_init_read_db_done'] = TRUE;
		
		
		if ($this->_params['_first_db']) {
			// for this to work, need to either extract complete table info
			// OR need to change create_db to not drop DB, but instead drop all tables except Reclist
			// and then need to make sure setup_db doesn't try to recreate the table
			/*			if ($this->_parentlist->hastable('Reclist') && empty($this->_params['redo_reclist']))
				$this->_read_reclist();*/
			// This automatically drops any existing DB and clears out data from previous DB
			// But it doesn't create any tables -- just the DB itself
			$this->_parentlist->create_db();
		}
		$this->_datadef_check();
		$this->setup_db();

		if (!empty($this->_params['setup_scripts'])) {
			$this->setup_scripts();
		}
	}
	
	function _finish_read() {
		if (!empty($this->_params['_datadef_check_active']))
			return;
		
		// Do final print of all records
		// Most of these will be duplicates -- but this ensures that any records found below top-level groups
		// don't get missed
		if (!empty($this->_params['scanrecs']) && !empty($this->_params['testrecs'])) {
			ksort($this->_recstats);
			foreach ($this->_recstats as $rectype => $x) {
				$this->print_test_sub($rectype);
			}
		}
		
		if ($this->_parentlist->get_datamode()==CSData::DATAMODE_WRITE) {
			if (!empty($this->_params['_first_db'])) {
				$this->_finish_first_db();
			}
			if ($this->_params['calc_levlists'])
				$this->_calc_levlists();
			if ($this->_params['setup_cells'])
				$this->setup_cells();
			
			if (isset($this->_params['_history_setid'])) {
				$query = "UPDATE History SET active=0 WHERE setid=".$this->_params['_history_setid'];
				$this->_parentlist->do_query($query);
			}
		}
	}
	
	function set_fileinfo($file, $filenum=NULL, $no_create=FALSE) {
		if (!empty($this->_params['_datadef_check_active'])) {
			$this->_filemodnum = $this->_fileid = 0;
			return;
		}

		// Do NOT want to just automatically call init_read_db here: need to know whether _first_read is set first!!
		$this->_init_read();
		$this->_params['_first_read'] = $this->_params['_first_db'];
		if ($this->_params['_first_db']) {
			if ($no_create)
				return;
			$this->_init_read_db();
		}
		
		if (!isset($filenum)) {
			foreach ($this->_files as $n => $knownfile) {
				if ($knownfile==$file) {
					$filenum = $n;
					break;
				}
			}
		}
		
		// if using DB, check whether file already exists in DB
		$this->_filemodnum = $this->_fileid = NULL;
		$abbrev = NULL;
		if (!empty($this->_params['_use_db'])) {
			$query = 'SELECT fileid, filemodabbrev FROM Files WHERE filename = "'.$file.'"';
			$res = $this->_parentlist->do_query($query, 'onerow');
			if (!empty($res)) {
				$this->_fileid = $res['fileid'];
				$abbrev = $res['filemodabbrev'];
				// Should I also update Files entry??
			}
			else {
				$query = 'SELECT fileid, filemodabbrev FROM Files WHERE fileofficial = "'.$file.'"';
				$res = $this->_parentlist->do_query($query, 'onerow');
				if (!empty($res)) {
					$this->_fileid = $res['fileid'];
					$abbrev = $res['filemodabbrev'];
				}
			}

			if (!isset($abbrev)) {
				$this->_params['_first_read'] = TRUE;

				if ($no_create)
					return;
				// Files.fileid doesn't have autoincrement set... and changing autoincrement means that fileid=0 wouldn't be valid any more
				// But manually setting fileid should be safe
				if (isset($filenum)) {
					$fileid = $filenum;
				}
				else {
					$fileid = count($this->_files);
					$query = 'SELECT MAX(fileid) as maxid FROM Files';
					$res = $this->_parentlist->do_query($query);
					if (!empty($res) && $res['maxid']>=$fileid)
						$fileid = $res['maxid']+1;
				}
				$query = "INSERT INTO Files (fileid, filename, fileofficial, filenamespace, filemodname, filemodabbrev) VALUES ";
				$query .= '('.$fileid.', \''.$file.'\'';
				foreach (array('fileofficial', 'filenamespace', 'filemodname', 'filemodabbrev') as $key) {
					if (isset($this->{'_'.$key}[$filenum])) {
						$query .= ", '".$this->{"_".$key}[$filenum]."'";
					}
					else {
						$query .= ', NULL';
					}
				}
				$query .= ')';
				$this->_parentlist->do_query($query);
				$this->_fileid = $fileid;
				if (isset($this->_filemodabbrev[$filenum]))
					$abbrev = $this->_filemodabbrev[$filenum];
			}
			if (isset($abbrev)) {
				$query = 'SELECT min(fileid) as filemodnum FROM Files WHERE filemodabbrev="'.$abbrev.'"';
				$res = $this->_parentlist->do_query($query, 'onerow');
				if (!empty($res)) {
					$this->_filemodnum = $res['filemodnum'];
				}
			}
		}
		elseif (isset($filenum)) {
			$this->_filemodnum = $this->_fileid = $filenum;
		}
		else {
			$this->_filemodnum = $this->_fileid = 0;
		}
		
		// this is where _curr_nfile gets set in csdata.inc -> which is the value assigned to all newitems' fileid
		if (isset($this->_parentlist)) {
			if (isset($filenum)) {
				$this->_parentlist->add_file($filenum, $file, $this->_filenamespace[$filenum],  $this->_filemodname[$filenum], $this->_filemodabbrev[$filenum]);
			}
			else {
				$this->_parentlist->add_file($this->_filemodnum, $file, NULL, NULL, NULL);
			}
		}
	}
	
	// Temporary-type function to create and add new mod_status field to AllItems
	// combo of three flags:
	// 1 = is_unique formid -- to easily know whether to even look for other values of formid
	//   (arguably could be put into a separate field... but like idea of only having to alter one field based on updates/patches/mods)
	// 2 = default version of formid, regardless of mod (current in mod where formid was introduced)
	// 4 = current in a given mod (for cases where a mod alters a base formid)
	function create_mod_status() {
		$this->_parentlist->do_query('alter table AllItems add column mod_status int');
		$this->_parentlist->do_query('alter table AllItems add index mod_status (mod_status)');


// Globally set is_unique part of flag
		$query = 'update AllItems as A1 left join AllItems as A2 on (A1.formid=A2.formid AND A2.ordid!=A1.ordid) set A1.mod_status=IF(ISNULL(A2.ordid),1,0)';
		print $query."\n";
		$this->_parentlist->do_query($query);

		$res = $this->_parentlist->do_query('select * from Files order by fileid');
		$mod0 = NULL;
		$is_mod = FALSE;
		$abbrev_ids = array();
		while ($row=$this->_parentlist->row_query($res)) {
			$fileid = $row['fileid'];
			if (!isset($mod0)) {
				$mod0 = $row['filemodabbrev'];
			}
			$cv = 6;

// set active if this is first instance of formid -- or if it is first instance for this mod
// unset previous instances

print "fileid=$fileid cv=$cv\n";

// Set active=6 if no previous instance of formid 
			$query = 'update AllItems as A1 left join AllItems as A2 on (A1.formid=A2.formid AND A2.fileid<'.$fileid.') set A1.mod_status=A1.mod_status|6 where A1.fileid='.$fileid.' AND A2.ordid is null';
			print $query."\n";
			$this->_parentlist->do_query($query);
// Set active=4 if no previous instance in this mod
// (will include cases where active previously set to 6, but doesn't matter, because those cases won't be altered by update)
			if (!empty($abbrev_ids[$row['filemodabbrev']])) {
				$query = 'update AllItems as A1 left join AllItems as A2 on (A1.formid=A2.formid AND A2.fileid IN ('.implode(',',$abbrev_ids[$row['filemodabbrev']]).')) set A1.mod_status=A1.mod_status|4 where A1.fileid='.$fileid.' AND A2.ordid is null';
				print $query."\n";
				$this->_parentlist->do_query($query);
			}
			else {
				$query = 'update AllItems as A1 left join AllItems as A2 on (A1.formid=A2.formid) set A1.mod_status=A1.mod_status|4 where A1.fileid='.$fileid;
				print $query."\n";
				$this->_parentlist->do_query($query);
			}

// Set active if there is a previous instance of formid in same mod -- and de-activate previous instance
			if (!empty($abbrev_ids[$row['filemodabbrev']])) {
				$query = 'update AllItems as A1 inner join AllItems as A2 on (A1.formid=A2.formid and A2.fileid IN ('.implode(',',$abbrev_ids[$row['filemodabbrev']]).') AND A2.mod_status&4>0) set A1.mod_status=A1.mod_status|(A2.mod_status&6), A2.mod_status=0 where A1.fileid='.$fileid;
				print $query."\n";
				$this->_parentlist->do_query($query);
			}

			@ $abbrev_ids[$row['filemodabbrev']][] = $fileid;
		}

// also need to update all existing references to any altered formids -> want all ordid values to point to current value, not out-of-date value
		$query = 'SELECT ordid FROM AllItems where mod_status&0x01=0 AND mod_status&0x04>0 ';
		$res = $this->_parentlist->do_query($query);
		while ($row=$this->_parentlist->row_query($res)) {
			$item = $this->_parentlist->get_item($row['ordid']);
			$item->update_db_current();
			$item->release();
		}
	}

	// Read the next record OR group
	// Return FALSE at end of file or unrecoverable error, TRUE if file can still be read
	function read_next($nest=0) {
		// Note that this reads 'GRUP' records as if they were standard records -- then re-reads them if necessary
		// This is only possible because both record types start with 4-byte type followed by 4-byte size
		// Advantage is that read_header can just process entire header in one shot (without having to assume
		// that rectype was already read) -- and can directly handle all rectype checks
		$header = $this->_read_header($this->_header_rec);
		if (!$header)
			return FALSE;
		
		if ($header['type']=='GRUP') {
			fseek($this->_fpin, $header['loc']);
			$header = $this->_read_header($this->_header_group);
			return $this->read_group($nest, $header);
		}
		else
			return $this->read_rec($nest, $header);
	}
	
	// Function to read any header
	// Includes sanity checks for whether file is being read correctly
	// The checks are fairly generic -- so that this function can be used even to do exploratory
	// reads of new file types with unknown formats
	protected function _read_header($format) {
		$headerok = TRUE;
		$header = array();
		$header['loc'] = ftell($this->_fpin);
		if (is_null($this->_datachunk)) {
			if (feof($this->_fpin))
				return FALSE;
			elseif ($header['loc']==$this->_fpsize) {
				// read one more char to trigger eof
				fread($this->_fpin, 1);
				return FALSE;
			}
		
			if ($header['loc']>$this->_fpsize) {
				die("Attempt to start reading header at location ".$header['loc']." past end of file\n");
			}
		}
		else {
			$header['header_data'] = $this->_datachunk;
		}
		
		foreach ($format as $field => $datatype) {
			if ($field=='data') {
				// XXXX records are specific to subrecords (and not present in MW)
				// But having the code here won't hurt 
				if (!is_null($this->_xxxxsize))
					$size = $this->_xxxxsize;
				else
					$size = $header['datasize'];
				if (!$size)
					$header[$field] = '';
				else
					$header[$field] = $this->extract_single($size);
				$header['next'] = ftell($this->_fpin);
			}
			else {
				$header[$field] = $this->extract_single($datatype);
			}
			if (feof($this->_fpin))
				return FALSE;
			
			// Check values immediately -- so that datasize errors are caught before trying to read data
			if ($field=='type') {
		      // Check that type is a character string
				if (!preg_match('/^[A-Z][A-Z0-9_]+$/', $header['type'])) {
					if (!preg_match('/IAD$/', $header['type']) && !preg_match('/^..TX$/', $header['type'])) {
					//					var_dump($this->_reclens);
						print("Invalid header type string: '".$this->display_raw_data($header['type'])."'\n");
						$headerok = FALSE;
					}
				}
			}
			elseif ($field=='datasize') {
				if ($header[$field]<0) {
					print("Header $field (".$header[$field].") is less than 0.\n");
					$headerok = FALSE;
				}
				if (isset($this->_datachunk)) {
					if ($header[$field]>strlen($this->_datachunk)) {
						print("Header $field (".$header[$field].") is larger than available compressed datachunk (".strlen($this->_datachunk).").\n");
						$headerok = FALSE;
					}
				}
				else {
					if (($header[$field]+ftell($this->_fpin))>$this->_fpsize) {
						print("Header $field (".$header[$field].") is larger than available file size (".($this->_fpsize-ftell($this->_fpin)).").\n");
						$headerok = FALSE;
					}
				}
			}
		}
		
		// Value of 'next' (i.e., the file position of the next item in the file) depends upon the type of header being read
		// For GRUP -> next = loc + size
		// For other records -> next = ftell+size
		// For subrecords -> next = ftell (because data has already been read)
		if (!isset($header['next']) && isset($header['size'])) {
			if ($header['type']=='GRUP') {
				$header['next'] = $header['loc']+$header['size'];
			}
			else {
				$header['next'] = ftell($this->_fpin)+$header['size'];
			}
			if (($header['next'])>$this->_fpsize) {
				print("Header size (".$header['size'].") is larger than available file size (".($header['next']-$this->_fpsize).") -- next=".$header['next']." loc=".$header['loc']." fpsize=".$this->_fpsize."\n");
				$headerok = FALSE;
			}
			elseif ($header['next']<ftell($this->_fpin)) {
				print("Header size (".$header['size'].") is smaller than amount of data already read (".(ftell($this->_fpin)-$header['loc']).").\n");
				$headerok = FALSE;
			}
		}
	
		if (!$headerok) {
			if (isset($header['data']))
				unset($header['data']);
			var_dump($header);
			exit;
		}
		return $header;
	}

	// Extract altid, which depends on grtypenum
	function get_altid($header) {
		// Finish processing header -- in particular, extract altid, which depends on grtypenum
		if ($header['grtypenum']==0) {
			// top-level group
			$header['altid'] = $header['label'];
		}
		elseif ($header['grtypenum'] == 2 || $header['grtypenum'] == 3) {
			// interior cell block / sub-block
			$header['altid'] = $this->csunpack("L", $header['label']);
		}
		elseif ($header['grtypenum'] == 4 || $header['grtypenum'] == 5) {
			// exterior cell block / sub-block 
			// since x/y also specified in CELL record, this x/y is redundant?
			$x = $this->csunpack("S", substr($header['label'],0,2));
			$y = $this->csunpack("S", substr($header['label'],2,2));
			$header['altid'] = $x." ".$y;
		}
		else {
			$header['altid'] = $this->process_raw_formid($header['label']);
		}
		return $header['altid'];
	}
	
	// Group -- highest level collection in a esm file (introduced in OB)
	// Can contain sub-groups, or can contain records
	function read_group($nest, $header) {
		$header['altid'] = $this->get_altid($header);
		
		if (!empty($this->_params['doprint'])) {
			// if ($this->_scanrecs && ($this->_doprint || $header['grtypenum']==0)) {
			if ($header['grtypenum']==0) {
				$this->print_indent($nest);
				print $header['loc'];
				printf(" %0.2f", ($header['loc']/$this->_fpsize*100));
				print "%\n";
			}
			$this->print_indent($nest);
			print 'GRUP '.$header['grtypenum'].' '.$header['altid'];
			if (!is_null($this->_grouptype))
				print " grouptype=".$this->_grouptype;
			// ngroup hasn't been increased yet, so _ngroup still refers to group number for parent group
			if ($this->_ngroup>0 && !empty($this->_groupid[$this->_ngroup]))
				print " groupid=".$this->_groupid[$this->_ngroup]." groupordid=".$this->_groupordid[$this->_ngroup];
			print "\n";
		}
		
		// Skip this group entirely if I'm not interested in its contents
		$keep = TRUE;
		if ($header['grtypenum']==0 && empty($this->_params['scanrecs']) && empty($this->keep[$header['label']])) {
			$keep = FALSE;
			// BUT note that to read REFR/ACHR/ACRE data I need to do WRLD, CELL groups; ditto INFO->DIAL
			if (isset($this->_recgroup[$header['label']])) {
				foreach ($this->_recgroup[$header['label']] as $rectype => $x) {
					if (!empty($this->keep[$rectype]))  {
						$keep = TRUE;
						break;
					}
				}
			}
		}
		if (!$keep) {
			fseek($this->_fpin, $header['next']);
			return TRUE;
		}
		
		// Track group level and group info
		if ($header['grtypenum']==0) {
			$this->_grouptype = $header['label'];
			$this->_ngroup = 1;
			$this->_groupordid = array();
			$this->_groupid = array();
		}
		else {
			$this->_ngroup++;
			if (array_key_exists($this->_ngroup-1, $this->_groupordid)) {
				$this->_groupordid[$this->_ngroup] = $this->_groupordid[$this->_ngroup-1];
				$this->_groupid[$this->_ngroup] = $this->_groupid[$this->_ngroup-1];
			}
		}
		
		while (ftell($this->_fpin)<$header['next']) {
			$this->read_next($nest+1);
		}
		
		// unset group info for this group level
		if (array_key_exists($this->_ngroup, $this->_groupordid)) {
			unset($this->_groupordid[$this->_ngroup]);
			unset($this->_groupid[$this->_ngroup]);
		}
		$this->_ngroup--;
		// and once top-level group is done, make sure everything is cleared out
		if ($header['grtypenum']==0) {
			if (!empty($this->_params['scanrecs']) && !empty($this->_params['testrecs'])) {
				$this->print_test_sub($this->_grouptype);
				// Note that for WRLD GRUP, CELL GRUPs this will not print the results for all records
				// found beneath the top-level records
				// Pick those up after entire file has been read
			}
			
			$this->_grouptype = NULL;
			$this->_ngroup = 0;
			$this->_groupordid = array();
			$this->_groupid = array();
		}
		return TRUE;
	}
	
	// Records -- a collection of information all about a single item
	// read_rec creates the new item, then scans all the sub-records for that item
	function read_rec($nest, $header) {
		$rectype = $header['type'];
		if (empty($this->_params['scanrecs']) && empty($this->keep[$rectype])) {
			$this->_record_done($header, FALSE);
			fseek($this->_fpin, $header['next']);
			return TRUE;
		}
		if (!empty($this->_params['scanrecs'])) {
			// This just takes care of immediate need -> tracking when top-level groups need to be
			// opened up in order to fill data for a given rectype
			// Doesn't help with trying to figure out full group structure
			if (!empty($this->_grouptype) && $this->_grouptype != $rectype) {
				$this->_recgroup[$this->_grouptype][$rectype] = TRUE;
			}
			@ $this->_reclist[$rectype]['']++;
			@ $this->_reclens[$rectype][''][$header['size']]++;
			$this->_reccurr = array();
		}
		elseif (!empty($this->_params['testfunction'])) {
			$function = $this->_params['testfunction'];
			$function($rectype, $header, FALSE);
		}

		$flags = $header['flags'];
		
		if ($this->_params['doprint']>1) {
			$this->print_indent($nest);
			print $header['type'].' '.$header['size'].' '.$header['formid']."\n";
		}

		// reprocess header data if there are record-specific values I want to extract from any of the header data
		// start by just getting header data before potentially advancing through entire record to deal with compressed data
		if (isset($this->_datadef[$rectype]['____'])) {
			if (!isset($header['header_data'])) {
				$loc = ftell($this->_fpin);
				fseek($this->_fpin, $header['loc']);
				$header_data = fread($this->_fpin, $loc-$header['loc']);
			}
			else {
				$header_data = $header['header_data'];
			}
		}

		$compressed_subrec = ($header['flags']&0x40000?TRUE:FALSE);
		if ($compressed_subrec) {
			// note that data was uncompressed when creating files like Oblivion.esm.mod -- but compressed flag was not unset
			if ($this->_params['skip_compressed']) {
				$this->_record_done($header, FALSE);
				fseek($this->_fpin, $header['next']);
				return TRUE;
			}
			$finalsize = $this->extract_single('ulong');
			//			print "finalsize = $finalsize   headersize = ".$header['size']."\n";
			$compressed_data = $this->extract_single($header['size']-4);
			//			print strlen($compressed_data)."\n";
			$this->_datachunk = gzuncompress($compressed_data);
			if ($this->_datachunk===FALSE) {
				// If standard uncompress doesn't work, use alternative function that sticks more
				// closely to original zlib algorithms (but is much slower, since it's not compiled
				// code)
				// see http://code.google.com/p/zlibdecompress/
				require_once ('zlib/ZlibDecompress.php');
				$zlib = new ZlibDecompress;
				$this->_datachunk = $zlib->inflate(substr($compressed_data,2));
				if ($this->_datachunk===FALSE) {
					print "Both original gzuncompress and alternative ZlibDecompress failed to extract compressed data at loc ".$header['loc']."\n";
					print "finalsize = $finalsize origsize=".$header['size']."\n";
				
					print "base64_encoded=\n";
					$encoded = base64_encode($compressed_data);
					print "start = ".substr($encoded,0,20)."\n";
					print "end = ".substr($encoded,-20)."\n";
				
					var_dump($header);
					exit;
				}
				// if I want to proceed even with a bad uncompress, need to set _datachunk=NULL to skip
				// processing of this record
			}
		}
		else {
			$this->_datachunk = NULL;
		}
		
		if (empty($this->_params['scanrecs']) && isset($this->_parentlist)) {
			$this->_parentlist->newitem($header['type']);
			if (isset($header['formid']))
				$this->_parentlist->set_id($header['formid']);
			// this routine just copies groupid setting for all records: only saved to dbfile, though, for appropriate records
			// uses group-1 to get parent (don't want one cell's ordid copied as group to subsequent cells)
			if ($this->_ngroup>0) {
				if (isset($this->_groupordid[$this->_ngroup-1])) {
					$this->_parentlist->set_active($this->_groupordid[$this->_ngroup-1], 'group_id');
				}
				// for cells, also set top-level world group
				if ($rectype=='CELL' && isset($this->_groupordid[1]))
					$this->_parentlist->set_active($this->_groupordid[1], 'world_id');
			}
			$this->_set_rec_flags($flags, isset($header['flags_b'])?$header['flags_b']:NULL);
		}
		$this->_subset = NULL;
		$this->_xxxxsize = NULL;
		while (isset($this->_datachunk) || ftell($this->_fpin)<$header['next']) {
			$this->read_sub($header['type'], $nest+1);
		}
		// re-process main record header
		// doing after rest of subrecords so that datachunk has already been cleared (if exists)
		if (isset($this->_datadef[$rectype]['____'])) {
			$this->read_sub($header['type'], $nest+1, $header_data);
		}
		
		if (empty($this->_params['scanrecs'])) {
			$this->_record_done($header);
		}
		else {
			foreach ($this->_reclist[$rectype] as $subrec => $x) {
				if (isset($this->_reccurr[$subrec]))
					$count = $this->_reccurr[$subrec];
				else
					$count = 0;
				if (!isset($this->_reccount[$rectype][$subrec])) {
					$this->_reccount[$rectype][$subrec] = array('min' => $count, 'max' => $count);
				}
				else {
					$this->_reccount[$rectype][$subrec]['min'] = min($count, $this->_reccount[$rectype][$subrec]['min']);
					$this->_reccount[$rectype][$subrec]['max'] = max($count, $this->_reccount[$rectype][$subrec]['max']);
				}
			}
		}
		$this->_datachunk = NULL;
		return TRUE;
	}	
	
	function convert_file($file) {
		$this->open_file($file);
		$fpout = fopen($this->_params['inputdir'].$file.'.mod', 'w+b');
		
		while (($len=$this->convert_next(0, $fpout))!==FALSE) {
		}
		
		$this->close_file();
		fclose($fpout);
	}
	
	function convert_next($nest, $fpout=NULL) {
		$header = $this->_read_header($this->_header_rec);
		if (!$header)
			return FALSE;
		
		if ($header['type']=='GRUP') {
			fseek($this->_fpin, $header['loc']);
			$header = $this->_read_header($this->_header_group);
			$len = $this->convert_group($nest, $header, $fpout);
		}
		else {
			$len = $this->convert_rec($nest, $header, $fpout);
		}
		
		return $len;
	}
	
	protected function _create_header($format, $header) {
		$string = '';
		
		foreach ($format as $field => $datatype) {
			if ($field=='data' || is_int($datatype)) {
				$string .= $header[$field];
			}
			elseif ($datatype=='raw_formid') {
				$value = hexdec($header[$field]);
				$string .= pack('L', $value);
			}
			else {
				$string .= pack($this->_datatype[$datatype]['code'], $header[$field]);
			}
		}
		
		return $string;
	}

	// Actually want to keep STATs for lookups
	// But earlier converts also cleared a large fraction of REFR records
	
	// Notes from perl file:
	
# deleting:
#  all LAND, PGRD records
#  all REFR's based on STAT objects (except for markers)
#  all REFR's based on ANIO, CLMT, CSTY, EFSH, EYES, GRAS, HAIR, IDLE, LAND, LIGH, LTEX, PGRD, SOUN, TREE, WATR, WTHR objects
#     most of these probably don't have REFR objects.  LIGH|SOUN|TREE do have a good number, though
#     adding this list reduced size of Oblivion.esm.mod by 25%
# uncompressing all other compressed records (in particular NPC_)
	
# could expand to delete usless ACTI REFRs:
#   /DustPuffUp|DustCloud|DustFall|FlameNode|TwinkleLights|TorchTall|dripper|Brewersign|cellChain|LandscapeWater|OBCloud|OblivionSmokeEmitter/
	
# could also expand to delete unknown subrecords from REFRs:
#  XSED|XLOD|XPCI|XESP|XLCM|XRTM|XACT|XCNT
# would require extra coding, however, and several of these appeared in tests to be primarily
# used by deleted object types (e.g., TREE)
	
# primary purpose is to get the Oblivion.esm file down to a size
#   where it can be opened up in emacs
# secondary purpose is to reduce the processing time when manipulating
#   these files
	
# deletions selected to focus on the data that is taking up the
#   most space
# also want to be sure that global raw searches (e.g., for a formid)
#   always find the data, even if it's in a record whose purpose is
#   currently unknown... Therefore not just deleting all unknown
#   records
	
# largest GRUPs: WRLD >> CELL >> DIAL, SCPT, BOOK, NPC_, STAT
	
	
	function convert_group($nest, $header, $fpout) {
		if (empty($this->_params['fix_compressed'])) {
			if (in_array($header['label'], array('IDLE', 'IDLM', 'IMAD', 'LAND', 'NAVI', 'NAVM', 'STAT', 'WTHR'))) {
				fseek($this->_fpin, $header['next']);
				return 0;
			}
		}
		
		$headstr = $this->_create_header($this->_header_group, $header);
		$startloc = ftell($fpout);
		fwrite($fpout, $headstr);
		$len = strlen($headstr);
		
		$contents = '';
		while (ftell($this->_fpin)<$header['next']) {
			$len += $this->convert_next($nest+1, $fpout);
		}
		
		if ($header['size']!=$len) {
			$endloc = ftell($fpout);
			fseek($fpout, $startloc+4);
			fwrite($fpout, pack('L',$len));
			fseek($fpout, $endloc);
		}
		return $len;
	}
	
	function convert_rec($nest, $header, $fpout) {
		if (empty($this->_params['fix_compressed'])) {
			if (in_array($header['type'], array('IDLE', 'IDLM', 'IMAD', 'LAND', 'NAVI', 'NAVM', 'STAT', 'WTHR'))) {
				fseek($this->_fpin, $header['next']);
				return 0;
			}
		}
		
		$compressed_subrec = ($header['flags']&0x40000?TRUE:FALSE);
		if ($compressed_subrec && empty($this->_params['fix_compressed'])) {
			$finalsize = $this->extract_single('ulong');
			//			print "finalsize = $finalsize   headersize = ".$header['size']."\n";
			$compressed_data = $this->extract_single($header['size']-4);
			//			print strlen($compressed_data)."\n";
			$contents = gzuncompress($compressed_data);
			$header['size'] = strlen($contents);
		}
		else {
			if ($compressed_subrec)
				print "Record ".$header['type']." ".$header['formid']." fixed\n";
			$contents = $this->extract_single($header['size']);
		}
		$header['flags'] &= ~(0x40000);
		
		$headstr = $this->_create_header($this->_header_rec, $header);
		$len = strlen($headstr.$contents);
		fwrite($fpout, $headstr.$contents);
		return $len;
	}
		
	// Takes care of setting group info, too -- regardless of whether or not the record was actually processed
	protected function _record_done($header, $save_record=TRUE) {
		if (!isset($this->_parentlist))
			return NULL;
		
		$rectype = $header['type'];
		$getid = NULL;
		$ordid = NULL;
		if ($this->_ngroup>=0 && !is_null($this->_grouptype)) {
			$getid = $this->_ngroup;
			if ($save_record)
				$activeid = $this->_parentlist->get_active('id');
			elseif (isset($header['formid']))
				$activeid = $header['formid'];
			else
				return NULL;
			$this->_groupid[$getid] = $activeid;
		}
		// this is a semi-kludge to get INFO/DIAL groups working in MW... given that INFO/DIAL are always at end of MW files, it should only affect those records
		// one remaining problem for MW  INFO/DIAL: unique identifiers (NAME probably should not be used as edid for DIAL ... but is INAM even unique or only unique within one DIAL set?)
		elseif ($rectype=='DIAL') {
			$this->_ngroup=1;
			$getid = 0;
			if ($save_record)
				$activeid = $this->_parentlist->get_active('id');
			elseif (isset($header['formid']))
				$activeid = $header['formid'];
			else
				return NULL;
			$this->_groupid[$getid] = $activeid;
		}
		if ($save_record) {
			$ordid = $this->_parentlist->inactive();
		}
		elseif (isset($activeid)) {
			// when get_ordid was revamped, I wasn't entirely sure how to update his instance of get_ordid...
			// assuming that want version of formid appropriate to this mod (assuming mainly used to set groupordid)
			$ordid = $this->_parentlist->get_ordid($activeid, $this->_parentlist->get_file_abbrev($this->_filemodnum), $rectype);
		}
		if (isset($getid) && isset($ordid)) {
			$this->_groupordid[$getid] = $ordid;
		}
	}

	// define values for main record flags	
	// for now, only setting flags for which I understand the significance and I am likely to use
	// use values 32-47 for flags in Skyrim's extra 4-bytes?
	protected function _get_rec_flags() {
		return array(5 => 'deleted', 10 => 'questitem', 11 => 'disabled');
// 12 => 'ignored', 17 => 'dangerous'
# REFR page says this is the "disabled by default" flag, but
# main page says differently. And checks show that it's set for NPCs that are enabled by default, but generally dead ones ==> is this the 'dead' flag??
// 19 => 'nowait'
	}

	protected function _set_rec_flags($flags, $flags_b=NULL) {
		$flag_names = $this->_get_rec_flags();

		foreach ($flag_names as $offset => $name) {
			if ($offset<32)
				$value = ($flags>>$offset)&0x1?true:false;
			elseif (isset($flags_b))
				$value = ($flags_b>>($offset-32))&0x1?true:false;
			else
				continue;
			$this->_parentlist->set_active($value, $name);
		}
// TEMPORARY!!
// Used for testing values in rec_flags -> but requires manually adding test1/test2 columns to AllItems
//$this->_parentlist->set_active($flags, 'test1');
//$this->_parentlist->set_active($flags_b, 'test2');
	}
	
	// Sub-records: the actual data for a record
	// There can be multiple sub-records of the same type, (multi=true) in which an array is created for the data
	// Sub-records start with an entry providing the size of the sub-record,
	//  so that amount of data can immediately be read from stream; subsequent data-handling then has to
	//  just work with the already-read binary string
		
	function read_sub($rectype, $nest, $srcdata=NULL) {
		if (isset($srcdata)) {
			// reprocessing main record header to extract record-specific flags
			$header['type'] = '____';
			$header['data'] = $srcdata;
			$header['datasize'] = strlen($srcdata);
		}
		else {
			$header = $this->_read_header($this->_header_sub);
			if (!$header)
				return FALSE;
		}
		
		if (!empty($this->_params['scanrecs']) && !isset($srcdata)) {
			@ $this->_reclist[$rectype][$header['type']]++;
			@ $this->_reclens[$rectype][$header['type']][$header['datasize']]++;
			@ $this->_reccurr[$header['type']]++;
		}
		// Printing at the subrecord level is typically overkill -- uncomment if wanted for testing purposes
		if ($this->_params['doprint']>2) {
			$this->print_indent($nest);
			print "Sub ".$header['type']." ".$header['datasize']."\n";
		}
		
		if ($header['type']=='XXXX') {
			$this->_xxxxsize = $this->csunpack('L', $header['data']);
			return TRUE;
		}
		elseif (!isset($srcdata)) {
			$this->_xxxxsize = NULL;
		}
		
		if (!empty($this->_params['scanrecs'])) {
			if (!empty($this->_params['testrecs']) && !isset($srcdata)) {
				$this->test_sub_format($rectype, $header);
			}
			return TRUE;
		}
		elseif (!empty($this->_params['testfunction'])) {
			$function = $this->_params['testfunction'];
			$function($rectype, $header, TRUE);
			return TRUE;
		}
		
		$type = $header['type'];
		$size = $header['datasize'];
		$data = $header['data'];
		// aliases such as CTDT => CTDA
		if (!empty($this->_datadef_alias[$type])) {
			$type = $this->_datadef_alias[$type];
		}
		
		if (!is_null($this->_subset)) {
			$subs = explode('-', $this->_subset);
			while (count($subs)) {
				$last = array_pop($subs);
				if ($last==$type)
					$settype = implode('-', array_merge($subs, array($type)));
				else 
					$settype = implode('-', array_merge($subs, array($last), array($type)));
				if (isset($this->_datadef[$rectype][$settype])) {
			// skipping because subrecord is valid, but no data requested from subrecord
					if (!count($this->_datadef[$rectype][$settype]) || !array_key_exists($settype, $this->keep[$rectype]))
						return FALSE;
					break;
				}
				$settype = NULL;
			}
			if (!isset($settype)) {
				// skipping because neither main records nor subrecord requested
				if (!isset($this->_datadef[$rectype][$type]) || !array_key_exists($type, $this->keep[$rectype]))
					return FALSE;
			}
		}
		elseif (!array_key_exists($type, $this->_datadef[$rectype]) || !array_key_exists($type, $this->keep[$rectype])) {
			// add flag to control this message -- but don't want it when doing prelim scans with cleanup_only=TRUE
			/*			if (!array_key_exists($type, $this->_datadef[$rectype])) {
				print "skipping unknown subrecord $type in record $rectype (size $size)\n";
			}*/
			
			return FALSE;
		}
		
		if (!($outstr=$this->_do_special_sub($rectype, $type, $data))) {
			$outstr = $this->extract_sub_data($rectype, $type, $data, $size);
		}
		
		if ($this->_params['doprint']>3 && !empty($outstr)) {
			$this->print_indent($nest);
			if (strlen($outstr)>100)
				$outstr = substr($outstr,0,100)."...";
			print "$outstr\n";
		}
		
		return TRUE;
	}
	
	function extract_sub_data($rectype, $type, $data, $size=NULL) {
		$settype = $type;
		$par = $rectype;
		if (!array_key_exists($rectype, $this->_datadef))
			return;
		if (!is_null($this->_subset)) {
			$subs = explode('-', $this->_subset);
			while (count($subs)) {
				$last = array_pop($subs);
				if ($last==$type)
					$settype = implode('-', array_merge($subs, array($type)));
				else 
					$settype = implode('-', array_merge($subs, array($last), array($type)));
				if (isset($this->_datadef[$rectype][$settype])) {
					if (!count($this->_datadef[$rectype][$settype]))
						return;
					break;
				}
				$settype = NULL;
			}
			if (!isset($settype)) {
			// revert to standard (non-subset) type if subset type never defined but standard type is
				if (isset($this->_datadef[$rectype][$type]))
					$settype = $type;
				else
					return;
			}
		}
		elseif (!array_key_exists($type, $this->_datadef[$rectype]))
			return;
		if (is_null($par))
			return;
		
		$outstr = "";
		$mkey = NULL;
		$saveindex = NULL;
		if (array_key_exists('multi', $this->_datadef[$par][$settype]))
			$mkey = $this->_datadef[$par][$settype]['multi'];
		elseif (!is_null($this->_subset) && $settype!=$type)
			$mkey = $this->_subset;
		
		if (!is_null($mkey)) {
			$saveindex = $this->_parentlist->set_active_index($mkey, ($mkey!=$settype)?FALSE:TRUE);
			$outstr = "$saveindex ";
			/*			print "saveindex=$saveindex mkey=$mkey settype=$settype subset=".$this->_subset."\n";
			if ($this->_parentlist->get_active('formid')=='0x0002522f' && substr($saveindex,0,6)=='ALST-2') {
				print "stopping for formid\n";
				exit;
			}*/
		}
		
		$typeex = $type;
		
		if (array_key_exists('len1', $this->_datadef[$par][$settype])) {
			if (strlen($data)<=$this->_datadef[$par][$settype]['len1'])
				$dname = 'data1';
			elseif (!array_key_exists('len3', $this->_datadef[$par][$settype]) || strlen($data)<=$this->_datadef[$par][$settype]['len2'])
				$dname = 'data2';
			else
				$dname = 'data3';
		}
		else
			$dname = 'data';
		
		if (!array_key_exists($dname, $this->_datadef[$par][$settype])) {
			print "missing dname $dname\n";
			print "{$par} {$settype}\n";
			var_dump($this->_datadef[$par][$settype]);
			exit;
		}
		
		if (isset($this->_datadef[$par][$settype]['repeat_data'])) {
			$chunklen = $this->_datadef[$par][$settype]['repeat_data'];
			for ($ichunk=0; ; $ichunk++) {
				$chunk = substr($data, $chunklen*$ichunk, $chunklen);
				if (strlen($chunk)<$chunklen)
					break;
				if ($ichunk) {
					$saveindex = $this->_parentlist->set_active_index($mkey, ($mkey!=$settype)?FALSE:TRUE);
					$outstr .= " | $saveindex ";
				}
				$currvals = $this->extract_generic_data($this->_datadef[$par][$settype][$dname], $chunk);
				foreach ($currvals as $valname => $value) {
					$this->_parentlist->set_active($value, $valname, $saveindex);
					$outstr .= $this->_parentlist->get_active($valname, $saveindex) . " ";
				}
				
			}
		}
		else {
			$currvals = $this->extract_generic_data($this->_datadef[$par][$settype][$dname], $data);
			foreach ($currvals as $valname => $value) {
				$this->_parentlist->set_active($value, $valname, $saveindex);
				$outstr .= $this->_parentlist->get_active($valname, $saveindex) . " ";
			}
		}
		if ($outstr != "") {
			$addstr = $typeex;
			if (isset($size))
				$addstr .= ' ('.$size.')';
			$outstr = $addstr.' '.$outstr;
		}
		
		if (isset($mkey) && array_key_exists('subset',  $this->_datadef[$par][$mkey])) {
			$this->_subset = $mkey;
		}
		elseif (array_key_exists('subset',  $this->_datadef[$par][$settype])) {
			$this->_subset = $settype;
		}
		// if a non-subset record was just processed, then we're no longer in subset mode
		elseif (!is_null($this->_subset) && $settype==$type)
			$this->_subset = NULL;
		
		return $outstr;
	}
	
	// Override by derived classes for any extra processing necessary to handle a sub-record
	protected function _do_special_sub($rectype, $type, $data) {
		return false;
	}
	
	/**
	 * Functions for testing unknown files
	 */
	function read_test_record() {
		$header = $this->_read_header($this->_header_rec);
		if (!$header) {
			print "empty record\n";
		}
		else {
			//var_dump($header);
		}
		return $header;
	}
	
	function find_string($minlen=4) {
		$startloc = NULL;
		$currlen = 0;
		$currstr = array();
		while (!feof($this->_fpin)) {
			$test = $this->extract_single(1);
			$ord = ord($test);
			if ($ord>=32 && $ord<127) {
				if (!$currlen)
					$startloc = ftell($this->_fpin)-1;
				$currstr[$currlen] = $test;
				$currlen++;
			}
			else {
				if ($currlen>=$minlen)
					break;
				$currlen = 0;
				$currstr = array();
			}
		}
		
		if ($currlen) {
			$currstr = implode('', $currstr);
			print "string '$currstr' found at location $startloc\n";
		}
	}
	
	function print_test_sub($rectype) {
		if (!isset($this->_recstats[$rectype]))
			return;
		print "\nStats for record $rectype\n";
		uksort($this->_recstats[$rectype], array($this, 'recstats_sort'));
		foreach ($this->_recstats[$rectype] as $subrec => $sdata) {
			print "  $subrec ";
			if (strlen($subrec)==4) {
				print $sdata['minsize'];
				if ($sdata['maxsize']>$sdata['minsize'])
					print '-'.$sdata['maxsize'];
			}
			print " t".$sdata['total'];
			
			if (!empty($sdata['offset'])) {
				print " offsets: ";
				foreach ($sdata['offset'] as $off => $odata) {
					foreach ($odata as $type => $count) {
						print " $off-$type-$count";
					}
				}
			}
			
			if (!empty($sdata['isstring']) && $sdata['isstring']==$sdata['total']) {
				print " string (all)";
			}
			elseif (!empty($sdata['isstring']) && !empty($sdata['quasistring']) && (($sdata['isstring']+$sdata['quasistring'])==$sdata['total'])) {
				print " string (quasi ".$sdata['quasistring'].", ".sprintf("%0.2f", $sdata['quasistring']/$sdata['total']*100)."%)";
				if (!empty($sdata['utfstring'])) {
					print "  UTF-8 (".$sdata['utfstring'].")";
				}
			}
			elseif (!empty($sdata['zero']) && $sdata['zero']==$sdata['total']) {
				print " zero (all)";
			}
			else {
				if (!empty($sdata['isstring'])) {
					print " string_frac (".$sdata['isstring'].", ".sprintf("%0.2f", $sdata['isstring']/($sdata['total'])*100)."%)";
					if (!empty($sdata['utfstring'])) {
						print "  UTF-8 (".$sdata['utfstring'].")";
					}
				}
				if (!empty($sdata['zero'])) {
					print " zero_frac (".$sdata['zero'].", ".sprintf("%0.2f", $sdata['zero']/($sdata['total'])*100)."%)";
				}
				else {
					$sdata['zero'] = 0;
				}
				
				$isfloat = FALSE;
				if (!empty($sdata['float_nan'])) {
					//					print " float_nan (".$sdata['float_nan'].", ".sprintf("%0.2f", $sdata['float_nan']/($sdata['total'])*100)."%)";
				}
				else {
					if (!empty($sdata['float_one'])) {
						print " float_one (".$sdata['float_one'].", ".sprintf("%0.2f", $sdata['float_one']/($sdata['total'])*100)."%)";
						if ($sdata['float_one']>10 || $sdata['float_one']>$sdata['total']*0.5)
							$isfloat = TRUE;
					}
					if (!empty($sdata['float_int'])) {
						print " float_int (".$sdata['float_int'].", ".sprintf("%0.2f", $sdata['float_int']/($sdata['total'])*100)."%)";
						if ($sdata['float_int']>10 || $sdata['float_int']>$sdata['total']*0.5)
							$isfloat = TRUE;
					}
					if (isset($sdata['float_min']) && $sdata['float_min']>-1.e10 && abs($sdata['float_min'])>0.0001 && $sdata['float_max']<1.e10) {
						print " floatrange (".$sdata['float_min']."|".$sdata['float_max'].")";
					}
				}
				
				if (empty($sdata['formid_bad']) && !empty($sdata['rectypes'])) {
					arsort($sdata['rectypes']);
					print " rectypes (";
					$nrec = $sdata['zero'];
					foreach ($sdata['rectypes'] as $rectype => $count) {
						$nrec += $count;
						print "$rectype=$count ";
					}
					print ")";
					// note that having all values look like rectypes is really only conclusive if
					// (1) all are the same rectype or
					// (2) there are refids>1000 ... otherwise, any set of low integers would trigger this
					if ((count($sdata['rectypes'])==1 && $nrec>$sdata['total']*0.9) || ($nrec==$sdata['total'] && isset($sdata['long_max']) && $sdata['long_max']>1000)) {
						print "\n";
						continue;
					}
				}
				
				if (!$isfloat && isset($sdata['long_min'])) {
					print " longrange (".$sdata['long_min']."|".$sdata['long_max'].")";
					// doesn't necessary mean value is long .... just that first three bytes are zero, so
					// not going to get anything out of remaining checks
					if (abs($sdata['long_min'])<200 && abs($sdata['long_max'])<200) {
						print "\n";
						continue;
					}
					/*					if (isset($sdata['ulong_min']))
						print " ulongrange (".$sdata['ulong_min']."|".$sdata['ulong_max'].")";
					if (isset($sdata['ulong_neg']))
						print " ulong_neg (".$sdata['ulong_neg'].", ".sprintf("%0.2f", $sdata['ulong_neg']/($sdata['total'])*100)."%)";*/
				}

				if (!$isfloat && isset($sdata['byte_min']) && $sdata['byte_min']>=-1 && $sdata['byte_max']<=100) {
					print " byterange (".$sdata['byte_min']."|".$sdata['byte_max'].")";
				}
				
				// working under assumption that if a variable is being used a flag, some of the low-level
				// bits are likely to be very unevenly distributed
				// (but skip bit=1, because even numbers probably more common than odd)
				if (!$isfloat && isset($sdata['flags']) && $sdata['total']>10) {
					$biased = FALSE;
					// should perhaps set kmax based on long_max
					// mainly don't want to trigger flags just because I'm testing bit at upper range of data values
					for ($k=1; $k<6; $k++) {
						if (empty($sdata['flags'][$k]))
							continue;
						$ratio = $sdata['flags'][$k]/$sdata['total'];
						if ($ratio<0.1 || $ratio>0.9)
							$biased = TRUE;
					}
					
					if ($biased) {
						print " flags_set ";
						for ($k=0; $k<16; $k++) {
							if (empty($sdata['flags'][$k]))
								continue;
							print " ".$sdata['flags'][$k]."@$k";
						}
					}
				}
			}
			print "\n";
		}
	}
	
	function test_sub_format($rectype, $header) {
		static $variable_len=NULL;
		if (!isset($variable_len)) {
			$variable_len = array();
			$query = "SELECT rectype, subrec, minsize from Reclist WHERE subrec<>'' AND maxsize>minsize";
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$variable_len[$row['rectype']][$row['subrec']] = max(4, $row['minsize']);
			}
		}
		
		// Never do rectypes I never plan on reading
		// Especially do *not* want to process subrecs such as NAVM.NVTR that are huge variable length non-strings
		if ($rectype=='LAND' || $rectype=='NAVI' || $rectype=='NAVM')
			return;
		// On first pass, don't bother with mega-sized rectypes -- since the details of those rectypes are low
		// priority
		// should there be a flexible way to change these rectypes?  Or should I just edit code here
		// as necessary?
		//		if ($rectype=='REFR' || $rectype=='CELL' || $rectype=='INFO')
		//			return;
		$type = $header['type'];
		$size = $header['datasize'];
		$data = $header['data'];
		
		@ $this->_recstats[$rectype][$type]['total'] ++;
		if (!isset($this->_recstats[$rectype][$type]['minsize'])) {
			$this->_recstats[$rectype][$type]['minsize'] = $size;
			$this->_recstats[$rectype][$type]['maxsize'] = $size;
		}
		else {
			$this->_recstats[$rectype][$type]['minsize'] = min($size, $this->_recstats[$rectype][$type]['minsize']);
			$this->_recstats[$rectype][$type]['maxsize'] = max($size, $this->_recstats[$rectype][$type]['maxsize']);
		}

		// Test for string-ish data
		// I'm assuming strings only contain ASCII chars (32-126)
		// But in reality occasional characters outside of that range do occur	
		// So extended string range check done -- only for variable-length records
		// Check is pretty non-discriminating at that point... 
		// except that numbers are most likely to include low ord values (0,1,etc)
		$isstring = TRUE;
		$nspace = 0;
		$nquasi = 0;
		$nutf = 0;
		// Need to add checks to determine whether using UTF-8, latin-1, or Windows-1252
		// (default assumption is Windows-1252, as in MW/OB/FO, but want to be sure it doesn't change in future)
		for ($i=0; $i<$size; $i++) {
			$ord = ord(substr($data,$i,1));
			if ($ord==0 && $i==$size-1 && ($i || !empty($variable_len[$rectype][$type])))
				break; // null-terminated string (but not if it's a single char)
			if ($ord<32 || $ord>=127) {
				if (empty($variable_len[$rectype][$type])) {
					$isstring = FALSE;
					break;
				}
				
				// UTF-8 check
				$utf = TRUE;
				$nex = 0;
				if ($ord<=191 || $ord>=248) {
					$utf = FALSE;
				}
				elseif ($ord<=223)
					$nex = 1;
				elseif ($ord<=239)
					$nex = 2;
				else
					$nex = 3;
				if ($nex) {
					for ($k=1; $k<=$nex; $k++) {
						if (($i+$k)>=$size) {
							$utf = FALSE;
							break;
						}
						$kord = ord(substr($data,$i+$k,1));
						if ($kord<=127) {
							$utf = FALSE;
							break;
						}
					}
				}
				if ($utf)
					$nutf++;
				
				// allow tabs, carriage-returns
				// allow extended ASCII, but not graphic-type characters
				// 169 is graphic-type, but used in FO
				// 184 used as weird comma in Ahzirr Trajijazaeri
				if ($ord==9 || $ord==10 || $ord==13) {
					$nspace++;
				}
				elseif ($utf || ($ord>=128 && $ord<=169) || ($ord>=171 && $ord<=175) || $ord==184 || ($ord>=224 && $ord<=253)) {
					$nquasi++;
				}
				else {
// Can't do this check for SR -> FULL and DESC are str_index
/*					if ($type=='EDID' || $type=='FULL' || $type=='DESC') {
						print "bad char $i $ord $rectype $type\n$data\n";
						exit;
					}*/
					$isstring = FALSE;
					break;
				}
			}
		}
		if ($isstring) {
			if (!$nspace && !$nquasi) {
				@ $this->_recstats[$rectype][$type]['isstring'] ++;
				if ($nutf)
					@ $this->_recstats[$rectype][$type]['utfstring']++;
			}
			else {
				//				if (($nquasi>2 && $nquasi/$size>0.1) || ($nspace>4 && $nspace/$size>0.33) || ($nspace+$nquasi)/$size>0.4) {
				if (($nquasi>2 && $nquasi/$size>0.1) || ($nspace>4 && $nspace/$size>0.5) || ($nspace+$nquasi)/$size>0.55) {
/*					if ($type=='EDID' || $type=='FULL' || $type=='DESC') {
						print "bad quasi $rectype $type check: $nquasi $nspace $size\n$data\n";
						//						exit;
					}*/
					$isstring = FALSE;
				}
				else {
					@ $this->_recstats[$rectype][$type]['quasistring'] ++;
					if ($nutf)
						@ $this->_recstats[$rectype][$type]['utfstring']++;
					$isstring = FALSE;
				}
			}
		}
/*		elseif (($type=='EDID' || $type=='FULL' || $type=='DESC') && ($size>1 || ord($data))) {
			print "bad string $rectype $type $size:\n".$data."\n";
			exit;
		}*/
		// don't keep testing if there are more than 4 legit chars in a row
		if ($isstring && $size>4)
			return;
		// don't do any more testing for MO* subrecords since I'm not interested in them
		// (and some get to be lengthy)
		// when doing this for Skyrim, should double-check what records are being skipped
		if (substr($type,0,2)=='MO')
			$doonce = TRUE;
		// only do full scan on short records or fixed-length records
		// otherwise just scan first 4 bytes
		elseif ($size>8 && !empty($variable_len[$rectype][$type])) {
			$size = min($size, $variable_len[$rectype][$type]);
			$doonce = FALSE;
		}
		elseif ($size<=4)
			$doonce = TRUE;
		else
			$doonce = FALSE;
		
		// scan for float offsets that are not multiples of 4
		// scan for refids, too?  but that ends up just being a test for ints within a certain range
		// unless I also check for all matches being same rectype
		// given how few cases so far look like offsets, not worth doing a refid scan
		if (!$doonce) {
			for ($i=0; $i<$size-4; $i++) {
				if (!($i%4))
					continue;
				$fltype = $this->test_float($this->csunpack('f', substr($data,$i,4)));
				if ($fltype=='one' || $fltype=='int') {
					@ $this->_recstats[$rectype][$type]['offset'][$i][$fltype] ++;
				}
			}
		}
		
		for ($j=0; $j<$size; $j+=4) {
			if ($doonce) {
				if ($j)
					break;
				$typeloc = $type;
			}
			else {
				$typeloc = $type.'-'.$j;
				@ $this->_recstats[$rectype][$typeloc]['total'] ++;
			}
			
			for ($i=0; $i<4 && $i+$j<$size; $i++) {
				$this->_set_test_sub_stats($rectype, $typeloc, 'byte', $this->csunpack('c', substr($data,$i+$j,1)));
			}
			if (($size-$j)<4)
				continue;
		
			$test = substr($data,$j,4);
			$lval = $this->csunpack('l',$test);
			if (!$lval) {
				@ $this->_recstats[$rectype][$typeloc]['zero'] ++;
				continue;
			}
		
			// do some type of flag testing?
			$bittest = $lval;
			for ($k=0; $k<16; $k++) {
				if ($bittest&0x01) {
					@ $this->_recstats[$rectype][$typeloc]['flags'][$k] ++;
				}
				$bittest = $bittest >> 1;
			}
			
			$this->_set_test_sub_stats($rectype, $typeloc, 'long', $lval);
			// no real point in checking ulongs -> they're all so large that PHP puts them back into negative values
			// long range is sufficient to tell whether negative vals exist
			/*
			if ($lval<0)
				$this->_set_test_sub_stats($rectype, $typeloc, 'ulong', $this->csunpack('L',$test));*/
		
			$this->_set_test_sub_stats($rectype, $typeloc, 'float', $this->csunpack('f',$test));
		
			if ($lval<-1) {
				@ $this->_recstats[$rectype][$typeloc]['formid_bad'] ++;
				continue;
			}
		// allow -1 as no-data flag; don't bother to check low integers
			if ($lval<500)
				continue;
			$formid = $this->process_raw_formid($test);
			$row = NULL;
			if (isset($formid)) {
				$query = "SELECT ordid, rectype from AllItems WHERE formid='$formid'";
				$row = $this->_parentlist->do_query($query, 'onerow');
			}
			if (!empty($row)) {
				$recname = $row['rectype'];
				if (in_array($recname, array('REFR', 'ACHR', 'ACRE'))) {
					$query = "SELECT rectype from {$recname}_Record INNER JOIN AllItems on ({$recname}_Record.base_id=AllItems.ordid) WHERE {$recname}_Record.ordid=".$row['ordid'];
					$srow = $this->_parentlist->do_query($query, 'onerow');
					if (!empty($srow)) {
						$recname .= '-'.$srow['rectype'];
					}
					else {
						print "query = $query\n";
						var_dump($srow);
						exit;
					}
				}
				@ $this->_recstats[$rectype][$typeloc]['rectypes'][$recname] ++;
			}
			else {
				@ $this->_recstats[$rectype][$typeloc]['formid_bad'] ++;
			}
		}
	}
	
	private function _set_test_sub_stats($rectype, $type, $format, $value) {
		if ($format=='float') {
			$fltype = $this->test_float($value);
			@ $this->_recstats[$rectype][$type]['float_'.$fltype] ++;
			if ($fltype=='nan')
				return;
		}
		// PHP weirdness -> results of unpack are stuffed into a long value in PHP, so ulongs come out negative
		if ($format=='ulong' && $value<0) {
			@ $this->_recstats[$rectype][$type]['ulong_neg'] ++;
			return;
		}
		if (!isset($this->_recstats[$rectype][$type][$format.'_min'])) {
			$this->_recstats[$rectype][$type][$format.'_min'] = $value;
			$this->_recstats[$rectype][$type][$format.'_max'] = $value;
		}
		else {
			$this->_recstats[$rectype][$type][$format.'_min'] = min($value, $this->_recstats[$rectype][$type][$format.'_min']);
			$this->_recstats[$rectype][$type][$format.'_max'] = max($value, $this->_recstats[$rectype][$type][$format.'_max']);
		}
	}
	// Function to test for distinctive float values
	// 1.0 in particular, and integer values in general
	function test_float($value) {
		if (is_nan($value))
			return 'nan';
		if (!$value)
			return 'zero';
		if (abs($value)<1.e-4)
			return 'nint';
		if (abs($value)>0.9999 && abs($value)<1.0001)
			return 'one';
		if (abs($value)<1000.) {
			$fmod = fmod(abs($value),1.);
			if ($fmod<0.0001 || $fmod>0.9999)
				return 'int';
		}
		return 'nint';
	}
	
	
	function file_seek($loc) {
		fseek($this->_fpin, $loc);
	}
	
	function file_tell() {
		return ftell($this->_fpin);
	}
	
	function display_loc() {
		$string = ftell($this->_fpin).' of '.$this->_fpsize;
		$string .= sprintf(" (%0.2f", (ftell($this->_fpin)/$this->_fpsize*100));
		$string .= '%)';
		return $string;
	}
	
	function print_reclist() {
		ksort($this->_reclist);
		foreach ($this->_reclist as $id => $data) {
			ksort($data);
			$maxcount = $data[''];
			$totsize = 0;
			foreach ($this->_reclens[$id][''] as $len => $lcount) {
				$totsize += $lcount*$len;
			}
			print "$id $maxcount# sz=$totsize (".sprintf("%.3f", $totsize/$this->_fpsize*100)."%) av=".sprintf("%d", $totsize/$maxcount)."\n";
			foreach ($data as $sub => $count) {
				if ($sub=='')
					continue;
				$minlen = NULL;
				$maxlen = NULL;
				$subsize = 0;
				foreach ($this->_reclens[$id][$sub] as $len => $lcount) {
					$subsize += $lcount*$len;
					if (!isset($minlen)) {
						$minlen = $len;
						$maxlen = $len;
					}
					else {
						$minlen = min($len, $minlen);
						$maxlen = max($len, $maxlen);
					}
				}
				print "  $sub $count# len=$minlen";
				if ($maxlen>$minlen)
					print "-$maxlen";
				if ($subsize>$this->_fpsize*0.0001) {
					print " sz=$subsize (".sprintf("%.3f", $subsize/$this->_fpsize*100)."%)";
				}
				print " min=".$this->_reccount[$id][$sub]['min'];
				print " max=".$this->_reccount[$id][$sub]['min'];
				print "\n";
			}
		}
	}
	/*
	   Internal data setup
	*/
	
	// Placeholder function
	// Initialization that is intended to be done by derived classes
	// Should not contain any processing that requires database or parentlist
	// Basically obsolete -- processing done here was all just basic initialization of protected variables
	protected function _general_setup() {
	}
	
	// Placeholder function: this is main location for game-specific initialization
	// Note that several parts of _datadef will be overwritten during subsequent processing
	// * _datadef_check
	// ** puts 'any' data into each individual rectype
	// ** inserts default values for various settings
	// ** overall validity checks for data
	// _parentlist **must** be setup before this function can be called
	protected function _datadef_setup() {
		$this->_datadef = array();
	}
	
	protected function _read_reclist() {
		$query = 'SELECT rectype, subrec FROM Reclist where subrec!=\'\'';
		$res = $this->_parentlist->do_query($query);
		while ($row=$this->_parentlist->row_query($res)) {
			$this->_reclist[$row['rectype']][$row['subrec']] = 1;
		}
		$query = 'SELECT grouptype, rectype FROM Recgroup';
		$res = $this->_parentlist->do_query($query);
		while ($row=$this->_parentlist->row_query($res)) {
			$this->_recgroup[$row['grouptype']][$row['rectype']] = 1;
		}
	}
	
	protected function _datadef_check($options=NULL) {
		if (!empty($this->_params['_datadef_check_done']))
			return;
		$this->_params['_datadef_check_done'] = TRUE;
		
		if (empty($this->_datadef)) {
			if (!isset($this->_parentlist))
				return;
			print "datadef_check calling datadef_setup\n";
			$this->_datadef_setup();
		}
		
		$setup_datatables = TRUE;
		$debug = TRUE;
		if (isset($options) && is_array($options)) {
			if (isset($options['debug']))
				$debug = $options['debug'];
		}
		
		//		$debug = $this->_doprint;
		// copy 'any' types to each of the individual records, then delete 'any'
		//   means that every subsequent routine doesn't need to include a check for 'any'
		// expand all 'vallist' entries into individual 'val' records
		//   means that several layers of extra treatment for vallist are no longer necessary
		// move all toplevel val/flag/vallist entries into a data record
		//   again, reduces amount of code redundancy necessary later to deal with the two cases separately
		
		// add deletion of keep=-1 records?
		if ($this->_params['_first_db']) {
			// variables used in AllItems table
			$flag_names = $this->_get_rec_flags();
			foreach ($flag_names as $valname) {
				$this->_parentlist->set_datainfo('', $valname, 'sqltable', 'AllItems');
				$this->_parentlist->set_datainfo('', $valname, 'sqlname', $valname);
				$this->_parentlist->set_datainfo('', $valname, 'sqlformat', 'boolean');
			}
			// variable in AllItems to flag which of multiple versions of a formid is one currently
			// in use in game
			// create as full int instead of just boolean to allow it to incorporate
			// mods as well as base game
			$this->_parentlist->set_datainfo('', 'mod_status', 'sqltable', 'AllItems');
			$this->_parentlist->set_datainfo('', 'mod_status', 'sqlname', 'mod_status');
			$this->_parentlist->set_datainfo('', 'mod_status', 'sqlformat', 'int');
			
			if (empty($this->_reclist)) {
				$orig_doprint = $this->_params['doprint'];
				$this->_params['scanrecs'] = TRUE;
				$this->_params['doprint'] = FALSE;
				$this->_params['_datadef_check_active'] = TRUE;
			
			// Need to scan all files to get all possible record types and subrecords
			// It means that counts will be summed over all files
			// Still haven't worked out how to update reclist info, though...
				foreach ($this->_files as $file) {
					$this->read_file($file);
				}
		
				$this->_params['scanrecs'] = FALSE;
				$this->_params['doprint'] = $orig_doprint;
				$this->_params['_datadef_check_active'] = FALSE;
			}
		}
		elseif (empty($this->_reclist)) {
			$this->_read_reclist();
		}
		
		foreach ($this->_datadef as $rectype => $recdata) {
			if ($rectype == 'any')
				continue;
			if ($debug && !empty($this->_params['_first_db']) && !array_key_exists($rectype, $this->_reclist)) {
				print "Warning: data definition provided for non-existent record type {$rectype}\n";
			}
			$anylist = array();
			foreach ($this->_datadef['any'] as $type => $tdata) {
				if (array_key_exists($type, $this->_datadef[$rectype])) {
					if ($debug && !($type=='FNAM' && ($rectype=='INFO' || $rectype=='GLOB'))) {
						print "Warning: {$type} found in record {$rectype} as well as in 'any'\n";
					}
				}
				// only merge types that actually exist for this record
				// (if in cleanup_only-mode, then scan hasn't been done to see what types exist for the record, so merge all types)
				elseif (isset($this->_reclist[$rectype][$type])) {
					$anylist[$type] = $tdata;
				}
				// unless force=1 is set ... in which case still want to check that any parent type exists
				elseif (!empty($tdata['force'])) {
					if (strlen($type)>5) {
						$chktype = substr($type,0,4);
						if (!isset($this->_reclist[$rectype][$chktype]))
							continue;
					}
					$anylist[$type] = $tdata;
				}
			}
			// merge arrays such that 'any' values are the first ones listed
			if (count($anylist)) 
				$this->_datadef[$rectype] = array_merge($anylist, $this->_datadef[$rectype]);
		}
		
		// Need to add minimum subrecords *only* to all rectypes, even those not in datadef
		// (Only necessary on _first_db -- but that's the only situation where _reclist will be set)
		foreach ($this->_reclist as $rectype => $recdata) {
			if (isset($this->_datadef[$rectype]))
				continue;
			foreach ($this->_datadef['any'] as $type => $tdata) {
				if (!isset($recdata[$type]) || $type=='')
					continue;
				if (!isset($tdata['keep']) || $tdata['keep']<2)
					continue;
				$this->_datadef[$rectype][$type] = $tdata;
			}
		}
		unset ($this->_datadef['any']);
		
	// on non-_first_db
	// need to be careful because _datatable and _datainfo have already been set up
	
	// for records that are being kept, should probably wipe out existing info then recreate
	// for records that aren't being kept... do I even want to do processing?
	//  delete those _datadef entries?
	// but where is this processing being done relative to keep settings?
	
	// then need to also keep track of which tables to wipe out, and re-create
	
		$recfound = array();
		foreach ($this->_datadef as $rectype => $recdata) {
			if (isset($this->_reclist) && empty($this->_reclist[$rectype])) {
				print "Warning: data definition provided for non-existent record type $rectype\n";
				unset($this->_datadef[$rectype]);
				continue;
			}
			// Clear out entire datadef if this is a re-read and I'm not keeping this rectype
			if (empty($this->_params['_first_db']) && empty($this->_params['keepall']) && !isset($this->keep[$rectype])) {
				unset($this->_datadef[$rectype]);
				continue;
			}
			if (empty($this->_params['_first_db']) && $setup_datatables) {
				// clear out datatable, datainfo entries
				// leave DB changes (dropping/creating) for when DB processing is done
				$this->_parentlist->clear_datainfo($rectype);
			}

			// check for duplicate names
			// needs to be done here, because later checks will be after many values have been removed
			$valnames = array('rectype' => 1, 'srcfile' => 1);
			foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
				$valnames[$valname][''] = 1;
			}
			$sqltables = array();
			foreach ($recdata as $type => $tdata) {
				$errorstring = " in record {$rectype}, type {$type}\n";
				$alttype = $type;
				if (strlen($type)>4)
					$alttype = substr($type,-4,4);
				
				if (isset($this->_reclist[$rectype]) && !isset($this->_reclist[$rectype][$alttype]) && $alttype!='____') {
					// some records may not be needed by base file, but still available/used in other files (e.g., DELE info)
					// need to figure out how to get around that -- scan each new file once and make sure all
					// rectypes are added to the Reclist table?
					if (empty($tdata['force'])) {
						print "Warning: data definition provided for non-existent record type".$errorstring;
						unset($this->_datadef[$rectype][$type]);
						continue;
					}
				}
				
				$recfound[$rectype][$alttype] = true;
				if (!array_key_exists('keep', $tdata)) {
					if ($debug)
						print "Warning: keep not provided".$errorstring;
					$this->_datadef[$rectype][$type]['keep'] = 0;
				}
				if (!array_key_exists('val', $tdata) && !array_key_exists('data', $tdata) && !array_key_exists('data1', $tdata) && !array_key_exists('flag', $tdata)) {
					if (strlen($type)>4)
						$this->_datadef[$rectype][$type]['val'] = substr($type,-4,4);
					else
						$this->_datadef[$rectype][$type]['val'] = $type;
				}
				if ($debug) {
					if (array_key_exists('val', $tdata) && (array_key_exists('data', $tdata) || array_key_exists('data1', $tdata) || array_key_exists('flag', $tdata))) {
						print "Warning: both val and data/flag specified".$errorstring;
					}
					if (array_key_exists('flag', $tdata) && (array_key_exists('data', $tdata) || array_key_exists('data1', $tdata))) {
						print "Warning: both flag and data specified".$errorstring;
					}
					if (array_key_exists('data', $tdata) && (array_key_exists('data1', $tdata) || array_key_exists('data2', $tdata) || array_key_exists('data3', $tdata))) {
						print "Warning: both data and data1 specified".$errorstring;
					}
					if (array_key_exists('data1', $tdata)!==array_key_exists('data2', $tdata)) {
						print "Warning: only one of data1 and data2 specified".$errorstring;
					}
					if (array_key_exists('data3', $tdata) && !array_key_exists('data2', $tdata)) {
						print "Warning: data3 specified but not data2".$errorstring;
					}
					if (array_key_exists('data1', $tdata)!==array_key_exists('len1', $tdata)) {
						print "Warning: only one of data1 and len1 specified".$errorstring;
					}
					if (array_key_exists('data2', $tdata)!==array_key_exists('len2', $tdata)) {
						print "Warning: only one of data2 and len2 specified".$errorstring;
					}
					if (array_key_exists('data3', $tdata)!==array_key_exists('len3', $tdata)) {
						print "Warning: only one of data3 and len3 specified".$errorstring;
					}
					if (array_key_exists('sqltable', $tdata) && (!array_key_exists('multi', $tdata) || $tdata['multi'] != $type)) {
						print "Warning: sqltable can only be specified for top-level multi records".$errorstring;
					}
					if (array_key_exists('sqltable', $tdata) && !preg_match('/^[A-Z_]{4}$/', $tdata['sqltable'])) {
						print "Warning: sqltable name ".$tdata['sqltable']." expected to be a 4-character all-caps string".$errorstring;
					}
					if (array_key_exists('sqltable', $tdata) && array_key_exists($tdata['sqltable'], $this->_datadef)) {
						print "Warning: sqltable name ".$tdata['sqltable']." matches an existing table name".$errorstring;
					}
					if (array_key_exists('repeat_data', $tdata) && !array_key_exists('multi', $tdata)) {
						print "Warning: repeat_data set but not multi".$errorstring;
					}
				}
				// needs to be set out here so that it gets done even if using keep=2 (and so it applies to all related variables)
				if (array_key_exists('sqltable', $tdata)) {
					$sqltables[$rectype."_".$type] = $this->_datadef[$rectype][$type]['sqltable'];
				}
				
				if (array_key_exists('val', $this->_datadef[$rectype][$type]) || array_key_exists('flag', $this->_datadef[$rectype][$type])) {
					foreach (array('val', 'type', 'len', 'factor', 'flag', 'default', 'lookup', 'lookup_create') as $key) {
						if (!array_key_exists($key, $this->_datadef[$rectype][$type]))
							continue;
						$this->_datadef[$rectype][$type]['data'][0][$key] = $this->_datadef[$rectype][$type][$key];
						unset ($this->_datadef[$rectype][$type][$key]);
					}
				}
				
				foreach ($this->_datadef[$rectype][$type] as $key => $vdata) {
					if (!$debug)
						continue;
					$errorstring = " in record {$rectype}, type {$type}\n";
					if (!preg_match('/^(keep|multi|repeat_data|subset|sqltable|data|len\d|data\d|desc|force)$/', $key)) {
						print "Warning: unrecognized key {$key}".$errorstring;
						continue;
					}
					if (($key=='keep' && $vdata!=-1 && $vdata!=0 && $vdata!=1 && $vdata!=2) ||
					    ($key=='multi' && !array_key_exists($vdata, $recdata)) ||
					    ($key=='type' && !array_key_exists($vdata, $this->_datatype)) ||
					    ($key=='lookup' && !$this->_parentlist->check_lookup($vdata))) {
						    print "Warning: unrecognized value {$vdata} for {$key}".$errorstring;
					    }
					if (preg_match('/^(data|flag)/', $key) && !is_array($vdata)) {
						print "Warning: entry for {$key} is not an array".$errorstring;
					}
				}
				
				foreach (array('data', 'data3', 'data2', 'data1') as $dname) {
					if (!array_key_exists($dname, $this->_datadef[$rectype][$type]))
						continue;
					for ($j=0; $j<count($this->_datadef[$rectype][$type][$dname]); $j++) {
						$xdata = $this->_datadef[$rectype][$type][$dname][$j];
						$errorstring = " for ${dname} value {$j} in record {$rectype}, type {$type}\n";
						if (!is_array($xdata)) {
							print "xdata is not array ".$errorstring;
						}
						if ($debug) {
							if (!array_key_exists('val', $xdata) && !array_key_exists('vallist', $xdata) && !array_key_exists('flag', $xdata)) {
								print "Warning: none of val, vallist, or flag specified".$errorstring;
							}
							if (array_key_exists('val', $xdata) && array_key_exists('vallist', $xdata)) {
								print "Warning: both val and vallist specified".$errorstring;
							}
							if ((array_key_exists('val', $xdata) || array_key_exists('vallist', $xdata)) && array_key_exists('flag', $xdata)) {
								print "Warning: both val and flag specified".$errorstring;
							}
						}
						if (!array_key_exists('type', $xdata)) {
							$this->_datadef[$rectype][$type][$dname][$j]['type'] = 'string';
						}
						if (array_key_exists('vallist', $xdata)) {
							if (!$this->_parentlist->check_lookup($xdata['vallist'])) {
								if ($debug)
									print "Warning: unrecognized value ".$xdata['vallist']." for vallist".$errorstring;
							}
							else {
								$newdata = array();
								$listnames = $this->_parentlist->get_lookup_list($xdata['vallist']);
								if (isset($xdata['valsuffix'])) {
									if (is_array($xdata['valsuffix']))
										$suffixes = $xdata['valsuffix'];
									else
										$suffixes = array($xdata['valsuffix']);
									unset($this->_datadef[$rectype][$type][$key][$j]['valsuffix']);
								}
								else
									$suffixes = array('');
								foreach ($listnames as $name) {
									$num = count($newdata);
									$newdata[$num] = $this->_datadef[$rectype][$type][$key][$j];
									unset ($newdata[$num]['vallist']);
									foreach ($suffixes as $suffix)
										$newdata[$num]['val'] = preg_replace('/\s+/', '_', strtolower($name.$suffix));
								}
								unset ($this->_datadef[$rectype][$type][$key][$j]['vallist']);
								$this->_datadef[$rectype][$type][$key][$j]['val'] = $newdata[0]['val'];
								array_shift($newdata);
								array_splice($this->_datadef[$rectype][$type][$key], $j+1, 0, $newdata);
							}
						}
						$xdata = $this->_datadef[$rectype][$type][$dname][$j];
						
						$namecheck = array();
						if (array_key_exists('val', $xdata)) {
							$namecheck = array($xdata['val']);
						}
						elseif (array_key_exists('flag', $xdata)) {
							$namecheck = $xdata['flag'];
						}
						foreach ($namecheck as $name) {
							if (is_null($name))
								continue;
							// strip leading ! symbol from flag names
							if (array_key_exists('flag', $xdata) && substr($name,0,1)=='!') {
								$name = substr($name,1);
							}
							// convert name to lowercase and spaces to check for similar names, not just identical names
							$altname = preg_replace('/[\s_]+/', ' ', strtolower($name));
							if ($debug && preg_match('/^un(def|know)/i', $altname))
								print "Warning: inappropriate value name {$name}".$errorstring;
							$lookup = NULL;
							if (array_key_exists('lookup', $xdata))
								$lookup = $xdata['lookup'];
							elseif (array_key_exists('lookup_create', $xdata)) {
								$this->_parentlist->set_lookup($xdata['lookup_create'], array());
								$lookup = $xdata['lookup_create'];
							}
							
							// even though sqltable doesn't need to be set here, this processing does need to be here, so that altrectype is set properly
							$altrectype = $rectype;
							$table = $rectype;
							// I don't think this check is being used anywhere
							//if (array_key_exists($table, $sqltables)) {
								//$table = $sqltables[$table];
								//$altrectype = $table;
								//
								//print "changing@1 for $name/$type table to $table and altrectype to $altrectype\n";
							//}
							
							if (array_key_exists('multi', $this->_datadef[$rectype][$type])) {
								$mname = $this->_datadef[$rectype][$type]['multi'];
								// prevent countFRMR from being added to CELL in MWData
								if (!isset($this->_datadef[$rectype][$type]['sqltable'])) {
									$countname = 'count'.substr($mname,-4,4);
									if ($setup_datatables && !isset($valnames[$countname][substr($mname,-4,4)])) {
										$valnames[$countname][substr($mname,-4,4)] = 1;
										if (strlen($mname)==4)
											$basetable = $table;
										else
											$basetable = $table.'_'.substr($mname,-9,4);
										$this->_parentlist->set_datainfo($altrectype, $countname, 'sqltable', $basetable.'_Record');
										$this->_parentlist->set_datainfo($altrectype, $countname, 'sqlname', $countname);
										$this->_parentlist->set_datainfo($altrectype, $countname, 'sqlformat', 'int UNSIGNED');
									}
								}
								if (strlen($mname)>4) {
									$this->_parentlist->set_dataoptions($altrectype, 'complex_index', TRUE);
								}
								$table .= "_".substr($mname,-4,4);
							}
							elseif (strlen($type)>4)
								$table = $rectype."_".substr($type,0,4);
							
							// This is where table actually ends up getting changed for CELL_FRMR -> REFR cases
							// (leaving debug message here just to check that)
							if (array_key_exists($table, $sqltables)) {
								$table = $sqltables[$table];
								$altrectype = $table;
							}
							// needed because some records (e.g., LOCK) by default have reserved names
							$table .= "_Record";
							
							if (!is_null($lookup)) {
								// make sure final name of any lookup variables does NOT have _lu at the end
								$name = preg_replace('/_lu$/', '', $name);
								$altname = preg_replace('/_(id|lu)$/', '', $altname);
								// but then make sure that integer read from input file is saved with _lu at the end
								$this->_datadef[$rectype][$type][$dname][$j]['val'] = $name."_lu";
								if ($setup_datatables)
									$this->_parentlist->set_datainfo($altrectype, $name, 'lookup', $lookup);
							}
							
							if ($setup_datatables) {
								if (array_key_exists('type', $xdata)) {
									if (preg_match('/^mgef(?:id|str)/', $xdata['type']))
										$this->_parentlist->set_datainfo($altrectype, $name, 'format', 'mgefid');
									elseif (preg_match('/id$/', $xdata['type']))
										$this->_parentlist->set_datainfo($altrectype, $name, 'format', $xdata['type']);
								}
								if (array_key_exists('default', $xdata))
									$this->_parentlist->set_datainfo($altrectype, $name, 'default', $xdata['default']);
							}
							
							/* Not how data1/2/3 being used in foread */
							/*if ($dname=='data1') {
								if ($debug && !array_key_exists($altname, $valnames)) {
									print "Warning: value {$name} found in data1 but not in data2".$errorstring;
								}
								continue;
							}*/
							if (($dname=='data2' && array_key_exists('data3', $this->_datadef[$rectype][$type])) || $dname=='data1') {
								if (array_key_exists($altname, $valnames)) {
									// ideally, should check whether the repeat name actually comes from data2/data3
									continue;
								}
							}
							// I don't fully understand this set of conditions any more...
							elseif ($this->_datadef[$rectype][$type]['keep']==2 && $name==$type) {
								continue;
							}
							elseif (array_key_exists($altname, $valnames)) {
								$key = array_pop(array_keys($valnames[$altname]));
								if ($key!=substr($type,-4,4) && $debug)
									print "Warning: multiple instances of value {$name}".$errorstring;
								continue;
							}
							if (!$setup_datatables)
								continue;
							
							$valnames[$altname][substr($type,-4,4)] = 1;
								
							// by default all records except those with keep<0 are read into database
							// also skip those with records keep==2 and a single, self-named variable, because those are being processed and turned into id values
							if ($this->_datadef[$rectype][$type]['keep']>=0) {
								$altname = $name;
									
								$this->_parentlist->set_datainfo($altrectype, $name, 'sqltable', $table);
									
							// can't just do $xdata['lookup']."_lu" because there can be multiple records that access same lookup array
								if (!is_null($lookup))
									$altname = $altname."_lu";
								$altname = preg_replace('/[\s-]/', '_', $altname);
								if (preg_match('/desc|index|each/i', $altname))
									$altname = 'var_'.$altname;
								$this->_parentlist->set_datainfo($altrectype, $name, 'sqlname', $altname);
										
								if (!array_key_exists('type', $xdata)) {
									print "missing type {$name} {$altname} ".$errorstring;
								}
								if (!is_null($lookup))
								    if ($xdata['type'] === 'string')
									$sqlformat = 'char(4)';
								    else
									$sqlformat = 'smallint UNSIGNED';
								elseif (array_key_exists('flag', $xdata))
									$sqlformat = 'boolean';
								elseif (!array_key_exists('factor', $xdata) || $xdata['factor']>=1)
									$sqlformat = $this->_datatype[$xdata['type']]['sql'];
								else
									$sqlformat = "float";
								$this->_parentlist->set_datainfo($altrectype, $name, 'sqlformat', $sqlformat);
							}
						}
						
						foreach ($xdata as $skey => $sdata) {
							if ($debug)
								continue;
							if (!preg_match('/^(val|len|index|type|factor|flag|lookup|lookup_create|default|extra)$/', $skey)) {
								print "Warning: unrecognized key {$skey}".$errorstring;
								continue;
							}
							if (($skey=='type' && !array_key_exists($sdata, $this->_datatype)) ||
							    ($skey=='lookup' && !$this->_parentlist->check_lookup($sdata))) {
								    print "Warning: unrecognized value {$sdata} for {$skey}".$errorstring;
							    }
							if ($skey=='flag' && !is_array($sdata)) {
								print "Warning: entry for {$skey} is not an array".$errorstring;
							}
						}
					}
				}
			}
		}
		
		// This check isn't as useful as it once was --
		// I'm now auto-creating datadefs entries for missing rectype
		// Plus I'm intentionally not creating datadefs for non-interesting rectypes
		// Need to re-make it useful in debug mode...
		if ($debug && $setup_datatables) {
			/*
			foreach ($this->_reclist as $rectype => $rdata) {
				if (!array_key_exists($rectype, $recfound)) {
					print "No data definition found for record {$rectype}\n";
					continue;
				}
				foreach ($rdata as $type => $tdata) {
					if (!array_key_exists($type, $recfound[$rectype])) {
						print "No data definition found for record {$rectype}, type {$type}\n";
					}
				}
			}*/
		}
	
		// If any level list is being read, redo entire calculation
		// It's possible calculation needs redoing other times, too... given amount of recursion in
		// the calculation
		if (!isset($this->_params['calc_levlists'])) {
			$this->_params['calc_levlists'] = FALSE;
			foreach ($this->_parentlist->get_levlist_rectypes() as $rectype => $x) {
				if (isset($this->_datadef[$rectype])) {
					print "setting calc_levlists because of rectype $rectype\n";
					$this->_params['calc_levlists'] = TRUE;
				}
			}
		}
		
		if ($this->_params['_first_db']) {
			$this->request_data('all');
		}
	}
	
	protected function _keep_init($option=0) {
		if (!empty($this->_params['keepall'])) {
			foreach ($this->_datadef as $rectype => $x) {
				if ($rectype!='all')
					$this->_do_keep_init($rectype, TRUE);
			}
		}
		else {
			foreach ($this->keep as $rectype => $keepopt) {
				if (isset($this->_datadef[$rectype]))
					$this->_do_keep_init($rectype, $keepopt);
			}
		}
		
		foreach ($this->keep as $id => $data) {
			if (!array_key_exists($id, $this->_datadef)) {
				unset($this->keep[$id]);
				continue;
			}
			if (!is_array($data)) {
				unset($this->$keep[$id]);
			}
			foreach ($this->keep[$id] as $type => $sdata) {
				if (!isset($this->_datadef[$id][$type]['keep'])) {
					print "Warning: data definitions not available for $id-$type; data skipped\n";
					unset($this->keep[$id][$type]);
				}
				elseif ($this->_datadef[$id][$type]['keep'] < 0) {
					print "Warning: data definitions not available for $id-$type; data skipped\n";
					unset($this->keep[$id][$type]);
				}
			}
		}
	}
	// Initialize the keep array
	// At a minimum, ensures that all entries in keep are valid _datadef entries
			
	// Need to make this work both for calls before datadef_check and calls after datadef_check
	// Needs to also ensure that keepall set true whenever _first_db true
	protected function _do_keep_init($inputid=NULL, $keepopt=NULL) {
		if (is_null($inputid))
			$idlist = $this->keep;
		else
			$idlist = array($inputid => array());
		
		foreach ($idlist as $id => $data) {
			if (!array_key_exists($id, $this->keep) || !is_array($this->keep[$id]))
				$this->keep[$id] = array();
			if (count($this->keep[$id]))
				continue;
			if (!array_key_exists($id, $this->_datadef))
				continue;
			
# eventually deal with overrides to any? set keep = 0?
			foreach ($this->_datadef[$id] as $type => $ddata) {
				if (!is_null($ddata) && array_key_exists('keep', $ddata) &&
				    ($ddata['keep']>0 || ($keepopt && $ddata['keep']>=0))) {
					    $this->keep[$id][$type] = 1;
				    }
			}
		}
	}
	
	function create_table($table) {
		$rectype = substr($table, 0, 4);
			// do I need this.. if so, why isn't it getting filtered out earlier on?
			//			if (!array_key_exists($rectype, $this->keep))
			//	continue;
		$query = "CREATE TABLE {$table} (\n";
		$query .= "ordid int NOT NULL,\n";
		// not doing auto increment because I want to be able to start arrays at zero
		if (strlen($table)>11) {
			$complex = $this->_parentlist->get_dataoptions($rectype, 'complex_index');
			if ($complex)
				// allow IDs as complex as INDX-254-QSDT-521-CTDA-995
				$query .= "subid varchar(26) NOT NULL,\n";
			else
				$query .= "subid int NOT NULL,\n";
		}
		
		$valnames = $this->_parentlist->get_datatables($rectype, $table);
		$ncol = 0;
		foreach ($valnames as $name) {
			$datainfo = $this->_parentlist->get_datainfo($rectype, $name);
			if (!is_array($datainfo) || !array_key_exists('sqlname', $datainfo))
				continue;
			if ($name=='name')
				continue;
			$ncol++;
			
			$newstr = $datainfo['sqlname']." ";
			$newstr .= $datainfo['sqlformat'];
			// Note that binary and blob types do not have any collation
			if (preg_match('/char/', $datainfo['sqlformat']))
				$newstr .= ' collate utf8_unicode_ci ';
			$newstr .= " COMMENT 'variable={$name} ; record={$rectype}";
			foreach (array('format', 'lookup') as $key) {
				if (array_key_exists($key, $datainfo))
					$newstr .= " ; {$key}=".$datainfo[$key];
			}
			$newstr .= "'";
			$query .= $newstr.",\n";
		}
		// Don't create a table that contains no entries
		if (!$ncol)
			return;
		
		$query .= "PRIMARY KEY (ordid";
		if (strlen($table)>11) 
			$query .= ", subid";
		$query .= ")\n";
		$query .= ") DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci";
		$this->_parentlist->do_query($query);
		$this->_parentlist->addtable($table);
	}
	
	// Actual drop/create database commands are handled by csdata
	// don't need to create NAME, FNAM, INTV, ITEX, MODL records for all tables...
	function setup_db() {
		// may need to optimize the keys (merge multiple variables, leftmost variable being one that is always used in query; leftmost should also have the most duplicates)
		// nevertheless, the keys will all be used in many searches
		// also, problems with extra keys (storage space and extra time when updating table) are not issues in this case
		// varbinary to make sure that edid is case sensitive
		if (!$this->_parentlist->hastable('History')) {
			$query = 'CREATE TABLE History (
				hid int UNSIGNED NOT NULL auto_increment,
				setid int UNSIGNED NOT NULL,
				active tinyint,
				time timestamp,
				rectype char(4),
				text blob,
				PRIMARY KEY (hid),
				KEY setid (setid),
				KEY time (time),
				KEY rectype (rectype)
				)';
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('History');
			$this->_params['_history_setid'] = 1;
			$query = "INSERT INTO History SET setid=1, active=1, rectype=NULL, text='Initial database creation'";
			$this->_parentlist->do_query($query);
		}
		else {
			$query = 'SELECT MAX(setid) AS maxid FROM History';
			$row = $this->_parentlist->do_query($query, 'onerow');
			$this->_params['_history_setid'] = $row['maxid']+1;
			$query = "INSERT INTO History SET setid=".$this->_params['_history_setid'].", active=1, rectype=NULL";
			$this->_parentlist->do_query($query);
		}
		
		if ($this->_params['_first_db']) {
			$query = 'CREATE TABLE AllItems (
				ordid int NOT NULL auto_increment,
				edid varbinary(100),
				formid char(10),
				rectype char(4),
				fileid tinyint UNSIGNED,
				name varchar(100),';
			foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
				$datainfo = $this->_parentlist->get_datainfo('', $valname);
				$query .= "\n".$datainfo['sqlname']." ".$datainfo['sqlformat'].",";
			}
			$query .= 'PRIMARY KEY (ordid),
				KEY formid (formid),
				KEY edid (edid),
				KEY rectype (rectype),
				KEY fileid (fileid)
				)';
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('AllItems');
			
			$query = 'CREATE TABLE Files (
				fileid tinyint UNSIGNED NOT NULL,
				filename varchar(50),
				fileofficial varchar(50),
				filenamespace varchar(20),
				filemodname varchar(50),
				filemodabbrev varchar(5),
				PRIMARY KEY (fileid)
				)';
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('Files');
			
			if (!empty($this->_reclist)) {
				$query = 'CREATE TABLE Reclist (
					rectype char(4),
					subrec char(4),
					nsub int,
					avsize float,
					minsize float,
					maxsize float,
					minnum int,
					maxnum int,
					PRIMARY KEY(rectype, subrec)
					)';
				$this->_parentlist->do_query($query);
				$this->_parentlist->addtable('Reclist');
				
				foreach ($this->_reclist as $rectype => $recdata) {
					foreach ($recdata as $subrec => $nsub) {
						// skip WTHR-*IAD subrecs -- register as duplicate keys in Reclist
						if (!preg_match('/^[A-Z0-9_]*$/', $subrec))
							continue;
						$totsize = 0;
						$minsize = NULL;
						$maxsize = NULL;
						foreach ($this->_reclens[$rectype][$subrec] as $size => $num) {
							$totsize += $num*$size;
							if (!isset($minsize)) {
								$minsize = $size;
								$maxsize = $size;
							}
							else {
								$minsize = min($minsize, $size);
								$maxsize = max($maxsize, $size);
							}
						}
						if (!isset($maxsize)) {
							$minsize = 0;
							$maxsize = 0;
						}
						if ($subrec=='') {
							$minnum = 'NULL';
							$maxnum = 'NULL';
						}
						else {
							$minnum = $this->_reccount[$rectype][$subrec]['min'];
							$maxnum = $this->_reccount[$rectype][$subrec]['max'];
						}
						$avsize = $totsize/$nsub;
						$query = "INSERT INTO Reclist SET rectype='$rectype', subrec='$subrec', nsub=$nsub, avsize=$avsize, minsize=$minsize, maxsize=$maxsize, minnum=$minnum, maxnum=$maxnum";
						$this->_parentlist->do_query($query);
					}
				}
				
				$query = 'CREATE TABLE Recgroup (
					grouptype char(4),
					rectype char(4),
					PRIMARY KEY(grouptype, rectype)
					)';
				$this->_parentlist->do_query($query);
				$this->_parentlist->addtable('Recgroup');
				
				foreach ($this->_recgroup as $grouptype => $grdata) {
					foreach ($grdata as $rectype => $x) {
						$query = "INSERT INTO Recgroup SET grouptype='$grouptype', rectype='$rectype'";
						$this->_parentlist->do_query($query);
					}
				}
			}
		}
			
		foreach ($this->_datadef as $rectype => $recdata) {
			$query = "INSERT INTO History SET setid=".$this->_params['_history_setid'].", active=1, rectype='$rectype', text='".addslashes(serialize($recdata))."'";
			$this->_parentlist->do_query($query);
		}
		
		// how do I figure out which lookup tables need to be redone?
		foreach ($this->_parentlist->get_lookup_names() as $lookup) {
			$tname = $lookup."_Lookup";
			if ($this->_parentlist->hastable($tname))
				continue;
			$str = is_string(reset($this->_parentlist->get_lookup_list($lookup)));
			// _lu needs to be larger than tinyint -> ugly arrays such as CTDA_function have more than 255 values
			if ($str)
			    $query = "CREATE TABLE {$tname} (
				{$lookup}_lu char(4) NOT NULL,
				{$lookup} varchar(50),
				PRIMARY KEY ({$lookup}_lu)
				)";
			else
			    $query = "CREATE TABLE {$tname} (
				{$lookup}_lu smallint UNSIGNED NOT NULL,
				{$lookup} varchar(50),
				PRIMARY KEY ({$lookup}_lu)
				)";
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable($tname);
			
			$query = "INSERT INTO {$tname} ({$lookup}_lu, {$lookup}) VALUES ";
			$first = true;
			foreach ($this->_parentlist->get_lookup_list($lookup) as $lu => $luname) {
				if (!$first)
					$query .= ", ";
				else
					$first = false;
				if ($str)
					$query .= "('".addslashes($lu)."'";
				else
					$query .= "({$lu}";
				$query .= ", '".addslashes($luname)."')";
			}
			if (!$first)
				$this->_parentlist->do_query($query);
		}
		
		if (!empty($this->_params['_first_db']) || empty($this->_params['_first_read'])) {
			foreach ($this->_datadef as $rectype => $def) {
				$tables = $this->_parentlist->get_datatables($rectype);
				// kludge for Morrowind to get REFR_Record re-done whenever CELL is re-read
				if ($rectype=='CELL' && substr(get_class($this),0,2)=='MW')
					$tables[] = 'REFR_Record';
				foreach ($tables as $table) {
					$query = "DROP TABLE IF EXISTS $table";
					$this->_parentlist->do_query($query);
					$this->create_table($table);
				}
			}
		}
		/*		$tables = $this->_parentlist->get_datatables();
		foreach ($tables as $table) {
			$this->create_table($table);
		}*/
		
		if ($this->_params['calc_levlists']) {
			$query = "DROP TABLE IF EXISTS LevelList";
			$this->_parentlist->do_query($query);
			$query = "CREATE TABLE LevelList (
				list_ordid int NOT NULL,
				item_ordid int NOT NULL,
				pav float,
				pav_lvl float,
				pmin float,
				pmax float,
				countav float,
				countmin int,
				countmax int,
				lmin int,
				lmax int,
				llast int,
				PRIMARY KEY (list_ordid, item_ordid),
				KEY item_ordid (item_ordid)
				)";
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('LevelList');
		}
		
			/*foreach (array('LevelList', 'LevelList_PLvl', 'LevelList_Src') as $table) {
print "about to drop commented-out tables\n";
exit;
				$query = "DROP TABLE IF EXISTS $table";
				$this->_parentlist->do_query($query);
			}
			$query = "CREATE TABLE LevelList (
				list_ordid int NOT NULL,
				item_ordid int NOT NULL,
				pav float,
				pav_lvl float,
				pmin float,
				pmax float,
				countav float,
				countmin int,
				countmax int,
				lmin int,
				lmax int,
				llast int,
				PRIMARY KEY (list_ordid, item_ordid),
				KEY item_ordid (item_ordid)
				)";
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('LevelList');
			
			$query = "CREATE TABLE LevelList_PLvl (
				list_ordid int NOT NULL,
				item_ordid int NOT NULL,
				level int NOT NULL,
				count int NOT NULL,
				prob float NOT NULL,
				PRIMARY KEY (list_ordid, item_ordid, level, count)
				)";
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('LevelList_PLvl');
			
			$query = "CREATE TABLE LevelList_Src (
				list_ordid int NOT NULL,
				item_ordid int NOT NULL,
				seqid int NOT NULL,
				nseq int NOT NULL,
				src_ordid int NOT NULL,
				PRIMARY KEY (list_ordid, item_ordid, seqid, nseq)
				)";
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('LevelList_Src');
		}*/
	}
	
	/**
	 * Functions to process the raw file data
	 * * Functions that read (or process) data based on a data definition
	 * * Functions that process individual datatypes, converting them from raw binary to proper format
	**/
	
	// Centralized function to take a data definition and do everything to obtain that data
	// Works either with a pre-read string of binary data or by reading directly from input file
	// Can be called recursively
		
	// input
	// * $ddef is the data definition (an array of arrays providing 'val', 'type', etc.)
	// * $input is the previously-read string of binary data
	//   (processed data is removed from $input at end of function, so after call $input is shorter)
	// * $data is primarily the returned data
	//   $data provided as an input parameter primarily so that Count values can be passed in
	function extract_generic_data($ddef, &$input=NULL, $data=NULL) {
		$index = 0;
		$endloc = NULL;
		if (!isset($data))
			$data = array();
		// in case $ddef is just an individual entry
		// normally ddef should just contain numeric keys
		if (isset($ddef['val']) || isset($ddef['type']))
			$ddef = array($ddef);
		
		foreach ($ddef as $entry) {
			if (array_key_exists('extra', $entry))
				continue;
			if (isset($entry['index']))
				$index = $entry['index'];
			
			if (isset($entry['flag']))
				$vallist = $entry['flag'];
			elseif (isset($entry['val']))
				$vallist = array($entry['val']);
			else
				$vallist = array();
			
			$valcount = 1;
			$docount = false;
			if (isset($entry['count'])) {
				$valcount = $entry['count'];
				$docount = true;
			}
			elseif (isset($entry['val']) && isset($data[$entry['val'].'Count'])) {
				$valcount = $data[$entry['val'].'Count'];
				$docount = true;
			}
			
			$len = NULL;
			if (isset($entry['len'])) {
				$len = $entry['len'];
			}
			elseif (isset($entry['type']) && isset($this->_datatype[$entry['type']]['len'])) {
				$len = $this->_datatype[$entry['type']]['len'];
			}
			
			if (!empty($entry['skip']) && isset($len)) {
				$len *= $valcount;
				$index += $len;
				if (!isset($input)) {
					$loc = ftell($this->_fpin)+$len;
					fseek($this->_fpin, $loc);
				}
				continue;
			}
			
			for ($ival=0; $ival<$valcount; $ival++) {
				// for zero-length boolean records
				if (isset($input) && $index==strlen($input) && isset($entry['type']) && substr($entry['type'],0,7)=='boolean') {
					$curr = true;
				}
				else {
					$currstr = NULL;
					if (isset($input)) {
						if (isset($len)) {
							if (($index+$len)>strlen($input))
								break 2;
							$currstr = substr($input,$index,$len);
						}
						else {
							if ($index>strlen($input))
								break 2;
							$currstr = substr($input,$index);
						}
						$prevstr = $currstr;
					}
					
					if (isset($entry['sub'])) {
						$curr = $this->$entry['sub']($entry, $currstr);
					}
					elseif (isset($entry['data'])) {
						$curr = $this->extract_generic_data($entry['data'], $currstr);
					}
					else {
						$curr = $this->extract_single($entry, $currstr);
					}
					
					if (isset($len)) {
						$index += $len;
					}
					elseif (isset($input)) {
						$lendiff = strlen($prevstr)-strlen($currstr);
						if ($lendiff)
							$index += $lendiff;
						else
							$index = strlen($input)+1;
					}
				}
				if (!empty($entry['skip']))
					continue;
				if (!isset($curr))
					continue;
				if ((isset($entry['data']) || isset($entry['sub'])) && empty($vallist) && is_array($curr)) {
					$data = array_merge($data, $curr);
					continue;
				}
				foreach ($vallist as $iflag => $valname) {
					if (is_null($valname))
						continue;
					// _datasize is special keyword for value that provides size of entire remaining datadef
					if ($valname == '_datasize') {
						if (isset($input))
							$endloc = $index + $curr;
						else
							$endloc = ftell($this->_fpin) + $curr;
						continue;
					}
					
					if (isset($entry['flag'])) {
						$currval = (($curr >> $iflag)&1) ? true : false;
						if (substr($valname,0,1)=='!') {
							//name starting with ! is a signal to reverse the flag (save a true value as false, vice versa)
							$valname = substr($valname,1);
							$currval = !$currval;
						}
					}
					else {
						$currval = $curr;
					}
					
					if ($docount)
						$data[$valname][$ival] = $currval;
					else {
						$data[$valname] = $currval;
					}
				}
			}
		}
		
		if (isset($input)) {
			if (isset($endloc))
				$input = substr($input, $endloc);
			else
				$input = substr($input, $index);
		}
		elseif (isset($endloc)) {
			//			print "tell =".ftell($this->_fpin)." seek=$endloc\n";
			fseek($this->_fpin, $endloc);
		}
		return $data;
	}
	
	// Read a single piece of information from binary file OR
	// convert a piece of information that was already read into correctly-formatted data
	// $ddef - an array of specifications for this piece of data
	//            OR can be a string equivalent to $dtype (shortcut option for quick-code)
	//            OR can be an integer to simply read the given number of bytes and return it
	// $input - input string of raw binary data
	//         OR null to directly read correct amount of binary data from the input stream
	
	// Original input arguments were ($curr, $entry, $dtype=NULL) // $entry equiv to $ddef
	// re-arranged so it can more conveniently be used as a one-off function to read data from input file in test code
	function extract_single($ddef, &$input=NULL) {
		$orig_ddef = $ddef;
		if (is_array($ddef)) {
			if (isset($ddef['type']))
				$dtype = $ddef['type'];
			else
				$dtype = 'string';
		}
		elseif (is_int($ddef)) {
			$dtype = 'raw';
			$ddef = array('len' => $ddef);
		}
		else {
			$dtype = $ddef;
			$ddef = array();
		}
		
		if ($dtype=='zstring') {
			// zero-terminated string
			// don't have a pre-determined length
			$curr = '';
			if (isset($input)) {
				$len = NULL;
				for ($j=0; $j<strlen($input); $j++) {
					if (substr($input,$j,1)=="\0") {
						$len = $j;
						break;
					}
				}
				if (!isset($len))
					$len = strlen($input);
			}
			elseif (isset($this->_datachunk)) {
				$len = NULL;
				for ($j=0; $j<strlen($this->_datachunk); $j++) {
					if (substr($this->_datachunk,$j,1)=="\0") {
						$len = $j;
						break;
					}
				}
				if (!isset($len))
					$len = strlen($this->_datachunk);
			}
			else {
				while (!feof($this->_fpin)) {
					$next = fread($this->_fpin,1);
					if ($next=="\0")
						break;
					$curr .= $next;
				}
				return $curr;
			}
		}
		// original code was always reading a byte from fpin to get length (even if using datachunk)
		elseif ($dtype=='string') {
			if (isset($input)) {
				$len = strlen($input);
			}
			elseif (isset($this->_datachunk)) {
				$len = strlen($this->_datachunk);
			}
			else {
				// don't even know why this read is being done... this function probably shouldn't be
				// getting called for strings without length coming directly from fpin
				$char = fread($this->_fpin, 1);
				$len = $this->csunpack('C', $char);
			}
		}
		elseif ($dtype=='bstring') {
			if (isset($input)) {
				$char = substr($input,0,1);
				$input = substr($input,1);
			}
			elseif (isset($this->_datachunk)) {
				$char = substr($this->_datachunk,0,1);
				if (strlen($this->_datachunk)>1)
					$this->_datachunk = substr($this->_datachunk,1);
				else
					$this->_datachunk = NULL;
			}
			else {
				$char = fread($this->_fpin, 1);
			}
			$len = $this->csunpack('C', $char);
		}
		elseif ($dtype=='wstring') {
			if (isset($input)) {
				$char = substr($input,0,2);
				$input = substr($input,2);
			}
			elseif (isset($this->_datachunk)) {
				$char = substr($this->_datachunk,0,2);
				if (strlen($this->_datachunk)>2)
					$this->_datachunk = substr($this->_datachunk,2);
				else
					$this->_datachunk = NULL;
			}
			else {
				$char = fread($this->_fpin, 2);
			}
			$len = $this->csunpack('s', $char);
		}
		elseif (isset($ddef['len']))
			$len = $ddef['len'];
		elseif (isset($this->_datatype[$dtype]['len']))
			$len = $this->_datatype[$dtype]['len'];
		elseif (isset($input))
			$len = strlen($input);
		else {
			// These dies should probably print out some sort of backtrace first
			die("No input data length (ddef=".serialize($orig_ddef).") provided to extract_single\n");
		}
		if ($len<0)
			die("Invalid input data length (ddef=".serialize($orig_ddef).") provided to extract_single\n");
		if ($len==0)
			return NULL;
		
		if (isset($input)) {
			$curr = substr($input, 0, $len);
			$input = substr($input, $len);
		}
		elseif (isset($this->_datachunk)) {
			$curr = substr($this->_datachunk, 0, $len);
			if (strlen($this->_datachunk)>$len)
				$this->_datachunk = substr($this->_datachunk, $len);
			else
				$this->_datachunk = NULL;
		}
		else {
			$curr = fread($this->_fpin, $len);
		}
		
		if ($dtype=='raw')
			return $curr;
		
		if (!array_key_exists($dtype, $this->_datatype)) {
			// do this check after curr/len checks -- so that if possible the correct amount of data is
			// removed from input stream
			print "Input data type not recognized (dtype=$dtype); data skipped\n";
			return NULL;
		}
			
		if (array_key_exists('code', $this->_datatype[$dtype])) {
			$curr = $this->csunpack($this->_datatype[$dtype]['code'], $curr);
		}
		else if (array_key_exists('sub', $this->_datatype[$dtype])) {
			if ($this->_datatype[$dtype]['sub']==3 || !method_exists($this, $this->_datatype[$dtype]['sub'])) {
				print "dtype = $dtype\n";
				var_dump($this->_datatype);
				var_dump($orig_ddef);
			}
			$curr = $this->{$this->_datatype[$dtype]['sub']}($curr);
		}
		
		if (isset($ddef)) {
			if (array_key_exists('factor', $ddef))
				$curr *= $ddef['factor'];
			if (array_key_exists('lookup_create', $ddef))
				$curr = $this->_parentlist->get_lookup_id($ddef['lookup_create'], $curr, true, true);
			// transform -1 values for skill_lu and other out-of-range values into NULLs right away
			// **BUT** anticipating reading in Skyrim with incomplete lookup tables...
			// do not want to throw away legit values just because the lookup table needs to be fixed
			elseif (array_key_exists('lookup', $ddef)) {
				if ($curr<0 || $curr>1000)
					//				if (is_null($this->_parentlist->get_lookup($ddef['lookup'], $curr)))
					$curr = NULL;
			}
			// actually convert values into strings (generally not desirable, given much larger size)
			elseif (array_key_exists('lookup_conv', $ddef)) {
				$curr = $this->_parentlist->get_lookup($ddef['lookup_conv'], $curr);
			}
		}
		return $curr;
	}
	
	function process_boolean($input) {
		if (!strlen($input)) 
			$value = TRUE;
		elseif (strlen($input<2))
			$value = $this->csunpack("C", $input);
		elseif (strlen($input<4))
			$value = $this->csunpack("S", $input);
		else
			$value = $this->csunpack("L", $input);
		if ($value)
			return TRUE;
		else
			return FALSE;
	}
	
	function process_rev_boolean($input) {
		$bool = $this->process_boolean($input);
		return (!$bool);
	}
	
	function process_raw_mgefid($input) {
		if (strlen($input)<4)
			$index = $this->csunpack("S", $input);
		else
			$index = $this->csunpack("L", $input);
		if ($index<0)
			return NULL;
		$out = "MGEF".sprintf("%03d", $index);
		return $out;
	}

	// Function to match an id found in a subrec to an AllItems record
	// Looks for an existing record, if not creates a dummy record
	// Normally called without $rectype -- only exception at this point is DIAL lookups

	// Originally, this function always returned the formid corresponding to fileid=0
	// Now, returns the default (mod_status&0x2) or mod-specific (mod_status&0x4) version of formid
	function _lookup_ordid($id, $rectype=NULL) {
		if (empty($id) || $id===-1 || $id==='0xffffffff' || $id==='0x00000000')
			return NULL;

		// get_ordid will automatically check against current mod or base version as appropriate
		if (isset($this->_parentlist) && isset($this->_filemodnum))
			$modabbrev = $this->_parentlist->get_file_abbrev($this->_filemodnum);
		else
			$modabbrev = NULL;
		$ordid = $this->_parentlist->get_ordid($id, $modabbrev, $rectype);

		// if no matching record found AND if we're reading a file for the first time, then create a placeholder record
		// (on re-reads, all records have already been created, so no chance that placeholder will subsequently be turned into proper record)
		if (empty($ordid) && !empty($this->_params['_first_read'])) {
			$ordid = $this->_parentlist->create_placeholder_ordid($id, $rectype);
		}

		return $ordid;
	}
	
	function process_mgefid($input) {
		if (is_null($index = $this->process_raw_mgefid($input)))
			return NULL;
		$ordid = $this->_lookup_ordid($index);
		if (!is_null($ordid))
			return $ordid;
		return $index;
	}
	
	function process_mgefstr($input) {
		$ordid = $this->_lookup_ordid($input);
		if (!is_null($ordid))
			return $ordid;
		return $input;
	}
	
	function process_float($input) {
		$out = floor($this->csunpack("f",$input)*10000+0.5)/10000;
		return $out;
	}
	
	function process_raw_formid($input) {
		if (is_string($input))
			$raw = $this->csunpack("L", $input);
		else
			$raw = $input;
		
		if ($raw<=0)
			return NULL;
		// although I'm now saving fileid and using unique ordids to allow multiple records with the same formid,
		// this procedure is comparable to what's done in game when multiple mods are loaded and it limits the
		// possibility of mod records getting accidentally cross linked
		$main = $raw % 0x01000000;
		
		$pre = ($raw-$main)/0x01000000;
		if (isset($this->_filemodnum)) {
			// Skyrim Dawnguard's formids are all 0x02 values... are formids for mods being handled differently in Skyrim?
			// Creation Kit states that 0x01 is still changed based on load order
			// BUT Dawnguard uses 0x01 for changes to Update.esm; 0x02 for actual Dawnguard content
			// FOR NOW: only change 0x02 to filemodnum.  Probably need to read the .esm/.esp file's list of dependencies
			// Each dependency probably gets a number assigned, and then next available number is used for this file's content
//			if ($pre == 0x01 && $this->_filemodnum) {
//				$pre *= $this->_filemodnum;
//			if ($pre == 0x02 && $this->_filemodnum) {
//				$pre = $this->_filemodnum;
	// Just specify the prefix manually for each file for now
			if ($this->_filemodnum && $pre == $this->_fileidprefix[$this->_filemodnum]) {
				$pre = $this->_filemodnum;
			}
		}
		elseif ($pre) {
			// translate prefix coming from a save file into the correct prefix for the DB
			$orig = $pre;
			if (isset($this->_modids[$pre]))
				$pre = $this->_modids[$pre];
			
			//			print "input=$input, orig=$orig, pre=$pre, main=".sprintf("%06lx",$main)."\n";
		}
		
		$raw = $pre*0x01000000+$main;
		$out = "0x".sprintf("%08lx",$raw);
		return $out;
	}
	
	function process_formid($input) {
		if (strlen($input)) {
			$formid = $this->process_raw_formid($input);
			if (!isset($formid))
				return NULL;
			$ordid = $this->_lookup_ordid($formid);
			if (!is_null($ordid))
				return $ordid;
			else
				return $formid;
		}
		else {
			return $input;
		}
	}
	
	function process_bytes($input) {
		$return = '';
		for ($l=0; $l<strlen($input); $l++) {
			if ($return!='')
				$return .= '.';
			$return .= sprintf("%02x", $this->csunpack("C",substr($input,$l,1)));
		}
		return $return;
	}

	function process_str_index($input) {
		$index = $this->csunpack("L", $input);
		if (empty($index))
			return NULL;

		// Needs to be able to handle both mod and original strings files
		// But also once I've got a multitude of possible files, keeping all fp's open could be problematic
		// Probably need to have an array of stringfiles -> containing filename, fp(if open), and filetype
		// Only keep current set of files open

		if (!isset($this->_stringindex) || !isset($this->_stringindex['openset'][$this->_fileid])) {
			if (!isset($this->_stringindex))
				$this->_stringindex = array('file' => array(), 'loc' => array(), 'openset' => array(), 'filedata' => array());

			for ($ifile=0; $ifile<=$this->_fileid; $ifile++) {
				if (isset($this->_stringindex['openset'][$ifile]))
					continue;
				$this->_stringindex['openset'][$ifile] = TRUE;
				for ($nfile=0; $nfile<3; $nfile++) {
					$fileindex = $ifile*10 + $nfile;
					$this->_stringindex['filedata'][$fileindex] = array();

					$filename = $this->_params['inputdir'].'strings/'.preg_replace('/\.[^\.]+$/', '', $this->_files[$ifile]).'_English.';
					if ($nfile==1)
						$filename .= 'DL';
					elseif ($nfile==2)
						$filename .= 'IL';
					$filename .= 'STRINGS';
					if (!file_exists($filename))
						continue;

					$this->_stringindex['filedata'][$fileindex]['filename'] = $filename;
					$fp = fopen($filename, 'r');
					$count = $this->csunpack('l', fread($fp, 4));
					$totsize = $this->csunpack('l', fread($fp, 4));
					for ($c=0; $c<$count; $c++) {
						$cindex = $this->csunpack('l', fread($fp, 4));
						$cloc = $this->csunpack('l', fread($fp, 4));
						$cloc += ($count+1)*8;
							
						$this->_stringindex['loc'][$cindex] = $cloc;
						$this->_stringindex['file'][$cindex] = $fileindex;
					}
					fclose($fp);
				}
			}
			
			for ($nfile=0; $nfile<3; $nfile++) {
				$open = NULL;
				for ($ifile=$this->_fileid; $ifile>=0; $ifile--) {
					$fileindex = $ifile*10 + $nfile;
					if (!isset($this->_stringindex[$fileindex]['filename']))
						continue;
					if (!isset($open)) {
						$open = $ifile;
						if (!isset($this->_stringindex[$fileindex]['fp']))
							$this->_stringindex['filedata'][$fileindex]['fp'] = fopen($this->_stringindex['filedata'][$fileindex]['filename'], 'r');
					}
					else {
						if (isset($this->_stringindex['filedata'][$fileindex]['fp'])) {
							fclose($this->_stringindex['filedata'][$fileindex]['fp']);
							$this->_stringindex['filedata'][$fileindex]['fp'] = NULL;
						}
					}
				}
			}
		}

		if (!isset($this->_stringindex['loc'][$index]))
			return NULL;

		$loc = $this->_stringindex['loc'][$index];
		$file = $this->_stringindex['file'][$index];
		$opened = FALSE;
		if (!isset($this->_stringindex['filedata'][$file]['fp'])) {
			$opened = TRUE;
			$this->_stringindex['filedata'][$file]['fp'] = fopen($this->_stringindex['filedata'][$file]['filename'], 'r');
		}
		$fp = $this->_stringindex['filedata'][$file]['fp'];
		fseek($fp, $loc);
		if (($file%10)>0) {
			$size = $this->csunpack('l', fread($fp, 4));
			$input = fread($fp, $size);
		}
		else {
			$input = '';
			while (!feof($fp)) {
				$char = fread($fp, 1);
				if ($char==="\0")
					break;
				$input .= $char;
			}
		}

		if ($opened) {
			fclose($this->_stringindex['filedata'][$file]['fp']);
			$this->_stringindex['filedata'][$file]['fp'] = NULL;
		}

// process_string strips any trailing "\0" (can still be present for file=1/file=2
// and does UTF-8 conversion
		return $this->process_string($input);
	}
	
	function process_string($input) {
		for ($l=0; $l<strlen($input); $l++) {
			if (substr($input, $l, 1) == "\0") {
				$input = substr($input, 0, $l);
				break;
			}
		}
$orig = $input;
		// I'm assuming that tests are being done elsewhere to confirm that input strings are all Windows-1252
		$input = iconv('Windows-1252', 'UTF-8', $input);
if ($input===FALSE) {
print "orig = $orig\n";
exit;
}
#	while (length($data) && substr($data,length($data)-1,1) eq "\0") {
#	    chop($data);
#	}
		return $input;
	}
	
	function process_edid($input) {
		$input = $this->process_string($input);
		if (strlen($input)) {
			$ordid = $this->_lookup_ordid($input);
			if (!is_null($ordid))
				return $ordid;
		}
		return $input;
	}
	
	function process_infoid($input) {
		$input = $this->process_string($input);
		//		if (strlen($input)) {
		//			$input = "DIAL".$this->_groupordid[$this->_ngroup-1]."_INFO".$input;
		//			$ordid = $this->_lookup_ordid($input);
		//			if (!is_null($ordid))
		//				return $ordid;
		//		}
		return $input;
	}
	
	function process_dialid($input) {
		$input = $this->process_string($input);
		if (strlen($input)) {
			$ordid = $this->_lookup_ordid($input, 'DIAL');
			if (!is_null($ordid))
				return $ordid;
		}
		return $input;
	}
	// custom function for GLOB-FLTV variables
	// it is always a float variable, regardless of value of GLOB-FNAM
	// but keep custom function because some of the valuables are just bizarre (NaN, super-large, etc)
	function process_glob($input) {
		//		$format = $this->_parentlist->get_active('FNAM');
		//		if ($format=='s')
		//			$value = $this->csunpack("S", $input);
		//		elseif ($format=='l')
		//			$value = $this->csunpack("L", $input);
		//		elseif ($format=='f')
		//			$value = $this->process_float($input);
		//		else {
		//			print "set glob error: format={$format}, input={$input}\n";
		//			return NULL;
		//		}
		$value = $this->process_float($input);
		if (is_nan($value) || $value>1.e10)
			$value = NULL;
		return $value;
	}
	
	function process_gmst($input) {
		$format = substr($this->_parentlist->get_active('edid'), 0, 1);
		if ($format=='s')
			return $this->process_string($input);
		elseif ($format=='f')
			return $this->process_float($input);
		elseif ($format=='i') {
			if (strlen($input)==2)
				return $this->csunpack("s", $input);
			else
				return $this->csunpack("l", $input);
		}
		return $input;
	}
	
// Skyrim-style gmst records -- where strings come from a separate string file
	function process_gmst4($input) {
		$format = substr($this->_parentlist->get_active('edid'), 0, 1);
		if ($format=='s')
			return $this->process_str_index($input);
		elseif ($format=='f')
			return $this->process_float($input);
		elseif ($format=='i')
			return $this->csunpack("l", $input);
	}
	function process_PACK_CNAM($input) {
		$index = $this->_parentlist->get_active_index('ANAM');
		$anam = $this->_parentlist->get_active('activity_type', $index);
		if ($anam=='Bool') {
			return $this->extract_single('boolean', $input);
		}
		elseif ($anam=='Float' || $anam=='ObjectList') {
			return $this->extract_single('float', $input);
		}
		elseif ($anam=='Int') {
			return $this->extract_single('long', $input);
		}
	}
	function process_PACK_UNAM($input) {
		$value = $this->extract_single('byte', $input);

		$xnam = $this->_parentlist->get_active('XNAM');
// Artificially move first set of UNAM fields into previous ANAM entries
		if (!isset($xnam)) {
			$index_num = $this->_parentlist->get_active('fake_anam');
			if (!$index_num)
				$index_num = '0';
			$index = 'ANAM-'.$index_num;
			$this->_parentlist->set_active($value, 'activity_name_ref', $index);
			$index_num++;
			$this->_parentlist->set_active($index_num, 'fake_anam');
			$this->_parentlist->reset_active_index('UNAM');
			return NULL;
		}
		return $value;
	}
	// Copy current activity_type to CITC to start a new multi-value section
	function process_PACK_CITC($input) {
		$anam_index = $this->_parentlist->get_active_index('ANAM');
		$anam = $this->_parentlist->get_active('activity_type', $anam_index);
		$citc_index = $this->_parentlist->get_active_index('CITC');
		$this->_parentlist->set_active($anam, 'activity_type_ref', $citc_index);

		$value = $this->extract_single('ulong', $input);
		return $value;
	}

	// It would be nice to do a simplified check here -- assume that values<100 or going to be numbers,
	// >100 are going to be formids.  Simplified check might also be more robust for Skyrim, where
	// type values could be redefined
	// *BUT* includes formid=0x00000014 = 20, which I is the formid of the player's ACHR ref
	// Also note that I can't do a lookup of formid, because formid might not exist yet
// Re: above: simplified check ultimately isn't working well for CTDA stuff

// These functions work significantly differently in SR than in OB!!

// Overridden in SR by SR-specific function in srread.inc
	function process_packid_PLDT($input) {
		$type = $this->csunpack("L",substr($input,0,4));
		$this->_parentlist->set_active($type, 'PLDT_type_lu');
		return $this->process_packid(substr($input,4,4), ($type>=5 ? FALSE : TRUE));
	}
// Not present in SR
	function process_packid_PTDT($input) {
		$type = $this->csunpack("L",substr($input,0,4));
		$this->_parentlist->set_active($type, 'PTDT_type_lu');
		return $this->process_packid(substr($input,4,4), ($type>=2 ? FALSE : TRUE));
	}
// Only present in SR
	function process_packid_PTDA($input) {
		$index = $this->_parentlist->get_active_index('ANAM');
		$type = $this->csunpack("L",substr($input,0,4));

		if (in_array($type, array(0,1,3))) {
			$ordid = $this->process_packid(substr($input,4,4), TRUE);
			if (isset($ordid))
				$this->_parentlist->set_active($ordid, 'activity_formid', $index);
		}
		else {
			$raw = $this->csunpack('L', substr($input,4,4));
			if (isset($raw))
				$this->_parentlist->set_active($raw, 'activity_value', $index);
		}
		return $type;
	}
// Only present in SR
	function process_packid_PDTO($input) {
		$index = $this->_parentlist->get_active_index('ANAM');
		$type = $this->csunpack("L",substr($input,0,4));

		if ($type==0) {
			$ordid = $this->process_packid(substr($input,4,4), TRUE);
			if (isset($ordid))
				$this->_parentlist->set_active($ordid, 'activity_formid', $index);
			return NULL;
		}
		else {
			return substr($input,4,4);
		}
	}
	function process_packid($input, $is_formid=TRUE) {
		$raw = $this->csunpack("L", $input);
		
		if ($raw<0)
			return NULL;
		elseif (!$is_formid) {
			if (empty($raw))
				return 0;
			else
				return -1*$raw;
		}
		else {
			if (empty($raw))
				return NULL;
			$formid = $this->process_raw_formid($input);
			if (!isset($formid))
				return NULL;
			$ordid = $this->_lookup_ordid($formid);
			if (!is_null($ordid))
				return $ordid;
			else
				return NULL;
		}
	}
	
	// decode this structure later
	function process_systemtime($input) {
		$valnames = array('wYear', 'wMonth', 'wDayOfWeek', 'wDay', 'wHour', 'wMinute', 'wSecond', 'wMilliseconds');
		$vals = array();
		for ($i=0; $i<count($valnames); $i++) {
			$substr = substr($input,$i*2,2);
			$vals[$valnames[$i]] = $this->csunpack("s", $substr);
		}
		// ignoring milliseconds
		$ts = mktime($vals['wHour'], $vals['wMinute'], $vals['wSecond'], $vals['wMonth'], $vals['wDay'], $vals['wYear']-1900);
		return strftime("%D %R", $ts);
			//		return $ts;
	}
	
	function process_lvld($input) {
		$value = $this->csunpack("C", $input);
		// explicitly set other values -- these will be overridden if there is a LVLF subrecord
		// note that useall is only necessary for LVSP records; other records will just ignore/drop the data
		$this->_parentlist->set_active((($value & 128) ? true : false), 'alllvl');
		$this->_parentlist->set_active(false, 'each');
		$this->_parentlist->set_active(false, 'useall');
		return $value % 128;
	}

	function process_vmad_property($version, $format, &$input, &$propname) {
		$rectype = $this->_parentlist->get_active('rectype');

		$propname = $this->extract_single('wstring', $input);

		$otype = $type = $this->extract_single('byte', $input);
		if ($version>=4)
			$unk3 = $this->extract_single('byte', $input);
		else
			$unk3 = 1;
			
		if ($version>=5 && $type>10 && $type<16) {
			$arraycount = $this->extract_single('long', $input);
			$type -= 10;
		}
		else {
			$arraycount = 1;
		}
			
		$values = array();
		for ($array=0; $array<$arraycount; $array++) {
			$value = NULL;
			if ($type==1) {
				if ($format==1) {
					$id = sprintf('0x%08lx', $this->extract_single('long', $input));
					$index = $this->extract_single('short', $input);
					$zero = $this->extract_single('short', $input);
				}
				else {
					$zero = $this->extract_single('short', $input);
					$index = $this->extract_single('short', $input);
					$id = sprintf('0x%08lx', $this->extract_single('long', $input));
				}
				$edid = $this->_parentlist->get_edid($id);
				$ordid = $this->_parentlist->get_ordid($id);
				$value = "$id $index";

if ($zero!=0) {
//else {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "non-zero sub0 for alias reference record propname=$propname arraycount=$arraycount format=$format zero=$zero array=$array value=$value edid=$edid\n";
}
elseif ($index>=0 && $this->_parentlist->get_rectype($ordid)!='QUST') {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "non-QUST rectype for index>=0 rectype=".$this->_parentlist->get_rectype($ordid)." propname=$propname arraycount=$arraycount format=$format zero=$zero array=$array value=$value edid=$edid\n";
}			}
			elseif ($type==2) {
				$value = $this->extract_single('wstring', $input);
			}
			elseif ($type==3) {
				$value = $this->extract_single('long', $input);
			}
			elseif ($type==4) {
				$value = $this->extract_single('float', $input);
			}
			elseif ($type==5) {
				$value = $this->extract_single('boolean', $input);
			}
			else {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "Unknown property type $type\n";
}
			if (isset($value))
				$values[] = $value;
		}
		return $values;
	}
	
	// This function is able to parse an entire VMAD record
	// *BUT* all it is doing right now is dumping info to the screen
	// Still need to set up how/where to save info to DB
	// Definitely need VMAD to be multi'd in srread.inc... but is having a single VMAD table enough?
	// 
// Can't properly store data at this point, because VMAD is always second field (after EDID) -- rest of quest info has not yet been processed
// So can't save alias value with alias, or stage scripts with stage

// tested so far: NPC_, WEAP, MGEF, FLOR, KEYM, MISC, ACTI, PERK, FURN, ACTI, BOOK, ARMO, INGR, DOOR, CONT,.. REFR, ACHR
// extra for PERK, PACK
// need to work out INFO, SCEN
	function process_vmad($input) {
			// * $ddef is the data definition (an array of arrays providing 'val', 'type', etc.)
	// * $input is the previously-read string of binary data
	//   (processed data is removed from $input at end of function, so after call $input is shorter)
	// * $data is primarily the returned data
	//   $data provided as an input parameter primarily so that Count values can be passed in
		//		function extract_generic_data($ddef, &$input=NULL, $data=NULL) {

		$rectype = $this->_parentlist->get_active('rectype');
		$version = $this->extract_single('short', $input);

		$format = $this->extract_single('short', $input);
if ($format!=1 && $format!=2) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "unexpected format value format=$format\n";
}
		$nfile = $this->extract_single('short', $input);		
		for ($nf=0; $nf<$nfile; $nf++) {
			$mainfile = $this->extract_single('wstring', $input);
			$query = 'SELECT sid FROM scripts WHERE filename=\''.addslashes($mainfile).'\' AND mod_status&0x02>0';
			$sidrow = $this->_parentlist->do_query($query, 'onerow');
			if (isset($sidrow['sid']))
				$sid = $sidrow['sid'];
			else
				$sid = NULL;
			if (isset($sid)) {
				$this->_parentlist->set_active('', 'VSCR', 'VMAD-0-VSCR-'.$nf);
				$this->_parentlist->set_active($sid, 'sid', 'VMAD-0-VSCR-'.$nf);
			}

			if ($version>=4)
				$unk2 = $this->extract_single('byte', $input);
			else
				$unk2 = 0;

			$propcount = $this->extract_single('short', $input);
			for ($prop=0; $prop<$propcount; $prop++) {
				$mainprop = $this->process_vmad_property($version, $format, $input, $propname);
				if (isset($sid) && !empty($propname)) {
					$this->_parentlist->set_active('', 'VPRP', 'VMAD-0-VSCR-'.$nf.'-VPRP-'.$prop);
					$this->_parentlist->set_active($propname, 'propname', 'VMAD-0-VSCR-'.$nf.'-VPRP-'.$prop);
					$this->_parentlist->set_active(serialize($mainprop), 'propvalue', 'VMAD-0-VSCR-'.$nf.'-VPRP-'.$prop);
				}
			}
		}

		if (empty($input))
			return $version;
		$this->_parentlist->set_active(TRUE, 'has_vmad_extra');
if ($rectype!='QUST') {
return $version;
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "non-empty extra data for non-QUST record input.len=".strlen($input)."\n";
print $this->display_raw_data($input, TRUE);
return $version;
}

		$unk4 = $this->extract_single('byte', $input);
		if ($rectype=='QUST') {
			$nstage = $this->extract_single('short', $input);
		}
		elseif ($rectype=='PACK' || $rectype=='SCEN') {
			$nstage = 0;
			$bits = $nunkx = $this->extract_single('byte', $input);
			for ($nstage = 0; $bits; $bits >>= 1) {
				$nstage += ($bits&1);
			}
		}
		$stage_filename = $this->extract_single('wstring', $input);
		$query = 'SELECT sid, contents FROM scripts WHERE filename=\''.addslashes($stage_filename).'\' AND mod_status&0x02>0';
		$sidrow = $this->_parentlist->do_query($query, 'onerow');
		if (isset($sidrow['sid'])) {
			$this->_parentlist->set_active($sidrow['sid'], 'fragment_sid');
			$fragment_file = $sidrow['contents'];
		}
		else {
			$fragment_file = NULL;
		}
		if ($rectype=='PERK') {
			$nstage = $this->extract_single('short', $input);
		}

//print "stage_filename=$stage_filename nstage=$nstage nunkx=$nunkx\n";
		for ($ns=0; $ns<$nstage; $ns++) {
			if ($rectype=='QUST' || $rectype=='PERK')
				$index = $this->extract_single('short', $input);
			else
				$index = $ns;
			$unks = array();
			if ($rectype=='QUST')
				$nunks = 3;
			elseif ($rectype=='PERK')
				$nunks = 1;
			elseif ($rectype=='PACK' || $rectype=='SCEN')
				$nunks = 0;
			$unks = array();
			for ($ui=0; $ui<$nunks; $ui++) {
				$unks[$ui] = $this->extract_single('short', $input);
			}
			$unk5 = $this->extract_single('byte', $input);
			$scripts = $this->extract_single('wstring', $input);
			$fragments = $this->extract_single('wstring', $input);

//			$this->_parentlist->set_active($fragments, 'fragment_name', $index);
			if (isset($fragment_file) && preg_match('/Fragment_(\d+)$/', $fragments, $matches)) {
				$fragment_num = $matches[1];
				$this->_parentlist->set_active($fragment_num, 'raw_fragment_num', $index);
				$frag_str = ';BEGIN FRAGMENT '.$fragments;
				$found = FALSE;
				$offset = 0;
				while (!$found) {
					$fragment_loc = strpos($fragment_file, $frag_str, $offset);
					if ($fragment_loc!==FALSE) {
						$fragment_loc += strlen($frag_str);
						$next_char = substr($fragment_file, $fragment_loc, 1);
						if (preg_match('/\d/', $next_char)) {
							$offset = $fragment_loc;
							continue;
						}
						$found = TRUE;
						$fragment_loc++;
						$this->_parentlist->set_active($fragment_loc, 'raw_fragment_loc', $index);
						$fragment_len = strpos(substr($fragment_file,$fragment_loc),';END FRAGMENT');
						if ($fragment_len!==FALSE) {
							$this->_parentlist->set_active($fragment_len, 'raw_fragment_len', $index);
						}
						break;
					}
					else {
						break;
					}
				}
			}
			
//print "unk5=$unk5, script=".$scripts." frag=".$fragments."\n";
if ($scripts!=$stage_filename) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "discrepancy for script name ns=$ns index=".$indices." script=".$scripts." stage=".$stage_filename."\n";
}
if (!preg_match('/^Fragment_\d+$/', $fragments)) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "strange fragment name script name ns=$ns index=".$indices." fragment=".$fragments."\n";
}
		}
if ($rectype=='QUST') {
		$nalias=$this->extract_single('short', $input);

		for ($na=0; $na<$nalias; $na++) {
			$unks = array();
			for ($ui=0; $ui<7; $ui++) {
				$unks[$ui] = $this->extract_single('short', $input);
			}
			$aliasid = $unks[1];
			$nfile=$unks[6];
			$propformat = $unks[5];
if ($propformat!=1 && $propformat!=2) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "unexpected propformat value na=$na propformat=$propformat\n";
}
if ($propformat!=$format) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "propformat diff from format value na=$na propformat=$propformat format=$format\n";
}
			for ($nf=0; $nf<$nfile; $nf++) {
				$aliasfile = $this->extract_single('wstring', $input);
				$query = 'SELECT sid FROM scripts WHERE filename=\''.addslashes($aliasfile).'\' AND mod_status&0x02>0';
				$sidrow = $this->_parentlist->do_query($query, 'onerow');
				if (isset($sidrow['sid']))
					$sid = $sidrow['sid'];
				else
					$sid = NULL;
				if (isset($sid))
					$this->_parentlist->set_active($sid, 'alias_sid', 'alias-'.$aliasid.'-VSCR-'.$nf);

				if ($version>=4)
					$unk2 = $this->extract_single('byte', $input);
				else
					$unk2 = 0;

				$propcount = $this->extract_single('short', $input);
				for ($prop=0; $prop<$propcount; $prop++) {
					$aliasprop = $this->process_vmad_property($version, $propformat, $input, $propname);
					if (isset($sid) && !empty($propname)) {
						$this->_parentlist->set_active($propname, 'alias_propname', 'alias-'.$aliasid.'-VSCR-'.$nf.'-VPRP-'.$prop);
						$this->_parentlist->set_active(serialize($aliasprop), 'alias_propvalue', 'alias-'.$aliasid.'-VSCR-'.$nf.'-VPRP-'.$prop);
					}
				}
			}
		}
}
	return $version;

if (!empty($input)) {
print $this->_parentlist->get_active('edid').' '.$this->_parentlist->get_active('formid').' '.$this->_parentlist->get_name('name')."\n";
print "Remaining input: ".strlen($input)."\n";
print $this->display_raw_data($input, TRUE);
}
	}
	
	/*
	Post-read processing when mode is DATAMODE_WRITE
	*/
	// Any special cleanup after the DB has been read for the first time
	// So far, mainly done to take care of 0x14 record for PlayerRef
	// Also adds index to AllItems.name; fixes record counts in Reclist
	protected function _finish_first_db() {
		$item7 = $this->_parentlist->get_item('0x00000007', 0, 'NPC_', TRUE);
		if (!empty($item7)) {
			// Note this 0x14 (which is decimal 20)
			$item14 = $this->_parentlist->get_item('0x00000014', 0);
			$ordid = NULL;
			if (empty($item14)) {
				$query = "INSERT INTO AllItems SET formid='0x00000014', rectype='ACHR', edid='PlayerRef'";
				$ordid = $this->_parentlist->do_query($query, 'getid');
			}
			elseif (is_null($item14->get('rectype'))) {
				$ordid = $item14->get('ordid');
				$query = "UPDATE AllItems SET rectype='ACHR', edid='PlayerRef' WHERE ordid=".$ordid;
				$this->_parentlist->do_query($query);
			}
			if (!empty($ordid)) {
				$query = 'INSERT INTO ACHR_Record SET ordid='.$ordid.', base_id='.$item7->get('ordid');
				$this->_parentlist->do_query($query);
			}
		}
		
		$query = 'ALTER TABLE AllItems ADD KEY name (name)';
		$this->_parentlist->do_query($query);
		
		$query = "UPDATE Reclist INNER JOIN (SELECT COUNT(*) AS nrec, rectype FROM AllItems GROUP BY rectype) AS S USING (rectype) INNER JOIN Reclist AS R2 USING (rectype) SET Reclist.nsub=ROUND(GREATEST(1, Reclist.nsub * S.nrec / R2.nsub)) WHERE (R2.subrec IS NULL OR R2.subrec='')";
		$this->_parentlist->do_query($query);
	}
	// Need to think about other ways items appear:
	// death items
	// birthsign spells
	// BUT I'm not likely to ever get to point of searching scripts
	// (I may eventually need to have a way for editors to manually add that type of info...)
	
	// FO adds item_hlth -> not of interest when listing places to get an item, especially given that
	//  item_hlth unlikely to be in SR
	// FO adds LVLG -> conditional appearance of items
	//  For now treating cases with LVLG as if LVLD=0 -> but when displaying location, it needs to
	//  add a note about LVLG-dependent (display functions need to check through all sources to find info!)
	
	// This needs to be changed to where Levellist_Src and Levellist_PLvl are not written to DB
	// (or at least so that it's optional)
	// which means all levlist functions need to be adapted to calculate PLvl on the fly
	// (but they only need to do it for a single item at a time)
	// For now I've just dropped those tables for MW/OB so I can move forward with SR with as little
	// coding work as possible
	protected function _calc_levlists() {
		$debug = 0;
		if ($debug)
			print "before levlists ndata=".$this->_parentlist->get_data_count()." usage=".memory_get_usage()." real=".memory_get_usage(true)." peak=".memory_get_peak_usage(true)."\n";
		
		//$allrecs = $this->_parentlist->get_levlist_rectypes();
		//var_dump($allrecs);
	
		// this function automatically scans all rectypes for ones containing 'item_id'
		// So no special treatment needed to add new lists such as LVLN, FLST
		foreach ($this->_parentlist->get_levlist_rectypes() as $rectype => $x) {
			if ($debug)
				print "Creating leveled lists for $rectype\n";
			$query = "SELECT ordid FROM AllItems WHERE rectype='{$rectype}' and (mod_status%0x04)>0 order by ordid";
			$res = $this->_parentlist->do_query($query);
			
			while ($row=$this->_parentlist->row_query($res)) {
				$ordid = $row['ordid'];
				$item = $this->_parentlist->get_item($ordid);
				$list = $item->contains_list();
				
				// save a bit of memory... it doesn't (immediately) clear up recursively created items, but it does 
				// clean up some items and the ones that are left should be high-level lists that are needed by other records
				// (shouldn't need to reopen all of the sub-sub-lists on subsequent calls)
				if ($debug)
					print "before mid cleanup ndata=".$this->_parentlist->get_data_count()." usage=".memory_get_usage()." real=".memory_get_usage(true)." peak=".memory_get_peak_usage(true)."\n";
				$item->release();
				//$this->_parentlist->clean_data();
				if ($debug)
					print "after mid cleanup ndata=".$this->_parentlist->get_data_count()." usage=".memory_get_usage()." real=".memory_get_usage(true)." peak=".memory_get_peak_usage(true)."\n";
			}
		}
		// Clean up empty placeholder rows in DB
		$query = "DELETE FROM LevelList WHERE item_ordid=0";
		$this->_parentlist->do_query($query);
	}

// Read all externally-stored scripts	
	public function setup_scripts() {
		if (empty($this->_params['scripts_dir']))
			return;
		if (!empty($this->_params['redo_scripts']) || !$this->_parentlist->hastable('scripts')) {
			$query = "DROP TABLE IF EXISTS scripts";
			$this->_parentlist->do_query($query);

			$query = 'CREATE TABLE scripts (';
			$query .= 'sid int NOT NULL auto_increment';
			$query .= ', filename varchar(255)';
			$query .= ', filetime int';
			$query .= ', contents blob';
			$query .= ', fileid tinyint UNSIGNED';
			$query .= ', mod_status tinyint UNSIGNED';
			$query .= ', primary key (sid)';
			$query .= ', index filename (filename)';
			$query .= ', index mod_status (mod_status)';
			$query .= ') DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci';

			// indexes
			$this->_parentlist->do_query($query);
			$this->_parentlist->addtable('scripts');
		}

// fileid is set independently of overall file being read ... given that I don't have history of past versions of file, or anything other than
// modification time to guess at what update introduced a given version of the file
   		$fileid = 0;
   		foreach ($this->_files as $filenum => $filename) {
			if ($this->_filemodabbrev[$filenum]==$this->_filemodabbrev[0])
				$fileid = $filenum;
		}


		$dp = opendir($this->_params['scripts_dir']);
		while (($file=readdir($dp))!==FALSE) {
			$fullname = $this->_params['scripts_dir'].'/'.$file;
			if (is_dir($fullname) || !is_file($fullname) || substr($file,0,1)=='.')
				continue;
			$timestamp = filemtime($fullname);
			$query = 'SELECT * from scripts where filename=\''.addslashes(substr($file,0,-4)).'\' order by filetime desc limit 1';
			$prevfile = $this->_parentlist->do_query($query, 'onerow');
			// file is already in database and has not been modified
			if (!empty($prevfile) && $prevfile['filetime']<($timestamp+60))
				continue;

			$query = 'INSERT INTO scripts SET ';
			$query .= ' filename=\''.addslashes(substr($file,0,-4)).'\''; // remove extension from filename
			$query .= ', filetime='.$timestamp;
			$query .= ', contents=\''.addslashes(file_get_contents($fullname)).'\'';
			$query .= ', fileid='.$fileid;
			$query .= ', mod_status='.(!empty($prevfile)?6:7);

			$this->_parentlist->do_query($query);

			// need to find sid entries and update if replacing a version of a file
			// only need to scan VMAD tables and INDX tables
			// BUT for INDX tables need to re-calc fragment location
			// EXCEPT... should new script only be shown for modified items?
			if (!empty($prevfile)) {
				// For now, just make sure that scripts table flags are updated appropriately; sort out rest later
				$query = 'UPDATE scripts SET mod_status=0 where sid='.$prevfile['sid'];
				$this->_parentlist->do_query($query);
			}
		}
		closedir($dp);
	}

	public function setup_cells() {
		$debug = 1;
		// get a list of doors leading FROM valid regions to interiors
		// regions without any inbound doors must be test cells
		
		// could do a left join on AllItems and pull out DOOR records... but it seems much faster to just check whether door coords have ben set
		// n.b. since this procedure is using data directly from SQL queries (instead of setting up items) 
		//  correct variable names are the SQL version (door_x NOT door-x)
		if ($this->_parentlist->get_classid()=='MW') {
			$refrec = "REFR";
			$cellrec = "CELL";
			$fromcell = "cell_id";
			$tocell = "cell_rev_id";
			$doorquery = "SELECT * FROM
				{$refrec}_Record LEFT JOIN AllItems USING (ordid)
				WHERE (DELE IS NULL OR DELE!=TRUE) AND door_x IS NOT NULL
				ORDER BY {$fromcell}, {$tocell}, z, x, y";
		}
		else {
			$refrec = "REFR";
			$cellrec = "CELL";
			$fromcell = "cell_id";
			$tocell = "cell_rev_id";
			// need to figure out later how to remove deleted records... which, for that matter, also depends upon mods....
			$doorquery = "SELECT * FROM
				{$refrec}_Record LEFT JOIN AllItems USING (ordid)
				WHERE door_x IS NOT NULL
				ORDER BY {$fromcell}, door_rev_id, z, x, y";
		}
		
		if (!empty($this->_params['redo_cells'])) {
			// clear out results of any previous setup_cells to restart from square one
			$this->_parentlist->do_query("UPDATE {$refrec}_Record set door_dup_id=NULL, cell_rev_id=NULL");
			// exterior, world_id -> are these set during initial read? ... appears yes
			$this->_parentlist->do_query("UPDATE {$cellrec}_Record set parentcell_id=NULL, extcell_id=NULL, extdoor_id=NULL, extloc_x=NULL, extloc_y=NULL, worldspace_lu=NULL, worldtype_lu=NULL");

// I then need to refill any values that normally are set when the data is first read
// Which I think is just worldspace_lu and worldtype_lu 
// This kind of a pain, but necesary to be able to update this info without re-reading all CELL data
			$query = "SELECT ordid FROM AllItems where rectype='$cellrec'";
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$item = $this->_parentlist->get_item($row['ordid']);
				$ws = $item->get('worldspace');
				$wt = $item->get('worldtype');
				$item->release();
				if (!isset($wt) && !isset($ws))
					continue;
				$query = "UPDATE {$cellrec}_Record SET ";
				if (isset($ws))
					$query .= " worldspace_lu=".$this->_parentlist->get_lookup_id('Worldspace', $ws);
				if (isset($ws) && isset($wt))
					$query .= ', ';
				if (isset($wt))
					$query .= " worldtype_lu=".$this->_parentlist->get_lookup_id('Worldtype', $wt);
				$query .= " WHERE ordid=".$row['ordid'];
				$this->_parentlist->do_query($query);
			}
// after doing all of this I need to clear out the cache
// force any items to be re-read from DB
			$this->_parentlist->clean_data();
		}
			
		$res = $this->_parentlist->do_query($doorquery);
		
		$doors = array();
		while ($row=$this->_parentlist->row_query($res)) {
			foreach ($row as $valname => $value) {
				if (is_null($value))
					unset($row[$valname]);
			}
			$doors[] = $row;
		}
		
		for ($i=0; $i<count($doors); $i++) {
			// convert exterior cell_rev_id into an ordid value
			// also look for cell_rev_ids leading outside that need to be fixed.... not finding any right now?
			// (not that there are many to find... just a couple cells such as Vivec, Temple or Vivec, Foreign Quarter
			if (empty($doors[$i][$tocell]) || is_null($this->_parentlist->get_rectype($doors[$i][$tocell]))) {
				if (!empty($doors[$i]['door_rev_id'])) {
					$door_rev = NULL;
					for ($i2=0; $i2<count($doors); $i2++) {
						if ($doors[$i2]['ordid']==$doors[$i]['door_rev_id']) {
							$door_rev = $i2;
							break;
						}
					}
					if (!is_null($door_rev)) {
						$doors[$i][$tocell] = $doors[$door_rev]['cell_id'];
					}
					else {
						print "unable to find reverse for door_rev_id=".$doors[$i]['door_rev_id']." door_id=".$doors[$i]['ordid']." formid=".$doors[$i]['formid']."\n";
					}
				}
				else {
// I think this is only used by MW... and not sure how it's supposed to work
					$doors[$i][$tocell] = $this->_parentlist->get_cell_ordid($doors[$i]['door_x'], $doors[$i]['door_y'], NULL, $doors[$i]['fileid']);
				}
			}

			// Fill in all of the cell items (should be possible now that cells contain minimal info)
			// Note that any exterior doors may officially be children of the 0,0 cell instead of the correct
			// local cell.  If so, recalculate the cell to obtain the local one
			$doors[$i]['fromitem'] = $this->_parentlist->get_item($doors[$i][$fromcell]);
			if (!is_null($doors[$i]['fromitem']->get('world_id'))) {
				$doors[$i][$fromcell] = $this->_parentlist->get_cell_ordid($doors[$i]['x'], $doors[$i]['y'], $doors[$i]['fromitem']->get('world_id'), $doors[$i]['fileid']);
				$doors[$i]['fromitem'] = $this->_parentlist->get_item($doors[$i][$fromcell]);
			}

			if (!is_null($doors[$i][$tocell])) {
				$doors[$i]['toitem'] = $this->_parentlist->get_item($doors[$i][$tocell]);
				if (!is_null($doors[$i]['toitem']->get('world_id'))) {
					$doors[$i][$tocell] = $this->_parentlist->get_cell_ordid($doors[$i]['door_x'], $doors[$i]['door_y'], $doors[$i]['toitem']->get('world_id'), $doors[$i]['fileid']);
					$doors[$i]['toitem'] = $this->_parentlist->get_item($doors[$i][$tocell]);
				}
			}
		}
		
		// identify duplicate or side-by-side doors
		// not checking both modded and unmodded versions of cells...
		for ($i=0; $i<count($doors); $i++) {
			if ($i &&
			    $doors[$i][$fromcell]==$doors[$i-1][$fromcell] &&
			    $doors[$i][$tocell]==$doors[$i-1][$tocell] &&
			    abs($doors[$i]['z']-$doors[$i-1]['z'])<200 &&
			    abs($doors[$i]['x']-$doors[$i-1]['x'])<200 &&
			    abs($doors[$i]['y']-$doors[$i-1]['y'])<200 &&
			    abs($doors[$i]['door_z']-$doors[$i-1]['door_z'])<200 &&
			    abs($doors[$i]['door_x']-$doors[$i-1]['door_x'])<200 &&
			    abs($doors[$i]['door_y']-$doors[$i-1]['door_y'])<200) {
				    if (array_key_exists('door_dup_id', $doors[$i-1]) && $doors[$i-1]['door_dup_id'])
					    $doors[$i]['door_dup_id'] = $doors[$i-1]['door_dup_id'];
				    else
					    $doors[$i]['door_dup_id'] = $doors[$i-1]['ordid'];
			    }
		}
		
		// find door in reverse direction -- only necessary for MW
		// matching may not be completely accurate
		// * in general, x-y-z locations don't match as well as I'd expect
		// * there are cases where no matches exist, (e.g., sewer discharges in Vivec) and may be trying to hard to force matches,
		//   therefore may be matching up with an entrance on another level
		// * there are also cases where only apparent match is much too far away and therefore isn't being used
		// * should perhaps allow one-way matches (door that appears where you teleport to, even if that door doesn't take you back)
		$nfound = $nnot = 0;
		$multi = array();
		// nrep=0: standard cases
		// nrep=1: difficult cases: exterior cell_rev_id off by one cell, multiple matches, self-match
		// nrep=2: just for printing error message (with calc'd values of best)
		for ($nrep=0; $nrep<3; $nrep++) {
			for ($i=0; $i<count($doors); $i++) {
				if (array_key_exists('door_rev_id', $doors[$i]) && $doors[$i]['door_rev_id'])
					continue;
				$best = NULL;
				$bestsum = NULL;
				$bestsumb = NULL;
				$bestz = NULL;
				$bestzb = NULL;
				for ($i2=0; $i2<count($doors); $i2++) {
					// on second pass, allow doors to match with themselves (e.g., in Clutter Warehouse)
					if (!$nrep && $i==$i2)
						continue;
					// fix multi cases on nrep=1 (skip on nrep=0 because want to do full scan of area)
					if (array_key_exists($i, $multi)) {
						if (!$nrep)
							continue;
					}
					// other than multi cases, on second rep don't want to look at any already-matched doors
					elseif ($nrep==1 && array_key_exists('door_rev_id', $doors[$i2]) && $doors[$i2]['door_rev_id'])
						continue;
					if ($doors[$i2][$fromcell] != $doors[$i][$tocell])
						continue;
					// on second rep, check cases where there is no match on an exterior door
					// (only interested in true exteriors, where cell ID could be off slightly)
					if ($doors[$i][$fromcell] != $doors[$i2][$tocell]) {
						if (!$nrep)
							continue;
						if (!$doors[$i]['fromitem']->get('exterior'))
							continue;
					}
					
					$delt = array('sum' => 0, 'sumb' => 0);
					foreach (array('x', 'y', 'z') as $dir) {
						$delt[$dir] = abs($doors[$i][$dir]-$doors[$i2]['door_'.$dir]);
						$delt['sum'] += $delt[$dir]*$delt[$dir];
						$delt[$dir.'b'] = abs($doors[$i2][$dir]-$doors[$i]['door_'.$dir]);
						$delt['sumb'] += $delt[$dir.'b']*$delt[$dir.'b'];
					}
					$delt['sum'] = sqrt($delt['sum']);
					$delt['sumb'] = sqrt($delt['sumb']);
					if (is_null($best) || $delt['sum']+$delt['sumb']<$bestsum+$bestsumb) {
						$best = $i2;
						$bestsum = $delt['sum'];
						$bestsumb = $delt['sumb'];
						$bestz = $delt['z'];
						$bestzb = $delt['zb'];
					}
				// initially, tried requiring a closer match on z than other coordinates:
				// may need to teleport to a position in front of door (instead of embedded in door),
				// but should end up standing at same elevation.  And don't want to mix upper doors and lower doors to same building
				// however, failed in majority of cases
				// instead: look for best match based on all directions and just pick that door..
				//				if ($delt['sum']>500 || $delt['z']>50)
				//					continue;
				//				if ($delt['sumb']>500 || $delt['zb']>50)
				//					continue;
				}
				if ($nrep<2 && !is_null($best) && $bestsum<1000 && $bestsumb<1000) {
					if (array_key_exists($i, $multi)) {
						if (!$nrep)
							continue;
						if ($debug)
							print "multiple case {$i} set to ordid=".$doors[$best]['ordid']."\n";
					}
					$nfound++;
					$doors[$i]['door_rev_id'] = $doors[$best]['ordid'];
					if ((array_key_exists('door_rev_id', $doors[$best]) && $doors[$best]['door_rev_id'] && $doors[$best]['door_rev_id']!=$doors[$i]['ordid']) || (array_key_exists($best, $multi) && !$nrep)) {
						if ($debug) {
							print "multiple reverse doors found for door $best\n";
							print "  ordid=".$doors[$best]['ordid']." fromid=".$doors[$best][$fromcell]." fromedid=".$doors[$best]['fromitem']->edid()."\n";
							print "  toid=".$doors[$best][$tocell]." toedid=".$doors[$best]['toitem']->edid()."\n";
							print "  original rev ordid=";
							if (array_key_exists('door_rev_id', $doors[$best]) && $doors[$best]['door_rev_id'])
								print $doors[$best]['door_rev_id'];
							else
								print "deleted";
							print "  new=".$doors[$i]['ordid']."\n";
						}
						$multi[$best] = true;
						unset($doors[$best]['door_rev_id']);
					}
					elseif (!array_key_exists($best, $multi)) {
						if ($doors[$best][$tocell]!=$doors[$i][$fromcell])
							$doors[$best][$tocell]=$doors[$i][$fromcell];
						$doors[$best]['door_rev_id'] = $doors[$i]['ordid'];
					}
				}
				elseif ($debug && $nrep==2 && (!array_key_exists('door_rev_id', $doors[$i]) || !$doors[$i]['door_rev_id'])) {
					$nnot++;
					print "Unable to find reverse of door at: ordid=".$doors[$i]['ordid'].", fromid=".$doors[$i][$fromcell].", fromedid=".$doors[$i]['fromitem']->edid()."\n";
					print "  destination=".$doors[$i][$tocell].", edid=".((array_key_exists('toitem', $doors[$i]) && is_object($doors[$i]['toitem']))?$doors[$i]['toitem']->edid():"missing")." bestsum=".$bestsum." bestsumb=".$bestsumb." bestz={$bestz} bestzb={$bestzb}\n";
				}
			}
		}
		
		if ($debug) {
			print "door_rev_id found=$nfound, not found=$nnot\n";
		}
		
		$cells_sep = array();
		foreach ($this->_get_cells_separate() as $data) {
			foreach ($data as $cell1 => $cell2) {
				if (!($ordid1 = $this->_parentlist->get_ordid($cell1))) {
					print "unable to identify cell {$cell1} from cells_separate list\n";
				}
				if (!($ordid2 = $this->_parentlist->get_ordid($cell2))) {
					print "unable to identify cell {$cell2} from cells_separate list\n";
				}
				$cells_sep[$ordid1][$ordid2] = true;
				$cells_sep[$ordid2][$ordid1] = true;
			}
		}
//print "cells_sep\n";
//var_dump($cells_sep);

		$cells_pref = array();
		foreach ($this->_get_cells_preferred() as $cell => $data) {
			if (!($ordid = $this->_parentlist->get_ordid($cell)) || $this->_parentlist->get_rectype($ordid)!=$cellrec) {
				print "unable to identify cell {$cell} from cells_preferred list\n";
			}
			$cells_pref[$ordid] = $data;
			foreach (array('exit', 'nonexit') as $exit) {
				if (array_key_exists($exit, $data)) {
					if (!($exitordid = $this->_parentlist->get_ordid($data[$exit])) || $this->_parentlist->get_rectype($exitordid)!=$refrec) {
						print "unable to identify $exit door ".$data[$exit]." from cells_preferred list\n";
						continue;
					}
					$cells_pref[$ordid][$exit."_id"] = array();
					for ($i=0; $i<count($doors); $i++) {
						if ($doors[$i]['ordid'] == $exitordid || (array_key_exists('door_rev_id', $doors[$i]) && $doors[$i]['door_rev_id'] == $exitordid)) {
							if ($doors[$i][$fromcell] != $ordid && $doors[$i][$tocell] != $ordid)
								print "specified $exit door ".$data[$exit].", ordid={$exitordid} does not connect to specified cell $ordid (checking door $i, fromcell=".$doors[$i][$fromcell].", tocell=".$doors[$i][$tocell].")\n";
							else
								$cells_pref[$ordid][$exit."_id"][$i] = 1;
						}
					}
				}
			}
		}
		
		$donedoors = array();
		$cells = array();
		$problems = array();
		$groups = array();
		$ngroup = -1;
		$secondary = array();
		
		while (count($donedoors)<count($doors)) {
			$ndonestart = count($donedoors);
			foreach ($doors as $ndoor => $row) {
				if (array_key_exists($ndoor, $donedoors))
					continue;
				// don't need doors from inside to outside; also, skip duplicate doors
				// for this check, only look for true exteriors (if a fake exterior has a connecting door, I may want to use it to get worldspace, etc)
				if ($row['toitem']->get('exterior') || (array_key_exists('door_dup_id', $row) && $row['door_dup_id'])) {
					$donedoors[$ndoor] = true;
					continue;
				}
				
				if (array_key_exists($row[$tocell], $cells_pref))
					$pref = $cells_pref[$row[$tocell]];
				else
					$pref = NULL;
				$extdoor = false;
				// exterior doors, doors with specified worldspace values, and doors whose cell's exterior has already been determined
				// note that this instance of cells pref checks tocell, not fromcell
				if ($row['fromitem']->get('exterior_any') || (array_key_exists($row[$fromcell], $cells_pref) && array_key_exists('worldspace', $cells_pref[$row[$fromcell]]))) {
					$extdoor = true;
				}
				elseif (!array_key_exists($row[$fromcell], $cells))
					continue;	
				$donedoors[$ndoor] = true;
				
				if (array_key_exists($row[$fromcell], $cells))
					$ngrfrom = $cells[$row[$fromcell]];
				else
					$ngrfrom = NULL;
				if (array_key_exists($row[$tocell], $cells))
					$ngrto = $cells[$row[$tocell]];
				else
					$ngrto = NULL;
				
				$dosep = false;
				// If door connects two separated cells, want to basically skip the door *except*
				// it needs to be available as a fall back in case one set of cells don't otherwise
				// connect to the exterior (e.g., The Old Way)
				if (array_key_exists($row[$fromcell], $cells_sep) && array_key_exists($row[$tocell], $cells_sep[$row[$fromcell]])) {
					$connect = false;
					$dosep = true;
				}
				// door specified as a non-exit door, so again, skip
				elseif (!is_null($pref) && array_key_exists('nonexit_id', $pref) && array_key_exists($ndoor, $pref['nonexit_id']))
					$connect = false;
				else
					$connect = true;
				
				if ($connect) {
					if (is_null($ngrto) && is_null($ngrfrom)) {
						$ngroup++;
						$ngrto = $ngrfrom = $ngroup;
					}
					elseif (is_null($ngrto))
						$ngrto = $ngrfrom;
					elseif (is_null($ngrfrom))
						$ngrfrom = $ngrto;
					// two different groups need to be merged
					elseif ($ngrfrom != $ngrto) {
						foreach ($groups[$ngrto]['cell'] as $cell => $x){
							$cells[$cell] = $ngrfrom;
						}
						foreach (array('cell', 'prefexit', 'exit', 'nonexit', 'secexit') as $sub) {
							if (!array_key_exists($sub, $groups[$ngrto]))
								continue;
							if (!array_key_exists($sub, $groups[$ngrfrom]))
								$groups[$ngrfrom][$sub] = $groups[$ngrto][$sub];
						// not just using array_merge, because it's likely to misinterpret numerical keys
							else {
								foreach ($groups[$ngrto][$sub] as $key => $value)
									$groups[$ngrfrom][$sub][$key] = $value;
							}
						}
						foreach ($secondary as $np => $pair) {
							foreach ($pair as $pgr => $pval) {
								if ($pgr==$ngrto) {
									$secondary[$np][$ngrfrom] = $pval;
									unset($secondary[$np][$ngrto]);
								}
							}
						}
						$groups[$ngrto] = NULL;
						$ngrto=$ngrfrom;
					}
				}
				// keep track of both cells, but assign them to separate groups
				else {
					// shouldn't happen...
					if (!is_null($ngrfrom) && !is_null($ngrto) && $ngrfrom==$ngrto) {
						print "separated cells have already been assigned to the same group: fromcell=".$doors[$ndoor][$fromcell]." tocell=".$doors[$ndoor][$tocell]."\n";
					}
					else {
						if (is_null($ngrto)) {
							$ngroup++;
							$ngrto = $ngroup;
						}
						if (is_null($ngrfrom) && !$extdoor) {
							$ngroup++;
							$ngrfrom = $ngroup;
						}
						// Keep track of secondary connections
						// Only fill them in after doors loop (so that all doors to/from both sets of cells
						// are transferred as secondary doors
						if ($dosep) {
//print "separating $ngrto -> ".$doors[$ndoor][$tocell]."  $ngrfrom -> ".$doors[$ndoor][$fromcell]."\n";
							$secondary[] = array($ngrto => $doors[$ndoor][$tocell], $ngrfrom => $doors[$ndoor][$fromcell]);
}
					}
				}
				
				$groups[$ngrto]['cell'][$row[$tocell]] = 1;
				$cells[$row[$tocell]] = $ngrto;
				if (!$extdoor) {
					$cells[$row[$fromcell]] = $ngrfrom; // in some cases, this will be redundant, but it won't hurt
					$groups[$ngrfrom]['cell'][$row[$fromcell]] = 1;
				}
				
				// all doors are kept track of in tocell (although the door is physically located in fromcell, we're interested in how (whether) you get in to the cell, not how you get out)
				if (!$connect) {
					$groups[$ngrto]['nonexit'][$ndoor] = 1;
				}
				elseif ($extdoor) {
					if (!is_null($pref) && array_key_exists('exit_id', $pref) && array_key_exists($ndoor, $pref['exit_id']))
						$groups[$ngrto]['prefexit'][$ndoor] = 1;
					else
						$groups[$ngrto]['exit'][$ndoor] = 1;
				}
			}
			// no more doors found on last check, so break out of loop
			if (count($donedoors)==$ndonestart)
				break;
		}
	
		foreach ($secondary as $pair) {
			$keys = array_keys($pair);
			foreach ($keys as $i => $ngr) {
				if ($i)
					$ngrb = $keys[0];
				else
					$ngrb = $keys[1];
				foreach (array('prefexit', 'exit', 'secexit') as $sub) {
					if (isset($groups[$ngr][$sub])) {
						foreach ($groups[$ngr][$sub] as $key => $value) {
							$groups[$ngrb]['secexit'][$key] = $pair[$ngrb];
						}
					}
				}
			}
		}

		foreach ($groups as $ngr => $groupdata) {
			if (is_null($groupdata))
				continue;
			
			$tochk = array();
			$exit = NULL;
			$groups[$ngr]['error'] = "";
			$issecexit = FALSE;
			// use preferred exits
			if (array_key_exists('prefexit', $groupdata) && count($groupdata['prefexit']))
				$tochk = array_keys($groupdata['prefexit']);
			// use standard exits
			elseif (array_key_exists('exit', $groupdata) && count($groupdata['exit']))
				$tochk = array_keys($groupdata['exit']);
			// at this point there are effectively no identified exits -- which means no connection to exterior for this group
			// instead look to see whether there is an internal (separated) connection -- secondary exit
			elseif (array_key_exists('secexit', $groupdata) && count($groupdata['secexit'])) {
				$tochk = array_keys($groupdata['secexit']);
				$issecexit = TRUE;
			}
			// at this point there are effectively no identified exits -- which means no connection to exterior for this group
			// instead look to see whether there is an internal (separated) connection
			elseif (array_key_exists('nonexit', $groupdata) && count($groupdata['nonexit'])) {
				foreach ($groupdata['nonexit'] as $ndoor => $x) {
					// skip external doors
					if ($doors[$ndoor]['fromitem']->get('exterior_any') || (array_key_exists($doors[$ndoor][$fromcell], $cells_pref) && array_key_exists('worldspace', $cells_pref[$doors[$ndoor][$fromcell]])))
						continue;
					$tochk[] = $ndoor;
				}
				if (!count($tochk)) {
					$groups[$ngr]['error'] .= "Available doors were all specified as non-exit doors, therefore forced to use a non-exit door anyway\n";
					$tochk = array_keys($groupdata['nonexit']);
				}
			}
			// I don't think this should happen....
			else {
				print "group found with zero exits\n";
				var_dump($groupdata);
				$groups[$ngr] = NULL;
				continue;
			}
			
			// only one exit door for this group
			if (count($tochk)==1) {
				$exit = $tochk[0];
				$exitcell = $doors[$exit][$tocell];
			// need to decide which door to use
			}
			else {
				$exitcell = NULL;
				$exitcells = array();
				$donecells = array();
				foreach ($tochk as $ndoor) {
					$cell = $doors[$ndoor][$tocell];
					if (array_key_exists($cell, $donecells))
						continue;
					$donecells[$cell] = 1;
					$exitcells[] = $cell;
				}
				if (count($exitcells)==1) {
					$exitcell = $exitcells[0];
				}
				else {
					$prefcells = array();
					foreach ($exitcells as $cell) {
						if (array_key_exists($cell, $cells_pref)) {
							$prefcells[] = $cell;
						}
					}
					if (count($prefcells)==1) {
						$exitcell = $prefcells[0];
					}
					elseif (count($prefcells))
						$exitcells = $prefcells;
				}
				if (is_null($exitcell)) {
					$celledids = array();
					foreach ($exitcells as $i => $cell) {
						$edid = $this->_parentlist->get_edid($cell);
						$celledids[$edid] = $i;
					}
					uksort($celledids, 'cell_edid_sort');
					foreach ($celledids as $edid => $i) {
						$exitcell = $exitcells[$i];
						break;
					}
					$groups[$ngr]['error'] .= "Exit cell determined using cell_edid_sort\n";
				}
				
				// at last: a single exitcell
				$keep = array();
				foreach ($tochk as $ndoor) {
					if ($doors[$ndoor][$tocell]==$exitcell)
						$keep[] = $ndoor;
				}
				if (count($keep)==1)
					$exit = $keep[0];
				elseif (count($keep))
					$tochk = $keep;
				else {
					// Given how exitcell was found, I don't think this is possible...
					$groups[$ngr]['error'] .= "No valid exit doors found in selected exitcell\n";
				}
				if (is_null($exit)) {
					$exitdirs = array();
					foreach ($tochk as $ndoor) {
						// door is defined as door _to_ cell, so want 
						$exitdirs[$ndoor] = array('x' => $doors[$ndoor]['door_x'], 'y' => $doors[$ndoor]['door_y'], 'z' => $doors[$ndoor]['door_z']);
					}
					uksort($exitdirs, array($this, 'door_zyx_sort'));
					foreach ($exitdirs as $ndoor => $x) {
						$exit = $ndoor;
						break;
					}
					$groups[$ngr]['error'] .= "Multiple valid exit doors found in cell; door chosen based on zyx criteria\n";
				}
			}
			$groups[$ngr]['exitdoor'] = $exit;
			$groups[$ngr]['exitcell'] = $exitcell;
			$groups[$ngr]['issecexit'] = $issecexit;
			
			if ($groups[$ngr]['error'] != '') {
				print "\n\nProblem assigning cell exit door\n".$groups[$ngr]['error'];
				
				foreach ($groupdata['cell'] as $cell => $x) {
					print "Cell {$cell}";
					if ($val=$this->_parentlist->get_name($cell))
						print ", name={$val}";
					if ($val=$this->_parentlist->get_edid($cell))
						print ", edid={$val}";
					if ($val=$this->_parentlist->get_formid($cell))
						print ", formid={$val}";
					if (array_key_exists($cell, $cells_pref))
						print " (preferred cell)";
					if ($cell==$exitcell)
						print " (selected as exit cell)";
					print " is entered using doors:\n";
				
					foreach ($doors as $ndoor => $row) {
						if ($row[$tocell]!=$cell)
							continue;
						print "  door ".$row['ordid'];
						foreach (array('edid', 'formid', 'FRMR') as $key) {
							if (array_key_exists($key, $row))
								print ", {$key}=".$row[$key];
						}
						print " from ".$row[$fromcell];
						if ($row['fromitem']->get('exterior_any'))
							print " (exterior)";
						foreach (array('exit', 'prefexit', 'nonexit') as $etype) {
							if (!array_key_exists($etype, $groupdata))
								continue;
							if (!array_key_exists($ndoor, $groupdata[$etype]))
								continue;
							print " ({$etype})";
						}
						if ($ndoor==$exit)
							print " (selected as exit door)";
						print "\n";
						print "    ";
						foreach (array('door_x', 'door_y', 'door_z') as $key) {
							print "{$key}=".$row[$key]." ";
						}
						print "\n";
					}
				}
			}
		}
		
		$celldata = array();
		$newfound = true;
		while ($newfound) {
			$newfound = false;
			foreach ($groups as $ngr => $groupdata) {
				if (is_null($groupdata))
					continue;
				if (array_key_exists('done', $groupdata))
					continue;
				$exitdoor = $groupdata['exitdoor'];
				$exitcell = $groupdata['exitcell'];
				if ($groupdata['issecexit']) {
					$intexitcell = NULL;
					foreach ($groupdata['secexit'] as $key => $value) {
						if ($key==$exitdoor)
							$intexitcell = $value;
					}
				}
				else {
					$intexitcell = $exitcell;
				}
				$ws = NULL;
				$wt = NULL;
				if (array_key_exists($exitcell, $cells_pref) && array_key_exists('worldspace', $cells_pref[$exitcell]))
					$ws = $this->_parentlist->get_lookup_id('Worldspace', $cells_pref[$exitcell]['worldspace']);
				if (array_key_exists($exitcell, $cells_pref) && array_key_exists('worldtype', $cells_pref[$exitcell]))
					$wt = $this->_parentlist->get_lookup_id('Worldtype', $cells_pref[$exitcell]['worldtype']);
				
				if ($doors[$exitdoor]['fromitem']->exterior_any()) {
					$newfound = true;
					$groups[$ngr]['done'] = true;
					if (is_null($ws))
						$ws = $doors[$exitdoor]['fromitem']->get('worldspace_lu');
					if (is_null($wt))
						$wt = $doors[$exitdoor]['fromitem']->get('worldtype_lu');
					// it's an exterior cell, so directly use this info
					foreach ($groupdata['cell'] as $cell => $x) {
						$celldata[$cell]['parentcell_id'] = $intexitcell;
						$celldata[$cell]['extcell_id'] = $doors[$exitdoor][$fromcell];
						$celldata[$cell]['extdoor_id'] = $doors[$exitdoor]['ordid'];
						$celldata[$cell]['extloc_x'] = $doors[$exitdoor]['x'];
						$celldata[$cell]['extloc_y'] = $doors[$exitdoor]['y'];
						$celldata[$cell]['worldspace_lu'] = $ws;
						$celldata[$cell]['worldtype_lu'] = $wt;
					}
				}
				else if (array_key_exists($exitcell, $celldata) && array_key_exists('extdoor_id', $celldata[$exitcell])) {
					$newfound = true;
					$groups[$ngr]['done'] = true;
					if (is_null($ws))
						$ws = $celldata[$exitcell]['worldspace_lu'];
					if (is_null($wt))
						$wt = $celldata[$exitcell]['worldtype_lu'];
					foreach ($groupdata['cell'] as $cell => $x) {
						$celldata[$cell]['parentcell_id'] = $intexitcell;
						$celldata[$cell]['extcell_id'] = $celldata[$exitcell]['extcell_id'];
						$celldata[$cell]['extdoor_id'] = $doors[$exitdoor]['ordid'];
						$celldata[$cell]['extloc_x'] = $celldata[$exitcell]['extloc_x'];
						$celldata[$cell]['extloc_y'] = $celldata[$exitcell]['extloc_y'];
						$celldata[$cell]['worldspace_lu'] = $ws;
						$celldata[$cell]['worldtype_lu'] = $wt;
					}
				}
				else {
					continue;
				}
			}
		}
		
		foreach ($doors as $ndoor => $row) {
			if (array_key_exists($ndoor, $donedoors))
				continue;
			print "Door not used {$ndoor}\n";
			print "  from cell=".$row[$fromcell].", edid=".$row['fromitem']->edid()."\n";
			print "  to cell=".$row[$tocell].", edid=".$row['toitem']->edid()."\n";
		}
		
		// merge all queries into one
		//		$query = "UPDATE {$refrec}_Record SET";
		foreach ($doors as $ndoor => $row) {
			$query = "UPDATE {$refrec}_Record SET {$tocell}=".$row[$tocell];
			if (array_key_exists('door_dup_id', $row) && $row['door_dup_id'])
				$query .= ", door_dup_id=".$row['door_dup_id'];
			if (array_key_exists('door_rev_id', $row) && $row['door_rev_id'])
				$query .= ", door_rev_id=".$row['door_rev_id'];
			if (array_key_exists('cell_rev_id', $row) && $row['cell_rev_id'])
				$query .= ", cell_rev_id=".$row['cell_rev_id'];
			$query .= " WHERE ordid=".$row['ordid'];
			$this->_parentlist->do_query($query);
			
			// done with door, so release sub-items
			$row['fromitem']->release();
			unset($doors[$ndoor]['fromitem']);
			if (!is_null($row[$tocell])) {
				$row['toitem']->release();
				unset($doors[$ndoor]['toitem']);
			}
		}
		
		$queries = array();
		// need to also add toplevel cells
		// do worldspace for exterior cells, too
		foreach ($celldata as $cellid => $data) {
			$query = "UPDATE {$cellrec}_Record SET ";
			$first = true;
			foreach (array('parentcell_id', 'extcell_id', 'extdoor_id', 'extloc_x', 'extloc_y', 'worldspace_lu', 'worldtype_lu') as $key) {
				if (!array_key_exists($key, $data) || is_null($data[$key]))
					continue;
				if (!$first)
					$query .= ", ";
				else
					$first = false;
				$query .= "{$key}=".$data[$key];
			}
			if (!$first) {
				$queries[$cellid] = $query;
				$query .= " WHERE ordid=".$cellid;
				$this->_parentlist->do_query($query);
			}
		}
		
		foreach ($cells_pref as $cellid => $data) {
			if (array_key_exists($cellid, $cells))
				continue;
			$query = "UPDATE {$cellrec}_Record SET ";
			$docell = false;
			if (array_key_exists('worldspace', $data)) {
				if ($docell)
					$query .= ", ";
				$docell = true;
				$query .= " worldspace_lu=".$this->_parentlist->get_lookup_id("Worldspace", $data['worldspace']);
			}
			if (array_key_exists('worldtype', $data)) {
				if ($docell)
					$query .= ", ";
				$docell = true;
				$query .= " worldtype_lu=".$this->_parentlist->get_lookup_id('Worldtype', $data['worldtype']);
			}
			if ($docell) {
				$queries[$cellid] = $query;
				$query .= " WHERE ordid=".$cellid;
				$this->_parentlist->do_query($query);
			}
		}
		
		$query = "SELECT * FROM {$cellrec}_Record LEFT JOIN AllItems USING (ordid) WHERE worldspace_lu IS NULL";
		$res = $this->_parentlist->do_query($query);
		// any cells that do not have an exterior must be test cells: CHECK!
		// (for OB.. need to check exterior's ID)
		while ($row=$this->_parentlist->row_query($res)) {
			$query = "UPDATE {$cellrec}_Record SET ";
			// modded version of cell: set to be same as base version
			if (($altordid=$this->_parentlist->get_ordid($row['edid'],0,'CELL'))!=$row['ordid'] && array_key_exists($altordid, $queries)) {
				$query = $queries[$altordid];
			}
			elseif ((!empty($row['exterior']) || !empty($row['fake_exterior'])) && (!is_null($ws = $this->_parentlist->get_t_item($row['ordid'])->get('worldspace')))) {
				$query .= " worldspace_lu=".$this->_parentlist->get_lookup_id('Worldspace', $ws);
			}
			else {
				$query .= " worldspace_lu=".$this->_parentlist->get_lookup_id('Worldspace', 'Test');
				$query .= ", worldtype_lu=".$this->_parentlist->get_lookup_id('Worldtype', 'Test');
			}
			$query .= " WHERE ordid=".$row['ordid'];
			$this->_parentlist->do_query($query);
		}
	}
	
	/* placeholder functions */
	protected function _get_cells_separate() {
		return array();
	}
	protected function _get_cells_preferred() {
		return array();
	}
	
	/**
	 * Miscellaneous helper functions (static functions)
	 */
	static function print_indent($nest) {
		print str_repeat(" ", 2*$nest);
	}
	
	// Take an arbitrary raw string read from a binary file and return a printable string representing its contents
	static function display_raw_data($value, $multiline = FALSE) {
		if (ctype_print($value))
			return $value;
		
		$string = '';
		for ($i=0; $i<strlen($value); $i++) {
			if ($i && !$multiline)
				$string .= ' . ';
			$ord = ord(substr($value,$i,1));
			$string .= sprintf("0x%02x", $ord);
			if ($ord>32 && $ord<127)
				$string .= '('.substr($value,$i,1).')';
			if ($multiline)
				$string .= "\n";
		}
		return $string;
	}
	
	static function csunpack($string, $input) {
		$data = $input;
	/*	
	if (preg_match('/^c/i', $string))
		$data = substr($data,0,1);
	else if (preg_match('/^s/i', $string))
		$data = substr($data,0,2);
	else
		$data = substr($data,0,4);
	$data = strrev($data);
	if (strlen($data)==0) {
		print strlen($input)." ".strlen($data)."\n";
	}
	if ($string=='L' && strlen($data)<4) {
		print strlen($input)." ".strlen($data)."\n";
	} */
		$values = unpack($string, $data);
		if (count($values)<1) {
			print "error processing unpack for $string, input_len=".strlen($input).", $input\n";
		}
		return $values[1];
	}
	
	static function door_zyx_sort($a, $b) {
		foreach (array('z', 'y', 'x') as $dir) {
			$delt = $a[$dir] - $b[$dir];
			if (abs($delt)>200 || $dir=='x') {
				if ($delt<0)
					return -1;
				else
					return 1;
			}
		}
		return 0;
	}
}

	
# datadef options
#  top level;
#    keep : 2 (always if record requested -- any record used in AllItems table), 1 (by default), 0 (only if explicitly requested), -1 (never, generally means no info provided on record format)
#    multi: name of record used to determine index (can be its own name)
#    repeat_data: if data repeats internally (within a single field), e.g. for KWDA records, this is length
#        of each individual set of data
#        multi *must* be set if repeat_data is set
#    subset: triggers the start of a subset.  Members of subset are multi'd on the subset key; data format key is prefixed with
#            subset.  Used in cases where a key is used with multiple meanings within the same record (e.g., NAME in CELL)
#    data: list of variables to extract from record
#    OR val/type/etc for single-variable
#    sqltable: specify a different name for the sqltable where this variable should be stored.  
#              this option triggers the correct CREATE TABLE sql statements but it DOES NOT cause the data to be automatically
#              moved into the new structure.  The code to do such reorg should be added to the item's finish_init() routine.
#  individual variables
#    val: name of variable (use name 'flag' for for a variable containing multiple flags)
#    vallist: array that provides a list of variable names to use, all with same format
#    valsuffix: suffix to append to vallist names
#      (vallist and valsuffix are replaced with individual val entries when datadef is processed)
#    flag: array that provides list of individual entries with a flag variable
#    index: location of value within data string (if some values are being skipped)
#    type: format of variable (default format is 'string')
#    len: length of data for variable (only necessary when overriding default, in particular for string values with a specific length -- or else for structures that are to be skipped)
#    factor: multiply value by this factor (default value is 1)
#    default: default value for variable (currently only used in creating database table -- so a value won't
#      end up set to the default value unless it goes through the database)
#    lookup: name of array to use to convert integer index into correct value (name should be an entry in the _lookup array)
#    lookup_create: take the value, put it into the provided _lookup array, and then change the saved value into an integer index
#                   (used primarily for keys such CELL-RGNN where the values are likely to be used repeatedly)
#    len1, len2, len3: expected length of data
#    data1, data2, data3: format to use for data of the corresponding length (for cases where there are two or three different formats)
#    extra: variable does not exist in .esm file; only listed in datadef to reserve a place in the database for the variable
#    desc: description of variable (not being used anywhere right now)
#    force: force variable to be added to mysql tables, even if it doesn't exist in files that were checked
	
#  extra individual variables used primarily by non-standard processing (especially situations where input data
#    does not have a specified size)
#    sub: a function name (to be called as $this->$sub()) to be called at this point within the processing
#    skip: don't store the data that is read (alternative to index when size of skipped data is not known ahead of time)
#
#  if a variable is named {valname}Count, it is assumed to be the number of individual values of {valname}
#    that need to be processed
#  if a variable is named _datasize, it is assumed to be the size of all remaining entries in the data definition
	
# remember: edids can be empty strings

// Thoughts on how to handle the chaos of QUST subrecords
// allow $index to be a string such as 'INDX-2-QSDT-0-2'
// * store it like this in DB for complex records
// * all CTDA data, regardless of nesting, put into single array -> let index sort out the nesting
// _datadef would need to recognize entries such as 'CTDA-QSDT-INDX'
// and subset would need to be more complex, including able to handle stepping back just one nested level
	
