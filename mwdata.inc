<?php
require_once 'csdata.inc';
require_once 'mw_genitem.inc';
require_once 'genitem.inc';

class MWData extends CSData {
	protected $_namespace = 'Morrowind';
	
	function __construct() {
		parent::__construct();
		
		// for armor, primary implies that it has a full set and therefore armor totals should be calculated
		$this->_styles['ARMO'] =
			array("Other" => array('primary' => 1, 'exclude' => 'brother'),
			      "Nordic Fur" => array('primary' => 1, 'idname' => 'fur', 'parent' => 'Nordic'),
			      "Netch Leather" => array('primary' => 1, 'idname' => 'netch', 'exclude' => 'boiled'),
			      "Boiled Netch Leather" => array('primary' => 0, 'idname' => 'boiled', 'parent' => 'Netch Leather'),
			      "Chitin" => array('primary' => 1),
			      "Iron" => array('primary' => 1),
			      "Bonemold" => array('primary' => 1, 'exclude' => 'armun-an|gah-julan'),
			      "Armun-An Bonemold" => array('primary' => 0, 'idname' => 'armun-an', 'parent' => 'Bonemold'),
			      "Steel" => array('primary' => 1, 'exclude' => 'nordic'),
			      "Nordic Iron" => array('primary' => 0, 'parent' => 'Nordic', 'exclude' => 'trollbone'),
			      "Gah-Julan Bonemold" => array('primary' => 0, 'idname' => 'gah-julan', 'parent' => 'Bonemold'),
			      "Imperial Steel" => array('primary' => 1, 'parent' => 'Imperial'),
			      "Nordic Trollbone" => array('primary' => 0, 'idname' => 'trollbone', 'parent' => 'trollbone'),
			      "Imperial Silver" => array('primary' => 0, 'idname' => 'silver', 'parent' => 'Imperial'),
			      "Imperial Templar" => array('primary' => 1, 'idname' => 'templar', 'parent' => 'Imperial'),
			      "Imperial Chain" => array('primary' => 0, 'parent' => 'Imperial'),
			      "Imperial Dragonscale" => array('primary' => 0, 'idname' => 'dragonscale', 'parent' => 'Imperial'),
			      "Dwemer" => array('primary' => 1),
			      "Orcish" => array('primary' => 1),
			      "Dreugh" => array('primary' => 0),
			      "Indoril" => array('primary' => 1),
			      "Glass" => array('primary' => 1),
			      "Ebony" => array('primary' => 1),
			      "Daedric" => array('primary' => 1),
			      "Nordic" => array('primary' => 0, 'parentonly' => 1),
			      "Imperial" => array('primary' => 0, 'parentonly' => 1),
			      "Dark Brotherhood" => array('primary' => 1, 'namespace' => 'Tribunal', 'idname' => 'darkbroth'),
			      "Adamantium" => array('primary' => 1, 'namespace' => 'Tribunal'),
			      "Royal Guard" => array('primary' => 1, 'namespace' => 'Tribunal', 'idname' => 'helsethguard'),
			      "Her Hand" => array('primary' => 1, 'namespace' => 'Tribunal', 'idname' => 'indoril_MH'),
			      "Wolf" => array('primary' => 1, 'namespace' => 'Bloodmoon'),
			      "Bear" => array('primary' => 1, 'namespace' => 'Bloodmoon'),
			      "Ice" => array('primary' => 1, 'namespace' => 'Bloodmoon'),
			      "Nordic Mail" => array('primary' => 1, 'namespace' => 'Bloodmoon'),
			      "Goblin" => array('primary' => 0, 'namespace' => 'Tribunal'));
		$this->_styles['WEAP'] =
			array("Other" => array('primary' => 1, 'exclude' => 'brother'),
			      "Chitin" => array('primary' => 1),
			      "Iron" => array('primary' => 1),
			      "Imperial" => array('primary' => 1),
			      "Steel" => array('primary' => 1),
			      "Nordic" => array('primary' => 1),
			      "Silver" => array('primary' => 1),
			      "Bonemold" => array('primary' => 1),
			      "Dreugh" => array('primary' => 1),
			      "Dwarven" => array('primary' => 1),
			      "Orcish" => array('primary' => 1),
			      "Glass" => array('primary' => 1),
			      "Ebony" => array('primary' => 1),
			      "Daedric" => array('primary' => 1),
			      "Adamantium" => array('primary' => 1, 'namespace' => 'Tribunal'),
			      "Goblin" => array('primary' => 1, 'namespace' => 'Tribunal'));
		$this->_styles['CLOT'] =
			array("Common" => array('primary' => 1),
			      "Expensive" => array('primary' => 1),
			      "Extravagant" => array('primary' => 1),
			      "Exquisite" => array('primary' => 1));
		
		$this->_types['ARMO'] =
			array ("Helmet" => array(),
			       "Cuirass" => array(),
			       "Left Pauldron" => array('pair' => 'Right Pauldron'),
			       "Right Pauldron" => array('pair' => 'Left Pauldron'),
			       "Left Gauntlet" => array('pair' => 'Right Gauntlet', 'equiv' => 'Left Bracer'),
			       "Right Gauntlet" => array('pair' => 'Left Gauntlet', 'equiv' => 'Right Bracer'),
			       "Left Bracer" => array('pair' => 'Right Bracer', 'equiv' => 'Left Gauntlet'),
			       "Right Bracer" => array('pair' => 'Left Bracer', 'equiv' => 'Right Gauntlet'),
			       "Greaves" => array(),
			       "Boots" => array(),
			       "Shield" => array());
		
// core means it is added to core totals; base means it is printed in table with other base items
		$this->_fixed_group_items['ARMO'] =
			array('Bonemold' => array('Helmet' => array("redoran_master_helm" => array())),
			      'Chitin' => array('Helmet' => array("cephalopod_helm" => array(),
			                                          "dust_adept_helm" => array(),
			                                          "mole_crab_helm" => array())),
			      'Daedric' => array('Helmet' => array("daedric_fountain_helm" => array('core'=>1, 'base'=>1))),
			      'Imperial Steel' => array('Shield' => array("imperial shield" => array('core'=>1, 'base'=>1))),
			      'Imperial Templar' => array('Shield' => array("imperial shield" => array('core'=>1))),
			      'Nordic Fur' => array('Shield' => array("nordic_leather_shield" => array('core'=>1, 'base'=>1))),
			      'Other' => array('Boots' => array("heavy_leather_boots" => array()),
			                       'Helmet' => array("fur_colovian_helm" => array(),
			                                         "morag_tong_helm" => array(),
			                                         "gondolier_helm" => array()),
			                       'Left Bracer' => array("cloth bracer left" => array()),
			                       'Right Bracer' => array("cloth bracer right" => array()))
			     );
		$this->_fixed_group_items['WEAP'] =
			array('Iron' => array('Marksman, Bows' => array('short bow' => array(),
				'long bow' => array())),
			      'Other' => array('Axe, 2-Hand' => array('miner\'s pick' => array()),
			                       'Blunt Weapon, 1-Hand' => array('spiked club' => array()),
			                       'Blunt Weapon, 2-Hand Close' => array('6th bell hammer' => array()),
			                       'Blunt Weapon, 2-Hand Wide' => array('wooden staff' => array()),
			                       'Marksman, Arrows' => array('corkbulb arrow' => array()),
			                       'Marksman, Bolts' => array('corkbulb bolt' => array())),
			     );
	}
		
	function convert_to_cell($location) {
		$cell = floor($location/8192);
		return $cell;
	}
	
	function get_id($input, $fileid=NULL, $rectype=NULL, $options=array()) {
		return $this->_get_anyid('edid', $input, $fileid, $rectype, $options);
	}
	
	function get_levellist_dlvl() {
		return NULL;
	}
}


/* 
	MWItem
*/

class MWItem_ARMO extends GenItem_ARMO {
	protected function _class_setup() {
		$this->_column_data['charge_uses'] = array('display' => array('list_wiki', 'list_csv', 'details_html'));
	}
	protected function derive_skill() {
		// these values are provided in the CS as global variables
		// iHelmWeight, iCuirassWeight, etc
		// fLightMaxMod = 0.6, fMedMaxMod = 0.9
		$weight_limits = array ("Helmet" => 5,
		                        "Cuirass" => 30,
		                        "Left Pauldron" => 10,
		                        "Right Pauldron" => 10,
		                        "Left Gauntlet" => 5,
		                        "Right Gauntlet" => 5,
		                        "Left Bracer" => 5,
		                        "Right Bracer" => 5,
		                        "Greaves" => 15,
		                        "Boots" => 20,
		                        "Shield" => 15);
		$ratio = $this->get('weight')/$weight_limits[$this->get('type')];
		if ($ratio<=0.6)
			return "Light Armor";
		else if ($ratio<=0.9)
			return "Medium Armor";
		else
			return "Heavy Armor";
	}
	
	/*
	function INDX($index=NULL) {
		$value = $this->_getexact('INDX', $index);
		if (is_null($value) || $value=='') {
			$vallu = $this->_getexact('INDX_lu', $index);
			$datainfo = $this->_parentlist->get_datainfo($this->_rectype, 'INDX');
			if (is_array($datainfo) && array_key_exists('lookup', $datainfo))
				return $this->_parentlist->get_lookup($datainfo['lookup'], $vallu);
		}
		else
			return $value;
	}*/
}

/* This *INTENTIONALLY* is not GenItem_CELL
   The functions in GenItem_CELL are all written for OB/FO/SR not MW
   MW needs to override all the functions in GenItem_CELL plus MW does *not* want the
    derive functions defined there
*/
class MWItem_CELL extends GenItem {
	//	protected $_query_filter = 'name IS NOT NULL';
	
	// transform group_id into cell_id
	public function set_group_id($value, $index=NULL) {
		$this->_data['cell_id'] = $value;
	}
	
	// tests whether cell is an exterior cell according to any criteria
	public function exterior_any() {
		if ($this->get('exterior') || $this->get('fake exterior'))
			return true;
		else
			return false;
	}
	
	public function edid() {
		// need to call raw_edid first to make sure edid gets moved to raw_edid if necessary
		$raw = $this->raw_edid();
		if (!is_null($this->_edid) && $this->_edid)
			return $this->_edid;
		
		$edid = "";
		if (!$this->get('exterior'))
			$edid = $raw;
		else {
			if (!($edid=$raw)) {
				if (is_null($edid = $this->get('RGNN'))) {
					$edid = "Wilderness";
				}
			}
			$edid .= ' ['.$this->get('cell_x').','.$this->get('cell_y').']';
		}
		$this->_edid = $edid;
		return $edid;
	}
	
	public function raw_edid() {
		if (array_key_exists('raw_edid', $this->_data))
			return $this->_data['raw_edid'];
		elseif ($this->_edid) {
			$this->_data['raw_edid'] = $this->_edid;
			$this->_edid = NULL;
		}
		else 
			$this->_data['raw_edid'] = '';
		return $this->_data['raw_edid'];
	}
	
	// DATAMODE_READ is somehow messing up with this in place: it's automatically filling in many cells with worldspace_lu=0
	protected function derive_worldspace() {
		if (!$this->get('exterior_any'))
			return NULL;
		$edid = $this->edid();
		// no exterior cells added by Tribunal, so this function should never need to return Mournhold as a value, but REGN exists....
		if (preg_match('/^Mournhold/', $edid))
			return "Mournhold";
		// manual list of Solstheim specific regions... seems easier right now than getting the list of Bloodmoon-added regions, since
		// that list would need to be generated every time this routine is called
		elseif (preg_match('/^Felsaad|Moesring|Isinfier|Hirstaang|Brodir|Thirsk/', $edid))
			return "Solstheim";
		else
			return "Vvardenfell";
	}
	
	protected function derive_worldtype() {
		if (!$this->get('exterior_any'))
			return NULL;
		return "Main";
	}
	
	// Split FRMR subrecords off into their own records
	public function finish_init() {
		// figure out whether this an update to an existing cell
		$ordid0 = $this->_parentlist->get_ordid($this->id(), $this->get('fileid'), 'CELL', array('prev_fileid' => TRUE));
		if (!is_null($ordid0)) {
			if ($this->is_identical($ordid0, true)) {
				print "identical CELL ".$this->id()."\n";
				$samecell = true;
			}
			else {
				$samecell = false;
			}
			//			$cell0 = $this->_parentlist->get_item($ordid0);
			//			$samecell = true;
			//			foreach ($this->_data as $val_id => $data) {
			//				if (!is_array($data))
			//					continue;
			//				if ($this->get($val_id)!=$cell0->get($val_id)) {
			//				$samecell = false;
			//					break;
			//				}
			//			}
		}
		else {
			$samecell = false;
		}
		
		// no subrecords exist in this cell: keep if it contains new data; delete if it's just a repeat of existing cell
		// (repeat could be because of static records that were deleted in file compression, so it doesn't necessarily
		// have any special significance if there's an empty cell repeat)
		if (!array_key_exists('base_id', $this->_data)) {
			if ($samecell)
				return false;
			else
				return true;
		}
		
		// need to create an ordid for this record now so that I can assign it to FRMR's cellid
		$ordid = NULL;
		if (!$samecell) {
			if ($this->_parentlist->get_datamode()==CSData::DATAMODE_READ) {
				$ordid = $this->_parentlist->add_active_to_data($this);
			}
			else {
				$ordid = $this->_parentlist->get_ordid($this->id(), $this->get('fileid'), 'CELL', array('match_fileid' => TRUE));
			}
			if (is_null($ordid0))
				$ordid0 = $ordid;
		}
		
		// transfer each FRMR subrecord to its own item
		for ($i=0; $i<count($this->_data['base_id']); $i++) {
			// if in READ mode and this sub-record is for a non-requested item type, just skip: don't want to keep the data anyway
			if ($this->_parentlist->get_datamode()==CSData::DATAMODE_READ && (is_null($ref=$this->get('base_id',$i)) || !is_int($ref)))
				continue;
			$refr = $this->_parentlist->newitem('REFR');
			$refr->set($ordid0, 'cell_id');
			// not sure whether FRMRs are truly unique... for now, tack on ordid0 and fileid to be more likely ID is unique
			// but actually, don't really want to create fake edids... just leave blank instead
			//			$refr->set('FRMR'.$this->get('FRMR', $i)."-".$ordid0."-".$this->get('fileid'), 'edid');
			
			foreach ($this->_data as $val_id => $data) {
				if (!is_array($data))
					continue;
				if ($val_id=='subid' || $val_id=='ordid' || $val_id=='FRMR')
					continue;
				if (!is_null($val = $this->get($val_id, $i)))
					$refr->set($val, $val_id);
			}
			
			// update DNAM to non-mod version of cell
			if ($this->get('fileid')!=0 && !is_null($dnam=$this->get('DNAM', $i))) {
				$dnam0 = $this->_parentlist->get_ordid($this->_parentlist->get_edid($dnam), $this->get('fileid'), 'CELL', array('prev_fileid' => TRUE));
				if (!is_null($dnam0) && $dnam0!=$dnam)
					$refr->set($dnam0, 'DNAM');
			}
			
			// save new items
			if ($this->_parentlist->get_datamode()==CSDATA::DATAMODE_WRITE) {
				$refr->write_db();
				$this->_parentlist->add_active_to_data($refr);
			}
			else
				$this->_parentlist->add_active_to_data($refr);
			$refr->release();
		}
		
		foreach ($this->_data as $val_id => $data) {
			if (!is_array($data))
				continue;
			unset($this->_data[$val_id]);
		}
		
		// same criteria as above for deciding whether to keep cell
		if ($samecell)
			return false;
		else
			return true;
	}
}

class MWItem_DIAL extends GenItem {
	protected $_column_data =
                array(
                       'dialogue' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'link_info'), 
                       'dialogue_text' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue')),
                     );

	 public function _get_index_dialogue_shared() {
                if (!isset($this->_data['dialogue'])) {
                        $this->_data['dialogue'] = array();
                        $this->_data['dialogue_text'] = array();
			$this->_data['group_id'] = $this->get('ordid');

                        $query = "SELECT DISTINCT INAM, infotext FROM AllItems inner join INFO_Record using (ordid) WHERE group_id=".$this->get('ordid');
                        $res = $this->do_query($query);
                        while ($row=$this->row_query($res)) {
                                $this->_data['dialogue'][] = $row['INAM'];
                                $this->_data['dialogue_text'][] = $row['infotext'];
                        }
                }
                if (empty($this->_data['dialogue']))
                        return FALSE;
                return count($this->_data['dialogue'])-1;
        }

        public function get_index_dialogue() {
                return $this->_get_index_dialogue_shared();
        }
        public function get_index_dialogue_text() {
                return $this->_get_index_dialogue_shared();
        }
}

class MWItem_FACT extends GenItem_FACT {
	protected function _class_setup() {
		unset($_column_data['evil']);
		unset($_display_filters['evil']);
	}
}

class MWItem_MGEF extends GenItem_MGEF {
	function derive_name() {
		return $this->_parentlist->get_lookup('MGEF_name', 1*substr($this->_edid,4));
	}
	
	function specific_name($skillname, $attribname) {
		$ename = $this->get('name');
		
		if ($skillname)
			$ename = preg_replace('/\s+Skill/', ' '.$skillname, $ename);
		elseif ($attribname)
			$ename = preg_replace('/\s+Attribute/', ' '.$attribname, $ename);
		
		return $ename;
	}
	
	function units($mag) {
		$ename = $this->get('name');
		if (preg_match('/Blind|Chameleon|Dispel|Reflect|Resist|Weakness/', $ename))
			$unit = "%";
		elseif (preg_match('/Detect|Telekinesis/', $ename))
			$unit = " ft";
		else {
			if (preg_match('/Command/', $ename))
				$unit = " Level";
			else
				$unit = " pt";
			if ($mag!='1')
				$unit .= "s";
		}
		
		return $mag.$unit;
	}
}

//class MWItem_INFO extends MWItem {
	// this is a real kludge, and won't work on records that have been read from db
//	function edid() {
//		return "DIAL".$this->_parentlist->get_lastdialid()."_INFO".$this->_data['INAM'];
//	}
//}

class MWItem_SCPT extends GenItem {
	function edid() {
		return $this->_data['script_name'];
	}
}

class MWItem_SKIL extends GenItem {
	function derive_name() {
		$index = (int) substr($this->get('edid'),4);
		return $this->_parentlist->get_lookup('skilllist', $index);
	}
}

class MWItem_WEAP extends GenItem_WEAP {
	protected function _class_setup() {
		// want new entries to be at front of list
		$old_col = $this->_column_data;
		$this->_column_data =
			array('choprng' => array('display' => 'details_html',
			                         'title' => 'Chop',
			                         'vars' => array('chopmin', 'chopmax')),
			      'slashrng' => array('display' => 'details_html',
			                          'title' => 'Slash',
			                          'vars' => array('slashmin', 'slashmax')),
			      'thrustrng' => array('display' => 'details_html',
			                           'title' => 'Thrust',
			                           'vars' => array('thrustmin', 'thrustmax')),
			      'damage' => array('display' => array('list_html', 'list_csv')),
			      'skill' => array('display' => 'all'),
			     );
		unset($old_col['damage']);
		unset($old_col['type']);
		
		$this->_column_data = array_merge($this->_column_data, $old_col);
		
	}
	public function choprng() {return $this->_damage_range('chop');}
	public function slashrng() {return $this->_damage_range('slash');}
	public function thrustrng() {return $this->_damage_range('thrust');}
	protected function _damage_range($valname) {
		if (($vmin=$this->get($valname.'min')) && ($vmax=$this->get($valname.'max')))
			return $vmin." - ".$vmax;
		else
			return NULL;
	}
	
	public function chopav() {return $this->_damage_average('chop');}
	public function slashav() {return $this->_damage_average('slash');}
	public function thrustav() {return $this->_damage_average('thrust');}
	protected function _damage_average($valname) {
		if (($vmin=$this->get($valname.'min')) && ($vmax=$this->get($valname.'max')))
			return ($vmin+$vmax)/2;
		else
			return NULL;
	}
	
	// simplified damage values to use in non-MW-specific displays
	// range of min - max
	protected function damagerng() {
		$dmin = $dmax = NULL;
		foreach (array('chop', 'slash', 'thrust') as $dname) {
			foreach (array('min','max') as $dwhich) {
				$dval = $this->get($dname.$dwhich);
				if (isset($dval)) {
					$dref = 'd'.$dwhich;
					if (!isset($$dref))
						$$dref = $dval;
					elseif ($dwhich=='min')
						$$dref = min($$dref,$dval);
					else
						$$dref = max($$dref,$dval);
				}
			}
		}
		if (isset($dmin)) {
			if ($dmax==$dmin)
				return $dmin;
			else
				return $dmin.'-'.$dmax;
		}
		else
			return NULL;
	}
	// average of all values
	protected function damage() {
		$dmin = $dmax = NULL;
		$dsum = 0;
		$num = 0;
		foreach (array('chop', 'slash', 'thrust') as $dname) {
			foreach (array('min','max') as $dwhich) {
				$dval = $this->get($dname.$dwhich);
				if (isset($dval)) {
					$dsum += $dval;
					$num++;
				}
			}
		}
		if ($num) {
			return $dsum/$num;
		}
		else
			return NULL;
	}
	
	protected function derive_skill() {
		$type = $this->get('type');
		if (preg_match('/(.*),/', $type, $matches))
			return $matches[1];
		else
			return $type;
	}
}

class MWItem_NPC_ extends GenItem_NPC_ {
	public function experience_needed($skill) {
		$skilllevel = $this->skill($skill);
		
		$experience = $skilllevel + 1;
		if ($_data[$skill.'_type']=='major')
			$experience *= 0.75;
		elseif ($_data[$skill.'_type']=='misc')
			$experience *= 1.25;
		if ($_data[$skill.'_spec'])
			$experience *= 0.8;
		
		return $experience;
	}
	
	public function numACHR() {
		$query = "SELECT COUNT(*) AS count FROM REFR_Record WHERE base_id=".$this->get('ordid');
		$row = $this->_parentlist->do_query($query, 'onerow');
		return $row['count'];
	}
	
	public function ACHR() {
		$query = "SELECT ordid FROM REFR_Record WHERE base_id=".$this->get('ordid');
		$res = $this->_parentlist->do_query($query);
		$num = 0;
		$ordid = NULL;
		while (($row=$this->_parentlist->row_query($res))!==false) {
			$num++;
			if ($num==1)
				$ordid = $row['ordid'];
		}
		if ($num==1 && isset($ordid))
			return $ordid;
	}
	
}

// print out a single value for a GMST, comparable to treatment in later games
class MWItem_GMST extends GenItem_GMST {
	public function DATA() {
		$value = NULL;
		foreach (array('STRV', 'FLTV', 'INTV') as $valname) {
			$value = $this->get($valname);
			if (isset($value))
				return $value;
		}
		return NULL;
	}
}

class MWItem_INGR extends GenItem_INGR {
	// Special treatment for MW effect list -- loop through eff1_id, eff2_id, etc.
	function effect_desc() {
		$output = '';
		for ($i=1; $i<5; $i++) {
			$ename = $this->get('eff'.$i);
			if (empty($ename))
				continue;
			
			// In general, both skillname and attribname will end up being set
			// since default value for the lookups is 0, which is a valid value
			if (!is_null($skillname = $this->get('skl'.$i)))
				$ename = preg_replace('/\s+Skill/', ' '.$skillname, $ename);
			if (!is_null($attribname = $this->get('att'.$i)))
				$ename = preg_replace('/\s+Attribute/', ' '.$attribname, $ename);
			
			if (!empty($output))
				$output .= '; ';
			$output .= $ename;
		}
		return $output;
	}
}

class MWItem_INFO extends GenItem {
	protected $_column_data =
		array('INAM' => array('display' => 'all', 'format' => 'link_main'),
		      'NNAM' => array('format' => 'link_info'),
		      'PNAM' => array('format' => 'link_info')
		     );
}

class MWItem_SPEL extends GenItem_SPEL {
	function source() {
		$query = 'SELECT ordid, name FROM AllItems INNER JOIN NPC__Record USING (ordid) INNER JOIN NPC__NPCS_Record USING (ordid) WHERE mercspel=1 AND spell_id='.$this->get('ordid').' ORDER BY name';
		$res = $this->_parentlist->do_query($query);
		$ordids = array();
		while ($row=$this->_parentlist->row_query($res)) {
			$ordids[] = $row['ordid'];
		}
		return implode(' ', $ordids);
	}
}
