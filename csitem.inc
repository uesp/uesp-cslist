<?php
require_once 'common.inc';

class CSItem {
	protected $_edid, $_formid, $_ordid, $_rectype, $_fileid;
	protected $_data;
	protected $_itemset=NULL;
	protected $_parentlist;
	protected $_itemlist;
	protected $_data_toread;
	protected $_data_read;
	protected $_complex_index=FALSE;
	
	// Column data for widely-used values
	// This data gets used for all individual item types unless overridden in that item's $_column_data
	// Parameters:
	// 'title'
	// 'format_title'
	// 'format'  -- these three all get to output format as is
	// 'display' -- what types of displays this variable should be shown on
	//    generally blank for the _generic data -> which means the rest of the associated data
	//    will only be used if/when an individual item type provides a display variable
	//    values that are recognized so far:
	//    'all', 'list', 'details_html', 'details_wiki'
	//   (and for generic data if display is set -> variable will only be shown if it exists)
	// 'vars' -- vars that are used to derive this column's data.  Main significance is that
	//    any variables listed here will not be forcibly added by any default show-all-vars handlers
	//    (default is that vars=column_key)
	// 'display_option' -- extra options dependent upon the display
	//    for 'details_html' => provide the section where value should be displayed
	//    for 'details_wiki' => provide the param used for this value (optionally also provide template, if non-standard)
	//   (this is done separately from display so that generic data can provide display_option without
	//    triggering actual display of item)
	// significance of order is that this is the default order used for displaying the variables
	protected $_generic_column_data =
		array( 'name' => array( 'display' => 'all',
		                        'display_option' => array( 'details_html' => 'intro'),
		                        'format' => 'link_main',
		                      ),
		       'edid' => array( 'display' => array('list_html', 'list_csv', 'details_html', 'details_csv'),
		                        'title' => 'edID',
		                        'display_option' => array( 'details_html' => 'intro'),
		                        'format' => 'link_main',
		                      ),
		       // special handling done later to remove formid from MW lists
		       'formid' => array( 'display' => array('list_html', 'list_csv', 'details_html', 'details_csv'),
		                          'title' => 'formID',
		                          'display_option' => array( 'details_html' => 'intro'),
		                          'format' => array('formid', 'link_main'),
// just to make these variables not get displayed
					  'vars' => array('script_version'),
		                      ),
		       'id' => array( 'display' => array('list_wiki'),
		                      'title' => 'ID',
		                    ),
		       'weight' => array( 'format_title' => 'title_icon',
		                          'display' => 'all',
		                          'display_option' => array( 'details_html' => 'main'),
		                        ),
		       'value' => array( 'format_title' => 'title_icon',
		                         'display' => 'all',
		                         'display_option' => array( 'details_html' => 'main'),
		                       ),
		       'health' => array( 'format_title' => 'title_icon',
		                          'display' => 'all',
		                          'display_option' => array( 'details_html' => 'main'),
		                        ),
		       // END OF display=all variables
		       'magic_desc' => array( 'title' => 'Enchantment',
		                              'format' => 'magic_desc',
		                              'display_option' => array( 'list_html' => -1, 'list_wiki' => -1, 'list_csv' => -1, 'details_html' => 'main'),
		                              // adding in skill,attrib,etc could cause problems for WEAP
		                              // should I change all such vars to start with 'eff_' to prevent problems?
		                              //'vars' => array('enchant_id', 'effect_id', 'eff_mag', 'eff_magmin', 'eff_magmax', 'eff_area', 'eff_dur', 'eff_range', 'eff_act', 'eff_skill', 'eff_attrib', 'effectb_id'),
		                            ),
		       'SCTX' => array('format' => 'pre_tags'),
		       'function_ref' => array('format' => 'cond_param'),
		       'cond_op' => array('title' => 'Condition Operator', 'format' => 'cond_op'),
		       'cond_param1' => array('format' => 'cond_param'),
		       'cond_param2' => array('format' => 'cond_param'),
		       'cond_param3' => array('format' => 'cond_param'),
		       'cond_param4' => array('format' => 'cond_param'),
		       'plain_item_ordid' => array('title' => 'Base Item',
		                                   'format' => 'ordid_as_name',
		                                   'display_option' => array('list_wiki' => array('location' => 2))),
		       'charge_uses' => array('title' => 'Charge/Cost = Uses',
		                              'vars' => array('enchantcharge'),
		                              'display_option' => array('list_wiki' => array('location' => -2))),
		       'keyword' => array('format' => 'ordid_as_name'),
// not doing anything yet!!
		       'sid' => array('display' => 'details_html', 'title' => 'Scripts', 'display_option' => array('details_html' => 'main'), 'format' => 'sid'),
		     );
	//	protected $_columns_base = array('name', 'edid', 'formid', 'value', 'weight', 'health');
	protected $_query_filter = '';
	
	// this function should generally not be called directly
	// instead, call parentlist->newitem, which handles of storage/re-use/etc. of items
	
	// data_toread was mindata
	// data_toread=1 -> just AllItems (was mindata=TRUE)
	// data_toread=2 -> AllItems and rectype_Record (standard read for itemsets)
	// data_toread=3 -> all data
	static public function newitem($rectype, $ordid=NULL, $data_toread=NULL, $parentlist=NULL) {
		global $gCSData;
		if (!isset($parentlist))
			$parentlist = $gCSData;
		if (!isset($parentlist))
			return NULL;
		
		$orectype = $rectype;
		if (is_null($rectype)) {
			$has_formid = FALSE;
			$rectype = $parentlist->get_rectype($ordid, $has_formid);
			// allow null rectypes to continue for cases where I'm trying to display information that links to
			// a missing formid
			if (is_null($rectype) && !$has_formid)
				return NULL;
		}
		$classid = $parentlist->get_classid();
		
		if (!isset($data_toread))
			$data_toread = 3;
		if (empty($rectype) && $data_toread>1)
			$data_toread = 1;
		$recclass = NULL;
		if (isset($rectype) && $data_toread>=0) {
			$recclass = $classid."Item_".$rectype;
			if (!class_exists($recclass)) {
				$recclass = "GenItem_".$rectype;
				if (!class_exists($recclass))
					$recclass = NULL;
			}
		}
		if (!isset($recclass))
			$recclass = "GenItem";
		
		$newitem = new $recclass($parentlist, $rectype, $ordid, $data_toread);
		return $newitem;
	}
	
	public function __construct($parentlist, $rectype, $id=NULL, $data_toread=NULL) {
		$this->_parentlist = $parentlist;
		$this->_rectype = $rectype;
		$this->_ordid = NULL;
		$this->_fileid = NULL;
		$this->_data_read = 0;
		if (empty($rectype)) {
			$this->_data_toread = 1;
		}
		elseif (isset($data_toread)) {
			$this->_data_toread = $data_toread;
		}
		elseif (!isset($this->_data_toread)) {
			$this->_data_toread = 3;
		}
		
		// set both edid and formid to NULL
		$this->_edid = NULL;
		$this->_formid = NULL;
		if (isset($id))
		// then overwrite one of them (dependent on game) with provided id
			$this->set_id($id);
		
		if ($this->_parentlist->get_dataoptions($rectype, 'complex_index'))
			$this->_complex_index=TRUE;
		$this->_itemlist = NULL;
		$this->_data = array();
		$this->_set_default();
		$this->_class_setup();
	}
	
	protected function _set_default() {
		if ($this->_parentlist->get_datamode()!=CSData::DATAMODE_DB) {
			if (empty($this->_rectype))
				return;
			$tablelist = $this->_parentlist->get_datatables($this->_rectype);
			if (is_array($tablelist)) {
				foreach ($tablelist as $table) {
					$valnames = $this->_parentlist->get_datatables($this->_rectype, $table);
					foreach ($valnames as $valname) {
						$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $valname);
						if (array_key_exists('default', $datainfo)) {
							$this->_data[$valname] = $datainfo['default'];
						}
					}
				}
			}
		}
	}
	protected function _class_setup() {
	}
	
	// explicitly get rid of itemlists: because itemlist references back to item, memory is probably not getting released
	// until those backreferences are all cleaned up
	public function __destruct() {
		$this->clean();
	}
	
	public function clean($keep_parentlist=FALSE) {
		if (!is_null($this->_itemlist)) {
			// _itemlist is not a CSItem but an ItemList
			$this->_itemlist->clean();
			unset($this->_itemlist);
			$this->_itemlist=NULL;
		}
		if (count($this->_data)) {
			foreach ($this->_data as $valname => $d1) {
				if (is_array($d1)) {
					foreach ($d1 as $sub => $d2) {
						if (is_object($d2) && method_exists($d2, 'release'))
							$d2->release();
						unset($this->_data[$valname][$sub]);
					}
				}
				elseif (is_object($d1) && method_exists($d1, 'release'))
					$d1->release();
				unset($this->_data[$valname]);
			}
			unset($this->_data);
			$this->_data = array();
			
			$this->_ordid = NULL;
			$this->_edid = NULL;
			$this->_formid = NULL;
			$this->_fileid = NULL;
			$this->_data_read = 0;
		}
		if (!$keep_parentlist)
			unset($this->_parentlist);
	}
	
	public function release() {
		if (!is_null($this->ordid()))
			$this->_parentlist->release_data($this->ordid());
	}
	
	protected function _release_items(&$itemarray) {
		foreach ($itemarray as $i => $item) {
			$item->release();
			unset($itemarray[$i]);
		}
	}
	
	// stub function: overwritten by children if there is any post-read processing to do (in particular, MW'skip' => true CELL records)
	// returns true to keep record; false to delete record
	public function finish_init() {
		return true;
	}
	
	// specifically written to take the player NPC_ record and convert it into and ACHR record
	public function create_copy($copy_rectype, $copy_id=NULL) {
		$recclass = substr(get_class($this),0,6)."_".$copy_rectype;
		if (!class_exists($recclass))
			$recclass = substr($recclass,0,6);
		if (is_null($copy_id))
			$copy_id = $this->id();
		$newitem = new $recclass($this->_parentlist, $copy_rectype, $copy_id);
		
		// for now, not copying _itemlist.... not sure whether it's needed, or what's best way to do it
		// rest of values were handled by __construct
		$newitem->_data = array_merge($this->_data, $newitem->_data);
		$newitem->_fileid = $this->_fileid;
		
		if ($copy_rectype!=$this->_rectype && ($copy_rectype=='REFR' || $copy_rectype=='ACHR' || $copy_rectype=='ACRE')) {
			$newitem->_data['base_id'] = $this->ordid();
		}
		
		return $newitem;
	}
	
	public function add_change_data($data) {
		foreach ($data as $key => $value) {
			if (substr($key,-5,5)=='Count')
				continue;
			if (!is_array($value)) {
				$this->_data[$key] = $value;
				continue;
			}
			foreach ($value as $oldindex => $subdata) {
				if (!is_array($subdata)) {
					if (isset($this->_data[$key]) && is_array($this->_data[$key])) {
						$index = array_search($subdata, $this->_data[$key]);
						if ($index===false)
							$index = count($this->_data[$key]);
					}
					else {
						$index = 0;
					}
					$this->_data[$key][$index] = $subdata;
					continue;
				}
				
				// generic handling to take an input array of form [item][$i]{id, num, equipped}
				// into item_id, item_num, item_equipped
				if (isset($subdata['id']) && is_array($this->_data[$key.'_id'])) {
					$index = array_search($subdata['id'], $this->_data[$key.'_id']);
					if ($index===false)
						$index = count($this->_data[$key.'_id']);
				}
				else
					continue;
						
				foreach ($subdata as $subkey => $subvalue) {
					if ($subkey=='changed')
						continue;
					$newkey = $key.'_'.$subkey;
					if ($subkey=='num') {
						if (!isset($this->_data[$newkey][$index]))
							$this->_data[$newkey][$index] = 0;
						$this->_data[$newkey][$index] += $subvalue;
					}
					else {
						$this->_data[$newkey][$index] = $subvalue;
					}
				}
			}
		}
	}
		
	/*
	   ALL GET-type functions
	*/
	
	// id will return the most appropriate unique identifier for this dataset
	// for MW data, returns edid; for other data, returns formid
	// if formid or edid specifically required, use formid/edid instead
	public function id() {
		return $this->_formid;
	}
	
	public function formid() {
		return $this->_formid;
	}

	public function wiki_formid() {
		$formid = strtoupper(substr($this->_formid,2));
		if (substr($formid,0,2)!='00')
			$formid = 'xx'.substr($formid,2);
		return $formid;
	}
	
	public function ordid() {
		return $this->_ordid;
	}
	
	public function rectype() {
		return $this->_rectype;
	}
	
	public function fileid() {
		return $this->_fileid;
	}
	
	public function srcfile() {
		return $this->fileid();
	}
	
	public function edid() {
		return $this->_edid;
	}
	
	public function file_ns() {
		return $this->_parentlist->get_file_ns($this->get('srcfile'));
	}
	
	public function file_name() {
		return $this->_parentlist->get_file_name($this->get('srcfile'));
	}
	
	public function modname() {
		return $this->_parentlist->get_file_mod($this->get('srcfile'));
	}
	
	public function modabbrev() {
		return $this->_parentlist->get_file_abbrev($this->get('srcfile'));
	}

	public function mod_status_unique() {
		return !isset($this->_data['mod_status'])?NULL:(($this->_data['mod_status']&0x01)?TRUE:FALSE);
	}
	public function mod_status_default() {
		return !isset($this->_data['mod_status'])?NULL:(($this->_data['mod_status']&0x02)?TRUE:FALSE);
	}
	public function mod_status_current() {
		return !isset($this->_data['mod_status'])?NULL:(($this->_data['mod_status']&0x04)?TRUE:FALSE);
	}
	
	public function parentlist() {
		return $this->_parentlist;
	}
	
	public function refr_rectype() {
		return 'REFR';
	}
	// calls _getexact repeatedly using different variations of name's capitalization
	public function get($valname, $index=NULL) {
		if (!is_null($value=$this->_getderived($valname, $index)))
			return $value;
		if (!is_null($value=$this->_getderived(strtolower($valname), $index)))
			return $value;
		if (!is_null($value=$this->_getderived(preg_replace('/\s+/', '_', strtolower($valname)), $index)))
			return $value;
		if (!is_null($value=$this->_getderived(preg_replace('/_+/', ' ', strtolower($valname)), $index)))
			return $value;
		if (!is_null($value=$this->_getderived(ucfirst($valname), $index)))
			return $value;
		if (!is_null($value=$this->_getderived(ucwords($valname), $index)))
			return $value;
		
		return NULL;
	}
	
	// tests multiple options to match valname, in decreasing priority (all options recognize index as appropriate)
	// * a function with the valname
	// * a _data[$valname] entry
	// * a _data[$valname."_lu"] entry which is then looked up and transformed using lookup tables
	// * a _data[$valname."_id"] entry which is assumed to be ordid and transformed into ordid's name (or edid)
	// * a function "derive_".$valname (unlike a $valname function, this one is only called if the value does not already
	//   exist.  Primarily intended for processing that happens after original DB-read to create new variables, but after
	//   first processing can just be accessed directly from value saved in DB)
	protected function _getderived($valname, $index=NULL) {
		if (method_exists($this, $valname)) {
			if (!is_null($index))
				return $this->$valname($index);
			else
				return $this->$valname();
		}
		
		if (!array_key_exists($valname, $this->_data) && $this->_data_read<3) {
			$this->read_db();
		}
		
		if (array_key_exists($valname, $this->_data))
			$srcval = $valname;
		elseif (array_key_exists($valname."_lu", $this->_data))
			$srcval = $valname."_lu";
		elseif (array_key_exists($valname."_id", $this->_data))
			$srcval = $valname."_id";
		elseif (method_exists($this, "derive_".$valname)) {
			if (!is_null($index))
				return $this->{"derive_".$valname}($index);
			else
				return $this->{"derive_".$valname}();
		}
		else
			return NULL;
		
		if (is_null($value=$this->_getexact($srcval, $index)))
			return NULL;
		
		$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $valname);
		// Originally returned NULL if value didn't exist in lookup
		// But now want to return original value to add flexiblity for Skyrim lookups
		if (is_array($datainfo) && array_key_exists('lookup', $datainfo) && $srcval!=$valname) {
			$lookup = $this->_parentlist->get_lookup($datainfo['lookup'], $value);
			if (!isset($lookup) && $value>=0)
				return $value;
			else
				return $lookup;
		}
		//		elseif (is_array($datainfo) && array_key_exists('format', $datainfo) && $datainfo['format']=='mgefid')
		//			return 'MGEF'.sprintf("%03d", $value);
		elseif ($srcval!=$valname && substr($srcval,-3,3)=='_id') {
			if (($name=$this->_parentlist->get_name($value))!=NULL && $name)
				return $name;
			else
				return $this->_parentlist->get_edid($value);
		}
		else
			return $value;
	}
	
	protected function _getexact($valname, $index=NULL) {
		if (!array_key_exists($valname, $this->_data) && $this->_data_read<3) {
			$this->read_db();
		}
		
		if (!isset($this->_data[$valname]))
			return NULL;
		// this will return a single value OR an entire array
		// (previously returned NULL if data was an array)
		elseif (is_null($index)) {
			return $this->_data[$valname];
		}
		elseif (!is_null($index) && is_array($this->_data[$valname]) && isset($this->_data[$valname][$index]))
			return $this->_data[$valname][$index];
		else
			return NULL;
	}
	
	// This function originally returned the maximum value of the index, +1
	//   designed for loops as ($i=0; $i<get_index(); $i++)
	// But that definition failed for new complex indices, and was inherently flawed
	// Revamped to now return the ''largest'' value of the index -- whether complex or not
	public function get_index($valname, $subindex=NULL) {
		$mainval = $valname;
		if ($this->_complex_index && substr($valname,4,1)=='-') {
			$mainval = substr($valname,0,4);
		}
		if (!array_key_exists($mainval, $this->_data) && $this->_data_read<3) {
			$this->read_db();
		}
		
		if (method_exists($this, "get_index_".$mainval))
			return $this->{"get_index_".$mainval}();
		if (!array_key_exists($mainval, $this->_data))
			return FALSE;
		elseif (!is_array($this->_data[$mainval]))
			return FALSE;
		else {
			// count is not the right function here
			// for example if there are 3 NULL entries, then a real one, count returns 1 instead of 4
			if (!count($this->_data[$mainval]))
				return FALSE;
			if ($this->_complex_index) {
				$indices = $this->indices($valname, $subindex);
				if (empty($indices))
					return FALSE;
				return (array_pop($indices));
			}
			else {
				return (max(array_keys($this->_data[$mainval])));
			}
		}
	}
	
	public function indices($valname, $subindex=NULL, $getbase=FALSE) {
		$mainval = $valname;
		if ($this->_complex_index && substr($valname,4,1)=='-') {
			$mainval = substr($valname,0,4);
		}
		
		if (!array_key_exists($mainval, $this->_data) && $this->_data_read<3) {
			$this->read_db();
		}
		
		if (method_exists($this, "get_index_".$mainval) && !isset($this->_data[$mainval]))
			$this->{"get_index_".$mainval}();
		
		if (!array_key_exists($mainval, $this->_data) || !is_array($this->_data[$mainval]))
			return array();
		elseif (!$this->_complex_index) {
			$keys = array_keys($this->_data[$mainval]);
			sort($keys);
			return $keys;
		}
		else {
			$subvals = explode('-', $valname);
			if (count($subvals)==1 && is_null($subindex)) {
				$keys = array_keys($this->_data[$valname]);
				usort($keys, array($this, 'sort_indices'));
				return $keys;
			}
			
			for ($i=0; $i<count($subvals); $i++) {
				$currval = $subvals[$i];
				
				if (!isset($this->_data[$currval])) {
					if (!$getbase || $i==0)
						return array();
					return $subindex;
				}
				$indices = array();
				if (!empty($subindex)) {
					if (substr($subindex,-1,1)!='-')
						$subindex .= '-';
					$sublen = strlen($subindex);
				}
				foreach ($this->_data[$currval] as $index => $value) {
					if (!empty($subindex)) {
						if (substr($index,0,$sublen)!=$subindex)
							continue;
					}
					else {
						if (strlen($index)>8)
							continue;
					}
					$indices[] = $index;
				}
				
				if (($i<count($subvals)-1)) {
					if (count($indices))
						$subindex = array_pop($indices);
					else
						break;
				}
			}
			if (empty($indices) && count($subvals)>1 && isset($subindex) && $getbase)
				return $subindex;
			usort($indices, array($this, 'sort_indices'));
			return $indices;
		}
	}
	
	public function sort_indices($a, $b) {
		$as = explode('-',$a);
		$bs = explode('-',$b);
		$imax = max(count($as), count($bs));
		$as = array_pad($as, $imax+1, '');
		$bs = array_pad($bs, $imax+1, '');
		for ($i=0; $i<=$imax; $i++) {
			if ($as[$i]===$bs[$i])
				continue;
			if ($as[$i]<$bs[$i])
				return -1;
			else
				return 1;
		}
		return 0;
	}

	public function reset_index($valname) {
		if (!$this->_complex_index) {
			$indexname = $valname;
		}
		else {
			$valsubs = explode('-', $valname);
			$indexname = array_pop($valsubs);
		}
		unset($this->_data[$indexname]);
	}
	
	public function increment_index($valname) {
		if (!$this->_complex_index) {
			$index = $this->get_index($valname);
			if ($index===FALSE)
				$index = 0;
			else
				$index++;
			$indexname = $valname;
		}
		else {
			$valsubs = explode('-', $valname);
			if (count($valsubs)>1)
				$indices = $this->indices($valname, NULL, TRUE);
			else
				$indices = $this->indices($valname, $valname, TRUE);
			if (!is_array($indices)) {
				$index = $indices;
				foreach ($valsubs as $sub) {
					if (strpos($index, $sub)!==FALSE)
						continue;
					if (!empty($index) && substr($index,-1,1)!='-')
						$index .= '-';
					$index .= $sub.'-0';
				}
			}
			elseif (empty($indices)) {
				$index = $valname.'-0';
			}
			else {
				$index = array_pop($indices);
				$subs = explode('-', $index);
				$subs[count($subs)-1]++;
				$index = implode('-', $subs);
			}
			$indexname = array_pop($valsubs);
		}
		
		$this->set('', $indexname, $index);
		return $index;
	}
	
	public function get_as_item($valname, $index=NULL) {
		$ordid = $this->get($valname, $index);
		if (is_null($ordid))
			return NULL;
		return $this->_parentlist->get_item($ordid);
	}
	
	public function get_as_edid($valname, $index=NULL) {
		$ordid = $this->get($valname, $index);
		if (is_null($ordid))
			return NULL;
		return $this->_parentlist->get_edid($ordid);
	}	
	
	/* 
	   ALL SET-type functions
	*/
	
	// overwritten by MWData -> in MW, sets _edid instead of _formid
	public function set_id($id) {
		if (!is_null($id) && $id!='')
			$this->_formid = $id;
	}
	
	public function set_fileid($id) {
		$this->_fileid = $id;
	}
	
	public function set_srcfile($id) {
		$this->set_fileid($id);
	}
		
	public function set_ordid($ordid) {
		$this->_ordid = $ordid;
		$this->_data_read = 0;
	}
	
	public function set_edid($edid) {
		$this->_edid = $edid;
	}
	
	public function set($value, $valname, $index=NULL) {
		if (method_exists($this, "set_".strtolower($valname))) {
			if (is_null($index))
				$this->{"set_".strtolower($valname)}($value);
			else
				$this->{"set_".strtolower($valname)}($value, $index);
		}
		else {
			if (is_null($index))
				$this->_data[$valname] = $value;
			else
				$this->_data[$valname][$index] = $value;
		}
	}
	
	/*
	   LEVELED LISTS
	*/
	
	public function init_itemlist() {
		if (is_null($this->_itemlist))
			$this->_itemlist = new ItemList($this, $this->_parentlist);
	}
	
	public function hasitem($item) {
		if (is_null($item))
			return 0;
		$iordid = $this->_parentlist->get_ordid($item);
		if ($iordid == $this->ordid())
			return 1;
		if (!$this->contains_list()) // initializes _itemlist
			return 0;
		return $this->_itemlist->hasitem($iordid);
	}
	
	public function contains_list() {
		if ($this->get_index('item_id')===FALSE && $this->get_index('spell_id')===FALSE) {
			return FALSE;
		}
		$this->init_itemlist();
		if ($this->_itemlist->contains_list()) {
			//			print "contains list true for ".$this->id()."\n";
			return $this->_itemlist;
		}
		else {
			//			print "contains list false for ".$this->id()."\n";
			return false;
		}
	}
	
	public function get_plvl($lordid) {
		$this->init_itemlist();
		return $this->_itemlist->get_plvl($lordid);
	}
	public function list_subitem_ids() {
		$this->init_itemlist();
		return $this->_itemlist->list_subitem_ids();
	}
	/* 
	   DATABASE input/output
	*/
	
	// Prepare array-based values for comparison in is_identical
	// Change indices of any arrays in $this to match the array contents of $origitem
	protected function _match_indices($origitem) {
		if (empty($this->_rectype))
			return;
		$tablelist = $this->_parentlist->get_datatables($this->_rectype);
		// If there's only data in the main XXXX_Record table, there aren't any arrays
		if (count($tablelist)<=1)
			return;
		
		// For now, only really set up to handle a single extra table
		//		if (count($tablelist)>2)
		//		return;
		foreach ($tablelist as $table) {
			if ($table==$this->rectype().'_Record')
				continue;
			$keys = array();
			$imax = -1;
			if (count($tablelist)==2) {
				foreach (array($this->_data, $origitem->_data) as $datasrc) {
					foreach ($datasrc as $valname => $data) {
						if (is_array($data)) {
							// Ignore translated _lu, _id values in this check
							if (array_key_exists($valname.'_lu', $datasrc) || array_key_exists($valname.'_id', $datasrc)) {
								$keys[$valname] = FALSE;
							}
							else {
								$keys[$valname] = TRUE;
								$imax = max($imax, array_pop(array_keys($data)));
							}
						}
					}
				}
			}
			else {
				$tableinfo = $this->_parentlist->get_datatables($this->rectype(), $table);
				foreach ($tableinfo as $valname) {
					$keys[$valname] = TRUE;
					if (substr($valname,-3)=='_lu' || substr($valname,-3)=='_id') {
						$altvalname = substr($valname,0,-3);
						// Make sure that alt versions of valname also get re-sorted
						if (array_key_exists($altvalname, $this->_data))
							$keys[$altvalname] = FALSE;
						foreach (array($this->_data, $origitem->_data) as $datasrc) {
							if (!isset($datasrc[$valname]))
								continue;
							$imax = max($imax, array_pop(array_keys($datasrc[$valname])));
						}
					}
				}
			}
			if ($imax<=0) {
				continue;
			}
			
			$currindex = array();
			$origindex = array();
			$currstrs = array();
			$origstrs = array();
			
			// Assemble strings containing all data for each row of the table -- use strings for comparisons
			for ($rep=0; $rep<2; $rep++) {
				if (!$rep) {
					$datasrc = $this->_data;
					$strs = &$currstrs;
				}
				else {
					$datasrc = $origitem->_data;
					$strs = &$origstrs;
				}
				for ($i=0; $i<=$imax; $i++) {
					$strs[$i] = '';
					foreach ($keys as $key => $x) {
						if (!$x)
							continue;
						$strs[$i] .= '|';
						if (isset($datasrc[$key][$i])) {
							$strs[$i] .= $datasrc[$key][$i];
						}
					}
				}
			}
			
			// First get exact matches
			for ($i=0; $i<=$imax; $i++) {
				for ($j=0; $j<=$imax; $j++) {
					if (isset($currindex[$j]))
						continue;
					if ($currstrs[$i]==$origstrs[$j]) {
						$currindex[$j] = $i;
						$origindex[$i] = $j;
						break;
					}
				}
			}
			
			// Then look for closest matches
			// (should probably first check whether there's only one mismatch, or whether original
			//  arrays differed in length
			// most cases that are triggering this are basically unnecessary)
			if (count($origindex)<=$imax) {
				$sims = array();
				// Assemble grid of all similarities
				for ($i=0; $i<=$imax; $i++) {
					if (isset($origindex[$i]))
						continue;
					for ($j=0; $j<=$imax; $j++) {
						if (isset($currindex[$j]))
							continue;
						$sim = similar_text($currstrs[$i], $origstrs[$j], $percent);
						if ($sim) {
							$sims[$i][$j] = $percent;
						}
					}
				}
				// Go through and pair up best matches anywhere in grid
				while (!empty($sims)) {
					$simmax = 0;
					$simi = NULL;
					$simj = NULL;
					foreach ($sims as $i => $js) {
						foreach ($js as $j => $sim) {
							if ($sim>$simmax) {
								$simmax = $sim;
								$simi = $i;
								$simj = $j;
							}
						}
					}
					if (!$simmax)
						break;
					$origindex[$simi] = $simj;
					$currindex[$simj] = $simi;
					// Remove all matches for these entries from grid
					unset($sims[$simi]);
					foreach ($sims as $i => $js) {
						unset($sims[$i][$simj]);
					}
				}
				var_dump($origindex);
				var_dump($currindex);
			}
			
			// If no similarities found (unlikely), just skip rest of processing -- no point in reorganizing
			if (empty($origindex))
				continue;
			
			// Just arbitrarily fill in rest of grid
			$allmatch = TRUE;
			for ($i=0; $i<=$imax; $i++) {
				if (isset($origindex[$i])) {
					if ($origindex[$i]!=$i)
						$allmatch = FALSE;
					continue;
				}
				for ($j=0; $j<=$imax; $j++) {
					if (isset($currindex[$j]))
						continue;
					$origindex[$i] = $j;
					$currindex[$j] = $i;
					if ($i!=$j)
						$allmatch = FALSE;
					break;
				}
			}
			
			// If array indices match completely, no need to reorganize
			if ($allmatch)
				continue;
			
			// Now... reorganize this->_data accordingly
			// (Although I'd prefer to not mess with read-but-not-saved data -- which is what $this normally is --
			// I'd also like to have DB contain data sorted to use same index, which means $this has to be altered)
			foreach ($keys as $key => $x) {
				if (!isset($this->_data[$key]))
					continue;
				$origkey = $this->_data[$key];
				$this->_data[$key] = array();
				for ($i=0; $i<=$imax; $i++) {
					if (!array_key_exists($i, $origkey))
						continue;
					$this->_data[$key][$origindex[$i]] = $origkey[$i];
				}
			}
		}
	}
	
	// BUT needs to check values after lookup/id/derive/etc because several values get converted when saved to DB!!
	public function is_identical($origordid, $skiparrays=false) {
		if (!$origordid || is_null($origitem = $this->_parentlist->get_item($origordid)))
			return false;
		$identical = true;
		
		if (!$skiparrays) {
			$this->_match_indices($origitem);
		}
		
		foreach (array(0, 1) as $dir) {
			if ($dir==0) {
				$item1 = $this;
				$item2 = $origitem;
			}
			else {
				$item1 = $origitem;
				$item2 = $this;
			}
			
			foreach ($item1->_data as $valname => $data) {
				if (is_null($valname) || $valname=='' || $valname=='mod_status')
					continue;
				// don't do comparison of subrecord counts (doesn't exist yet for new records)
				if (strlen($valname)==9 && substr($valname,0,5)=='count')
					continue;
				if (is_array($data)) {
					if ($skiparrays)
						continue;
					$indices = $item1->indices($valname);
				}
				else {
					$indices = array(NULL);
				}
				$altvalname = preg_replace('/_(lu|id)$/', '', $valname);
				foreach ($indices as $icurr) {
					if (($v1=$item1->get($valname, $icurr))!=($v2=$item2->get($valname, $icurr))) {
						if (is_numeric($v1) && is_numeric($v2) && abs($v1-$v2)/max(abs($v1), abs($v2))<1.e-4)
							continue;
						// trim strings before comparing -- don't want to record that Imperial RACE record is different
						// just because a space was added to the end of the string
						if (is_string($v1))
							$v1 = trim($v1);
						if (is_string($v2))
							$v2 = trim($v2);
						if ($v1==$v2)
							continue;
						// group_id value that isn't being saved for this record
						if ($valname=='group_id' && is_null($this->_parentlist->get_datainfo($this->_rectype, $valname)))
							continue;
						// for _id values, compare original edid/formid (using get compares names, not original ids).  And do this check even if the values are both non-null
						if (substr($valname,-3)=='_id') {
							if (($av1=$this->_parentlist->get_id($v1))!=($av2=$this->_parentlist->get_id($v2))) {
								$this->_print_not_identical($this->id(), $this->get('fileid'), $valname, 'ordid->id', $icurr, $dir, $av1, $av2);
								$identical = false;
								break 3;
							}
						}
						elseif ($altvalname!=$valname && (is_null($v1) || is_null($v2))) {
							if (($av1=$item1->get($altvalname, $icurr))!=($av2=$item2->get($altvalname, $icurr))) {
								$this->_print_not_identical($this->id(), $this->get('fileid'), $valname, $altvalname, $icurr, $dir, $av1, $av2);
								$identical = false;
								break 3;
							}
						}
						else {
							$this->_print_not_identical($this->id(), $this->get('fileid'), $valname, NULL, $icurr, $dir, $v1, $v2);
							$identical = false;
							break 3;
						}
					}
				}
			}
		}
		$origitem->release();
		return $identical;
	}
	
	protected function _print_not_identical($id, $fileid, $valname, $altvalname, $icurr, $dir, $v1, $v2) {
		print "not identical for item={$id} fileid={$fileid} valname={$valname} ";
		if (!is_null($altvalname))
			print "altvalname={$altvalname} ";
		print "icurr={$icurr} rep={$dir}";
		if ((is_string($v1) && strlen($v1)>50) || (is_string($v2) && strlen($v2)>50)) {
			print "\n   len1=".strlen($v1)."  v1=";
			if (strlen($v1)>50)
				print substr($v1,0,25)."...".substr($v1,-25);
			else
				print $v1;
			print "\n   len2=".strlen($v2)."  v2=";
			if (strlen($v2)>50)
				print substr($v2,0,25)."...".substr($v2,-25);
			else
				print $v2;
			print "\n";
		}
		else {
			print " v1='{$v1}' v2='{$v2}'\n";
		}
	}
	
	protected function _finish_vmad() {
		if (empty($this->_data['has_vmad_extra']))
			return;
		if (isset($this->_data['raw_fragment_num']) && isset($this->_data['INDX'])) {
			foreach ($this->_data['raw_fragment_num'] as $fragment_index => $fragment_num) {
				$new_index = NULL;
				foreach ($this->_data['INDX'] as $subid => $quest_index) {
					if ($fragment_index==$quest_index) {
						$new_index = $subid;
						break;
					}
				}
				if (!isset($new_index))
					continue;
				foreach (array('fragment_num', 'fragment_loc', 'fragment_len') as $valname) {
					if (!isset($this->_data['raw_'.$valname][$fragment_index]))
						continue;
					$this->_data[$valname][$new_index] = $this->_data['raw_'.$valname][$fragment_index];
				}
			}
			unset($this->_data['raw_fragment_num']);
			unset($this->_data['raw_fragment_len']);
			unset($this->_data['raw_fragment_loc']);
		}
		if (isset($this->_data['alias_sid']) && isset($this->_data['ALID'])) {
			foreach ($this->_data['alias_sid'] as $sid_full_index => $sid) {
				if (!preg_match('/alias-(\d+)-/', $sid_full_index, $matches))
					continue;
				$sid_index = $matches[1];
				$sid_prefix = substr($sid_full_index,0,7+strlen($matches[1]));
				$new_index = NULL;
				foreach (array('ALLS', 'ALST') as $src) {
					if (!isset($this->_data[$src]))
						continue;
					foreach ($this->_data[$src] as $subid => $alias_index) {
						if ($sid_index==$alias_index) {
							$new_index = $subid;
							break 2;
						}
					}
				}
				if (!isset($new_index))
					continue;
				$new_index .= '-';
				$new_vindex = $new_index.substr($sid_full_index,strlen($sid_prefix));
				$this->_data['sid'][$new_vindex] = $sid;
				$this->_data['VSCR'][$new_vindex] = '';
				foreach (array('propname', 'propvalue') as $valname) {
					if (!isset($this->_data['alias_'.$valname]))
						continue;
					foreach ($this->_data['alias_'.$valname] as $vindex => $value) {
						if (substr($vindex,0,strlen($sid_prefix))!=$sid_prefix)
							continue;
						$new_vindex = $new_index.substr($vindex,strlen($sid_prefix));
						$this->_data['VPRP'][$new_vindex] = '';
						$this->_data[$valname][$new_vindex] = $value;
					}
				}
			}
			unset($this->_data['alias_sid']);
			unset($this->_data['alias_propname']);
			unset($this->_data['alias_propvalue']);
		}
	}

	public function write_db() {
		$this->_finish_vmad();

		// if this information is duplicate of existing information, no reason to save it
		// Note: $id is formid or edid(MW)
		$id = $this->id();
		// note: saving all records to AllItems, even if ID is blank or null (REFR and INFO records are no longer being assigned fake IDs)
		$doinsert = NULL;
		$repeat = FALSE;
		if (!is_null($id) && !$this->id()) {
			$this->set_id(NULL);
			$id = NULL;
		}

		$ordid = $currordid = NULL;
 		// See whether this exact record (same fileid) already exists in database -> meaning this is a re-read
		// match_fileid is a special variant of strictmatch especially to handle this call
		// (taking into account that an existing record might have rectype=NULL if created as placeholder)
		if (!is_null($id))
			$ordid = $this->_parentlist->get_ordid($id, $this->get('fileid'), $this->get('rectype'), array('match_fileid' => TRUE));

		// don't even bother to do identical comparison if ordid already exists
		// -> even if this is identical to a previous record, I still need to fill in more than just the AllItems entry
		if (is_null($ordid) && !is_null($id) && !is_null($prevordid = $this->_parentlist->get_ordid($id, $this->get('fileid'), $this->get('rectype'), array('prev_fileid' => TRUE)))) {
			// prev_fileid option has already ensured that any requested record is from a *previous* fileid (prevordid!=ordid)
			if ($this->is_identical($prevordid)) {
				return $prevordid;
			}
			// modified version of record (from updated .esm or .esp)
			// I was setting $doinsert = TRUE here ("force record to be given a new ordid")
			// *BUT* that was forcing multiple copies of fileid=1 version to be created on re-read
			// now insert/update being determined based on ordid check
		}

		if (!is_null($id))
			$currordid = $this->_parentlist->get_ordid($id, $this->modabbrev(), $this->get('rectype'), array('strictmatch' => TRUE));

		// assign mod_status
		// 0x01 -> is_unique
		// 0x02 -> default version of item (current version in mod where item was introduced)
		// 0x04 -> current version of item in given mod
		//  (0x02 and 0x04 will only differ in cases where a mod alters an item that appears in base version of game)
		// every formid should have exactly ONE record with 0x02 set
		$status_changed = FALSE;
		if (isset($currordid)) {
			$curritem = $this->_parentlist->get_item($currordid);
			if (isset($ordid) && $ordid==$currordid) {
				// same item: either means full save was previously done OR means curritem is a placeholder
				if (is_null($curritem->get('rectype'))) {
					// placeholder means this is the first item with this formid
					$this->_data['mod_status'] = 0x01 | 0x02 | 0x04;
				}
				else {
					$this->_data['mod_status'] = $curritem->get('mod_status');
				}
			}
			else {
				$curr_changed = FALSE;
				$curr_status = $curritem->get('mod_status');

				// neither item is unique
				if ($curr_status&0x01>0) {
					$curr_changed = TRUE;
					$curr_status &= ~1;
				}
				$this->_data['mod_status'] = 0;

				// curritem should always be current ... but new item only takes over current status if it is in a newer fileid
				if ($curritem->get('fileid')<$this->get('fileid')) {
					if ($this->modabbrev()==$curritem->modabbrev()) {
						// both items are in same mod -- new item completely takes over for curr item
						$this->_data['mod_status'] = $curr_status;
						$curr_status = 0;
						$curr_changed = TRUE;
						$status_changed = TRUE;
					}
					else {
						// must mean that curritem is in base, and new item is in mod (and is first instance in mod)
						// base item doesn't need to be changed
						// new item only gets 0x04 flag
						$this->_data['mod_status'] = 4;
						$status_changed = TRUE;
					}
				}
				// and update curritem if necessary
				if ($curr_changed) {
					$query = "UPDATE AllItems set mod_status=".$curr_status." WHERE ordid={$currordid}";
					$this->_parentlist->do_query($query);
				}
			}
			$curritem->release();
		}
		else {
			// first item with this formid: is_unique; is default; is current in given mod
			$this->_data['mod_status'] = 0x01 | 0x02 | 0x04;
			// no need to do status_changed if formid is unique
		}

		
		// set both formid and edid -- even though one may have already been set in a placeholder record, which one has
		// been set depends upon the game.  So just set both to be sure.
		$qucore = "SET fileid=".$this->get('fileid').", rectype='".$this->_rectype."', edid=".$this->format_db_value('edid').", name=".$this->format_db_value('name');
		if (!is_null($this->formid()))
			$qucore .= ", formid=".$this->format_db_value('formid');
		foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
			if ($valname=='formid')
				continue;
			$datainfo = $this->_parentlist->get_datainfo('', $valname);
			$qucore .= ", ".$datainfo['sqlname'].'='.$this->format_db_value($valname);
		}
		
		if (isset($ordid)) {
			$query = "UPDATE AllItems ".$qucore." WHERE ordid={$ordid}";
			$this->_parentlist->do_query($query);
			// Note: never do update on other tables
			// If existing AllItems is placeholder then no other tables were ever created
			// If existing AllItems from previous read then other tables should have been dumped/restarted
		}
		else {
			$query = "INSERT INTO AllItems ".$qucore;
			$ordid = $this->_parentlist->do_query($query, 'getid');
		}
		$this->_ordid = $ordid;
		$this->_data_read = 3;
		$this->_parentlist->add_active_to_data($this);
		
		$tablelist = $this->_parentlist->get_datatables($this->_rectype);
		if (!is_array($tablelist))
			return $this->_ordid;
		
		foreach ($tablelist as $table) {
			$indices = array();
			$valnames = $this->_parentlist->get_datatables($this->_rectype, $table);
			$query = "INSERT INTO {$table} (ordid";
			if ($table==$this->_rectype."_Record")
				// will this be seen as empty or not?
				$indices = array(0);
			else {
				$indices = $this->indices(substr($table,5,4));
				$query .= ", subid";
			}
			foreach ($valnames as $valname) {
				if ($valname=='name')
					continue;
				$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $valname);
				$query .= ", ".$datainfo['sqlname'];
			}
			$query .= ") VALUES ";
			
			if (!empty($indices)) {
				$qvalues = array();
				foreach ($indices as $n) {
					$nonnull = FALSE;
					$qvalue = ' ('.$ordid;
					if ($table==$this->_rectype."_Record") {
						$ni = NULL;
					}
					else {
						$ni = $n;
						if ($this->_complex_index)
							$qvalue .= ", '$ni'";
						else
							$qvalue .= ', '.$ni;
					}
					
					foreach ($valnames as $valname) {
						if ($valname=='name')
							continue;
						$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $valname);
						if (strlen($valname)==9 && substr($valname,0,5)=='count') {
							$counts = $this->indices(substr($valname,5), $n);
							$entry = count($counts);
							if ($entry>0)
								$nonnull = TRUE;
						}
						else {
							$entry = $this->format_db_value($valname, $ni, (array_key_exists('lookup', $datainfo)?$datainfo['lookup']:NULL));
							if ($entry!=='NULL')
								$nonnull = TRUE;
						}
						$qvalue .= ', '.$entry;
					}
					$qvalue .= ')';
					if ($nonnull)
						$qvalues[] = $qvalue;
				}
				if (!empty($qvalues)) {
					$query .= implode(', ', $qvalues);
					$this->_parentlist->do_query($query);
				}
			}
		}

		if ($status_changed) {
			$this->update_db_current();
		}

		return $this->_ordid;
	}
	
// Update all existing uses of formid if mod_status has been changed for this record
// In some ways, needing to update the entire database is inefficient
// Arguably, storing formid as basic value instead of ordid could make more sense
// But loading new modifications to formid is relatively rare, so it does make more sense to do as much data-intensive processing when new data
// is loaded, and allow subsequent lookups to be as fast as possible
	public function update_db_current() {
// need to loop through ALL values (in all tables) that use a formid
// replace origordid (or any other equiv ordids) with $this->_ordid.... as long as filemodabbrev's match

// should I first do a double-check that this item is even set to be current??
		$query = 'SELECT * from AllItems where formid="'.$this->get('formid').'" and ordid!='.$this->_ordid;
		$res = $this->_parentlist->do_query($query);
		$std_ordids = array();
		$mod_ordids = array();
		$poss_ordids = array();
		if ($this->modabbrev()==$this->_parentlist->get_file_abbrev(0))
			$is_base = TRUE;
		else
			$is_base = FALSE;
		$mods_done = array();
		$poss_fileids = array();
		// get list of ordids for other versions of this formid that need to be replaced
		while ($row=$this->_parentlist->row_query($res)) {
			if ($row['mod_status']&0x04) {
				$mods_done[$this->_parentlist->get_file_abbrev($row['fileid'])] = TRUE;
			}
			// NOT elseif -> want to add any 0x04 ordids to poss_ordids if they're from base
			if ($this->modabbrev()==$this->_parentlist->get_file_abbrev($row['fileid'])) {
				$std_ordids[] = $row['ordid'];
			}
			elseif ($is_base) {
				$mod_ordids[$this->_parentlist->get_file_abbrev($row['fileid'])][] = $row['ordid'];
			}
			elseif ($this->_parentlist->get_file_abbrev($row['fileid'])==$this->_parentlist->get_file_abbrev(0)) {
// else -- need to pick up ordids from base when they are used in this mod!!
				$poss_ordids[] = $row['ordid'];
			}
		}

		if ($is_base && !empty($mod_ordids)) {
			// Only include ordids corresponding to other mods *if* this item is the base item and there is no mod-specific version of item
			foreach ($mods_done as $modabbrev => $x) {
				unset($mod_ordids[$modabbrev]);
			}
			foreach ($mod_ordids as $modabbrev => $ordids) {
				$std_ordids = array_merge($std_ordids, $ordids);
			}
		}

		if (!empty($poss_ordids)) {
			$poss_fileids = $this->_parentlist->get_fileids_for_abbrev($this->modabbrev());
		}

		foreach ($this->_parentlist->get_datatables() as $table) {
			$rectype = substr($table,0,4);
			$valnames = $this->_parentlist->get_datatables($rectype, $table);
			foreach ($valnames as $valname) {
				if ($valname=='ordid')
					continue;
				$info = $this->_parentlist->get_datainfo($rectype, $valname);
				if (empty($info['format']) || $info['format'] != 'formid')
					continue;

				$query = "UPDATE $table inner join AllItems using (ordid) SET $table.$valname=".$this->ordid();
				if (!empty($std_ordids)) {
					$query1 = $query . " where $table.$valname in (".implode(',',$std_ordids).")";
					$this->_parentlist->do_query($query1);
				}
				if (!empty($poss_ordids)) {
					$query2 = $query . " where AllItems.fileid in (".implode(',',$poss_fileids).") and $table.$valname in (".implode(',',$poss_ordids).")";
					$this->_parentlist->do_query($query2);
				}
			}
		}
	}
	
	public function format_db_value($valname, $index=NULL, $lookup=NULL) {
		if (!is_null($lookup)) {
			// start by looking for _lu value (otherwise will always end up with converted value)
			$value = $this->get($valname."_lu", $index);
			if (is_null($value)) {
				$value = $this->get($valname, $index);
				if (!is_int($value)) {
					$value = $this->_parentlist->get_lookup_id($lookup, $value);
				}
			}
		}
		else {
			$value = $this->get($valname, $index);
		}
		
		if (is_null($value))
			$outstr = "NULL";
		elseif (is_bool($value)) {
			if ($value===true)
				$outstr = "TRUE";
			else
				$outstr = "FALSE";
		}
		elseif (is_numeric($value) && substr($value,0,2)!='0x') {
				$outstr = $value;
		}
		else
			$outstr = "'".addslashes($value)."'";
		
		return $outstr;
	}
	
	// $row is a row of data already read from AllItems table
	// This function transfers data from $row into $this
	protected function _set_data_from_table($row, $table) {
		if ($table=='AllItems') {
			// don't rewrite rectype: entire class is based upon rectype, so should only be set when item first initialized
			$this->_edid = $row['edid'];
			$this->_formid = $row['formid'];
			$this->_ordid = (integer) $row['ordid'];
			$this->_fileid = (integer) $row['fileid'];
			$this->set($row['name'], 'name');
			foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
				$datainfo = $this->_parentlist->get_datainfo('', $valname);
				$this->set($row[$datainfo['sqlname']], $valname);
			}
			$this->_data_read = 1;
			return;
		}
		elseif ($table==$this->_rectype."_Record") {
			$primary = TRUE;
			$subid = NULL;
			$this->_data_read = max(2, $this->_data_read);
		}
		else {
			// don't set data_read=3 here -> want to make sure all subtables are read
			// (and need to set to 3 if no subtables even exist, but check was done)
			$primary = FALSE;
			$subid = $row['subid'];
		}
		
		$varnames = $this->_parentlist->get_datatables($this->_rectype, $table);
		if (empty($varnames))
			return NULL;
		
		foreach ($varnames as $varname) {
			if ($varname=='name')
				continue;
			if (isset($row[$varname]))
				$this->set($row[$varname], $varname, $subid);
			if (array_key_exists($varname."_lu", $row) && isset($row[$varname.'_lu']))
				$this->set((integer) $row[$varname."_lu"], $varname."_lu", $subid);
		}
	}
	
	protected function _add_table_to_query($table) {
		if ($table=='AllItems') {
			return array('vars' => array('AllItems.*'),
			             'tables' => array('AllItems' => NULL));
		}
		if (empty($this->_rectype)) {
		print "attempt to add_table_to_query for empty rectype\n";
		var_dump($this->_rectype);
		print "edid=".$this->_edid." formid=".$this->_formid." ordid=".$this->_ordid." fileid=".$this->_fileid."\n";
		return NULL;
		}

		if ($table==$this->_rectype."_Record")
			$primary = TRUE;
		else
			$primary = FALSE;
		
		$varnames = $this->_parentlist->get_datatables($this->_rectype, $table);
		if (empty($varnames))
			return NULL;
		
		$query_vars = array('ordid');
		$query_tables = '';
		
		$query_vars = array();
		$query_tables = '';
		if (!$primary)
			$query_vars[] = 'subid';
		
		// scan once to get output variable names and to add LEFT JOIN clauses for any necessary entries
		$nlookup = 0;
		$nvars = 0;
		foreach ($varnames as $varname) {
			if ($varname=='name')
				continue;
			$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $varname);
			$nvars++;
			if (array_key_exists('lookup', $datainfo) && $datainfo['lookup']) {
				$lu = $datainfo['lookup'];
				// Check that lookup table exists
				if ($this->_parentlist->hastable($lu.'_Lookup')) {
					$nlookup++;
					$query_tables .= "\n LEFT JOIN {$lu}_Lookup AS Table_L{$nlookup} ON (Table_L{$nlookup}.{$lu}_lu={$table}.".$datainfo['sqlname'].')';
				
					$query_vars[] = "Table_L{$nlookup}.{$lu} AS '{$varname}'";
					$query_vars[] = "{$table}.".$datainfo['sqlname']." AS '{$varname}_lu'";
				}
			}
			elseif (!empty($datainfo['sqlname']) && $datainfo['sqlname'] != $varname)
				$query_vars[] = $datainfo['sqlname']." AS '{$varname}'";
			else
				$query_vars[] = $varname;
		}
		if (!$nvars)
			return NULL;
		return array('vars' => $query_vars, 'tables' => array($table => $query_tables));
	}
	
	public function read_db($ordid=NULL, $data_toread=NULL) {
		if (!isset($data_toread)) {
			if (!isset($this->_data_toread))
				$data_toread = 3;
			else
				$data_toread = $this->_data_toread;
		}

		if (!isset($ordid)) {
			if (!isset($this->_ordid))
				return;
			$ordid = $this->_ordid;
		}
		if (isset($this->_ordid) && $ordid==$this->_ordid) {
			if ($this->_data_read >= $data_toread)
				return;
		}
		else {
			$this->_data_read = 0;
			$this->_ordid = $ordid;
		}
		
		if (empty($this->_parentlist) || $this->_parentlist->get_datamode()==CSData::DATAMODE_READ) {
			$this->_data_read = $data_toread;
			return;
		}
		
		if ($this->_data_read<1) {
			// do AllItems separate from the rest of the tables, since for some records there may not be any
			// additional data
			$query = "SELECT * FROM AllItems WHERE ordid={$ordid}";
			$row = $this->_parentlist->do_query($query, 'onerow');
			$this->_set_data_from_table($row, 'AllItems');
		}
		
		if ($data_toread<2)
			return; // _data_read was set by _set_allitems_data
		if (empty($this->_rectype)) {
			$this->_data_read = $data_toread;
			return;
		}

		$tablelist = $this->_parentlist->get_datatables($this->_rectype);
		foreach ($tablelist as $table) {
			if ($table==$this->_rectype."_Record") {
				// Primary table for this record
				if ($this->_data_read>=2)
					continue; // already read
			}
			else {
				// Secondary tables for this record
				if ($data_toread<3)
					continue; // don't want to read
			}
			
			$query_data = $this->_add_table_to_query($table);
			if (empty($query_data))
				continue;
			
			$query = 'SELECT ';
			$query .= implode(', ', $query_data['vars']);
			$query .= "\n".' FROM '.$table;
			$query .= $query_data['tables'][$table];
			$query .= "\n".' WHERE ordid='.$this->ordid();
			
			$res = $this->_parentlist->do_query($query);
			while (($row=$this->_parentlist->row_query($res))!==false) {
				$this->_set_data_from_table($row, $table);
			}
		}
		// set _data_read based on what we tried to read, regardless of what tables actually exist for
		// this particular record
		$this->_data_read = $data_toread;
	}
	
	public function dump_data() {
		print "id=".$this->get('id').", fileid=".$this->get('fileid').", rectype='".$this->_rectype."', edid=".$this->format_db_value('edid').", name=".$this->format_db_value('name');
		if (!is_null($this->formid()))
			print ", formid=".$this->format_db_value('formid');
		foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
			$datainfo = $this->_parentlist->get_datainfo('', $valname);
			print ", ".$datainfo['sqlname'].'='.$this->format_db_value($valname);
		}
		print "\n";
		
		if (empty($this->_rectype))
			return;
		$tablelist = $this->_parentlist->get_datatables($this->_rectype);
		if (!is_array($tablelist))
			return;
		
		foreach ($tablelist as $table) {
			$indices = array();
			$valnames = $this->_parentlist->get_datatables($this->_rectype, $table);
			if ($table==$this->_rectype."_Record")
				$indices = array(0);
			else {
				$indices = $this->indices(substr($table,5,4));
			}
			$nindex = count($indices);
			
			foreach ($indices as $n) {
				if ($table==$this->_rectype."_Record") {
					$ni = NULL;
				}
				else {
					$ni = $n;
				}
				print "  Table $table  index $ni (of $nindex)\n";
					
				foreach ($valnames as $valname) {
					if ($valname=='name')
						continue;
					$datainfo = $this->_parentlist->get_datainfo($this->_rectype, $valname);
					print "    $valname (".$datainfo['sqlname'].") = ".$this->format_db_value($valname, $ni, (array_key_exists('lookup', $datainfo)?$datainfo['lookup']:NULL))."\n";
				}
			}
		}
	}
	
	public function get_itemset_query($options=NULL) {
		if (!isset($this->_rectype))
			return NULL;
		
		// Somewhere the display filters need to do a check that the filtered variable exists
		// in the query -- particularly in regard to SR where the variable may be part of a subrec
		// that no longer exists
		$query_filter = '';
		if (isset($this->_display_filters)) {
			foreach ($this->_display_filters as $filter => $fdata) {
				if (!isset($_GET['filter_'.$filter]))
					continue;
				$fval = $_GET['filter_'.$filter];
				if (isset($fdata['options'])) {
					if (isset($fdata['sql'][$fval]))
						$query_filter .= ' AND '.$filter.' '.$fdata['sql'][$fval];
					else
						$query_filter .= ' AND '.$filter.'='.$fval;
				}
				elseif (isset($fdata['rectype']))
						$query_filter .= ' AND '.$filter.'_id='.$fval;
				else
						$query_filter .= ' AND '.$filter.'_lu='.$fval;
			}
		}
		
		if (!empty($options['getcount']) && empty($query_filter)) {
			$query = 'SELECT COUNT(*) as count FROM AllItems';
		}
		else {
			$allitem_vals = $this->_parentlist->get_datatables('', 'AllItems');
		
			$query_vars = array('AllItems.*');
			$query_tables = 'AllItems ';
		
			$query_data = $this->_add_table_to_query($this->_rectype.'_Record');
			if (!empty($query_data)) {
				$query_vars = array_merge($query_vars, $query_data['vars']);
				$query_tables .= "\n".' LEFT JOIN '.$this->_rectype.'_Record USING (ordid)';
				$query_tables .= $query_data['tables'][$this->_rectype.'_Record'];
			}
		
			if (empty($options['getcount'])) {
				$query = 'SELECT ';
				$query .= implode(', ', $query_vars);
			}
			else {
				$query = 'SELECT COUNT(*) as count';
			}
			$query .= "\n".' FROM '.$query_tables;
		}
		
		$query .= " WHERE rectype='".$this->_rectype."'";
		if (!empty($this->_query_filter))
			$query .= ' AND '.$this->_query_filter;
		$query .= $query_filter;
		
		if (!empty($options['getcount']))
			return $query;
		
		$query .= ' ORDER BY name IS NULL, name, edid IS NULL, edid, formid, fileid';
		
		if (isset($options['offset']) || isset($options['limit'])) {
			$query .= ' LIMIT ';
			if (isset($options['offset']) && $options['offset']>0)
				$query .= $options['offset'].', ';
			if (isset($options['limit']) && $options['limit']>0)
				$query .= $options['limit'];
			else
				$query .= '100';
		}
		return $query;
	}
	
	public function start_itemset($options=NULL) {
		$query = $this->get_itemset_query($options);
		if (empty($query))
			return NULL;
		$this->_itemset = $this->_parentlist->do_query($query);
		if (empty($this->_itemset))
			return FALSE;
		else
			return TRUE;
	}
	
	public function next_itemset() {
		if (empty($this->_itemset))
			return NULL;
		$row = $this->_parentlist->row_query($this->_itemset);
		if (empty($row))
			return NULL;
		
		// go through parentlist to setup newitem, so that it's added to csdata's memory management stuff
		$curritem = $this->_parentlist->newitem($row['rectype'], $row['ordid'], 0);
		$curritem->_set_data_from_table($row, 'AllItems');
		$curritem->_set_data_from_table($row, $this->_rectype.'_Record');
		$curritem->_data_read = 2;
		$curritem->_data_toread = $this->_data_toread;
		return $curritem;
	}
	
	public function get_display_columns($display) {
		// Generate first-round version of coldata by merging generic_column_data and column_data
		// Only include items that belong in this display
		$coldata = array();
		foreach ($this->_generic_column_data as $cname => $cdata) {
			$keep = FALSE;
			if (isset($cdata['display'])) {
				if (!is_array($cdata['display'])) {
					if ($cdata['display']=='all' || $cdata['display']==$display)
						$keep = TRUE;
				}
				else {
					if (in_array('all', $cdata['display']) || in_array($display, $cdata['display']) || !empty($cdata['display']['all']) || !empty($cdata['display'][$display]))
						$keep = TRUE;
				}
			}
			if (!$keep && isset($this->_column_data[$cname]['display'])) {
				if (!is_array($this->_column_data[$cname]['display'])) {
					if ($this->_column_data[$cname]['display']=='all' || $this->_column_data[$cname]['display']==$display)
						$keep = TRUE;
				}
				else {
					if (in_array('all', $this->_column_data[$cname]['display']) || in_array($display, $this->_column_data[$cname]['display']) || !empty($this->_column_data[$cname]['display']['all']) || !empty($this->_column_data[$cname]['display'][$display]))
						$keep = TRUE;
				}
			}
			
			if (!$keep)
				continue;
			
			$coldata[$cname] = $cdata;
			// vars and display_option will get overwritten by this merge, instead of merging the sub-arrays
			// that's what I want for vars -- allow individual item types to override generic vars
			if (isset($this->_column_data[$cname])) {
				$coldata[$cname] = array_merge($coldata[$cname], $this->_column_data[$cname]);
			}
		}
		if (isset($this->_column_data)) {
			foreach ($this->_column_data as $cname => $cdata) {
				if (isset($coldata[$cname]))
					continue;
				$keep = FALSE;
				if (isset($cdata['display'])) {
					if (!is_array($cdata['display'])) {
						if ($cdata['display']=='all' || $cdata['display']==$display)
							$keep = TRUE;
					}
					else {
						if (in_array('all', $cdata['display']) || in_array($display, $cdata['display']) || !empty($cdata['display']['all']) || !empty($cdata['display'][$display]))
							$keep = TRUE;
					}
				}				
				if (!$keep)
					continue;
				
				$coldata[$cname] = $cdata;
			}
		}
		// First round of setting option -- default values filled in later
		foreach ($coldata as $cname => $cdata) {
			$coldata[$cname]['option'] = NULL;
			$option = NULL;
			if (isset($cdata['display_option'][$display])) {
				$option =$cdata['display_option'][$display];
			}
			elseif (isset($this->_generic_column_data[$cname]['display_option'][$display])) {
				$option = $this->_generic_column_data[$cname]['display_option'][$display];
			}
			if (isset($option)) {
				if (!is_array($option))
					$coldata[$cname]['option']['location'] = $option;
				else
					$coldata[$cname]['option'] = $option;
			}
		}
		
		// Get list of all valnames for this item type
	
		// valnames that are always present, and not listed in get_datatables 
		$donevals = array();
		$valnames =
			array('ordid' => 'AllItems',
			      'edid' => 'AllItems',
			      'id' => 'AllItems',
			      'name' => 'AllItems',
			      'fileid' => 'AllItems',
			      'rectype' => 'AllItems');
		if ($this->_parentlist->get_classid()!='MW')
			$valnames['formid'] = 'AllItems';
		foreach ($this->_parentlist->get_datatables('', 'AllItems') as $valname) {
			$valnames[$valname] = 'AllItems';
		}
		if (!empty($this->_rectype))
			$tablelist = $this->_parentlist->get_datatables($this->_rectype);
		else
			$tablelist = NULL;
		if (is_array($tablelist)) {
			foreach ($tablelist as $table) {
				foreach ($this->_parentlist->get_datatables($this->_rectype, $table) as $valname) {
					if (substr($valname,0,5)=='count' && strlen($valname)==9 && preg_match('/^count[A-Z0-9]{4}$/', $valname))
						continue;
					$valnames[$valname] = $table;
				}
			}
		}
		
		// Remove generic_column_data entries that don't exist in this item type
		foreach ($coldata as $cname => $cdata) {
			if (!isset($valnames[$cname]) && !isset($this->_column_data[$cname])) {
				unset($coldata[$cname]);
				continue;
			}
			if (!isset($cdata['vars'])) {
				$donevals[$cname] = TRUE;
			}
			elseif (!is_array($cdata['vars'])) {
				$donevals[$cdata['vars']] = TRUE;
			}
			else {
				foreach ($cdata['vars'] as $vname) {
					$donevals[$vname] = TRUE;
				}
			}
		}
		
		// For details_html table, make sure every variable has been accounted for
		if ($display=='details_html') {
			// values done in intro
			$donevals['ordid'] = TRUE;
			$donevals['rectype'] = TRUE;
			$donevals['fileid'] = TRUE;
			foreach ($valnames as $cname => $table) {
				if (!empty($donevals[$cname]))
					continue;
				if (isset($this->_generic_column_data[$cname]))
					$coldata[$cname] = $this->_generic_column_data[$cname];
				elseif (isset($this->_column_data[$cname]))
					$coldata[$cname] = $this->_column_data[$cname];
				else
					$coldata[$cname] = array();
				$datainfo = $this->_parentlist->get_datainfo($this->_rectype,$cname);
				if (empty($coldata[$cname]['format']) && isset($datainfo['format']) && ($datainfo['format']=='formid' || $datainfo['format']=='mgefid' || $datainfo['format']=='edid')) {
					$coldata[$cname]['format'] = array('ordid_as_name');
				}
			}
			foreach ($coldata as $cname => $cdata) {
				if (!empty($coldata[$cname]['option']['location']))
					continue;
				if (!isset($valnames[$cname]) || $valnames[$cname]==$this->_rectype.'_Record') {
					$coldata[$cname]['option']['location'] = 'main';
				}
				elseif ($valnames[$cname]=='AllItems') {
					$coldata[$cname]['option']['location'] = 'intro_ex';
				}
				else {
					$coldata[$cname]['option']['location'] = 'table_'.substr($valnames[$cname],5,4);
				}
			}
		}
		elseif (substr($display,0,4)=='list') {
			// take 'end' options and place at end
			$endkeys = array();
			$colkeys = array_keys($coldata);
			$redo = FALSE;
			foreach ($coldata as $cname => $cdata) {
				if (empty($cdata['option']['location']))
					continue;
				$redo = TRUE;
				$loc = $cdata['option']['location'];
				$cindex = array_search($cname, $colkeys);
				if ($loc === 'end') {
					$endkeys[] = $cname;
					unset($colkeys[$cindex]);
				}
				elseif ($loc<0) {
					// don't want to replace an existing key....
					if (isset($endkeys[abs($loc)-1]))
						$endkeys[] = $cname;
					else
						$endkeys[abs($loc)-1] = $cname;
					unset($colkeys[$cindex]);
				}
				elseif (is_int($loc)) {
					unset($colkeys[$cindex]);
					array_splice($colkeys,$loc,0,$cname);
				}
			}
			if ($redo) {
				krsort($endkeys);
				$tdata = $coldata;
				$coldata = array();
				foreach (array_merge($colkeys,$endkeys) as $cname) {
					$coldata[$cname] = $tdata[$cname];
				}
			}
		}
		
		// special handling for unchecked columns
		// needs to be done after everything else so that removing columns doesn't mess up any positioning
		// that's done
		if ($this->_parentlist->is_datachecked(NULL,NULL)) {
			foreach ($coldata as $cname => $cdata) {
				if (!$this->_parentlist->is_datachecked($this->_rectype, $cname)) {
					if (isset($_GET['format']) && $_GET['format']=='wiki') {
						// this should probably be done more elegantly, but don't want to put a huge amount of
						// effort into a temporary feature
						unset($coldata[$cname]);
					}
					else {
						if (!empty($coldata[$cname]['format']) && !is_array($coldata[$cname]['format'])) {
							$coldata[$cname]['format'] = array($coldata[$cname]['format']);
						}
					$coldata[$cname]['format'][] = 'unchecked';
					}
				}
			}
		}
		return $coldata;
	}
	
	public function display_filters() {
		global $gOutput;
		if (empty($this->_display_filters))
			return '';
		$selects = array();
		
		foreach ($this->_display_filters as $filter => $fdata) {
			$options = array();
			if (isset($fdata['options'])) {
				$options = $fdata['options'];
			}
			elseif (isset($fdata['rectype'])) {
				$query = 'SELECT ordid, name, formid, edid from AllItems where rectype="'.$fdata['rectype'].'" order by name';
				$res = $this->_parentlist->do_query($query);
				$details = array();
				while ($row=$this->_parentlist->row_query($res)) {
					if (empty($row['name']) || strlen(trim($row['name']))<2)
						continue;
					$details[$row['name']][$row['ordid']] = $row;
					$options[$row['ordid']] = $row['name'];
				}
				foreach ($details as $name => $ddata) {
					if (count($ddata)==1)
						continue;
					foreach ($ddata as $ordid => $odata) {
						$options[$odata['ordid']] = $odata['name'].' ('.(isset($odata['formid'])?$odata['formid']:$odata['edid']).')';
					}
				}
			}
			else {
				$datainfo = $this->_parentlist->get_datainfo($this->rectype(),$filter);
				if (empty($datainfo) || !isset($datainfo['lookup']))
					continue;
				$lu = $datainfo['lookup'];
				if (!$this->_parentlist->check_lookup($lu))
					continue;
				$options = $this->_parentlist->get_lookup_list($lu);
			}
			if (empty($options))
				continue;
			
			$sel = '<select name="filter_'.$filter.'">';
			if (isset($fdata['title_pl']))
				$sel .= '<option value="-1">-- All '.$fdata['title_pl'].'--</option>';
			else
				$sel .= '<option value="-1">-- All '.$fdata['title'].'s--</option>';
			foreach ($options as $id => $value) {
				$sel .= '<option value="'.$id.'"';
				if (isset($_GET['filter_'.$filter]) && $_GET['filter_'.$filter]==$id)
					$sel .= ' selected';
				$sel .= '>'.$value.'</option>';
			}
			$sel .= '</select>';
			$selects[] = $sel;
		}
		if (empty($selects))
			return '';
		$text = '';
		$url = $_SERVER['PHP_SELF'];
		$text .= '<form action="'.$url.'" method="get">'."\n";
		$text .= '<b>Filter records by:</b><br />'."\n";
		$text .= '<input type="hidden" name="game" value="'.$_GET['game'].'" />'."\n";
		$text .= '<input type="hidden" name="rec" value="'.$this->rectype().'" />'."\n";
		$text .= implode("\n", $selects)."\n";
		$text .= '<input type="submit" value="Apply" />'."\n";
		$text .= "</form>\n";
		return $text;
	}

	protected function _output_row_format($valname, $defaultitem=NULL, $index=NULL) {
		global $gOutput;

		$value = $this->get($valname, $index);
		if (!empty($defaultitem)) {
			$defvalue = $defaultitem->get($valname, $index);
			if ((!isset($value)||$value==='')&&(!isset($defvalue)||$defvalue===''))
				return;
		}
		else {
			if (!isset($value) || $value==='')
				return;
		}

		$gOutput->start_new_row();
		$title = $gOutput->format_title($valname);
		$gOutput->add_row_value('parameter', $title);
		if (is_array($value)) {
			$fvalues = array();
			foreach ($value as $v)
				$fvalues[] = $gOutput->format_value($valname, $v, $this);
			$gOutput->add_row_value('value', implode('; ', $fvalues));
		}
		else {
			$fvalue = $gOutput->format_value($valname, $value, $this);
			$gOutput->add_row_value('value', $fvalue);
		}
		if (!empty($defaultitem)) {
			if ($defvalue!==$value) {
				$defvalue = $gOutput->format_value($valname, $defvalue, $defaultitem);
				$gOutput->add_row_value('default', $defvalue);
				$gOutput->set_cell_skip('default', false);
			}
			else {
				$gOutput->set_cell_skip('default', 'left');
			}
		}
		$gOutput->end_row();
	}

	protected function _output_column_format($valname, $defaultitem=NULL, $index=NULL) {
		global $gOutput;

		$value = $this->get($valname, $index);
		$fvalue = $gOutput->format_value($valname, $value, $this);

		if (!empty($defaultitem)) {
			$defvalue = $defaultitem->get($valname, $index);
			if ($defvalue!==$value) {
				// sometimes raw values can differ but processed values don't
				// (e.g., on CTDA operator, if difference is in extra, unused bits)
				$defvalue = $gOutput->format_value($valname, $defvalue, $defaultitem);
				if ($defvalue!==$fvalue) {
					$fvalue .= '<br/>(Default: '.$defvalue.')';
				}
			}
		}

		$gOutput->add_row_value($valname, $fvalue);
	}

	protected function _get_section_indices($table, $section_data, $is_main, $subindex, &$keyname) {
		$keyname = NULL;
		if ($is_main) {
			$section_name = 'table_'.$table;
			if (!isset($section_data[$section_name]))
				return array();
			if (isset($section_data[$section_name][$table]))
				$keyname = $table;
			else
				$keyname = array_shift(array_keys($section_data[$section_name]));
		}
		else {
			if (isset($section_data[$table]))
				$keyname = $table;
			else
				$keyname = array_shift(array_keys($section_data));
		}
		$indices = $this->indices($keyname, $subindex);
		if (empty($indices))
			return array();
		return $indices;
	}
	protected function _get_section_keyval($table, $indexname, $i, $index) {
		$value = NULL;
		if ($indexname==$table)
			$value = $this->get($table, $index);
		if (isset($value))
			return $value;
		return $i;
	}

	// NB needs to work with formids or edids as input -> MW doesn't have formids
	
	// Function to display all known information about an item
	// Start by just setting up the HTML version
	// If a text version is done, likely to be just a component of page (e.g., the text of a book)
	// If a wiki version is done, likely to be specifically tuned to an output template
	public function display_item_data() {
		global $gOutput;

		$defaultitem = NULL;
		if (!$this->get('mod_status_default')) {
			$defaultitem = $this->_parentlist->get_item($this->get('id'),NULL,$this->get('rectype'));
		}
		
		$coldata = $this->get_display_columns('details_html');
		$sections = array();
		foreach ($coldata as $cname => $cdata) {
			if (!isset($cdata['option']['location']))
				$section = 'main';
			else
				$section = $cdata['option']['location'];
			$sections[$section][$cname] = $cdata;
		
		}
		
		$name = $this->get('name');
		$edid = $this->get('edid');
		$formid = $gOutput->format_formid($this->get('formid'), $this);
		
		if (!empty($name)) {
			if (!empty($formid))
				$page_title = $name.' ('.$formid.')';
			else
				$page_title = $name.' ('.$edid.')';
		}
		elseif (!empty($edid)) {
			$page_title = $edid;
			if (!empty($formid))
				$page_title .= ' ('.$formid.')';
		}
		else
			$page_title = $formid;
		$gOutput->set_param($page_title, 'page_title');
		
		$gOutput->start_list();
		$gOutput->list_item('Record Type: '.$gOutput->format_link_rectype($this->get('rectype')), FALSE);
		if (!empty($name))
			$gOutput->list_item('Name: '.$name);
		if (!empty($edid))
			$gOutput->list_item('Editor ID: '.$edid);
		if (!empty($formid))
			$gOutput->list_item('Form ID: '.$formid);
		$gOutput->list_item('Source File: '.$gOutput->format_value('fileid', $this->get('fileid'), $this));
		$gOutput->list_item('Patch Status: '.$gOutput->format_value('mod_status', $this->get('mod_status'), $this), FALSE);
		
		$gOutput->set_columns($sections['intro_ex']);
		$flags = array();
		foreach ($sections['intro_ex'] as $cname => $cdata) {
			if (!in_array(strtolower($cname),array('id','fileid','mod_status','formid')) && $this->get($cname))
				$flags[] = $gOutput->format_title($cname);
		}
		if (!empty($flags)) {
			$gOutput->list_item('Flags: '.implode(' ', $flags)."\n");
		}
		$gOutput->end_list();
		
		if (!empty($sections['main'])) {
			$gOutput->init_table();
			// this really just sets _coldata for formatting
			$gOutput->set_columns($sections['main']);
			if (!empty($defaultitem))
				$gOutput->set_columns(array('parameter', 'value', 'default'));
			else
				$gOutput->set_columns(array('parameter', 'value'));
			$gOutput->start_table();
			
			foreach ($sections['main'] as $cname => $cdata) {
				$this->_output_row_format($cname, $defaultitem);
			}
			$gOutput->end_table(FALSE);
		}
		
		foreach ($sections as $section => $sdata) {
			if (substr($section,0,6)!='table_')
				continue;
			$table = substr($section,6);
			if ($this->_rectype=='QUST' && in_array($table, array('INDX', 'QOBJ', 'ALLS', 'ALST')))
				continue;
			
			$cname = array_shift(array_keys($sdata));
			if ($this->_complex_index && !preg_match('/[a-z]/', $table)) {
				$indices = $this->indices($cname, $table);
			}
			else {
				$indices = $this->indices($cname);
			}
			if (empty($indices))
				continue;
			
			$gOutput->init_table();
			$gOutput->set_columns($sdata);
			$gOutput->start_table();
			foreach ($indices as $i) {
				$gOutput->start_new_row();
				foreach ($sdata as $sname => $sx) {
					$this->_output_column_format($sname, $defaultitem, $i);
				}
				$gOutput->end_row();
			}
			$gOutput->end_table(FALSE);
		}
		
		// should only be done for Skyrim
		if ($this->_rectype=='QUST') {
			foreach (array('INDX' => 'QSDT') as $t1 => $t2) {
				$iset1 = $this->_get_section_indices($t1, $sections, TRUE, NULL, $cname1);
				// some QOBJ indices don't show up in QSDT array
				if ($t1=='INDX' && isset($sections['table_QOBJ'])) {
					$ifull = array();
					foreach ($iset1 as $i1 => $index1) {
						$name1 = $this->_get_section_keyval($t1, $cname1, $i1, $index1);
						$ifull[$name1] = $index1;
					}
					
					foreach ($this->_get_section_indices('QOBJ', $sections, TRUE, NULL, $c) as $indexq) {
						$name1 = $this->get($c, $indexq);
						if (isset($ifull[$name1]))
							continue;
						$ifull[$name1] = $indexq;
					}
					ksort($ifull);
					$iset1 = $ifull;
				}
				if (empty($iset1))
					continue;
				
				foreach ($iset1 as $i1 => $index1) {
					$name1 = $this->_get_section_keyval($t1, $cname1, $i1, $index1);
					$gOutput->text('<hr><h2>'.$t1.'='.$name1."</h2>\n");
					
					// special treatment for QOBJ, since it doesn't use $index1 values -- rather, need to cross-reference INDX and QOBJ
					if ($t1=='INDX' && isset($sections['table_QOBJ'])) {
						foreach ($this->_get_section_indices('QOBJ', $sections, TRUE, NULL, $c) as $indexq) {
							if ($this->get($c, $indexq)!=$name1)
								continue;
							$gOutput->init_table();
							$gOutput->set_columns($sections['table_QOBJ']);
							$gOutput->start_table();
							$gOutput->start_new_row();
							foreach ($sections['table_QOBJ'] as $sname => $sx)
								$this->_output_column_format($sname, $defaultitem, $indexq);
							$gOutput->end_row();
							$gOutput->end_table(FALSE);
						}
					}

					foreach ($this->_get_section_indices($t2, $sections, TRUE, $index1, $cname2) as $i2 => $index2) {
						$name2 = $this->_get_section_keyval($t2, $cname2, $i2, $index2);
						$gOutput->text("<h4>$t1=$name1; $t2=$name2</h4>\n");
						
						$gOutput->init_table();
						$gOutput->set_columns($sections['table_'.$t2]);
						$gOutput->start_table();
						$gOutput->table_row($this, $index2);
						$gOutput->end_table(FALSE);
						
						foreach ($sections as $section => $sdata) {
							if (substr($section,0,6)!='table_')
								continue;
							$t3 = substr($section,6);
							if ($t3==$t1 || $t3==$t2)
								continue;
							$iset3 = $this->_get_section_indices($t3, $sdata, FALSE, $index2, $cname3);
							if (empty($iset3))
								continue;
							$gOutput->init_table();
							$gOutput->set_columns($sdata);
							$gOutput->start_table();
							foreach ($iset3 as $i3 => $index3) {
								$gOutput->start_new_row();
								foreach ($sdata as $sname => $sx)
									$this->_output_column_format($sname, $defaultitem, $index3);
								$gOutput->end_row();
							}
							$gOutput->end_table(FALSE);
						}
					}
					if ($this->get('fragment_sid')) {
						$frag_loc = $this->get('fragment_loc', $index1);
						$frag_len = $this->get('fragment_len', $index1);
						if (!empty($frag_loc) && !empty($frag_len)) {
							$query = 'SELECT * FROM scripts WHERE sid='.$this->get('fragment_sid');
							$script = $this->_parentlist->do_query($query, 'onerow');
							if (!empty($script)) {
								$fragment = substr($script['contents'],$frag_loc,$frag_len);
								$gOutput->text('<br/><b>Script Fragment_'.$this->get('fragment_num',$index1).'</b><br/><pre>'.$fragment.'</pre>');
							}
						}
					}
				}

				unset ($sections['table_'.$t1]);
			}
			unset ($sections['table_QOBJ']);

			$aliases = array();
			foreach (array('ALLS' => 'ALID', 'ALST' => 'ALID') as $t1 => $t2) {
				foreach ($this->_get_section_indices($t1, $sections, TRUE, NULL, $cname1) as $i1 => $index1) {
					$name1 = $this->_get_section_keyval($t1, $cname1, $i1, $index1);
					$aliases[$name1] = array('table' => $t1, 'index' => $index1, 'cname1' => $cname1);
				}
			}
			ksort($aliases);
			foreach ($aliases as $name1 => $alias_data) {
				$t1 = $alias_data['table'];
				$index1 = $alias_data['index'];
				$cname1 = $alias_data['cname1'];
				$t2 = 'ALID';

				foreach ($this->_get_section_indices($t2, $sections, TRUE, $index1, $cname2) as $i2 => $index2) {
					$name2 = $this->_get_section_keyval($t2, $cname2, $i2, $index2);
					$gOutput->text("<hr><h2>$t1=$name1; $t2=$name2</h2>\n");
					
					$gOutput->init_table();
					// this really just sets _coldata for formatting
					$gOutput->set_columns($sections['table_'.$t2]);
					$gOutput->set_columns(array('parameter', 'value'));
					$gOutput->start_table();

					// list values that identify alias first
					foreach (array('unique_actor', 'specific_reference', 'specific_location', 'location_ref_type', 'created_object', 'external_quest', 'event_type') as $valname) {
						$value = $this->get($valname, $index2);
						unset ($sections['table_'.$t2][$valname]);
						if (empty($value))
							continue;
						$gOutput->start_new_row();
						$title = 'Fill Type: '.$gOutput->format_title($valname);
						$gOutput->add_row_value('parameter', $title);
						$fvalue = $gOutput->format_value($valname, $value, $this);
						$gOutput->add_row_value('value', $fvalue);
						$gOutput->end_row();
					}
					// followed by parameters related to those values
					foreach (array('ALFA', 'ALCA', 'ALCL', 'ALFD', 'external_aliasid') as $valname) {
						$value = $this->get($valname, $index2);
						unset ($sections['table_'.$t2][$valname]);
						if (empty($value))
							continue;
						$gOutput->start_new_row();
						$title = $gOutput->format_title($valname);
						$gOutput->add_row_value('parameter', $title);
						$fvalue = $gOutput->format_value($valname, $value, $this);
						$gOutput->add_row_value('value', $fvalue);
						$gOutput->end_row();
					}

					foreach ($sections['table_'.$t2] as $valname => $vdata) {
						if ($valname=='ALID')
							continue;
						$value = $this->get($valname, $index2);
						if (empty($value))
							continue;
						$gOutput->start_new_row();
						$title = $gOutput->format_title($valname);
						$gOutput->add_row_value('parameter', $title);
						$fvalue = $gOutput->format_value($valname, $value, $this);
						$gOutput->add_row_value('value', $fvalue);
						$gOutput->end_row();
					}
					$gOutput->end_table(FALSE);
						
					foreach ($sections as $section => $sdata) {
						if (substr($section,0,6)!='table_')
							continue;
						$t3 = substr($section,6);
						if ($t3==$t1 || $t3==$t2)
							continue;
						$iset3 = $this->_get_section_indices($t3, $sdata, FALSE, $index2, $cname3);
						if (empty($iset3))
							continue;
						$gOutput->init_table();
						$gOutput->set_columns($sdata);
						$gOutput->start_table();
						foreach ($iset3 as $i3 => $index3) {
							$gOutput->table_row($this, $index3);
						}
						$gOutput->end_table(FALSE);
					}
				}
			}
			unset ($sections['table_ALLS']);
			unset ($sections['table_ALST']);
		}
		

/* this seems to be unfinished code with no output and produces an error for MW
// comment out for now - Alfwyn
		if ($this->get('rectype')=='CELL') {
			foreach (array('ACHR', 'REFR') as $reftype) {
				$query = 'SELECT rectype, count(*) as reccount FROM AllItems inner join '.$reftype.'_Record on (AllItems.ordid='.$reftype.'_Record.base_id) WHERE cell_id='.$this->get('ordid').' GROUP BY rectype ORDER BY rectype';
				$res = $this->_parentlist->do_query($query);
				while ($row=$this->_parentlist->row_query($res)) {
				}
			}		
		}
*/
		// Then need to do any misc sections
	
		// then loop through main x_Record table
		// display each variable
		// if x_lu or x_id -> only display x (unless x empty)
	
		// loop through extra tables and create sections which display all columns from a table together
	
		// need a way to customize on a per-rectype basis
		// split off BOOK desc into a box
		// translate/group items
		//  -> need to provide 'get' value to display
		//  -> plus need to provide _data values that are taken care of by that get
		// *BUT* even if given customization, still want to loop through all data and make sure it's been
		// handled
		$gOutput->finish();
		if ($defaultitem)
			$defaultitem->release();
	}
	
	// For armor, weapons, and clothes: get the base/plain item with the same appearance as this item
	// should add in a check against basic levellist
	// or even just count number of LVLI appearances?
	function plain_item_ordid() {
		$rectype = $this->rectype();
		
		// Use template_id whenever possible
		$ordid = $this->get('template_id');
		if (!empty($ordid))
			return $ordid;
		
		$modelname = 'model';
		$model = $this->get('model');
		if (empty($model)) {
			$modelname = 'MODL';
			$model = $this->get('MODL');
		}
		if (empty($this->_parentlist) || empty($rectype) || empty($model))
			return NULL;
		$query = 'SELECT AllItems.ordid, AllItems.name, AllItems.edid FROM AllItems INNER JOIN '.$rectype.'_Record USING (ordid) WHERE '.$modelname.'="'.addslashes($model).'" AND AllItems.name IS NOT NULL AND AllItems.name <> \'\' ';
		$edata = $this->_parentlist->get_datainfo($rectype, 'countENAM');
		if (!empty($edata) && $edata['sqltable']==$rectype.'_Record') {
			$query .= ' AND countENAM=0';
		}
		else {
			$edata = $this->_parentlist->get_datainfo($rectype, 'countEITM');
			if (!empty($edata) && $edata['sqltable']==$rectype.'_Record') {
				$query .= ' AND countEITM=0';
			}
			else {
				return NULL;
			}
		}
		$query .= ' ORDER BY fileid, length(edid), length(name)';
		
		$res = $this->_parentlist->do_query($query);
		if (empty($res))
			return NULL;
		$ordid = NULL;
		$nlvluse = 0;
		
		while ($row = $this->_parentlist->row_query($res)) {
			if (empty($row['ordid']))
				continue;
			
			if ($this->_parentlist->hastable('LevelList')) {
				$cquery = 'SELECT count(*) as nuse FROM LevelList where item_ordid='.$row['ordid'];
			}
			elseif ($this->_parentlist->hastable('LVLI_LVLO_Record')) {
				$cquery = 'SELECT count(*) as nuse FROM LVLI_LVLO_Record where item_id='.$row['ordid'];
			}
			else {
				$cquery = NULL;
			}
			
			if (!empty($cquery)) {
				$crow = $this->_parentlist->do_query($cquery, 'onerow');
				$nuse = $crow['nuse'];
				if ($nuse>$nlvluse) {
					$ordid = $row['ordid'];
					$nlvluse = $nuse;
				}
			}
			else {
				$ordid = $row['ordid'];
			
			// semi-kludge to prevent Bound/Dremora version of items or test items from being seen as base item
				if (substr($row['name'],0,6)!='Bound ' && substr($row['name'],0,8)!='Dremora ' && substr($row['edid'],0,4)!='Test')
					break;
			}
		}
		if (!empty($ordid) && $ordid!=$this->ordid())
			return $ordid;
		else
			return NULL;
	}
	
	// For armor, weapons, clothes: figure out whether this item is part of a leveled list
	// If so, return id of list
	// Note that this doesn't do much for MW, since MW didn't really have leveled lists in same way
	// that OB did.  But function can safely be used with MW
			
	// It's probably better to do this using LevelList, but I can't count on that to exist when
	// levellist_id first derived
	// Well... for that matter, can't count LVLI or LEVI, either...
	// This probably needs to be re-done on a second read
	// Rethink whether this algorithm can be more accurate using LevelList
	function derive_levellist_id() {
		// not entirely sure *why* this happens on a data re-read
		$o = $this->get('ordid');
		if (empty($o))
			return NULL;
		// Can't just look for items with repeating names --
		// miss cases such as Amber Boots/Perfect Amber Boots; get false cases such as Acrobatics Pants
			
		// But I'm getting too many false matches in Skyrim data...
			
		// Use raw table data instead of counting on LevelList to exist
			
		if ($this->_parentlist->hastable('LVLI_LVLO_Record')) {
			$table = 'LVLI_LVLO_Record';
		}
		elseif ($this->_parentlist->hastable('LEVI_INAM_Record')) {
			$table = 'LEVI_INAM_Record';
		}
		else
			return NULL;
		
		// Instead look for lists where item has 100% chance of appearing, 1 time
		// Then check that list is level-dependent
		$query = "SELECT ordid FROM ".substr($table,0,4)."_Record INNER JOIN $table USING (ordid) WHERE item_id=".$this->get('ordid')." AND chancenone=0 ";
		if (substr($table,0,4)=='LVLI')
			$query .= " AND item_count=1 ";
		
		$res = $this->_parentlist->do_query($query);
		if (mysql_num_rows($res)>1)
			return NULL;
		while ($row=$this->_parentlist->row_query($res)) {
			$squery = "SELECT * FROM $table WHERE ordid=".$row['ordid']." ORDER BY item_lvl";
			$sres = $this->_parentlist->do_query($squery);
			if (mysql_num_rows($sres)<=1)
				break;
			$lvllast = NULL;
			$ok = TRUE;
			while ($srow=$this->_parentlist->row_query($sres)) {
				if (isset($srow['item_count']) && $srow['item_count']>1) {
					$ok = FALSE;
					break;
				}
				// More than 1 item per-level not allowed
				if (isset($lvllast) && $srow['item_lvl']==$lvllast) {
					$ok = FALSE;
					break;
				}
				$lvllast = $srow['item_lvl'];
			}
			if ($ok && $lvllast > 1) {
				return $row['ordid'];
			}
		}
		
		return NULL;
	}
	
	// This function is unlikely to be correct for MW
	// But, in the absence of any better data for MW, might as well use it
	// (esp to help work out spell schools)
	public function effect_cost($index) {
		if (!array_key_exists('effect_id', $this->_data) || !is_array($this->_data['effect_id']) || !array_key_exists($index, $this->_data['effect_id']))
			return NULL;
		if (!is_object($effect = $this->_parentlist->get_item($this->get('effect_id', $index))))
			return NULL;
		// GMST values known to have an effect here:
		// fMagicCostScale = 1.28
		// (NB some GMST values are *not* written to data file, but are just hard-wired into code...
		//  such as fMagicCEEnchantMagOffset)
		$basecost = $effect->get('basecost');
		$mag = max($this->get('eff_mag', $index), 1);
		$dur = max($this->get('eff_dur', $index), 1);
		$area = max($this->get('eff_area', $index)*0.15, 1);
		$cost = $basecost/10 * pow($mag, 1.28) * $dur * $area;
		if ($this->get('eff_range', $index)=='Target')
			$cost *= 1.5;
		$effect->release();
		return floor($cost);
	}
	
	public function effect_value($index) {
		if (!array_key_exists('effect_id', $this->_data) || !is_array($this->_data['effect_id']) || !array_key_exists($index, $this->_data['effect_id']))
			return NULL;
		if (!is_object($effect = $this->_parentlist->get_item($this->get('effect_id', $index))))
			return NULL;
		// only difference is barterfactor instead of basecost
		$basecost = $effect->get('barterfactor');
		//$mag = max($this->get('eff_mag', $index), 1);
		$mag = $this->get('eff_mag', $index);
		//print "basecost=$basecost mag=$mag\n";
		// Based on waterbreathing, waterwalking enchantments
		// (Possible this is a min function, for any mag<5 .. but haven't found any testable cases)
		if (!$mag)
			$mag = 5; 
		//$dur = max($this->get('eff_dur', $index), 1);
		//$area = max($this->get('eff_area', $index)*0.15, 1);
		//		$cost = $basecost * pow($mag, 1.28) * $dur * $area;
		//if ($this->get('eff_range', $index)=='Target')
		//	$cost *= 1.5;
		// I think this section only applies for constant-effect enchantments, so duration/area shouldn't be relevant
		// In any case, equation only tested on mag-only cases
		$cost = $basecost * $mag;
		$effect->release();
		return floor($cost);
	}
	
	// Get total value of an item (base value + enchantment value)
	function derive_value() {
		// This is a short-circuit for sake of items where value read from data file is the correct value
		$value = $this->_getexact('value');
		if (!empty($value))
			return $value;
		
		$base_value = $this->get('value_raw');
		$enchants = $this->get('enchant_id');
		if (empty($enchants))
			return $base_value;
		
		if (!is_array($enchants))
			$enchants = array($enchants);
		// Assume that anything with an enchant_charge is a weapon and uses weapon value equation
		$enchant_charge = $this->get('enchantcharge');
		$enchant_value = 0;
		foreach ($enchants as $eid) {
			$titem = $this->_parentlist->get_t_item($eid);
			$type = $titem->get('type');
			// don't know how to do this type-selection for Skyrim....
			if ($type=='Scroll') {
				// don't know where 0.5 comes from, but it's clearly needed
				$enchant_value += $titem->get('enchantcost')*0.5;
			}
			elseif ($type=='Apparel') {
				$enchant_value += $titem->get('enchantvalue');
			}
			else {
				$enchant_value += $titem->get('enchantcost')*0.4;
			}
		}
		// +0.5 appears necessary in Skyrim -- but what about previous games??
		// or is it just that Skyrim always adds 1 ??
		return $base_value + floor(0.4*$enchant_charge+0.5) + floor($enchant_value+0.5);
	}
	
	public function get_index_levlists() {
		if (!isset($this->_data['levlists'])) {
			$this->_data['levlists'] = array();
			
			$query = "SELECT ordid FROM LVLI_LVLO_Record WHERE item_id=".$this->get('ordid');
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$this->_data['levlists'][] = $row['ordid'];
			}
			$query = "SELECT ordid FROM LVLN_LVLO_Record WHERE item_id=".$this->get('ordid');
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$this->_data['levlists'][] = $row['ordid'];
			}
		}
		if (empty($this->_data['levlists']))
			return FALSE;
		$this->_data['levlists'] = array_unique($this->_data['levlists']);
		return count($this->_data['levlists'])-1;
	}
	
	public function do_query($query, $option=NULL) {
		return $this->_parentlist->do_query($query, $option);
	}
	public function row_query($resource=NULL, $querytype=MYSQL_ASSOC) {
		return $this->_parentlist->row_query($resource, $querytype);
	}
		
	// placeholder to be overwritten by game-specific versions of derived classes
	public function preferred_locations() {
		return array();
	}
	
	public function formatted_location_list($maxlocs=5, $add_conts=TRUE) {
		return $this->_do_formatted_location_list(array($this->_ordid), $maxlocs, $add_conts);
	}
	
	public function formatted_location_list_multi($ordids, $maxlocs=5, $add_conts=TRUE) {
		if (!is_array($ordids))
			$ordids = array($ordids);
		if (!in_array($this->_ordid, $ordids))
			$ordids[] = $this->_ordid;
		$ordids = array_unique($ordids);
		return $this->_do_formatted_location_list($ordids, $maxlocs, $add_conts);
	}
	
	public function unformatted_location_list($add_conts=TRUE) {
		return $this->_do_unformatted_location_list(array($this->_ordid), $add_conts);
	}
	
	public function unformatted_location_list_multi($ordids, $add_conts=TRUE) {
		if (!is_array($ordids))
			$ordids = array($ordids);
		if (!in_array($this->_ordid, $ordids))
			$ordids[] = $this->_ordid;
		$ordids = array_unique($ordids);
		return $this->_do_unformatted_location_list($ordids, $add_conts);
	}
	
	protected function _get_pcell($row) {
		//$pid = 'parentcell_id';
		$pid = 'location_id';
		$pcell = NULL;
		if ($row['exterior']) {
			$cell = $this->_parentlist->get_item($row['cell_id']);
			if ($cell->get('world_id')) {
				$world = $this->_parentlist->get_name($cell->get('world_id'));
				if ($world!='Skyrim' && $world!='Tamriel') {
					$pcell = $cell->get('world_id');
				}
			}
			if (empty($pcell))
				$pcell = $cell->find_nearest_marker($row['ordid']);
			$cell->release();
			if (!empty($pcell))
				return $pcell;
		}
		if (!empty($row[$pid]))
			return $row[$pid];
		return $row['cell_id'];
	}

	protected function _do_unformatted_location_list($ordids, $add_conts=TRUE) {
		if (count($ordids)==1)
			$ordidlist = '='.$ordids[0];
		else
			$ordidlist = ' IN ('.implode(', ',$ordids).')';
		$locs = array();
		$refr = $this->refr_rectype();
		//$pid = 'parentcell_id';
		$pid = 'location_id';
		$cquery = "SELECT cell_id, {$refr}_Record.ordid, CELL_Record.$pid, CELL_Record.exterior from {$refr}_Record inner join AllItems on ({$refr}_Record.ordid=AllItems.ordid) inner join CELL_Record on ({$refr}_Record.cell_id=CELL_Record.ordid) where disabled=0 and base_id".$ordidlist;
		$cres = $this->do_query($cquery);
		while ($crow=$this->row_query($cres)) {
			$pcell = $this->_get_pcell($crow);
			$locs[$pcell][$crow['cell_id']][$crow['ordid']] = 1;
		}
		$squery = "SELECT AllItems.ordid, AllItems.rectype, countmin from AllItems inner join LevelList on (AllItems.ordid=LevelList.list_ordid) WHERE item_ordid".$ordidlist." and pav>=1 and AllItems.edid not like 'QA%' and AllItems.edid not like 'E3%' and AllItems.edid not like 'DEMO%' and rectype!='LVLI'";
		$sres = $this->do_query($squery);
		while ($srow=$this->row_query($sres)) {
			$refr = $this->_parentlist->get_t_item($srow['ordid'])->refr_rectype();
			$cquery = "SELECT cell_id, {$refr}_Record.ordid, CELL_Record.$pid, CELL_Record.exterior from {$refr}_Record inner join AllItems on ({$refr}_Record.ordid=AllItems.ordid) inner join CELL_Record on ({$refr}_Record.cell_id=CELL_Record.ordid) where disabled=0 and base_id=".$srow['ordid'];
			$cres = $this->do_query($cquery);
			while ($crow=$this->row_query($cres)) {
				// needs to exclude merchant chests ... track them separately
				if ($srow['rectype']=='CONT') {
					$tquery = 'SELECT count(*) as num FROM FACT_Record where merccont_id='.$crow['ordid'];
					$trow = $this->do_query($tquery, 'onerow');
					if (!$add_conts || !empty($trow['num'])) {
						continue;
					}
				}
				$pcell = $this->_get_pcell($crow);
				$locs[$pcell][$crow['cell_id']][$crow['ordid']] = $srow['countmin'];
			}
		}
		$locnames = array();
		
		if ($add_conts) {
			$squery = "SELECT * from QUST_CNTO_Record where item_id".$ordidlist;
			$sres = $this->do_query($squery);
			while ($srow=$this->row_query($sres)) {
				$index = $srow['subid'];
				$qid = $srow['ordid'];
				$quest = $this->_parentlist->get_item($qid);
			
				$alindex = preg_replace('/\-CNTO.*/', '', $index);
				$qtext = '';
				if ($quest->get('ALCO', $alindex)) {
					$qtext = '[['.$this->_parentlist->get_name($quest->get('ALCO', $alindex)).']]';
				}
				elseif ($quest->get('ALDN', $alindex)) {
					$qtext = '[['.$this->_parentlist->get_name($quest->get('ALDN', $alindex)).']]';
				}
				else {
					$qtext = '[['.$quest->get('ALID', $alindex).']]';
				}
				if (!empty($qtext)) {
					$qname = $quest->get_questname();
					if (empty($qname) || $qtext=='[[]]') {
						print "problem with quest ".$quest->get('name')." ".$quest->get('ordid')."\n";
						print "qtext = $qtext\n";
						print "qname = $qname index=$index alindex=$alindex\n";
						exit;
					}
					$locnames[] = '* '.$qtext .= ' (only as part of quest [['.$qname.']])'."\n";
				}
			
				$quest->release();
			}
		}
		return array($locs, $locnames);
	}
	
	protected function _do_formatted_location_list($ordids, $maxlocs=5, $add_conts=TRUE) {
		$loctext = '';
		list($locs, $locnames) = $this->_do_unformatted_location_list($ordids, $add_conts);
		
		$ncopy = 0;
		$loccopies = array();
		foreach ($locs as $pcell => $cells) {
			$loccopies[$pcell] = 0;
			foreach ($cells as $cell => $items) {
				foreach ($items as $item => $count) {
					$loccopies[$pcell] += $count;
					$ncopy += $count;
				}
			}
		}
		if (count($locs)>$maxlocs) {
			$mainlocs = array();
			foreach ($this->preferred_locations() as $edid) {
				$o = $this->_parentlist->get_ordid($edid);
				if (empty($o)) {
					print "Unable to recognize location $edid\n";
				}
				if (isset($locs[$o])) {
					$mainlocs[$o] = $locs[$o];
				}
				else {
					foreach ($locs as $pcell => $cells) {
						if (isset($cells[$o])) {
							$mainlocs[$pcell] = $locs[$pcell];
							break;
						}
					}
				}
			}
			if (empty($mainlocs))
				$mainlocs = $locs;
		}
		else {
			$mainlocs = $locs;
		}

		if (!empty($mainlocs)) {
			$loctext = "$ncopy guaranteed samples found in ";
			if (count($locs)==1) {
				$loctext .= "the following location:\n";
			}
			else {
				$loctext .= count($locs).' different locations';
				if (count($mainlocs)<count($locs))
					$loctext .= ".  Notable locations are:\n";
				elseif (count($locs)<=$maxlocs)
					$loctext .= ":\n";
				else
					$loctext .= ".  Locations with the greatest numbers are:\n";
			}
		}
			
		arsort($loccopies);
		$multi = FALSE;
		$lastcount = NULL;
		$maxcount = 1;
		foreach ($loccopies as $pcell => $count) {
			if (!isset($mainlocs[$pcell]))
				continue;
			
			// do check at start of loop -- stop before adding a new item with a smaller count
			if (count($locnames)>=$maxlocs) {
				if ($count<$lastcount)
					break;
				elseif (count($locnames)>=$maxlocs*3)
					break;
			}
			$maxcount = max($count, $maxcount);
			
			$item_id = NULL;
			// exterior items grouped around a map marker
			if ($this->_parentlist->get_rectype($pcell)=='REFR') {
				// decide whether to just say 'around marker' or provide directions relative to marker
				$marker = $this->_parentlist->get_item($pcell);
				$xav = $yav = $num = 0;
				$xmin = $xmax = $ymin = $ymax = NULL;
				$deltav = 0;
				foreach ($mainlocs[$pcell] as $cell => $items) {
					foreach ($items as $item_id => $c) {
						$item = $this->_parentlist->get_item($item_id);
						$num++;
						$x = $item->get('x');
						$y = $item->get('y');
						$xav += $x;
						$yav += $y;
						$delt = $marker->get_dist($item,$angle,'raw');
						$deltav += $delt;
						if (!isset($xmin)) {
							$xmin = $xmax = $x;
							$ymin = $ymax = $y;
						}
						if ($x<$xmin)
							$xmin = $x;
						elseif ($x>$xmax)
							$xmax = $x;
						if ($y<$ymin)
							$ymin = $y;
						elseif ($y>$ymax)
							$ymax = $y;
						$item->release();
					}
				}
				$xav /= $num;
				$yav /= $num;
				$deltav /= $num;
				
				if ($deltav<2000)
					$usemark = TRUE;
				else {
					$spread = max($xmax-$xmin, $ymax-$ymin);
					if ($spread<$deltav)
						$usemark = FALSE;
					else
						$usemark = TRUE;
				}
				
				if ($usemark) {
					$item_id = $pcell;
					$mloc = array_shift(array_keys($mainlocs[$pcell]));
				}
				else {
					$distmin = NULL;
					$itemmin = NULL;
					$mloc = NULL;
					foreach ($mainlocs[$pcell] as $cell => $items) {
						foreach ($items as $item_id => $c) {
							$item = $this->_parentlist->get_item($item_id);
							$dist = pow($item->get('x')-$xav, 2) + pow($item->get('y')-$yav, 2);
							if (!isset($distmin) || $dist<$distmin) {
								$distmin = $dist;
								$mloc = $cell;
								$itemmin = $item_id;
							}
						}
					}
					$item_id = $itemmin;
				}
				$marker->release();
			}
			else {
			// really need to always generate text based on pcell... at least whenever there are multiple cells
			// being done together
				if (count($mainlocs[$pcell])==1)
					$mloc = array_shift(array_keys($mainlocs[$pcell]));
				elseif (isset($mainlocs[$pcell][$pcell]))
					$mloc = $pcell;
				else {
					$nmax = 0;
					foreach ($mainlocs[$pcell] as $cell => $items) {
						if ($this->_parentlist->get_name($cell)==$this->_parentlist->get_name($pcell)) {
							$mloc = $cell;
							break;
						}
						elseif (count($items)>$nmax) {
							$nmax = count($items);
							$mloc = $cell;
						}
					}
				}
			
				$item_id = array_shift(array_keys($mainlocs[$pcell][$mloc]));
			}
			
			$item = $this->_parentlist->get_item($item_id);
			// should add option to skip non-exact locations?  Or not necessary now that extra processing being done to find those?
			$tloc = $item->formatted_location();
			/*			print "\ntloc=$tloc  pcell=$pcell item_id=$item_id\n";
			var_dump($mainlocs[$pcell]);*/
			if (!empty($tloc)) {
				if (count($mainlocs[$pcell][$mloc])==1 && !in_array($item->get('base_id'),$ordids)) {
					if ($item->get('base_name')) {
						if ($item->get('base_rectype')=='CONT')
							$tloc .= ' in '.strtolower($item->get('base_name'));
						else
							$tloc .= ' on [['.$item->get('base_name').']]';
					}
				}
				if ($count>1 || $maxcount>1) {
					$multi = TRUE;
					if ($item_id==$pcell || $this->_parentlist->get_rectype($pcell)=='REFR')
						$tloc = $count.' around '.$tloc;
					elseif ($this->_parentlist->get_rectype($pcell)=='WRLD')
						$tloc = $count.' inside '.$tloc;
					else
						$tloc = $count.' in '.$tloc;
				}
			//testing!!
				//	$tloc .= " (pcell=$pcell, mloc=$mloc, item_id=$item_id, allitems=".implode('|',array_keys($mainlocs[$pcell][$mloc])).")\n";
				$locnames[] = '* '.$tloc."\n";
			}
			$item->release();
			
			$lastcount = $count;
		}
		if (!$multi)
			sort($locnames);
		foreach ($locnames as $loc) {
			if (preg_match('/^(\*\s+)(.*)(,\s+)(\d+\s+ft\s+[A-Z]+\s+of\s+)(marker)(.*)/s', $loc, $matches)) {
				if (preg_match('/^(\d+)\s+(?:around|in)\s+(.*)/', $matches[2], $smatch)) {
					if ($smatch[1]==1)
						$loc = $matches[1].$smatch[1].' at '.$matches[4].$smatch[2].$matches[6];
					else
						$loc = $matches[1].$smatch[1].' centered '.$matches[4].$smatch[2].$matches[6];
				}
				elseif (preg_match('/^(\d+)\s+(?:inside)\s+(.*)/', $matches[2], $smatch)) {
					$loc = $matches[1].$smatch[1].' in '.$smatch[2].$matches[6];
				}
				else {
					$loc = $matches[1].$matches[4].$matches[2].$matches[6];
				}
			}
			$loctext .= $loc;
		}
		return $loctext;
	}
	
	// rectypes that do *not* trigger disambig
	public function get_disambig_types() {
		return array($this->get('rectype'), $this->refr_rectype(), 'ACTI', 'TACT', 'MESG', 'DIAL');
	}
	
	// will need to be customized by nearly every derived class
	public function get_disambig_type() {
		return 'item';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		// this function assumes that name matches within the same rectype are OK
		// cases where within-rectype disambigs are needed have to be handled on an individual basis by
		// derived classes
		$dquery = "SELECT * from AllItems where rectype not in ('".implode("', '", $this->get_disambig_types())."') and name='".addslashes($this->get('name'))."'";
		$dres = $this->do_query($dquery);
		if (mysql_num_rows($dres)!=0) {
			if ($showtrigger) {
				while ($drow=$this->row_query($dres)) {
					fwrite (STDERR, "disambig triggered for rectype=".$drow['rectype']." edid=".$drow['edid']." 	name=".$drow['name']."\n");
				}
			}
			return " (".$this->get_disambig_type().")";
		}
		return '';
	}
	
	public function wiki_pagename($nospace=FALSE) {
		$name = $this->get('name');
		if (empty($name)) {
			$name = $this->get('edid');
		}
		else {
			$name .= $this->get_disambig_suffix();
		}
		if ($nospace)
			$name = preg_replace('/ /', '_', $name);
		return $name;
	}

	// Generic function to be used by functions such as get_index_member
	protected function _get_index_using_query($varname, $query) {
		if (!isset($this->_data[$varname])) {
			$this->_data[$varname] = array();
			
			if (!is_array($query))
				$query = array($query);
			foreach ($query as $q) {
				$res = $this->_parentlist->do_query($q);
				while ($row=$this->_parentlist->row_query($res)) {
					$this->_data[$varname][] = $row['ordid'];
				}
			}
		}
		if (empty($this->_data[$varname]))
			return FALSE;
		return count($this->_data[$varname])-1;
		
	}

	// Function intended specifically to convert any unprocessed formids found within a record into ordids
	// For example, to convert/extract CTDA parameters.
	// It takes care of changing the formid prefix as appropriate
	// Eventually probably also want it to take care of finding mod-specific ordid if appropriate
	public function convert_formid($rawid) {
		if (empty($rawid))
			return NULL;
		$formid = sprintf('0x%08lx', $rawid);

		// Initial rough way of doing prefix conversion
		// Eventually need to store orig_prefix => my_prefix conversions for each individual fileid
		// (which needs to be based upon reading all of the dependencies and looking each of those
		// up in my file table)
		// quick and dirty fix, 6 is the ID for HF, needs to be don properly
		if (substr($formid,2,2)>1 || (substr($formid,2,2)==1 && $this->fileid()==6)) {
			$prefix = sprintf('%02lx', $this->fileid());
			$formid = '0x'.$prefix.substr($formid,4);
		}
		$ordid = $this->_parentlist->get_ordid($formid);
		return $ordid;
	}
}
// at start: check whether any DB data exists
// flag that instance as the one instance to do entire scan
// subsequent reads 
// first pass: need to just get a list of all items and srcs
// this should probably even ignore lvls completely
// but how do I track whether whole tree has been scanned yet?
// then calc plvl for each individual item/src pairs
			
// switch data to have
// (a) a list of items directly contained in this list, with plvl range info
// (b) full details of individual subitems... but dynamically load each subitem as needed,
//     and discard as soon as done with that subitem
class ItemList {
	protected $_subitem_ids;
	protected $_localdata;
	protected $_item, $_parentlist;
	protected $_hasitems;
	protected $_hassublists;
	protected $_lvlmin, $_lvllast;
	protected $_iord;
	
	protected $_init_active;
	// may want to set this dynamically based on game
	// 40 for OB, 60 for SR/BM
	private $_lmaxcap = 60;
	
	// need to be able to call this for a single list/item pair -- and beyond that for
	// just a list/item/src combo
	// but then also want the ability to call in traditional fashion
			
	function __construct($item, $parentlist) {
		// this is a copy of the item reference.  But I don't want add a nuse count, because this object wouldn't be destroyed (and add to nrel) until
		// item's nuse=nrel
		$this->_item = $item;
		$this->_iord = $this->_item->get('ordid');
		$this->_parentlist = $parentlist;
		$datamode = $this->_parentlist->get_datamode();
		
		// explicitly state that the item is being used here (not done automatically, since creating a copy, rather than calling get_item)
		// BUT don't want this
		// it means that item can never be released (becuase this use isn't lifted until list is forcibly destroyed)
		//$this->_parentlist->use_data($item->ordid());
		$this->_localdata = array();
		$this->_subitem_ids = array();
		$this->_hassublists = FALSE;
		$this->_hasitems = NULL;
		
		// disable ItemList completely if operating without DB
		if ($datamode==CSData::DATAMODE_READ)
			return;
		$this->_init_active = true;
		$this->init();
		$this->_init_active = false;
	}
	
	function __destruct() {
		$this->clean();
	}
	// should only be called when list is about to destroyed: called explicitly to make sure back-references to parents
	// are blanked, allowing memory to be recycled
	function clean() {
		if (!is_null($this->_localdata) && !$this->_init_active) {
			$this->clean_data();
			unset($this->_item);
			$this->_item = NULL;
			unset($this->_parentlist);
			$this->_parentlist = NULL;
		}
	}
	
	function clean_data() {
		unset($this->_subitem_ids);
		// not sure whether I need to go through ldata and release any sub-arrays (plvl is only one?)
		foreach ($this->_localdata as $ordid => $ldata) {
			unset ($this->_localdata[$ordid]);
		}
		unset($this->_localdata);
		$this->_localdata = NULL;
	}
	
	// In DATAMODE_DB mode, primary purpose of this function is to just establish whether _hasitems is true or false
	// In DATAMODE_WRITE mode, this is the function that ends up creating DB entries if they do not yet exist
			
	// in DATAMODE_DB -> no rows means no entries
	// BUT in DATAMODE_WRITE -> need to have a flag in DB to know whether item is simply uninitialized or
	//   instead non-existent
	function init() {
		$datamode = $this->_parentlist->get_datamode();
		$query = "SELECT COUNT(*) AS nrow FROM LevelList WHERE list_ordid=".$this->_item->ordid();
		$row = $this->_parentlist->do_query($query, 'onerow');
		if ($datamode==CSData::DATAMODE_DB) {
			if (!empty($row['nrow']))
				$this->_hasitems = TRUE;
			else
				$this->_hasitems = FALSE;
			return;
		}
		else {
			if (empty($row['nrow'])) {
				// Note that this section should only ever be reached as a result of calling calc_levlists
				$this->_hasitems = NULL;
			}
			else {
				$query = "SELECT COUNT(*) AS nrow FROM LevelList WHERE item_ordid>0 && list_ordid=".$this->_item->ordid();
				$row = $this->_parentlist->do_query($query, 'onerow');
				if (!empty($row['nrow']))
					$this->_hasitems = TRUE;
				else
					$this->_hasitems = FALSE;
				return;
			}
		}
		
		$this->_localdata = array();
		$this->_subitem_ids = array();
			
		$this->list_subitem_ids();
		if (!count($this->_subitem_ids)) {
			$this->_hasitems = FALSE;
			$this->_write_db(NULL, array());
			return;
		}
		
		$this->_init_localdata();
			
		// foreach DB entry, calc pav data
		foreach ($this->_subitem_ids as $lordid => $x) {
			$plvl = $this->get_plvl($lordid);
			if (empty($plvl)) {
				continue;
			}
			$this->_write_db($lordid, $plvl);
			unset($plvl);
		}
		
		$this->_hasitems = TRUE;
		return;
	}
	
	function contains_list() {
		return $this->_hasitems;
	}
	
	// <s>This is only called if nothing has been set up in DB yet, and want to start setup process</s>
	// Needs to be called every time in initialize mode
	// ... because until this is called, don't know whether entries have been filled in yet
	// If recursively calling itself, then use scanonly=TRUE
	
	// just scan, ignoring all level-type info
	// need to get this to work as a direct call whether or not contains_list has been called
	function list_subitem_ids() {
		if ($this->_hasitems===FALSE) {
			return array();
		}
		elseif (!empty($this->_subitem_ids)) {
			return array_keys($this->_subitem_ids);
		}
		
		if ($this->_hasitems===TRUE) {
			unset($this->_subitem_ids);
			$this->_subitem_ids = array();
			$query = "SELECT item_ordid FROM LevelList WHERE list_ordid=".$this->_item->get('ordid');
			$res = $this->_parentlist->do_query($query);
			$this->_subitem_ids = array();
			while ($row=$this->_parentlist->row_query($res)) {
				$this->_subitem_ids[$row['item_ordid']] = TRUE;
			}
		}
		else {
			$this->_init_subitem_ids();
		}
		return array_keys($this->_subitem_ids);
	}
	
	// Note that this function may return false positives -- may include items that actually
	// end up being excluded based on non-overlapping level ranges
	// They'll be caught later when pavs are calc'ed
	protected function _init_subitem_ids() {
		// only get here if _hasitems=NULL, which implies we're in setup mode
		// don't bother to try to identify local items -- that should be done when local data is
		// fully processed
		foreach (array('item', 'spell') as $type) {
			$list_id = $type."_id";
			if ($this->_item->get_index($list_id)===FALSE)
				continue;
		
			// lid values may start as mix of edids and ordids (in DATAMODE_READ); transform all into ordids
			foreach ($this->_item->indices($list_id) as $i) {
				$lid = $this->_item->get($list_id, $i);
				$lordid = $this->_parentlist->get_ordid($lid);
				// prevent recursion
				if (is_null($lordid) || $lordid==$this->_item->ordid())
					continue;
				$this->_subitem_ids[$lordid] = TRUE;
			}
		}
		
		// do as separate loop to avoid calling repeated entries multiple times
		// do even if DB data exists to ID local sublists?
		foreach ($this->_subitem_ids as $lordid => $x) {
			$item_rec = $this->_parentlist->get_rectype($lordid);
			if ($this->_parentlist->is_levlist_rectype($item_rec)) {
				$litem = $this->_parentlist->get_item($lordid);
				if (is_null($litem))
					continue;
				$sublist = $litem->list_subitem_ids();
				if (!empty($sublist)) {
					$this->_hassublists = true;
					foreach ($sublist as $lnew) {
						$this->_subitem_ids[$lnew] = TRUE;
					}
				}
				$litem->release();
			}
		}
		
		return array_keys($this->_subitem_ids);
	}
	
	// NOTE: chancenone is no longer integrated as part of this step!!
	protected function _init_localdata($type='item') {
		if (is_null($this->_localdata))
			$this->_localdata = array();
		// don't try to process spell list if it doesn't exist
		$list_id = $type."_id";
		if ($this->_item->get_index($list_id)===FALSE)
			return;
		$list_count = $type."_count";
		if ($this->_item->get_index($list_count)===FALSE)
			$list_count=FALSE;
		$list_lvl = $type."_lvl";
		if ($this->_item->get_index($list_lvl)===FALSE)
			$list_lvl=FALSE;
		$dlvl = $this->_parentlist->get_levellist_dlvl();
		
		// alllvl is whether to use all items <= PC's level
		// (if false, only use items at first level below PC's level, but not lower-level items)
		// leveled items/rewards should have alllvl=FALSE, generic loot lists often have alllvl=TRUE
		if (is_null($alllvl = $this->_item->get('alllvl')))
			$alllvl = true;
		// each is whether or not this list's item gets recalc'ed each time it gets called
		// so if listA has 10 instances of listB:
		// each=FALSE means listA ends up with 10 identical items
		// each=TRUE means listA ends up with 10 probably-different items
		// set false for non-real lists
		if (is_null($each = $this->_item->get('each')))
			$each = false;
		// useall is whether or not to include every (level-appropriate) item in the list
		// for most lists, useall=FALSE
		// useall=TRUE for cases such as a list of vendor's guaranteed merchandise
		if (is_null($useall = $this->_item->get('useall')))
			$useall = true;
		
		$entries = array();
		$this->_lvlmin = $this->_lvllast = NULL;
		
// Entries may have non-numeric index!!		
		foreach ($this->_item->indices($list_id) as $i) {
			// don't want to convert to lordid yet -- might be null if something strange is happening, but
			// even so need to keep the info so that levels/weighting done correctly
			$entries[$i]['lid'] = $this->_item->get($list_id, $i);
			if ($list_lvl)
				$entries[$i]['lvl'] = $this->_item->get($list_lvl, $i);
			else
				$entries[$i]['lvl'] = 1;
			if ($list_count)
				$entries[$i]['count'] = $this->_item->get($list_count, $i);
			else
				$entries[$i]['count'] = 1;
			$entries[$i]['lvlmin'] = $entries[$i]['lvl'];
			set_min($this->_lvlmin, $entries[$i]['lvl']);
			set_max($this->_lvllast, $entries[$i]['lvl']);
			if (!$alllvl || !is_null($dlvl)) {
				if (!$alllvl)
					$clvl = $entries[$i]['lvl'];
				else
					$clvl = $entries[$i]['lvl']-$dlvl;
				foreach ($entries as $it => $entry) {
					if ($it===$i)
						continue;
					if (!array_key_exists('lvlmax', $entries[$it]) && $entries[$it]['lvl']<$clvl) {
						$entries[$it]['lvlmax'] = $entries[$i]['lvl']-1;
					}
				}
			}
		}

		for ($l=$this->_lvlmin; $l<=$this->_lvllast; $l++) {
			$nlvl = 0;
			foreach ($entries as $i => $entry) {
				if ($entries[$i]['lvlmin']<=$l && (!array_key_exists('lvlmax', $entries[$i]) || $entries[$i]['lvlmax']>=$l))
					$nlvl++;
			}
			foreach ($entries as $i => $entry) {
				if ($entries[$i]['lvlmin']<=$l && (!array_key_exists('lvlmax', $entries[$i]) || $entries[$i]['lvlmax']>=$l)) {
					if ($useall)
						$entries[$i]['plvl'][$l] = 1;
					else
						$entries[$i]['plvl'][$l] = 1/$nlvl;
				}
			}
		}
		
		$newitems = array();
		// lid values may start as mix of edids and ordids (in DATAMODE_READ); transform all into ordids
		foreach ($entries as $i => $entry) {
			$lid = $entries[$i]['lid'];
			$lordid = $this->_parentlist->get_ordid($lid);
			// prevent recursion
			if (is_null($lordid) || $lordid==$this->_item->ordid())
				continue;
			
			$newitems[$lordid] = TRUE;
			// For now, just want to flag that this entry has subentries -- don't want to process them in any way
			$item_rec = $this->_parentlist->get_rectype($lordid);
			if ($this->_parentlist->is_levlist_rectype($item_rec)) {
				$litem = $this->_parentlist->get_item($lordid);
				if (is_null($litem))
					continue;
				if (($sublist=$litem->contains_list())) {
					$this->_hassublists = true;
					$this->_localdata[$lordid]['sub'] = TRUE;
				}
				$litem->release();
			}
			
			if (empty($this->_localdata[$lordid]['plvl']))
				$this->_localdata[$lordid]['plvl'] = array();
			$count = $entries[$i]['count'];
			foreach ($entries[$i]['plvl'] as $l => $p) {
				if (!$useall) {
					@ $this->_localdata[$lordid]['plvl'][$l][$count] += $p;
				}
				else {
					if (!isset($this->_localdata[$lordid]['plvl'][$l]))
						@ $this->_localdata[$lordid]['plvl'][$l][$count] += $p;
					else
						$this->_integrate_count($this->_localdata[$lordid]['plvl'][$l], array($count => $p));
					// for useall entries, need to keep track of min count of each item, so plvl knows whether
					// to integrate data or do simple add
					$this->_localdata[$lordid]['plvl_counts'][$l][] = $count;
				}
			}
		}
		
		// This just calcs auxiliary stats such as countmin, countmax, etc.
		// Mainly called to make sure lmin, llast, etc known
		foreach ($newitems as $lordid => $x) {
			$this->_calc_stats($this->_localdata[$lordid], $this->_lvllast);
		}
	}
	
	protected function _write_db($itemid, $plvl) {
		if (!isset($itemid)) {
			// Signal to write a temporary empty record to signal that record has been checked, but doesn't contain anything
			$query = "INSERT INTO LevelList SET list_ordid=".$this->_item->ordid().", item_ordid=0";
			$this->_parentlist->do_query($query);
			return;
		}
		
		$data = array();
		$data['plvl'] = $plvl;
		$this->_calc_stats($data);
		if (is_null($data))
			return;
		
		$iord = $this->_item->ordid();
		if (empty($iord)) {
			print "in write_db\n";
			print "iord=$iord, this=".$this->_iord." itemid=$itemid\n";
			foreach (debug_backtrace() as $t => $tdata) {
				print "t=$t, function=".$tdata['function']."\n";
				foreach ($tdata['args'] as $a => $arg) {
					print "    arg a=$a ";
					if (!is_array($arg) && !is_object($arg))
						print "arg=$arg";
					print "\n";
				}
			}
			//var_dump(debug_backtrace());
		}
		
		$query = "INSERT INTO LevelList SET list_ordid=".$this->_item->ordid().", item_ordid={$itemid}";
		foreach (array('lmin', 'lmax', 'llast') as $valname) {
			if (!array_key_exists($valname, $data['plvl']) || is_null($data['plvl'][$valname]))
				continue;
			$query .= ", {$valname}=".$data['plvl'][$valname];
		}
		foreach (array('pav', 'pav_lvl', 'pmin', 'pmax', 'countav', 'countmin', 'countmax') as $valname) {
			$query .= ", {$valname}=";
			if (array_key_exists($valname, $data) && !is_null($data[$valname]))
				$query .= $data[$valname];
			else
				$query .= "NULL";
		}
		$this->_parentlist->do_query($query);
	}
	
	// alllast only used when being called from original initialization, at which point lmin, lmax, llast have not been set
	protected function _calc_stats(&$data, $alllast=NULL) {
		$pavtot = $nav = 0;
		$countav = $pavltot = $navl = 0;
		$pmin = $pmax = NULL;
		$countmin = $countmax = NULL;
		$lvlmin = $lvllast = NULL;
		$plast = NULL;
		
		// just in case _calc_stats for an item gets called while scanning spells: don't want to reset llast based on a false value of alllast
		if (!is_null($alllast) && array_key_exists('llast', $data['plvl']))
			$alllast = NULL;
		$lmax = $this->_lmaxcap;
		if (!is_null($alllast) && $alllast>$lmax)
			$lmax = $alllast;
		if (array_key_exists('llast', $data['plvl']) && $data['plvl']['llast']>$lmax)
			$lmax = $data['plvl']['llast'];
		
		// originally limited lmax to 40
		// but that doesn't work for Skyrim where lists go to 60
		// also caused problems for a few Bloodmoon leveled lists that extend to 50 or 60
		// BM nte:don't want to have to scan past 40 for every single list and don't want pav to extend so ridiculously high
		// on the other hand, want "hasitem" check to still work (i.e., don't want pav=0 for an item that only appears at level 60)
		// * extend scan when required by llast
		// * but weight all levels past 40 by 1/100 
		for ($l=1; $l<=$lmax; $l++) {
			if ($l<=$this->_lmaxcap)
				$wgt = 0.01;
			else
				$wgt = 1;
			$nav += $wgt;
			// array key could be missing because
			// below lmin -> do not count row
			//  * caught by is_null($lvllast)
			// missing data between lmin and llast -> do not count row
			//  * if on first call (llast missing), lvllast<alllast (alllast should be the highest possible l-value)
			//  * if llast set, l<llast
			// above lmax -> do not count row
			//  * if on first call ??
			//  * if lmax set, any value of l (if lmax set, data should not be extended past provided limits)
			// above llast but lmax not set -> count row
			if (!array_key_exists($l, $data['plvl'])) {
				if (is_null($lvllast) || (!is_null($alllast) && $lvllast < $alllast) || (array_key_exists('llast', $data['plvl']) && $l<$data['plvl']['llast']) || (array_key_exists('lmax', $data['plvl']))) {
					continue;
				}
				else {
					$pavtot += $plast*$wgt;
				}
			}
			elseif (!count($data['plvl'][$l]))
				continue;
			else {
				$navl += $wgt;
				set_min($lvlmin, $l);
				set_max($lvllast, $l);
				$plast = 0;
				$cav = 0;
				foreach ($data['plvl'][$l] as $c => $p) {
					$plast += $p;
					// averaging and stats use absolute value of count (ignore respawn-ness for now)
					// this is where each needs to be taken into account!!
					$cav += abs($c)*$p;
					set_min($countmin, abs($c));
					set_max($countmax, abs($c));
				}
				$countav += ($cav/$plast)*$wgt;
				$pavtot += $plast*$wgt;
				$pavltot += $plast*$wgt;
				set_min($pmin, $plast);
				set_max($pmax, $plast);
			}
		}
		if (!$navl) {
			print "no navl\n";
			$data = NULL;
			return;
		}
		if ($pavtot/$nav>0.99)
			$data['pav'] = 1;
		else
			$data['pav'] = $pavtot/$nav;
		if ($pavltot/$navl>0.99)
			$data['pav_lvl'] = 1;
		else
			$data['pav_lvl'] = $pavltot/$navl;
		$data['pmin'] = $pmin;
		//		if ($data['pmin']>0.99)
		//			$data['pmin']=1;
		$data['pmax'] = $pmax;
		//		if ($data['pmax']>0.99)
		//			$data['pmax']=1;
		
		$data['countav'] = $countav/$navl;
		$data['countmax'] = $countmax;
		$data['countmin'] = $countmin;
		
		$data['plvl']['lmin'] = $lvlmin;
		$data['plvl']['llast'] = $lvllast;
		if (!is_null($alllast) && $lvllast<$alllast)
			$data['plvl']['lmax'] = $lvllast;
		
		for ($l=$lvlmin; $l<$lvllast; $l++) {
			if (!array_key_exists($l, $data['plvl']))
				$data['plvl'][$l] = array();
		}
	}
	
	function hasitem($ordid) {
		if (!$this->_hasitems)
			return NULL;
		return $this->get($ordid, 'pav');
	}
	
	function get($ordid, $valname) {
		$query = "SELECT * FROM LevelList WHERE list_ordid=".$this->_item->ordid()." AND item_ordid={$ordid}";
		$row = $this->_parentlist->do_query($query, 'onerow');
		if (is_array($row) && array_key_exists($valname, $row))
			return $row[$valname];
		else
			return NULL;
	}
	
	// Simplified case of _integrate_plvl -- can only be used when p and p2 are both 1
	protected function _integrate_count(&$plvl, $newplvl) {
		$origcounts = $plvl;
		$plvl = array();
		foreach ($newplvl as $c => $p) {
			foreach ($origcounts as $c2 => $p2) {
				@ $plvl[$c2+$c] += $p*$p2;
			}
		}
	}
	
	// For useall and each
	// combine two instances where the chances need to be multiplied together
	protected function _integrate_plvl(&$plvl, $newplvl, $pmult=1, $cmult=1) {
		$origcounts = $plvl;
		
		$czero = 1;
		foreach ($origcounts as $count => $p) {
			$czero -= $p;
		}
		$origcounts[0] = max(0., $czero);
		
		$czero = 1;
		$modplvl = array();
		foreach ($newplvl as $newcount => $newchance) {
			$modplvl[$newcount*$cmult] = $newchance*$pmult;
			$czero -= $newchance*$pmult;
		}
		$modplvl[0] = max(0., $czero);
		
		$plvl = array();
		$pmax = 0;
		foreach ($modplvl as $newcount => $newchance) {
			foreach ($origcounts as $count => $p) {
				if (!($count+$newcount))
					continue;
				@ $plvl[$count + $newcount] += $p*$newchance;
				$pmax = max($pmax, $plvl[$count + $newcount]);
			}
		}
		
		// there are crazy cases such as 50 counts of LootGoldChange50
		// ... integrate 50 separate calcs of (50% chance 1-9 gold)
		// tracking to infinitely small probabilities is impractical
		// but don't want to completely truncate an item with a small chance...
		// (note that this ends up effectively setting the range over which countmin/countmax are valid)
		// Also don't want total non-zero range to change!!
		$pcut = min($pmax*1.e-3, 1.e-6);
		$psum = 0;
		$plost = 0;
		foreach ($plvl as $count => $p) {
			$psum += $p;
			if ($p<$pcut) {
				$plost += $p;
				unset($plvl[$count]);
			}
		}
		if ($plost) {
			$pfix = $psum/($psum - $plost);
			//			print "plost=$plost psum=$psum pfix=$pfix plvl_count=".count($plvl)."\n";
			foreach ($plvl as $count => $p)
				$plvl[$count] *= $pfix;
		}
		ksort($plvl);
	}
	
	protected function _combine_summed(&$output, $valcurr, $valsub) {
		$add = 0;
		if (is_null($valsub))
			$add = $valcurr;
		else
			$add = $valcurr*$valsub;
		$output += $add;
		return $add;
	}
	protected function combine_pav(&$output, $valcurr, $valsub) {
		return $this->_combine_summed($output, $valcurr, $valsub);
	}
	protected function combine_pmin(&$output, $valcurr, $valsub) {
		return $this->_combine_summed($output, $valcurr, $valsub);
	}
	protected function combine_pmax(&$output, $valcurr, $valsub) {
		return $this->_combine_summed($output, $valcurr, $valsub);
	}
	protected function combine_pav_lvl(&$output, $valcurr, $valsub) {
		return $this->_combine_summed($output, $valcurr, $valsub);
	}
	// countav... 
	protected function combine_countmin(&$output, $valcurr, $valsub) {
		if (is_null($valsub))
			set_min($output, abs($valcurr));
		else
			set_min($output, abs($valcurr*$valsub));
		return $output;
	}
	protected function combine_countmax(&$output, $valcurr, $valsub) {
		if (is_null($valsub))
			set_max($output, abs($valcurr));
		else
			set_max($output, abs($valcurr*$valsub));
		return $output;
	}
	// lvlmin no longer exists.....
	protected function combine_lvlmin(&$output, $valcurr, $valsub) {
		if (is_null($valsub))
			set_min($output, $valcurr);
		else
			// for items within the same list, take the minimum value (any of the items appearing would qualify)
			// for sublists, minimum value is the highest of the two minimums (most stringent requirement is the effective one)
			set_min($output, max($valcurr, $valsub));
		return $output;
	}
	protected function combine_lvlmax(&$output, $valcurr, $valsub) {
		// similar to lvlmin, except with complication of null values
		if (is_null($valsub) && is_null($valcurr))
			return $output;
		if (is_null($valsub))
			set_max($output, $valsub);
		elseif (is_null($valcurr))
			set_max($output, $valcurr);
		else
			set_max($output, min($valcurr, $valsub));
		return $output;
	}
	// what is this value even supposed to mean??
	// need to look at lvlmax, too.  If lvlmax defined, lvllast=lvlmax.  Otherwise, lvllast 
	protected function combine_lvllast(&$output, $valcurr, $valsub) {
		if (is_null($valsub))
			set_max($output, $valcurr);
		else
			set_max($output, min($valcurr, $valsub));
		return $output;
	}
	
	// Get full plvl data for individual ordid
	// Recursively calls sublists to generate information, then sum data from all sublists
	function get_plvl($ordid) {
		if ($this->_hasitems===FALSE)
			return array();
		if (empty($this->_localdata)) {
			$this->_init_localdata();
		}
		
		if (is_null($useall = $this->_item->get('useall')))
			$useall = true;
		// NULL means chancenone never defined for this item, so assume this is not a true leveled list...
		// although this will also get triggered if I just forget to read in the leveled list info
		// Treat same as useall list
		if (is_null($chancenone = $this->_item->get('chancenone'))) {
			$chancenone = 0;
			$useall = true;
		}
		// LVLG is a global variable that overrides the value of chancenone
		// If it exists, set chancenone=0 (assume script at some point will set chancenone=0)
		if (!is_null($lvlg = $this->_item->get('LVLG'))) {
			$chancenone = 0;
		}
		// NOTE: chancenone has to be applied *after* all other integrations have been done
		// (if each individual item has chancenone applied, a list with useall ends up with effective
		// chancenone near 0)
		
		$iord = $this->_item->ordid();
		if (empty($iord)) {
			print "iord=$iord, this=".$this->_iord." ordid=$iordid\n";
			var_dump(debug_backtrace());
		}
		$query = "SELECT COUNT(*) AS num FROM LevelList WHERE list_ordid=".$this->_item->ordid()." AND item_ordid={$ordid}";
		$row = $this->_parentlist->do_query($query, 'onerow');
		if (empty($row['num']) && ($this->_hasitems===TRUE || $this->_parentlist->get_datamode()==CSData::DATAMODE_DB))
			return array();
		
		// what about llast?? how to keep track of whether maximum level extends upwards at all nests?
		if (!$this->_hassublists) {
			if (!empty($this->_localdata[$ordid])) {
				$plvl = $this->_localdata[$ordid]['plvl'];
				foreach ($plvl as $l => $data) {
					if (!is_int($l))
						continue;
					foreach ($data as $c => $p)
						$plvl[$l][$c] = $p*(100-$chancenone)/100;
				}
				return $plvl;
			}
			else
				return array();
		}
		
		// scan each child for plvl info, and sum
		$plvl = array();
		foreach ($this->_localdata as $subid => $data) {
			if (!is_array($data['plvl'])) {
				print "ordid $ordid subid $subid iord=$iord\n";
				var_dump($data);
					exit;
			}
			
			$lmin = $data['plvl']['lmin'];
			$llast = $data['plvl']['llast'];
			if (array_key_exists('lmax', $data['plvl']))
				$lmax = $data['plvl']['lmax'];
			else
				$lmax = NULL;
			
			// first, establish whether this sublist (subid) provides the requested item (ordid)
			if ($subid==$ordid) {
				// sublist is the item, so level range is dictated simply by range at which subid exists within this->data
				$pl2 = NULL;
				$each = false;
			}
			elseif (!empty($data['sub'])) {
				// need to determine overlap of $data[$subid] and $pl2
				$litem = $this->_parentlist->get_item($subid);
				$pl2 = $litem->get_plvl($ordid);
				if (is_null($each = $litem->get('each')))
					$each = false;
				$litem->release();
				if (empty($pl2))
					continue;
				
				set_max($lmin, $pl2['lmin']);
				if (array_key_exists('lmax', $pl2) && !is_null($pl2['lmax']) && !is_null($lmax)) {
					set_min($llast, $pl2['lmax']);
					$lmax = $llast;
				}
				else if (array_key_exists('lmax', $pl2) && !is_null($pl2['lmax'])) {
					$lmax = $llast = $pl2['lmax'];
				}
				else if (is_null($lmax))
					set_max($llast, $pl2['llast']);
				
				// level ranges do not overlap (e.g., minotaur of the grove cannot get a high-level ring from the level 1-3 version of the minotaur)
				if (!is_null($lmax) && $lmax<$lmin)
					continue;
			}
			else
				continue;
			
			// compare new level range to the data that has been calculated so far
			// if existing data has no upper bound, but new data is at a higher level, need to extend existing data to fill new range
			if (array_key_exists('llast', $plvl) && !array_key_exists('lmax', $plvl) && $plvl['llast']<=$llast) {
				$lb = $llast;
				if (!is_null($lmax))
					$lb++;
				for ($l=$plvl['llast']+1; $l<=$lb; $l++) {
					if (!array_key_exists($plvl['llast'], $plvl)) {
						print "plvl[llast] does not have data, l={$l}, plvl[llast]=".$plvl['llast']." llast={$llast} listitem=".$this->_item->get('ordid')." listitemformid=".$this->_item->get('formid')." ordid={$ordid}, subid={$subid}\n";
						var_dump($plvl);
					}
					$plvl[$l] = $plvl[$plvl['llast']];
				}
				$plvl['llast'] = $lb;
			}
			
			for ($l=$lmin; $l<=$llast; $l++) {
				// multiply probabilities of (subid in this->data) times (ordid in subid)
				$leff = $l;
				if (!array_key_exists($l, $data['plvl'])) {
					if (array_key_exists('lmax', $data['plvl']) && !is_null($data['plvl']['lmax']) && $l>$data['plvl']['lmax'])
						continue;
					else
						$leff = $data['plvl']['llast'];
				}
				elseif (!count($data['plvl'][$l]))
					continue;
				$c2data = array();
				if (!is_null($pl2)) {
					$leff2 = $l;
					if (!array_key_exists($l, $pl2)) {
						if (array_key_exists('lmax', $pl2) && !is_null($pl2['lmax']) && $l>$pl2['lmax'])
							continue;
						else
							$leff2 = $pl2['llast'];
					}
					elseif (!count($pl2[$l]))
						continue;
					if ($each)
						$cmin = 1;
					elseif ($useall && count($data['plvl_counts'][$leff])>1)
						$cmin = 1;
					else
						$cmin = NULL;
					foreach ($data['plvl'][$leff] as $c => $p) {
						if (!isset($cmin) || abs($c)<=$cmin) {
							// The easy calculation -- just multiply p's
							foreach ($pl2[$leff2] as $c2 => $p2) {
								$cnew = $c*$c2;
								if ($c<0 && $c2<0)
									$cnew = -1*$cnew;
								@ $c2data[$cnew] += $p*$p2;
							}
						}
						else {
							// The integrated case... add each individual instance one at a time
							if ($each) {
								for ($ct=0; $ct<abs($c); $ct++) {
									$this->_integrate_plvl($c2data, $pl2[$leff2], $p, 1);
								}
							}
							else {
								foreach ($data['plvl_counts'][$leff] as $ct) {
									$this->_integrate_plvl($c2data, $pl2[$leff2], $p, $ct);
								}
							}
						}
					}
				}
				else {
					foreach ($data['plvl'][$leff] as $c => $p) {
						$c2data[$c] = $p*(100-$chancenone)/100;
					}
				}
				$sum = 0;
				foreach ($c2data as $c => $p) {
					$sum += $p;
				}
				
				set_min($plvl['lmin'], $l);
				// now that we have the new data, add it to existing data
				if (array_key_exists($l, $plvl) && !is_array($plvl[$l])) {
					print "plvl[l] is not an array, l={$l}, listitem=".$this->_item->get('ordid')." listitemformid=".$this->_item->get('formid')." ordid={$ordid}, subid={$subid}\n";
					var_dump($plvl[$l]);
				}
				if (!$useall || empty($plvl[$l])) {
					foreach ($c2data as $c => $p) {
						@ $plvl[$l][$c] += $p;
					}
				}
				else {
					$this->_integrate_plvl($plvl[$l], $c2data);
				}
			}
			set_max($plvl['llast'], $llast);
			if (!is_null($lmax) && $plvl['llast']==$lmax)
				$plvl['lmax'] = $lmax;
			// if data has been added on past a previous lmax limit, lmax no longer applies
			elseif (array_key_exists('lmax', $plvl) && (is_null($plvl['lmax']) || $plvl['lmax']!=$plvl['llast']))
				unset($plvl['lmax']);
		}
		// fill in any gaps in plvl range with empty arrays (to make it clear that probability is zero, rather than that llast data should be used)
		if (is_array($plvl) && array_key_exists('lmin', $plvl)) {
			for ($l=$plvl['lmin']; $l<=$plvl['llast']; $l++) {
				if (!array_key_exists($l, $plvl))
					$plvl[$l] = array();
				// This is where chancenone gets applied!!
				foreach ($plvl[$l] as $c => $p) {
					$plvl[$l][$c] = $p*(100-$chancenone)/100;
				}
			}
			if ($this->_item->ordid()==38981 || ($this->_item->ordid()==15263 && $ordid==15558)) {
				print "end for ".$this->_item->ordid()." item=$ordid\n";
				var_dump($plvl);
			}
			if ($this->_item->ordid()==19598 && $ordid==15520) {
		// Testing!!
				$data = array('plvl' => $plvl);
				$this->_calc_stats($data);
				var_dump($data);
			}
			return $plvl;
		}
		else
			return array();
	}
}

