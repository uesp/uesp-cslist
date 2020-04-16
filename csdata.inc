<?php
require_once 'common.inc';
require_once 'csitem.inc';

// add post-CSRead cleanup: scan all edid variables and convert to numbers
//   have flag for whether to replace non-converted edids to -1 or leave string in place

// add a getInstance function
// gets whichever child class is appropriate, based on what's loaded

class CSData {
	protected $_data;
	protected $_ndata;
	protected $_datastats;
	protected $_bytype;
	protected $_active;
	protected $_namespace;
	protected $_file_ns = array();
	protected $_file_name = array();
	protected $_file_mod = array();
	protected $_file_abbrev = array();
	protected $_curr_nfile;
			
	protected $_equiv;
	protected $_styles;
	protected $_types;
	protected $_keyvalues;
	protected $_fixed_group_items;
	protected $_levlist_rectypes;
	
	protected $_lookup, $_lookup_rev;
	protected $_datainfo, $_datatables, $_dataoptions;
	protected $_dbtables;
	
	protected $_classid;
	
	protected $_datamode;
	const DATAMODE_DB = 1;
	const DATAMODE_READ = 2;
	const DATAMODE_WRITE = 3;
	
	protected $_csread;
	protected $_connection;
	protected $_connfile = 'csdata_conn.inc';
	protected $_resource;
	protected $_datachecked = FALSE;
	
	protected $_pc_level;
	
	public function __construct() {
		global $gCSData;
		
		$this->_classid = substr(get_class($this),0,2);
		$this->_data = array();
		$this->_datastats = array();
		$this->_ndata = 0;
		$this->_bytype = array();
		$this->_active = NULL;
		
		$this->_equiv = array("High Elf" => "Altmer",
		                      "Wood Elf" => "Bosmer",
		                      "Dark Elf" => "Dunmer",
		                      'Old People Race' => 'Elder');
		
		$this->_styles = array();
		$this->_types = array();
		$this->_fixed_group_items = array();
		$this->_curr_nfile = 0;
		$this->_lookup = array();
		$this->_lookup_rev = array();
		$this->_datainfo = array();
		$this->_datatables = array();
		
		$this->_datamode = self::DATAMODE_DB;
		$this->_csread = NULL;
		$this->_connection = NULL;
		$this->_resource = NULL;
		
		$this->_cells_separate = array();
		$this->_cells_preferred = array();
		
		$this->_levlist_rectypes = NULL;
		
		$this->_pc_level = 20;
		
		if (!isset($gCSData))
			$gCSData = $this;
	}
	
	public static function getInstance() {
		$classes = get_declared_classes();
		$children = array();
		foreach ($classes as $class) {
			if (substr($class,0,2)=='__')
				continue;
			if (get_parent_class($class)==get_class())
				$children[] = $class;
		}
		if (empty($children)) {
			$children[] = get_class();
		}
		$child = array_shift($children);
		
		return new $child();
	}
	
	protected function _set_levlist_rectypes() {
		$this->_levlist_rectypes = array();
		foreach ($this->get_rectypes() as $rectype) {
			if (array_key_exists('item_id', $this->_datainfo[$rectype]) || array_key_exists('spell_id', $this->_datainfo[$rectype]))
				$this->_levlist_rectypes[$rectype] = true;
		}
	}
	
	function is_levlist_rectype($rectype) {
		if (is_null($this->_levlist_rectypes))
			$this->_set_levlist_rectypes();
		if (array_key_exists($rectype, $this->_levlist_rectypes))
			return true;
		else
			return false;
	}
	
	function get_levlist_rectypes() {
		if (is_null($this->_levlist_rectypes))
			$this->_set_levlist_rectypes();
		return $this->_levlist_rectypes;
	}
	
	function set_datamode($datamode) {
		$this->_datamode = $datamode;
	}
	
	function get_datamode() {
		return $this->_datamode;
	}
	
	function get_classid() {
		return $this->_classid;
	}
	
	function get_namespace() {
		return $this->_namespace;
	}
	
	function get_levellist_dlvl() {
		return 8;
	}
	
	function set_csread() {
		if (!isset($this->_csread)) {
			require_once strtolower($this->_classid)."read.inc";
			$classname = $this->_classid."Read";
			$this->_csread = new $classname($this);
		}
		return $this->_csread;
	}
	
	function get_data($datamode=NULL, $data=NULL) {
		if (!is_null($datamode))
			$this->_datamode = $datamode;
		
		if ($this->_datamode != self::DATAMODE_READ) {
			$this->init_db(); // calls set_csread as necessary
		}
		else {
			$this->set_csread();
		}
		
		if ($this->_datamode == self::DATAMODE_WRITE) {
			if (!isset($data))
				$this->_csread->request_data('all');
			else
				$this->_csread->request_data($data);
			
			$this->_csread->read_all_files();
		}
		
		return $this->_csread;
	}
	
	function create_db() {
		global $gConnection;
		
		// Drop then create DB
		$dbname = 'CSData_'.$this->_classid."Data";
		$this->do_query('DROP DATABASE IF EXISTS '.$dbname);
		$this->do_query('CREATE DATABASE '.$dbname.' DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci');
		
		// Clear out any data that may have come from the dropped DB
		// _datatables is most important one -- need to figure out with the others whether to force them to be
		// reset (means all initialization has to be done *after* last point where DB could be re-created)
		// OR allow carry over (means any initialization after this point has to be able to handle possible
		// existing values)
		/*$this->_lookup = array();
		$this->_lookup_rev = array();*/
		$this->_datainfo = array();
		$this->_datatables = array();
		$this->_dbtables = array();
		
		$gConnection->select_db($dbname);
	}
	
	// Originally, this was only be called by CSData, as part of get_data
	// Now trying to set up CSRead to be a bit more independent -- to work in more situations than just
	//  those created by get_data.  Which means this could be called by CSRead, too
	// Means there's some potential for going in circles...
	function set_connection($connfile=NULL) {
		global $gConnection;
		if (!empty($connfile))
			$this->_connfile = $connfile;
		if (!empty($this->_connection)) {
			if (!$gConnection->ping())
				$this->_connection = NULL;
		}
		if (empty($this->_connection)) {
			if (!empty($gConnection)) {
				if (!$gConnection->ping())
					$gConnection = NULL;
			}
			if (empty($gConnection))
				include($this->_connfile);
			$this->_connection = $gConnection;
		}
		$gConnection->set_charset('utf8');
		$dbname = 'CSData_'.$this->_classid."Data";
		$dbok = $gConnection->select_db($dbname);
		return $dbok;
	}
	
	function init_db() {
		$dbok = $this->set_connection();
		
		if ($this->_datamode==self::DATAMODE_WRITE && empty($this->_csread)) {
			$this->set_csread();
		}
		
		if (!empty($this->_datatables) || !$dbok)
			return;
		
		$query = "SHOW TABLES";
		$res = $this->do_query($query);
		if (empty($res))
			return;
		
		while ($row=$this->row_query($res, MYSQLI_NUM)) {
			$table = $row[0];
			$this->_dbtables[$table] = TRUE;
			if (preg_match('/(.*)_Lookup$/', $table, $matches)) {
				$lookup = $matches[1];
				$tquery = "SELECT * FROM {$table}";
				$tres = $this->do_query($tquery);
				while ($trow=$this->row_query($tres)) {
					$this->_lookup[$lookup][$trow[$lookup."_lu"]] = $trow[$lookup];
					$this->_lookup_rev[$lookup][$trow[$lookup]] = $trow[$lookup."_lu"];
				}
				continue;
			}
			elseif ($table=='Files') {
				$tquery = "SELECT * FROM {$table}";
				$tres = $this->do_query($tquery);
				while ($trow=$this->row_query($tres)) {
					$this->add_file($trow['fileid'], $trow['filename'], $trow['filenamespace'], $trow['filemodname'], $trow['filemodabbrev']);
				}
				continue;
			}
			
			if (preg_match('/^[A-Z_]{4}/', $table))
				$rectype = substr($table, 0, 4);
			elseif ($table=='AllItems')
				$rectype = '';
			else
				continue;
			
			$tquery = "SHOW FULL COLUMNS FROM {$table}";
			$tres = $this->do_query($tquery);
			while ($trow=$this->row_query($tres)) {
				$sqlname = $trow['Field'];
				if ($sqlname=='subid') {
					if (substr($trow['Type'],0,3)!='int')
						$this->set_dataoptions($rectype, 'complex_index', TRUE);
					continue;
				}
				elseif ($sqlname=='ordid' || $sqlname=='subid' || $sqlname=='edid' || $sqlname=='formid' || $sqlname=='rectype' || $sqlname=='fileid' || $sqlname=='name')
					continue;
				$comment = $trow['Comment'];
				$centries = preg_split('/\s*;\s*/', $comment);
				$vals = array();
				foreach ($centries as $centry) {
					$split = preg_split('/\s*=\s*/', $centry);
					if (count($split)<2)
						continue;
					$vals[$split[0]] = $split[1];
				}
				if (!array_key_exists('variable', $vals))
					$vals['variable'] = $sqlname;
				$this->set_datainfo($rectype, $vals['variable'], 'sqltable', $table);
				$this->set_datainfo($rectype, $vals['variable'], 'sqlname', $sqlname);
				if (array_key_exists('format', $vals))
					$this->set_datainfo($rectype, $vals['variable'], 'format', $vals['format']);
				if (array_key_exists('lookup', $vals))
					$this->set_datainfo($rectype, $vals['variable'], 'lookup', $vals['lookup']);
			}
		}
		
		$checkedfile = strtolower($this->_classid).'checked.txt';
		if (file_exists($checkedfile)) {
			$this->_datachecked = array();
			$fp = fopen($checkedfile, 'r');
			while ($line = fgets($fp)) {
				if ($line[0]=='#')
					continue;
				$fullname = trim($line);
				$loc = strpos($fullname,'-');
				$rectype = substr($fullname,0,$loc);
				$name = substr($fullname,$loc+1);
				// I should have a way of displaying problems when processing checkedfile
				// But I don't want to just print a message that gets shown on all webpages
				if ($rectype=='all') {
					// $table blank for AllItems
					// note that ordid/edid/name/fileid/rectype don't have datainfo entries
					// I need to assume they are always confirmed
					foreach ($this->_datainfo as $table => $ddata) {
						$this->_datachecked[$table][$name] = TRUE;
					}
				}
				else {
					$this->_datachecked[$rectype][$name] = TRUE;
				}
			}
			fclose($fp);
		}
	}
	
	function hastables() {
		if (count($this->_dbtables))
			return TRUE;
		else
			return FALSE;
	}
	
	function hastable($table) {
		if (!empty($this->_dbtables[$table]))
			return TRUE;
		else
			return FALSE;
	}
	
	function addtable($table) {
		$this->_dbtables[$table] = TRUE;
	}
	
	function get_data_count() {
		return count($this->_data);
	}
	
	function check_memory() {
                static $basemem = NULL;
		static $fskip = 0.65;
		static $fok = 0.55;
		static $ncall = 0;
		static $nskip = 0;
		// this is just set based upon when previous runs failed
		$maxmem = 33500000;
                // BUT on MacBook, min memory seems to be >29M (meaning fmem can't be forced below 88% if using 33.5MB)
		// memory_limit in php.ini is 256M
		// but what about when running on UESP? -- this value seems to work OK on UESP... no immediately apparent
		// problems even when re-reading WRLD/CELL/REFR
		//$maxmem = 85.e6;
		$fstart = memory_get_usage()/$maxmem;
		$ncall++;
		if ($fstart<$fskip) {
			$nskip++;
			return;
		}
		$dlist = array_keys($this->_data);
		usort($dlist, array($this, 'get_datarank'));
                $nrelease = 0;
		/*foreach(array_keys($this->_data) as $ordid)
			$dlist[$ordid] = $this->get_datarank($ordid);
		arsort($dlist);
		foreach ($dlist as $ordid => $rank) {
			if (($fmem=(memory_get_usage()/$maxmem))<0.55)
				break;
			if ($rank<0)
				break;*/
		/*		foreach ($dlist as $i => $ordid) {
			print "i=$i ordid=$ordid use=".$this->_datastats['nuse'][$ordid]." rel=".$this->_datastats['nrel'][$ordid]." dtime=".(time()-$this->_datastats['time'][$ordid])."\n";
		}
		if (count($dlist))
			exit;*/
		foreach ($dlist as $ordid) {
			if (($fmem=(memory_get_usage()/$maxmem))<$fok)
				break;
			
			if ($this->block_release($ordid))
				break;
			
                        $nrelease++;	
			$this->_unset_data($ordid);
		}
		$reset = FALSE;
		$chkmem = memory_get_usage();
		if ($nrelease>count($dlist)*0.01) {
			$memdiff = $fstart*$maxmem-$chkmem;
			$adj = $nrelease/count($dlist);
			if ($memdiff>$maxmem*0.0001) {
				$chkmem -= $memdiff*(1./$adj - 1.);
				$reset = TRUE;
			}
			elseif ($nrelease>=count($dlist)) {
				$reset = TRUE;
			}
		}
		if ($reset && $chkmem>$basemem) {
			$basemem = $chkmem;
			if ($basemem/$maxmem > $fok) {
// Not setting any upper limit on $fok and $fskip ... effectively allowing maxmem to be overridden
// (If I'm using up all the memory without any dlist entries, then not upping maxmem is going to cause problems)
				$fok = $basemem/$maxmem*1.2;
				$fskip = $fok*1.2;
			}
		}
	}
	
	function block_release($ordid) {
		// don't delete data for a temporary item
		if (!is_integer($ordid))
			return TRUE;
		// these used to be dtime<10 ; dtime<120 && frel<1
		// but caused too many entries to be protected (so in-use data was being deleted instead of released data)
		// and then changed to dtime<3 ; dtime<30 && frel<1
		// and then changed again, along with rewrite of get_datarank to ensure that fully-released data is
		// always deleted before non-released data
		
		// this assumes that I am always accurately releasing items....
		// otherwise I need to add dtime as a factor
		if ($this->_datastats['nrel'][$ordid] < $this->_datastats['nuse'][$ordid])
			return TRUE;
		/*		$dtime = time()-$this->_datastats['time'][$ordid];
		$frel = $this->_datastats['nrel'][$ordid]/$this->_datastats['nuse'][$ordid];
		if ($dtime<3)
			return TRUE;
		if ($dtime<30 && $frel<1)
			return TRUE;*/
		return FALSE;
	}
	
	// 1 means a>b; -1 means b>a
	// 1 means b should be used instead of a
	// -1 means a should be used instead of b
	function get_datarank($ordida, $ordidb) {
		$blocka = $this->block_release($ordida);
		$blockb = $this->block_release($ordidb);
		if ($blocka && $blockb)
			return 0;
		elseif ($blocka)
			return 1;
		elseif ($blockb)
			return -1;
		
		$usea = $this->_datastats['nuse'][$ordida] - $this->_datastats['nrel'][$ordida];
		$useb = $this->_datastats['nuse'][$ordidb] - $this->_datastats['nrel'][$ordidb];
		
		// make sure nrel=nuse items have higher rank than ones with nrel<nuse
		if ($usea>0 && $useb<=0)
			return 1;
		elseif ($useb>0 && $usea<=0)
			return -1;
		// the fewer remaining uses, the better (will normally only be used to compare items that are
		// still being used -- so hopefully it's never used)
		elseif ($usea<$useb)
			return -1;
		elseif ($useb<$usea)
			return 1;
		// the fewer times it's been accessed, the better
		elseif ($this->_datastats['nuse'][$ordida]>$this->_datastats['nuse'][$ordidb])
			return 1;
		elseif ($this->_datastats['nuse'][$ordidb]>$this->_datastats['nuse'][$ordida])
			return -1;
		else
			// the longer it's been since it was accessed, the better
			return $this->_datastats['time'][$ordida] - $this->_datastats['time'][$ordidb];
	}
	/*function get_datarank($ordid) {
		$dtime = time()-$this->_datastats['time'][$ordid];
		$frel = $this->_datastats['nrel'][$ordid]/$this->_datastats['nuse'][$ordid];
		// these used to be dtime<10 ; dtime<120 && frel<1
		// but caused too many entries to be protected (so in-use data was being deleted instead of released data)
		// and then changed to dtime<3 ; dtime<30 && frel<1
		if ($dtime<3)
			return -1;
		if ($dtime<30 && $frel<1)
			return -1;
		// don't delete data for a temporary item
		if (!is_integer($ordid))
			return -1;
		// the longer it's been since it was accessed, the better (dtime)
		// the fewer the times it's been accessed, the better (1/nuse)
		// the more the times it's been released, the better (nrel)
		$rank = $dtime;
		// make sure nrel=nuse items have higher rank than ones with nrel<nuse
		if ($frel>=1) {
			$rank *= (1 + 10/$this->_datastats['nuse'][$ordid]);
		}
		else
			$rank *= $frel;
		return $rank;
	}*/
	
	function use_data($ordid) {
		if (is_null($ordid))
			return;
		//if ($ordid==38894) {
		//print "adding nuse for $ordid\n";
		//}
		if (!isset($this->_datastats['nuse'][$ordid])) {
			$this->_datastats['nuse'][$ordid] = 1;
			$this->_datastats['nrel'][$ordid] = 0;
		}
		else
			$this->_datastats['nuse'][$ordid]++;
		$this->_datastats['time'][$ordid] = time();
	}
	
	function release_data($ordid) {
		if (is_null($ordid))
			return;
		//if ($ordid==38894) {
		//print "doing release_data for $ordid\n";
		//}
		
		if (!array_key_exists('nrel', $this->_datastats) || !array_key_exists($ordid, $this->_datastats['nrel'])) {
			print "nrel not set for $ordid";
			if (!array_key_exists('nuse', $this->_datastats) || !array_key_exists($ordid, $this->_datastats['nuse']))
				print " nuse not set either";
			else
				print " nuse=".$this->_datastats['nuse'][$ordid]." dtime=".time()-$this->_datastats['time'][$ordid];
			print "\n";
			exit;
		}
		$this->_datastats['nrel'][$ordid]++;
	}
	
	function _unset_data($ordid) {
		if (is_null($ordid) || !array_key_exists($ordid, $this->_data) || is_null($this->_data[$ordid]))
			return;
		$this->_data[$ordid]->clean();
		unset($this->_data[$ordid]);
		unset($this->_datastats['nuse'][$ordid]);
		unset($this->_datastats['nrel'][$ordid]);
		unset($this->_datastats['time'][$ordid]);
	}
	
	function clean_data() {
		foreach ($this->_data as $ordid => $data) {
			$this->_unset_data($ordid);
		}
		$this->_data = array();
	}
		
	function do_query($query, $option=NULL) {
		global $gConnection;
		if (empty($this->_connection)) {
			print "mysql error: connection does not exist\n";
			exit;
		}
		elseif (!$gConnection->ping()) {
			$this->set_connection();
		}
		
		if (($this->_resource=$gConnection->query($query))===false) {
			print "mysql error ".$gConnection->errno.": ".$gConnection->error."\n  query: ".$query."\n";
			exit;
			return NULL;
		}
		
		if (is_null($option)) {
			return $this->_resource;
		}
		
		if (strtolower($option)=='getid') {
			return $gConnection->insert_id;
		}
		
		if (strtolower($option)=='onerow') {
			if ($this->_resource->num_rows==1) {
				return $this->row_query($this->_resource);
			}
			else {
				//				print "mysql error.  Multiple rows returned from query: {$query}\n";
				return NULL;
			}
		}
	}
	
	function row_query($resource=NULL, $querytype=MYSQLI_ASSOC) {
		if (is_null($resource))
			$resource = $this->_resource;
		
		if (!is_array($row = $resource->fetch_array($querytype))) {
			return false;
		}
		
		// how to identify boolean values and change them back to boolean?
		foreach ($row as $valname => $value) {
			if ($value===true || $value===false)
				continue;
			if (preg_match('/^\-?[0-9]{1,6}$/', $value)) {
				$row[$valname] = (integer) $value;
			}
			elseif (preg_match('/^\-?[0-9]+\.[0-9]*$/', $value)) {
				$row[$valname] = (float) $value;
			}
		}
		return $row;
	}
	
	function query_item_list($query) {
		$res = $this->do_query($query);
		$list = array();
		while ($this->row_query($res)) {
			$list[] = $row['ordid'];
		}
		return $list;
	}
	
	// need to add an option here to somhow limit read_db to only read a specific set of subrecords $subs = array("CELL_FRMR" => array(0, 20, 35)) ?
	// make sure I'm not assuming subids are all contiguous
	function newitem($rectype, $ordid=NULL, $data_toread=NULL) {
		$newitem = CSItem::newitem($rectype, $ordid, $data_toread, $this);
		if (!isset($newitem))
			return NULL;
		$this->_active = $newitem;
		
		$this->check_memory();
		if (is_null($ordid)) {
			$this->_active->set($this->_curr_nfile, 'srcfile');
			return $this->_active;
		}
		else {
			$this->_data[$ordid] = $this->_active;
			// this is necessary to prevent multiple copies of same item from being created (which can kill memory, and can cause problems during data initialization)
			// but it also means I need to do some of my own garbage collection, if I want items ever released from memory
			// clean_data is a start, but need something more intelligent
			// do cleanup whenever memory usage becomes too large; keep track of number of uses of an item; add ability to "release" an item
			// (i.e., when done with an item, call a release function that tells csdata I'm done with it)
			// delete from _data when nrelease=nuse
			//    won't be 100% reliable, because there's no guarantee release will be called.  But if important cases do release, then most memory should be recovered
			//    and any nrelease=nuse case will be completely safe
			// also keep track of time of last use?  delete oldest unused items?  
			// priority by rectype (REFR first, given they're not used by other items)?  or will that be automatic with nuse?
			$this->use_data($ordid);
			$this->_active->read_db($ordid);
			if ($this->_active->ordid()!=$ordid) {
				print "problem reading item ".$this->_active->id()." requested ordid={$ordid}, obtained ordid=".$this->_active->ordid()."\n";
			}
			return $this->_active;
		}
	}
	
	// Specifically created to handle copying player NPC_ record to an ACHR record
	// need to add new item to array -- deal with lack of ordid (use formid as ordid)
	// ref_id is normally formid (except for MW, where it needs to be an edid)
	function create_ref_item($srcitem, $ref_rectype, $ref_id=NULL) {
		$refitem = $srcitem->create_copy($ref_rectype, $ref_id);
		$id = $refitem->id();
		// NB this is NOT a true ordid but actually a refid/edid used for an item that doesn't exist in DB
		$refitem->set_ordid($id);
		$this->use_data($id);
		$this->_data[$id] = $refitem;
		return $refitem;
	}
	
	// add other created items from save files -- items that aren't in DB and don't have ordids
	function add_created_item($item) {
		$id = $item->id();
		$item->set_ordid($id);
		$this->use_data($id);
		$this->_data[$id] = $item;
	}
	
	function set_id($id) {
		if ($id===false || $id=='')
			return false;
		$this->_active->set_id($id);
		return $this->_active;
	}
	
	function add_active_to_data($active = NULL) {
		if (is_null($active))
			$active = $this->_active;
		if (is_null($ordid=$active->ordid())) {
			$ordid = $this->_ndata++;
			$active->set_ordid($ordid);
		}
		$this->use_data($ordid);
		$this->_data[$ordid] = $active;
		if ($this->_datamode==self::DATAMODE_READ)
			$this->_bytype[$active->get('rectype')][] = $ordid;
		return $ordid;
	}
	
	function inactive($keep=true) {
		// save current value of _active so that secondary processing can freely change it if appropriate
		$ordid = NULL;
		$active = $this->_active;
		if ($keep && $active->finish_init()) {
			// in DB mode, delete record after writing it, so rest of processing unnecessary
			if ($this->_datamode==self::DATAMODE_WRITE) {
				// write_db calls add_active_to_data (has to be done by write_db, because only write_db knows whether this is new data or identical data)
				$ordid = $active->write_db();
			}
			// if ordid already set, then it's already been added to array (perhaps by finish_init)
			elseif (!($ordid=$active->get('ordid')) && ($id=$active->id())) {
				$ordid = $this->add_active_to_data($active);
				// this all commented out because treatment of mods has been substantially changed, so I don't think it's relevant any more
				/*
				if (!($ordid=$this->get_ordid($id))) {
					$ordid = $this->_ndata++;
					$this->_active->set_ordid($ordid);
					$this->_data[$ordid] = $this->_active;
					$this->_bytype[$this->_active->get('rectype')][] = $ordid;
				}
				elseif ($this->_data[$ordid]->get('srcfile') != $this->_active->get('srcfile')) {
					$this->_active->set_ordid($ordid);
					$this->_active->set_mod($ordid);
					$this->_data[$ordid] = $this->_active;
				}
				else {
					print "Error: inactive is overwriting existing data\n";
					print "$id $ordid ".$this->_active->get('RGNN')." ".$this->_active->get('x')." ".$this->_active->get('y')."\n";
					print $this->_data[$ordid]->get('RGNN')." ".$this->_data[$ordid]->get('x')." ".$this->_data[$ordid]->get('y')."\n";
					print $this->_active->get('exterior')." ".$this->_active->id()." ".$this->_active->get('id')."\n";
					print $this->_data[$ordid]->get('exterior')." ".$this->_data[$ordid]->id()." ".$this->_data[$ordid]->get('id')."\n";
					exit;
					$this->_active->set_ordid($ordid);
					$this->_data[$ordid] = $this->_active;
				}
				*/
			}
		}
		
		unset($active);
		$this->_active->release();
		unset($this->_active);
		$this->_active = NULL;
		// in WRITE mode, scan all _data entries and delete (in case any others were created during processing,
		// in particular when processing CELL records)
		// turning off for now: seeing whether use/release system will work instead
		//		if ($this->_datamode==self::DATAMODE_WRITE)
		//			$this->clean_data();
		return $ordid;
	}
	
	function set_active($value, $valname, $index=NULL) {
		if (is_null($value))
			return;
		$this->_active->set($value, $valname, $index);
	}
	function get_active($valname, $index=NULL) {
		return $this->_active->get($valname, $index);
	}
	function set_active_index($valname, $increment=FALSE) {
		if (!$increment) {
			return $this->_active->get_index($valname);
		}
		else {
			return $this->_active->increment_index($valname);
		}
	}
	function get_active_index($valname) {
		return $this->_active->get_index($valname);
	}
	function reset_active_index($valname) {
		$this->_active->reset_index($valname);
	}
	function get_active_item() {
		return $this->_active;
	}
	
	function set_lookup($lookupname, $lookuparray) {
		$this->_lookup[$lookupname] = $lookuparray;
		$this->_make_lookup_rev($lookupname);
	}
	
	protected function _make_lookup_rev($lookupname) {
		unset($this->_lookup_rev[$lookupname]);
		foreach($this->_lookup[$lookupname] as $i => $value) {
			$this->_lookup_rev[$lookupname][$value] = $i;
		}
	}
	
	function get_lookup($lookupname, $val_id) {
		if (!isset($val_id) || !array_key_exists($lookupname, $this->_lookup) || !array_key_exists($val_id, $this->_lookup[$lookupname]))
			return NULL;
		return $this->_lookup[$lookupname][$val_id];
	}
	
	function get_lookup_id($lookupname, $value, $casesensitive=true, $addmissing=false) {
		if (!isset($value))
			return NULL;
		if (!array_key_exists($lookupname, $this->_lookup_rev)) {
			if ($addmissing) {
				$this->_lookup[$lookupname] = array(0 => $value);
				$this->_lookup_rev[$lookupname][$value] = 0;
				if ($this->_datamode==self::DATAMODE_WRITE) {
					$query = "INSERT INTO {$lookupname}_Lookup SET {$lookupname}_lu=0, {$lookupname}='".addslashes($value)."'";
					$this->do_query($query);
				}
				return 0;
			}
			else {
				return NULL;
			}
		}
		if (array_key_exists($value, $this->_lookup_rev[$lookupname]))
			return $this->_lookup_rev[$lookupname][$value];
		if (!$casesensitive) {
			$altvalue = strtolower(str_replace(' ', '_', $value));
			foreach ($this->_lookup_rev[$lookupname] as $name => $index) {
				$altname = strtolower(str_replace(' ', '_', $name));
				if ($altvalue==$altname)
					return $index;
			}
		}
		if ($addmissing) {
			$nc = count($this->_lookup[$lookupname]);
			$this->_lookup[$lookupname][$nc] = $value;
			$this->_lookup_rev[$lookupname][$value] = $nc;
			if ($this->_datamode==self::DATAMODE_WRITE) {
				$query = "INSERT INTO {$lookupname}_Lookup SET {$lookupname}_lu=$nc, {$lookupname}='".addslashes($value)."'";
				$this->do_query($query);
			}
			return $nc;
		}
		else
			return NULL;
	}
	
	function check_lookup($lookupname) {
		if (array_key_exists($lookupname, $this->_lookup))
			return true;
		else
			return false;
	}
	
	function get_lookup_list($lookupname) {
		if (!array_key_exists($lookupname, $this->_lookup))
			return NULL;
		return $this->_lookup[$lookupname];
	}
	
	function get_lookup_names() {
		return array_keys($this->_lookup);
	}
	
	function clear_datainfo($rectype) {
		if (array_key_exists($rectype, $this->_datatables)) {
		// need to also do sub-tables of rectype
		// debug this code and look at 
			foreach ($this->_datatables[$rectype] as $subrec => $subdata) {
				unset($this->_datainfo[$subrec]);
				unset($this->_datatables[$subrec]);
			}
		}
		unset($this->_datainfo[$rectype]);
		unset($this->_datatables[$rectype]);
	}
	
	function set_datainfo($rectype, $valname, $infoname, $infovalue) {
		if (is_null($valname))
			return NULL;
		$this->_datainfo[$rectype][$valname][$infoname] = $infovalue;
		if ($infoname=='sqltable')
			$this->_datatables[$rectype][$infovalue][] = $valname;
	}
	
	function get_datainfo($rectype, $val_id) {
		if (array_key_exists($rectype, $this->_datainfo) && array_key_exists($val_id, $this->_datainfo[$rectype]))
			return $this->_datainfo[$rectype][$val_id];
		else
			return NULL;
	}
	
	function get_dataname($rectype, $val_id) {
		if (array_key_exists($rectype, $this->_datainfo) && array_key_exists($val_id, $this->_datainfo[$rectype]))
			return $this->_datainfo[$rectype][$val_id]['name'];
		else
			return NULL;
	}
	
	function set_dataoptions($rectype, $optname, $optvalue) {
		if (is_null($optname))
			return NULL;
		$this->_dataoptions[$rectype][$optname] = $optvalue;
	}
	
	function get_dataoptions($rectype, $optname=NULL) {
		if (!isset($this->_dataoptions[$rectype])) {
			if (!isset($optname))
				return array();
			else
				return NULL;
		}
		elseif (!isset($optname))
			return $this->_dataoptions[$rectype];
		elseif (isset($this->_dataoptions[$rectype][$optname]))
			return $this->_dataoptions[$rectype][$optname];
		else
			return NULL;
	}
	
	function get_datatables($rectype=NULL, $table=NULL) {
		if (is_null($table)) {
			if (is_null($rectype)) {
				$tables = array();
				foreach ($this->_datatables as $rectype => $data) {
					if ($rectype=='')
						continue;
					$tables = array_merge($tables, array_keys($data));
				}
				return $tables;
			}
			else {
				if (array_key_exists($rectype, $this->_datatables))
					return array_keys($this->_datatables[$rectype]);
				else
					return array();
			}
		}
		elseif (is_null($rectype)) {
			$valnames = array();
			foreach ($this->_datatables as $rectype => $data) {
				if ($rectype=='')
					continue;
				foreach ($data as $table => $tvals) {
					if ($table!=$table)
						continue;
					$valnames = array_merge($valnames, $tvals);
				}
			}
			return $valnames;
		}
		else {
			if (isset($this->_datatables[$rectype][$table]))
				return $this->_datatables[$rectype][$table];
			else
				return array();
		}
	}

	function get_rectypes() {
		$rectypes = array();
		foreach ($this->_datatables as $rectype => $data) {
			if ($rectype=='')
				continue;
			$rectypes[] = $rectype;
		}
		return $rectypes;
	}
	
	// This only handles raw values -- handling derived values has to be done in csitem
	function is_datachecked($rectype, $valname) {
		// passing NULL for rectype means I just want to know whether check is being done at all
		if (!isset($rectype))
			return (empty($this->_datachecked)?FALSE:TRUE);
		// return TRUE for all variables if I'm not tracking which individual values have been checked
		if (empty($this->_datachecked))
			return TRUE;
		// basic values that aren't in datainfo and are assumed to always be correct
		if (in_array(strtolower($valname), array('ordid', 'id', 'edid', 'formid', 'rectype', 'fileid', 'name')))
			return TRUE;
		if (isset($this->_datainfo[''][$valname]))
			$baserec = '';
		else
			$baserec = $rectype;
		if (isset($this->_datainfo[$baserec][$valname]) || isset($this->_datachecked[$baserec][$valname]))
			$baseval = $valname;
		elseif (isset($this->_datainfo[$baserec][$valname.'_lu']) || isset($this->_datachecked[$baserec][$valname.'_lu']))
			$baseval = $valname.'_lu';
		elseif (isset($this->_datainfo[$baserec][$valname.'_id']) || isset($this->_datachecked[$baserec][$valname.'_id']))
			$baseval = $valname.'_id';
		else
			$baseval = $valname;
		if (!empty($this->_datachecked[$baserec][$baseval]))
			return TRUE;
		else
			return FALSE;
	}
	
	function set_curr_nfile($nfile) {
		$this->_curr_nfile = $nfile;
	}
	
	function add_file($nfile, $file_name, $file_ns, $file_mod, $file_abbrev) {
		$this->_curr_nfile = $nfile;
		$this->_file_name[$nfile] = $file_name;
		$this->_file_ns[$nfile] = $file_ns;
		$this->_file_mod[$nfile] = $file_mod;
		$this->_file_abbrev[$nfile] = $file_abbrev;
	}
		
	function get_file_name($nfile) {
		if (array_key_exists($nfile, $this->_file_name))
			return $this->_file_name[$nfile];
		else
			return NULL;
	}

	function get_file_ns($nfile) {
		if (array_key_exists($nfile, $this->_file_ns))
			return $this->_file_ns[$nfile];
		else
			return NULL;
	}

	function get_file_mod($nfile) {
		if (array_key_exists($nfile, $this->_file_mod))
			return $this->_file_mod[$nfile];
		else
			return NULL;
	}
		
	function get_file_abbrev($nfile) {
		if (array_key_exists($nfile, $this->_file_abbrev))
			return $this->_file_abbrev[$nfile];
		else
			return NULL;
	}

	function get_fileids_for_abbrev($abbrev) {
		$fileids = array();
		foreach ($this->_file_abbrev as $fileid => $modabbrev) {
			if ($abbrev==$modabbrev)
				$fileids[] = $fileid;
		}
		return $fileids;
	}
		
	// these four get functions are both set up so that the input parameter can be any unique item identifier
	// * an object is assumed to be the item object
	// * an integer is assumed to be the ordid value
	// * a 10-character length string starting with 0x ($this->get_formid()===true) is assumed to be a formid
	// any other value is assumed to be an editor ID

	// one problem is that edid and formid are not truly unique
	// * in Morrowind, DIAL edids can be the same as non-DIAL edids -> need to know desired rectype (non-DIAL assumed unless overridden)
	// * if an object's information is updated/overwritten by a mod -> need to know desired fileid and/or mod (handled via mod_status... see below)
	// * code also includes options to match a cell edid if edid includes coordinates (presumed to be specifically for MW, but not sure right now)
	// * also on original data read, rectype has limited usefulness -> existing record may be a placeholder without any rectype
	//   handled by special match_fileid option used specifically by CSItem:write_db()

	// in typical usage, fileid should actually be a string specifying the modabbrev which should be searched, in which case
	//   the item with mod_status&0x4 in that mod is found
	// fileid should only be provided as a number (true fileid) in the context of original data-processing/reading
	//   if fileid is specified, then mod_status is ignored

	// originally, default was strictmatch=TRUE (plus called create_placeholder_ordid automatically)
	function get_ordid($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		// input is missing
		if (is_null($input) || $input=='')
			return NULL;
		// input is an item object
		if (is_object($input))
			return $input->ordid();
		// input is an ordid
		if (is_int($input) || ctype_digit($input))
			return $input;
		// fake ordids (for temporary records that don't exist in DB) -- ordid isn't integer, but it's the key used by _data
		if (isset($this->_data[$input]))
			return $input;
		// prev_fileid is special option for write_db when looking for comparison record
		// -- can simply skip if there are no previous fileids
		if (!empty($options['prev_fileid']) && (empty($fileid) || !is_integer($fileid)))
			return NULL;
		
		// easy options checked... need to find this item
		$formid = $this->is_formid($input);
		$modabbrev = NULL;
		if (!is_null($fileid) && !is_integer($fileid)) {
			$modabbrev = $fileid;
			$fileid = NULL;
		}
		if ($this->_datamode==self::DATAMODE_READ) {
			// Old-style datamode.
			// Has not been retrofitted to have strictmatch=false features (assuming that
			// the data that was read in was already limited to the appropriate range; strictmatch=false mainly
			// useful to narrow down options when processing every record in the game)
			// Also only limited retrofits based on mod_status -> mod_status is not currently being
			// set in this datamode
			if (!is_null($rectype))
				$types = array($rectype);
			else
				$types = array_keys($this->_bytype);
			foreach ($types as $type) {
				foreach ($this->_bytype[$type] as $ordid) {
					if (!array_key_exists($ordid, $this->_data))
						continue;
					if (!is_null($fileid)) {
						if ($this->_data[$ordid]->fileid()!=$fileid)
							continue;
					}
					if (!is_null($modabbrev)) {
						// unlike database-searches, this is not taking into account mod_status
						// (and also won't use base version of item if none found in that mod)
						if ($this->get_file_abbrev($this->_data[$ordid]->fileid())!=$modabbrev)
							continue;
					}
					if ((!$formid && $this->_data[$ordid]->edid()==$input) || ($formid && $this->_data[$ordid]->formid()==$formid))
						return $ordid;
				}
			}
		}
		else {
			$query = "SELECT ordid, formid, edid, rectype, fileid, name FROM AllItems WHERE ";
			$queryex = '';
			if ($formid)
				$query .= "formid='".addslashes($formid)."'";
			else
				$query .= "edid='".addslashes($input)."'";
			if ((!empty($options['strictmatch']) || !empty($options['prev_fileid'])) && !is_null($rectype)) {
				$query .= " AND rectype='{$rectype}'";
			}
			elseif (!empty($options['match_fileid'])) {
				$query .= " AND (rectype='{$rectype}' OR rectype IS NULL)";
			}

			if (!isset($fileid) && !isset($modabbrev)) {
				// simply request the default version of this item
				$queryex .= ' AND mod_status&0x02>0 ';
			}
			elseif (isset($fileid)) {
				// non-standard usage -- only current use is by CSItem::write_db()
				if (!empty($options['prev_fileid'])) {
					// special request for highest previous fileid -> again, specific to write_db()
					$queryex .= " AND fileid<{$fileid}";
				}
				else {
					$queryex .= " AND fileid={$fileid}";
				}
			}
			elseif (isset($modabbrev)) {
			// otherwise, request all current versions and sort out mods below
			// (could probably be done more efficiently using some fileid limits in mysql)
				$queryex .= ' AND mod_status&0x04>0 ';
			}
			if (isset($fileid)) 
				$queryex .= " ORDER BY fileid desc, ordid desc";
			else
				$queryex .= " ORDER BY mod_status&0x02 desc, ordid desc";
			$allrows = array();
			if (!is_null($res = $this->do_query($query.$queryex)) && ($nrow=$res->num_rows)>0) {
				if ($nrow==1) {
					$allrows[] = $this->row_query($res);
				}
				else {
					// multiple rows can be returned because:
					// * record is revised by mod files -> handled in subsequent sections via mod_status/fileid
					// * record edid is shared by DIAL entry and non-DIAL entry
					$prefrows = array();
					while ($row=$this->row_query($res)) {
						if (!is_null($rectype) && $row['rectype']==$rectype) {
							$prefrows[] = $row;
						}
						elseif (is_null($rectype) && $row['rectype']!='DIAL') {
							$prefrows[] = $row;
						}
						else {
							$allrows[] = $row;
						}
					}
					if (!empty($prefrows)) {
						$allrows = $prefrows;
					}
					
				}
			}

			if (empty($allrows) && !$formid && (empty($rectype) || $rectype=='CELL')) {
			// check for a cell with that name which has had x,y coords tacked on to it, but only if cell name is unique
			// -- the uniqueness check doesn't necessarily interact well with fileid/mod_status checks... but I think this check is only relevant to MW
				$query = "SELECT ordid, rectype, edid, fileid FROM AllItems WHERE edid like '".addslashes($input)." [%]' AND rectype='CELL'".$queryex;
				if (!is_null($row = $this->do_query($query, 'onerow'))) {
					$allrows[] = $row;
				}
			}

			if (!empty($allrows)) {
				if (empty($options['strictmatch']) && count($allrows)==1)
					return (integer) $allrows[0]['ordid'];

				if (isset($modabbrev) || !empty($options['prev_fileid'])) {
					$mod0 = $this->get_file_abbrev(0);
					if (!isset($modabbrev))
						$modabbrev = $this->get_file_abbrev($fileid);
					$filerows = array();
					foreach ($allrows as $row) {
						$filerows[$this->get_file_abbrev($row['fileid'])][] = $row;
					}
				
					if (array_key_exists($modabbrev, $filerows)) {
						$allrows = $filerows[$modabbrev];
					}
					elseif (array_key_exists($mod0, $filerows)) {
						$allrows = $filerows[$mod0];
					}
					elseif (!empty($options['strictmatch']))
						$allrows = array();
				}
				elseif (!empty($options['prev_fileid'])) {
				}

				// either there's only value left at this point... or else I just need to make an arbitrary choice
				// (most of arbitrariness already taken care of in query's ORDER BY -> primarily choosing highest value of ordid)
				// non-unique issues mainly encountered for interior cells with multiple identical names (esp Balmora)
				if (!empty($allrows))
					return (integer) $allrows[0]['ordid'];
					
				//					print "unable to choose which ordid to use for edid {$input}\n";
				//					print var_dump($allrows);
				//					exit;
			}
		}
		return NULL;
	}

	function create_placeholder_ordid($input, $rectype=NULL) {
		if ($this->_datamode!=self::DATAMODE_WRITE)
			return NULL;
		
		$formid = $this->is_formid($input);
		$query = "INSERT INTO AllItems SET";
		if ($formid)
			$query .= " formid='".addslashes($formid)."'";
		else
			$query .= " edid='".addslashes($input)."'";
		if (!is_null($this->_curr_nfile))
			$query .= ", fileid=".$this->_curr_nfile;
		// need to set mod_status&0x06 so subsequent calls to get_ordid will find this record
		// not sure whether to also set mod_status&0x01 -- will that get set properly once record truly created?
		$query .= ", mod_status=6";
		$ordid = $this->do_query($query, 'getid');
		return (integer) $ordid;
	}
	
	function is_formid($input) {
		if (is_null($input) || !is_string($input))
			return NULL;
		if (preg_match('/0x\d{2}[\da-f]{6}/i', $input))
			return $input;
		elseif (substr($input,0,2)!='0x' || strlen($input)<10)
			return FALSE;
		else {
			$modid = strtoupper(substr($input,2,2));
			$fileid = NULL;
			foreach ($this->_file_abbrev as $i => $abbrev) {
				if ($modid==$abbrev) {
					$fileid = max(1,$i);
					break;
				}
			}
			if (!isset($fileid)) {
				if (preg_match('/[\da-f]{2}/i', $modid))
					return $input;
				else
					return FALSE;
			}
			return '0x'.sprintf('%02x',$fileid).substr($input,4);
		}
	}

	// Display formid using mod_abbrev for first two chars instead of internal fileid
	function get_mod_formid($value) {
		if (substr($value,0,2)!='0x' && ctype_digit($value)) {
			$value = '0x'.sprintf('%08lx', $value);
		}
		$filenum = substr($value,2,2);
		if (!$filenum || $filenum==='00' || preg_match('/^[a-z]{2}$/', $filenum))
			return $value;
		$abbrev = $this->get_file_abbrev(hexdec($filenum));
		if (empty($abbrev)) {
			$abbrev = $filenum;
		}
		return '0x'.$abbrev.substr($value,4);
	}
	
	function get_item($input, $fileid=NULL, $rectype=NULL, $options=array(), $data_toread=NULL) {
		if (is_object($input))
			return $input;
		if (!($ordid = $this->get_ordid($input, $fileid, $rectype, $options)) || is_null($ordid))
			return NULL;
		if (isset($this->_data[$ordid])) {
			if ($this->_datamode!=self::DATAMODE_READ) {
				$this->_data[$ordid]->read_db($ordid, $data_toread);
			}
			$this->use_data($ordid);
			return $this->_data[$ordid];
		}
		elseif ($this->_datamode!=self::DATAMODE_READ) {
			return $this->newitem($rectype, $ordid, $data_toread);
		}
		else
			return NULL;
	}
	
	// this is specifically for temporary items that are used once to extract a variable then discarded
	function get_t_item($input, $fileid=NULL, $rectype=NULL, $options=array(), $data_toread=NULL) {
		$item = $this->get_item($input, $fileid, $rectype, $options, $data_toread);
		if (is_object($item)) {
			$item->release();
		}
		return $item;
	}
	
	protected function _get_anyid($anyid, $input, $fileid=NULL, $rectype=NULL, $options=array()) {
		if (is_object($input))
			return $input->get($anyid);
		if (!is_int($input) && !ctype_digit($input)) {
			$formid = $this->is_formid($input);
			if ($formid!==FALSE && $anyid=='formid')
				return $formid;
		}
		$ordid = $this->get_ordid($input, $fileid, $rectype, $options);
		if (array_key_exists($ordid, $this->_data))
			return $this->_data[$ordid]->get($anyid);
		elseif (($stats=$this->get_basicstats($ordid)))
			return $stats[$anyid];
		elseif ($item=$this->get_t_item($ordid, $fileid, $rectype, $options, true))
			return $item->edid();
		else
			return NULL;
	}
	function get_edid($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		return $this->_get_anyid('edid', $input, $fileid, $rectype, $options);
	}
	function get_formid($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		return $this->_get_anyid('formid', $input, $fileid, $rectype, $options);
	}
	function get_name($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		return $this->_get_anyid('name', $input, $fileid, $rectype, $options);
	}
	// overwritten by MWData
	function get_id($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		return $this->_get_anyid('formid', $input, $fileid, $rectype, $options);
	}
	function get_basicstats($ordid) {
		if (is_null($ordid))
			return NULL;
		if ($this->_datamode==self::DATAMODE_READ)
			return NULL;
		$query = "SELECT * from AllItems WHERE ordid={$ordid}\n";
		return ($this->do_query($query, 'onerow'));
	}
	
	function get_rectype($ordid, &$has_formid=NULL) {
		if (is_null($ordid))
			return NULL;
		if (array_key_exists($ordid, $this->_data))
			return $this->_data[$ordid]->get('rectype');
		if ($this->_datamode==self::DATAMODE_READ)
			return NULL;
		$query = "SELECT rectype, formid FROM AllItems WHERE ordid={$ordid}";
		if (($row = $this->do_query($query, 'onerow'))) {
			if (isset($row['rectype']))
				return $row['rectype'];
			elseif (!empty($row['formid']) && isset($has_formid))
				$has_formid = TRUE;
		}
		return NULL;
	}

	function find_ordid_uses($ordid) {
		$uses = array();
		$anyid = 'edid';
		// datatables list should not return AllItems, so I shouldn't just get a listing of the original record
		foreach ($this->get_datatables() as $table) {
			$rectype = substr($table,0,4);
			$valnames = $this->get_datatables($rectype, $table);
			$chkvals = array();
			foreach ($valnames as $valname) {
				$datainfo = $this->get_datainfo($rectype, $valname);
				if (array_key_exists('format', $datainfo) && $datainfo['format']==$anyid) {
					$chkvals[] = $datainfo['sqlname'];
				}
			}
			if (count($chkvals)) {
				$query = "SELECT * FROM $table WHERE ";
				for ($i=0; $i<count($chkvals); $i++) {
					if ($i)
						$query .= " OR ";
					$query .= " ".$chkvals[$i]."='{$ordid}'";
				}
				$res = $this->do_query($query);
				while ($row=$this->row_query($res)) {
					$use = $table." ordid=".$row['ordid']." edid=".$this->get_edid($row['ordid']);
					if (array_key_exists('subid', $row))
						$use .= " subid=".$row['subid'];
					foreach ($chkvals as $chk) {
						$use .= " {$chk}=".$row[$chk];
					}
					print "$use\n";
					$uses[] = $use;
				}
			}
		}
		return $uses;
	}
	
	function get_item_from_name($name, $type=NULL, $getall=false) {
		$allmatches = array();
		if ($this->_datamode==self::DATAMODE_READ) {
			if (is_null($type)) {
				foreach ($this->_data as $id => $item) {
					if (strtolower($item->name()) == strtolower($name))
						$allmatches[] = $item;
				}
			}
			elseif (array_key_exists($type, $this->_bytype)) {
				foreach ($this->_bytype[$type] as $item) {
					if (strtolower($item->name()) == strtolower($name))
						$allmatches[] = $item;
				}
			}
		}
		else {
			$query = "SELECT * from AllItems WHERE lower(name)='".addslashes(strtolower($name))."'";
			if (!is_null($type))
				$query .= " AND rectype='".$type."'";
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$allmatches[] = $this->get_item($row['ordid']);
			}
		}
		
		if (!count($allmatches))
			return false;
		usort($allmatches, 'sort_by_commonness');
		if ($getall)
			return $allmatches;
		else {
			for ($i=1; $i<count($allmatches); $i++)
				$allmatches[$i]->release();
			return $allmatches[0];
		}
	}
	
	// from here on functions need revamping/updating
	function list_items_by_type($type) {
		return $this->_bytype[$type];
	}
	
	function find($name, $type=NULL, $getall=false) {
		$allmatches = array();
		if (is_null($type)) {
			foreach ($this->_data as $id => $item) {
				if (strtolower($item->name()) == strtolower($name))
					$allmatches[] = $item;
			}
		}
		elseif (array_key_exists($type, $this->_bytype)) {
			foreach ($this->_bytype[$type] as $item) {
				if (strtolower($item->name()) == strtolower($name))
					$allmatches[] = $item;
			}
		}
		
		if (!count($allmatches))
			return false;
		usort($allmatches, 'sort_by_commonness');
		if ($getall)
			return $allmatches;
		else
			return $allmatches[0];
	}
	
	function find_id($id, $missingok=true) {
		if (array_key_exists($id, $this->_data))
			return $this->_data[$id];
		if ($missingok)
			return false;
// assume that problem finding an item by id is a coding error and stop code instead of cascading into other problems
		print "Unable to find item with id $id\n";
		exit();
	}
	
	function find_icon_matches($srcitem) {
		$outlist = array();
		$type = $srcitem->get('rectype');
		foreach ($this->_bytype[$type] as $item) {
			if ($item->id()==$srcitem->id())
				continue;
			// first check ensures no right pauldron/left pauldron matches (if item is not armor, it will just be ''==''
			if ($srcitem->get('armortype')==$item->get('armortype')) {
				if ($srcitem->icon()==$item->icon() || $srcitem->get('MODL')==$item->get('MODL'))
					$outlist[] = $item;
				elseif ($srcitem->get_index('BNAM')!==FALSE && $item->get_index('BNAM')!==FALSE && $srcitem->get('BNAM',0)==$item->get('BNAM',0))
					$outlist[] = $item;
			}
		}
		return $outlist;
	}
	
	function get_item_groups($itemtype, $grouptype, $option=NULL, $parent=NULL) {
		$output = array();
		$list = array();
		if ($grouptype=='styles') {
			if (array_key_exists($itemtype, $this->_styles))
				$list = $this->_styles[$itemtype];
		}
		foreach ($list as $key => $flag) {
			if ($option=='sub') {
				if (array_key_exists('parent', $flag) && $flag['parent']==$parent)
					array_push($output, $key);
			}
			else if ($option=='parentonly') {
				if (array_key_exists('parentonly', $flag) && $flag['parentonly'])
					array_push($output, $key);
			}
			else if ($flag['primary'] || $option=='all')
				array_push($output, $key);
		}
		return $output;
	}
	
	function item_group_parent($itemtype, $grouptype, $group) {
		if ($grouptype=='styles') {
			if (!array_key_exists($itemtype, $this->_styles) || !array_key_exists($group, $this->_styles[$itemtype]))
				return NULL;
			if (!array_key_exists('parent', $this->_styles[$itemtype][$group]))
				return NULL;
			return $this->_styles[$itemtype][$group]['parent'];
		}
		else {
			return NULL;
		}
	}
	
	function get_item_set($itemtype, $group, $option='core') {
		$possible = array();
		$matchstr = preg_replace('/[\s_]+/', '[\s_]+', $group);
		if (array_key_exists('idname', $this->_types[$itemtype]))
			$matchstr = '('.$matchstr.'|'.$this->_types[$itemtype]['idname'].')';
		foreach ($this->_bytype[$itemtype] as $item) {
			if (stristr($item->name(), $group) || preg_match('/'.$matchstr.'/i', $item->edid())) {
				if (array_key_exists('exclude', $this->_types[$itemtype]) &&
				    preg_match('/'.$this->_types[$itemtype]['exclude'].'/i', $item->edid()))
					continue;
				$key = $item->get($this->_keyvalues[$itemtype]);
				$possible[$key][] = $item;
			}
		}
		foreach ($possible as $type => $list) {
			usort($possible[$type], 'sort_by_commonness');
		}
		$corelist = array();
		foreach ($this->_types[$itemtype] as $key => $flags) {
			if (($item=$this->get_fixed_item($itemtype, $group, $key))) {
				$corelist[$key] = $item;
				continue;
			}
				
			if (array_key_exists('equiv', $flags) && array_key_exists($flags['equiv'], $corelist)) {
				if (!array_key_exists($key, $possible) || !is_null($corelist[$flags['equiv']]))
					continue;
				$corelist[$key] = array_shift($possible[$key]);
				unset($corelist[$flags['equiv']]);
			}
			elseif (!array_key_exists($key, $possible))
				$corelist[$key] = NULL;
			else {
				$corelist[$key] = array_shift($possible[$key]);
			}
		}
		if ($option=='core')
			return $corelist;
		
		$done = array();
		$listall = array();
		$previtem = NULL;
		foreach ($corelist as $key => $item) {
			if (is_null($item))
				continue;
			$listall[$item->id()] = array('core' => 1, 'base' => 1, 'group' => $group, 'item' => $item);
			
			if (array_key_exists('pair', $this->_types[$itemtype][$key]) && !is_null($previtem) && $this->_types[$itemtype][$key]['pair']==$previtem->get($this->_keyvalues[$itemtype])) {
				$listall[$previtem->id()]['pair'][] = $item;
			}
			$done[$item->id()] = true;
			$previtem = $item;
		}
			
		// add all items before continuing to next step so that pair checks will work
		foreach ($this->_types[$itemtype] as $key => $flags) {
			$checkfix = false;
			if (array_key_exists($itemtype, $this->_fixed_group_items) && array_key_exists($group, $this->_fixed_group_items[$itemtype]) && array_key_exists($key, $this->_fixed_group_items[$itemtype][$group])) {
				$checkfix =& $this->_fixed_group_items[$itemtype][$group][$key];
				foreach ($checkfix as $item_id => $flags) {
					$item = $this->find_id($item_id);
					if ($item && (!array_key_exists($item->id(), $done) || !$done[$item->id()]))
						$possible[$key][] = $item;
				}
			}
			if (!array_key_exists($key, $possible) || !count($possible[$key]))
				continue;
			usort($possible[$key], 'sort_by_commonness');
		}
		
		foreach ($this->_types[$itemtype] as $key => $flags) {
			if (!array_key_exists($key, $possible) || !count($possible[$key]))
				continue;
			$checkfix = false;
			if (array_key_exists($itemtype, $this->_fixed_group_items) && array_key_exists($group, $this->_fixed_group_items[$itemtype]) && array_key_exists($key, $this->_fixed_group_items[$itemtype][$group])) {
				$checkfix =& $this->_fixed_group_items[$itemtype][$group][$key];
			}
			
			foreach ($possible[$key] as $i => $item) {
				if (array_key_exists($item->id(), $done) && $done[$item->id()])
					continue;
				
				$listall[$item->id()] = array('item' => $item);
				$done[$item->id()] = true;
				
				if ($checkfix) {
					$item_id = $item->edid();
					if (array_key_exists($item_id, $checkfix)) {
						foreach (array('core', 'base') as $chk) {
							if (array_key_exists($chk, $checkfix[$item_id])) {
								$listall[$item->id()][$chk] = $checkfix[$item_id][$chk];
							}
						}
					}
				}
				if (!array_key_exists('core', $listall[$item->id()]))
					$listall[$item->id()]['core'] = 0;
				if (!array_key_exists('base', $listall[$item->id()])) {
					if ($item->get_isenchanted())
						$listall[$item->id()]['base'] = 1;
					else
						$listall[$item->id()]['base'] = 0;
				}
				if (!array_key_exists('group', $listall[$item->id()]))
					$listall[$item->id()]['group'] = 'Other Items';
				
				if (array_key_exists('pair', $this->_types[$itemtype][$key])) {
					// this replace will pick up more than just l as abbreviation for left, but doesn't matter for the comparison being done
					$ed = preg_replace('/(l(eft)?|r(ight)?)/i', '', $item->edid());
					$pairkey = $this->_types[$itemtype][$key]['pair'];
					if (array_key_exists($pairkey, $possible)) {
						foreach ($possible[$pairkey] as $ip => $pairitem) {
							if (array_key_exists($pairitem->id(), $done) && $done[$pairitem->id()])
								continue;
							$paired = preg_replace('/(l(eft)?|r(ight)?)/i', '', $pairitem->edid());
							if ($paired==$ed) {
								$listall[$item->id()]['pair'][] = $pairitem;
								break;
							}
						}
					}
				}
			}
		}
		
		return $listall;
	}

	function get_fixed_item($itemtype, $group, $key) {
		if (!array_key_exists($itemtype, $this->_fixed_group_items))
			return false;
		if (!array_key_exists($group, $this->_fixed_group_items[$itemtype]))
			return false;
		if (!array_key_exists($key, $this->_fixed_group_items[$itemtype][$group]))
			return false;
		foreach ($this->_fixed_group_items[$itemtype][$group][$key] as $item_id => $flags) {
			if (!array_key_exists('core', $flags))
			    continue;
			$item = $this->find_id($item_id);
			if ($item)
				return $item;
		}
		return false;
	}
		
	function get_types_data($rectype, $type, $value) {
		if (!array_key_exists($rectype, $this->_types) || !array_key_exists($type, $this->_types[$rectype]) || !array_key_exists($value, $this->_types[$rectype][$type]))
			return NULL;
		else
			return $this->_types[$rectype][$type][$value];
	}
	
	// this will only work with DB version of code
	function find_locs($item) {
		if ($this->get_datamode()==self::DATAMODE_READ) {
			print "find_locs function cannot be used in READ datamode\n";
			exit;
		}
		$item_ordid = $this->get_ordid($item);
		foreach (array("REFR", "ACHR", "ACRE") as $reftype) {
			$query = "SELECT * FROM {$reftype}_Record LEFT JOIN LevelList on ({$reftype}_Record.base_id=LevelList.ordid)
				WHERE LevelList.item_ordid={$item} AND LevelList.pav>0.99";
		}
		$ordid = $this->get_ordid($input);
		foreach ($this->_bytype['CELL'] as $cell) {
			$indices = $this->_data[$cell]->indices('FRMR');
			if (is_null($indices))
				continue;
			foreach ($indices as $i) {
				$ref_ordid = $this->_data[$cell]->get('refname', $i);
				if ($ref_ordid==$ordid) {
					print $this->_data[$cell]->id()."\n";
				}
				else {
					$refitem = $this->get_item($ref_ordid);
					if (($prob=($refitem->hasitem($ordid)))) {
						print $this->_data[$cell]->id()." ".$refitem->id()." ".$prob."\n";
					}
				}
			}
		}
	}
	
	function get_item_ids($type) {
		$output = array();
		foreach ($this->_bytype[$type] as $item) {
			array_push($output, $item->id());
		}
		return $output;
	}
	
	function search_cleanup($search, $searchopt) {
		$checklist = array();
		foreach ($search as $type => $list) {
			if (is_null($list))
				$checklist = array_merge($checklist, $this->_bytype[$type]);
			else {
				foreach ($list as $edid) {
					if ($ordid=$this->get_ordid($edid))
						$checklist[] = $ordid;
				}
			}
		}
		print "checklist\n";
		var_dump($checklist);
		
		$keep = array();
		foreach ($checklist as $chkordid) {
			$random = false;
			foreach (array('LEVI', 'NPC_', 'CREA', 'CONT') as $rectype) {
				foreach ($this->_bytype[$rectype] as $ordid) {
					$item = $this->get_item($ordid);
					if (is_null($item))
						continue;
					if (($prob=$item->hasitem($chkordid))) {
						if ($prob>0.99)
							$keep[$ordid] = true;
						else {
							$random = true;
							if (!($searchopt&1))
								$keep[$ordid] = true;
						}
					}
				}
			}
			if ($random) {
			}
		}
		
		foreach (array('LEVI', 'NPC_', 'CREA', 'CONT') as $rectype) {
			$newreclist = array();
			foreach ($this->_bytype[$rectype] as $ordid) {
				if (!array_key_exists($ordid, $keep) || !$keep[$ordid])
					unset ($this->_data[$ordid]);
				else
					$newreclist[] = $ordid;
			}
			$newreclist = array();
			$this->_bytype[$rectype] = $newreclist;
		}
	}
	
	//default is OB version; MWData::convert_to_cell exists for MW version
	// this is also the appropriate conversion for SR
	function convert_to_cell($location) {
		$cell = floor($location/4096);
		return $cell;
	}
	
	// accepts either form of x/y coordinates, and attempts to figure out which type of coordinates they are
	// (checks both magnitude and whether numbers are integer/float, so should be very reliable)
	protected function _get_cell_idany($id, $x, $y, $world, $fileid) {
		$cellcoords = true;
		if (abs($x)>50 || abs($y)>50)
			$cellcoords = false;
		elseif (is_float($x) || is_float($y))
			$cellcoords = false;
		elseif (is_string($x) && preg_match('/\./', $x))
			$cellcoords = false;
		elseif (is_string($y) && preg_match('/\./', $y))
			$cellcoords = false;
		if (!$cellcoords) {
			$x = $this->convert_to_cell($x);
			$y = $this->convert_to_cell($y);
		}
		$query = "SELECT *
			FROM CELL_Record LEFT JOIN AllItems USING (ordid)
			WHERE exterior=TRUE AND mod_status&0x02>0 AND cell_x={$x} AND cell_y={$y}";
		// this was originally doing 'AND worldspace={$world}'
		// but worldspace isn't a field any more (it's worldspace_lu), and I think really want to
		// tie to the exact same world, not just a superimposed world
		if (!is_null($world))
			$query .= " AND world_id={$world}";
// fileid in search disabled when mod_status introduced -- to re-add need to generalize to a mod_abbrev match, not a fileid match
// (plus primary purpose of the condition has been replaced by mod_status check)
/*		if (!is_null($fileid))
			$query .= " AND (fileid={$fileid} OR fileid=0) ORDER BY fileid DESC";
		else
			$query .= " ORDER BY fileid";*/
		$res = $this->do_query($query);
		if ($row = $this->row_query($res))
			return $row[$id];
		else
			return NULL;
	}
	function get_cell_ordid($x, $y, $world=NULL, $fileid=NULL) {
		return $this->_get_cell_idany('ordid', $x, $y, $world, $fileid);
	}
	function get_cell_edid($x, $y, $world=NULL, $fileid=NULL) {
		return $this->_get_cell_idany('edid', $x, $y, $world, $fileid);
	}
	function get_cell_formid($x, $y, $world=NULL, $fileid=NULL) {
		return $this->_get_cell_idany('formid', $x, $y, $world, $fileid);
	}
	
	function set_pc_level($pc_level) {
		$this->_pc_level = $pc_level;
	}
	
	function get_pc_level() {
		return $this->_pc_level;
	}
}

// primarily sorts based on name length
// but underlying purpose is to list the most common item first in a list
// therefore "unique" named items always come later than other items (e.g., Ebony Mail (artifact) comes after Ebony Cuirass)
function sort_by_commonness($a, $b) {
	if ($a->get_isenchanted()!=$b->get_isenchanted())
		return (($a->get_isenchanted() < $b->get_isenchanted()) ? -1 : 1);
	
	$edida = $a->edid();
	$edidb = $b->edid();
	if (strlen($a->get('name')) == strlen($b->get('name'))) {
		if (strlen($edida)==strlen($edidb))
			return strcmp($edida, $edidb);
		else
			return ((strlen($edida) < strlen($edidb)) ? -1 : 1);
	}
	else
		return ((strlen($a->get('name')) < strlen($b->get('name'))) ? -1 : 1);
}

// this includes game-specific code.  But I haven't experimented yet with how to make sort functions be class members
function cell_edid_sort($a, $b) {
	$edid['a'] = $a;
	$edid['b'] = $b;
	foreach (array('a', 'b') as $dir) {
		if (preg_match('/(RearDestroyed|(East|West)Wing)$/', $edid[$dir])) {
			$num[$dir] = 100;
			$let[$dir] = "";
		}
		elseif (preg_match('/(\d+)([abcde]?)(new|sub)?$/i', $edid[$dir], $matches)) {
			$num[$dir] = $matches[1];
			$let[$dir] = $matches[2];
			if (array_key_exists(3, $matches))
				$let[$dir] .= $matches[3];
		}
		else {
			$num[$dir] = 0;
			$let[$dir] = "";
		}
		
# hardwire order for NDVanua: NDVanua, NDVanua02, NDVanua01
		if ($edid[$dir] == "NDVanua01")
			$num[$dir] = 3;
#hardwire order for TG11 sewers
		elseif ($edid[$dir] == "ImperialSewerSystemTG11a") {
			$let[$dir] = "";
		}
		elseif ($edid[$dir] == "ImperialSewerSystemTG11a2") {
			$let[$dir] = "a";
			$num[$dir] = 11;
		}
# hardwire order of Temple of the Ancestor Moths
		elseif ($edid[$dir] == "TempleOfTheAncestorMothCrypt") {
			$edid[$dir] = "Temple";
		}
# hardwire Arkved's Tower
		elseif ($edid[$dir] == "ArkvedsTowerWorld") {
			$let[$dir] = "a";
			$num[$dir] = 3;
		}
# Sancre Tor Hub
		elseif ($edid[$dir] == "SancreTorHub") {
			$let[$dir] = "a";
			$num[$dir] = 1;
		}
		elseif ($edid[$dir] == "SancreTorTomb") {
			$num[$dir] = 5;
		}
# Lake Arrius
		elseif ($edid[$dir] == "LakeArriusShrineDagon") {
			$let[$dir] = "a";
			$num[$dir] = 1;
		}
	}
	
	if ($num['a']==$num['b']) {
		if ($let['a']==$let['b'])
			return strcmp($edid['a'], $edid['b']);
		elseif ($let['a']=='')
			return -1;
		elseif ($let['b']=='')
			return 1;
		else
			return strcmp($a, $b);
	}
	else
		return ($num['a']<$num['b'] ? -1 : 1);
}

/*
$sortbykey = "FNAM";
sub by_key {
    if ($alldata{$a}{$sortbykey} eq $alldata{$b}{$sortbykey}) {
	return $a cmp $b;
    }
    else {
	return $alldata{$a}{$sortbykey} cmp $alldata{$b}{$sortbykey};
    }
}

1;*/

?>
