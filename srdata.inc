<?php
require_once 'csdata.inc';
require_once 'sr_genitem.inc';
require_once 'genitem.inc';

class SRData extends CSData {
	protected $_namespace = 'Skyrim';
	function __construct() {
		parent::__construct();
	}
	
	protected function _set_levlist_rectypes() {
		parent::_set_levlist_rectypes();
		unset ($this->_levlist_rectypes['QUST']);
		unset ($this->_levlist_rectypes['COBJ']);
		/*// temporary
		unset ($this->_levlist_rectypes['LVSP']);
		// very temporary
		unset ($this->_levlist_rectypes['CONT']);*/
	}
	
	// no evidence of a iLevItemLevelDifferenceMax in Skyrim
	function get_levellist_dlvl() {
		return NULL;
	}
}

/* 
   SRItem
*/
class SRItem_AMMO extends GenItem_AMMO {
	protected function _class_setup() {
		unset($this->_display_filters['countENAM']);
		unset($this->_column_data['magic_desc']);
		$this->_column_data['value']['vars'] = array('value_raw');
	}
}

class SRItem_ARMO extends GenItem_ARMO {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
	protected function derive_skill() {
		$types = array('ArmorClothing' => 'Clothing',
		               'ArmorJewelry' => 'Jewelry',
		               'JewelryExpensive' => 'Jewelry');
		if ($this->get('clothing')) {
			foreach ($this->indices('keyword') as $k) {
				$kywd = $this->get('keyword', $k);
				$keyid = $this->_parentlist->get_t_item($kywd)->get('edid');
				if (isset($types[$keyid]))
					return $types[$keyid];
			}
			return 'Other';
		}
		elseif ($this->get('heavy'))
			return 'Heavy Armor';
		else
			return 'Light Armor';
	}

	function derive_playable() {
		return 1-($this->get('unplayable1') || $this->get('unplayable2'));
	}
	
	function derive_type() {
		$types = array('ArmorShield' => 'Shield',
		               'ArmorGauntlets' => 'Gauntlets',
		               'ArmorHelmet' => 'Helmet',
		               'ArmorBoots' => 'Boots',
		               'ArmorCuirass' => 'Armor',
		               'ClothingCirclet' => 'Circlet',
		               'ClothingNecklace' => 'Amulet',
		               'ClothingRing' => 'Ring',
		               'ClothingFeet' => 'Shoes',
		               'ClothingHands' => 'Gloves',
		               'ClothingHead' => 'Hood',
		               'ClothingBody' => 'Clothes',
		              );
		foreach ($this->indices('keyword') as $k) {
			$kywd = $this->get('keyword', $k);
			$keyid = $this->_parentlist->get_t_item($kywd)->get('edid');
			if (isset($types[$keyid]))
				return $types[$keyid];
		}
		// temporary kludge
		$name = $this->get('name');
		if (preg_match('/\bAmulet\b/', $name))
			return 'Amulet';
		elseif (preg_match('/\bRing\b/', $name))
			return 'Ring';
		return 'Other';
	}
	
	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('ENCH'));
	}
	
	public function get_disambig_type() {
		return 'item';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		$name = $this->get('name');
		if (in_array($name, array('Ring of Hircine')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
	
}

class SRItem_ARMA extends GenItem_ARMO {
	protected function derive_skill() {
		return NULL;
		if ($this->_data['heavy'])
			return "Heavy Armor";
		else
			return "Light Armor";
	}
	
	function derive_type() {
		$body_locs = array();
		foreach (array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail') as $loc) {
			if (!($this->get($loc)))
				continue;
			// ignore tail entries -- rarely used, and superfluous in those cases
			if ($loc=='tail')
				continue;
			$body_locs[$loc] = TRUE;
		}
		if (count($body_locs)==1) {
			if (!empty($body_locs['shield']))
				return 'Shield';
			if (!empty($body_locs['foot']))
				return 'Boots';
			if (!empty($body_locs['head']) || !empty($body_locs['hair']))
				return 'Helmet';
			if (!empty($body_locs['upper_body']))
				return 'Cuirass';
			if (!empty($body_locs['lower_body']))
				return 'Greaves';
			if (!empty($body_locs['hand']))
				return 'Gauntlets';
		}
		elseif (count($body_locs)==2) {
			if (!empty($body_locs['head']) && !empty($body_locs['hair']))
				return 'Helmet';
			if (!empty($body_locs['upper_body']) && !empty($body_locs['lower_body']))
				return 'Armor';
		}
		return 'Other';
	}
}

class SRItem_BOOK extends GenItem_BOOK {
	protected function _class_setup() {
		unset($this->_column_data['magic_desc']);
		unset($this->_display_filters['countENAM']);
		unset($this->_display_filters['scroll']);
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
		$this->_column_data['skill']['vars'] = array('skillspell');
		$this->_column_data['type'] = array('display' => 'details_html', 'vars' => array('type_raw'));
		$this->_column_data['value']['vars'] = array('value_raw');
	}
	
	function derive_type_lu() {
		$src = $this->get('type_raw')&0x05;
		if ($src==0)
			return 0;
		elseif ($src==1)
			return 1;
		elseif ($src==4)
			return 2;
		return NULL;
	}
	
	function derive_skill_lu() {
		if ($this->get('type_raw')&0x01) {
			$src = $this->get('skillspell');
			if ($src<0)
				return NULL;
			$skill = $this->_parentlist->get_lookup('actorlist', $src);
			if (!empty($skill))
				return $src;
		}
		return NULL;
	}
	
	// this may only work on a second read-through...
	function derive_spell_id() {
		if ($this->get('type_raw')&0x04) {
			$src = $this->get('skillspell');
			if ($src<0)
				return NULL;
			$ordid = $this->convert_formid($src);
			if (empty($ordid))
				return NULL;
			if ($this->_parentlist->get_rectype($ordid)!='SPEL')
				return NULL;
			return $ordid;
		}
		return NULL;
	}
	
	public function preferred_locations() {
		return array('SolitudeBardsCollege', 'WinterholdCollegeArcanaeum', 'WindhelmHjerim', 'WhiterunJorrvaskrBasement', 'MarkarthVlindrelHall', 'SolitudeProudspireManor', 'RiftenHoneyside', 'WhiterunDragonsreach');
	}

	public function get_disambig_type() {
		return 'book';
	}
	
}

class SRItem_COBJ extends GenItem {
	protected $_column_data =
		array('created' => array('display' => 'all', 'format' => 'ordid_as_name'),
		     );
}

class SRItem_CLOT extends GenItem_CLOT {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
	}
	function derive_type() {
		$body_locs = array();
		foreach (array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail') as $loc) {
			if (!($this->get($loc)))
				continue;
			// ignore tail entries -- rarely used, and superfluous in those cases
			if ($loc=='tail')
				continue;
			$body_locs[$loc] = TRUE;
		}
		if (count($body_locs)==1) {
			if (!empty($body_locs['foot']))
				return 'Shoes';
			if (!empty($body_locs['upper_body']))
				return 'Shirt';
			if (!empty($body_locs['lower_body']))
				return 'Pants/Skirt';
			if (!empty($body_locs['hand']))
				return 'Gloves';
			if (!empty($body_locs['amulet']))
				return 'Amulet';
			if (!empty($body_locs['right_ring']) || !empty($body_locs['left_ring']))
				return 'Ring';
			if (!empty($body_locs['head']) || !empty($body_locs['hair']))
				return 'Hood';
		}
		elseif (count($body_locs)==2) {
			if (!empty($body_locs['head']) && !empty($body_locs['hair']))
				return 'Hood';
			if (!empty($body_locs['right_ring']) && !empty($body_locs['left_ring']))
				return 'Ring';
		}
		return 'Other';
	}
}

class SRItem_CELL extends GenItem_CELL {
	// Purposes of worldspace/worldtype:
	// * Filter out test zones
	// * Identify when info from a city world should be superimposed on main map
	// Note, though, that these values aren't assigned until setup_cells is done
	
	// These need a lot of work .. but I don't know where any of the places are

	// Main map name (Tamriel, Shivering Isles, Pale Pass, Other)
	// can this even be done before setup_cells?  relies on info from other cells
	protected function derive_worldspace() {
		$dochk = FALSE;
		//		if (preg_match('/Goldenglow/', $this->get('edid')))
		//			$dochk = TRUE;
		if ($dochk) {
			print "derive worldspace being called for ".$this->get('name').' '.$this->get('edid').' '.$this->get('ordid')."\n";
		}
		// manually enter exceptions for a few places I suspect are entered by teleportation
		if (preg_match('/^vampireDreamworld0[123]$/', $this->get('edid'))) {
			return 'Other';
		}
		//CidhnaMine03? HelgenTorolfsMill? HelgenTheLostMansReprieve?
		elseif ($this->get('edid')=='LabyrinthianMaze' || $this->get('edid')=='LabyrinthianPriestSanctuary' || $this->get('edid')=='SolitudeCastleDourTower' || $this->get('edid')=='AzurasStarInterior01')
			return 'Other';
		
		if (is_null(parent::derive_worldspace())) {
			if ($dochk) {
				print "returning NULL bc parent function ".$this->get('name').' '.$this->get('edid').' '.$this->get('ordid');
				print "\n";
				/*exit;*/
			}
			return NULL;
			}
		$wid = $this->get('world_id');
		$edid = $this->_parentlist->get_edid($wid);
		// Tamriel is edid; name is Skyrim
		// NB Skyrim does not have a parent... need to return before doing parent check
		if ($edid=='Tamriel')
			return 'Skyrim';
		$world = $this->_parentlist->get_t_item($wid);
		$parent = $world->get('parent_id');
		if (empty($parent)) {
			if ($dochk) {
				print "returning NULL bc no parent ".$this->get('name').' '.$this->get('edid').' '.$this->get('ordid');
				print "\n world = $wid ".$world->get('name').' '.$world->get('edid')."\n";
				/*exit;*/
			}
			return NULL;
		}
		if (preg_match('/Test/', $edid)) {
			if ($dochk) {
				print "returning NULL because Test ".$this->get('name').' '.$this->get('edid').' '.$this->get('ordid');
				print "\n world = $wid ".$world->get('name').' '.$world->get('edid')."\n";
				/*exit;*/
			}
			
			return NULL;
		}
		
		if (preg_match('/Skyrim/', $edid))
			return 'Skyrim';
		// cities with separate spaces
		if (preg_match('/^(Solitude|Windhelm|Whiterun|Markarth|Riften)World$/', $edid))
			return 'Skyrim';
		
		$wloc_id = $world->get('location_id');
		if (empty($wloc_id))
			return 'Other';
		$wloc = $this->_parentlist->get_t_item($wloc_id);
		// Sovngarde is one case with empty PNAM
		if (is_null($wloc->get('PNAM')))
			return 'Other';
		
		return 'Skyrim';
	}
	
	// Main/City/Submap/Other
	protected function derive_worldtype() {
		if (preg_match('/^vampireDreamworld0[123]$/', $this->get('edid'))) {
			return 'Other';
		}
		elseif ($this->get('edid')=='LabyrinthianMaze' || $this->get('edid')=='LabyrinthianPriestSanctuary' || $this->get('edid')=='SolitudeCastleDourTower' || $this->get('edid')=='AzurasStarInterior01')
			return 'Other';
		if (is_null(parent::derive_worldtype()))
			return NULL;
		$wid = $this->get('world_id');
		$world = $this->_parentlist->get_t_item($wid);
		$edid = $world->get('edid');
		if ($edid=='Tamriel')
			return 'Main';
		if (preg_match('/Test|FXLight/', $edid) || preg_match('/Test/', $world->get('name')))
			return NULL;
		
		$wloc_id = $world->get('location_id');
		if (empty($wloc_id))
			return 'Other';
		
		$wloc = $this->_parentlist->get_t_item($wloc_id);
		if ($wloc->find_keyword('LocTypeCity', TRUE))
			return 'City';
		
		return 'Other';
	}
	
	public function get_parent_location() {
		if ($this->get('location_id'))
			return $this->get('location_id');
		$name = $this->get('name');
		if ($this->get('parentcell_id')) {
			// nb tcell might be a WRLD instead of a CELL
			$tcell = $this->_parentlist->get_item($this->get('parentcell_id'));
			$loc = $tcell->get('parent_location');
			if (empty($name))
				$name = $tcell->get('name');
			$tcell->release();
			if (!empty($loc))
				return $loc;
		}
		if (!empty($name)) {
			$query = "SELECT ordid from AllItems where rectype='LCTN' and name='".addslashes($name)."' and mod_status&0x02>0";
			$res = $this->do_query($query);
			if (mysql_num_rows($res)==1) {
				$row = $this->row_query($res);
				return $row['ordid'];
			}
			$query = "SELECT ordid from AllItems where rectype='LCTN' and edid='".addslashes($name)."Location' and mod_status&0x02>0";
			$res = $this->do_query($query);
			if (mysql_num_rows($res)==1) {
				$row = $this->row_query($res);
				return $row['ordid'];
			}
			
		}
		return NULL;
	}
		
	public function get_city() {
		$lctn = $this->get_parent_location();
		if (empty($lctn))
			return NULL;
		$tcell = $this->_parentlist->get_t_item($lctn);
		return $tcell->get_city();
	}
	
	public function get_hold() {
		$lctn = $this->get_parent_location();
		if (empty($lctn))
			return NULL;
		$tcell = $this->_parentlist->get_t_item($lctn);
		return $tcell->get_hold();
	}
	
	public function find_nearest_marker($refr_id) {
		$refr_cell = $this->_parentlist->get_item($refr_id);
		$x = $refr_cell->get('x');
		$y = $refr_cell->get('y');
		$z = $refr_cell->get('z');
		$refr_cell->release();
		
		$world = $this->get('world_id');
		$marker_id = $this->_parentlist->get_ordid('MapMarker');
		$homecell = $this->_parentlist->get_cell_ordid(0, 0, $world);
		if (empty($homecell))
			return NULL;
		
		$bestid = NULL;
		$bestdiff = NULL;
		for ($delt=1; $delt<8; $delt++) {
			$query = "SELECT ordid, x, y, z FROM AllItems INNER JOIN REFR_Record using (ordid) WHERE base_id=$marker_id AND cell_id=$homecell AND x>".($x-4096*$delt)." AND x<".($x+4096*$delt)." AND y>".($y-4096*$delt)." AND y<".($y+4096*$delt)." AND name is not null AND name<>'' and mod_status&0x02>0\n";
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$diff = pow($x-$row['x'],2) + pow($y-$row['y'],2);
				if (!isset($bestdiff) || $diff<$bestdiff) {
					$bestid = $row['ordid'];
					$bestdiff = $diff;
				}
			}
			if (!empty($bestid))
				break;
		}
		
		return $bestid;
	}
}

/*
class SRItem_INFO extends GenItem {
	protected function _class_setup() {
		$this->_column_data['cpar1_formid'] = array('title' => 'x', );
		//unset($this->_columns['cpar1_formid']);
	}
}
*/

class SRItem_SCRL extends GenItem_Magic {
	protected $_column_data =
		array('magic_desc' => array('display' => 'all'),
		     );
	protected function _class_setup() {
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
}

class SRItem_NPC_ extends GenItem_NPC_ {
	protected function _class_setup() {
		$this->_column_data['level_text'] = array('title' => 'Level', 'display' => 'details_html', 'vars' => array('level_raw'));
		$this->_column_data['health_text'] = array('title' => 'Health', 'display' => 'details_html', 'vars' => array('health_raw'));
		$this->_column_data['magicka_text'] = array('title' => 'Magicka', 'display' => 'details_html', 'vars' => array('magicka_raw'));
		$this->_column_data['fatigue_text'] = array('title' => 'Stamina', 'display' => 'details_html', 'vars' => array('fatigue_raw'));
		$this->_column_data['reln_type'] = array('display' => 'details_html', 'title' => 'Association Type', 'display_option' => array('details_html' => 'table_reln'));
		$this->_column_data['reln_id'] = array('display' => 'details_html', 'title' => 'Associate', 'display_option' => array('details_html' => 'table_reln'), 'format' => 'ordid_as_name');
		$this->_column_data['reln_rank'] = array('display' => 'details_html', 'title' => 'Relationship Level', 'display_option' => array('details_html' => 'table_reln'));
		$this->_column_data['reln_secret'] = array('display' => 'details_html', 'title' => 'Secret?', 'display_option' => array('details_html' => 'table_reln'));
		$this->_column_data['is_npc'] = array('display' => 'all', 'title' => 'Is NPC');
		//		$this->_column_data['is_crea'] = array('display' => 'all', 'title' => 'Is Crea');
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
		$this->_column_data['templatelist'] = array('display' => 'details_html', 'title' => 'Template Used By', 'display_option' => array('details_html' => 'table_templatelist'), 'format' => 'ordid_as_name');
		$this->_column_data['dialogue'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name');
		$this->_column_data['dialogue_text'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'));
	}
	public function experience_needed($skill) {
		return 0;
	}
	
	public function is_npc() {
		if ($this->find_keyword('ActorTypeNPC', TRUE))
			return TRUE;
		$race = $this->_parentlist->get_item($this->get('race_id'));
		$is_npc = $race->find_keyword('ActorTypeNPC', TRUE);
		$race->release();
		if ($is_npc)
			return TRUE;
		else
			return FALSE;
	}
	
	public function is_crea() {
		if ($this->find_keyword('ActorTypeCreature', TRUE))
			return TRUE;
		$race = $this->_parentlist->get_item($this->get('race_id'));
		$is_npc = $race->find_keyword('ActorTypeCreature', TRUE);
		$race->release();
		if ($is_npc)
			return TRUE;
		else
			return FALSE;
	}
	
	public function _setup_reln() {
		$this->_data['reln_assoc_id'] = array();
		$this->_data['reln_type'] = array();
		$this->_data['reln_id'] = array();
		$this->_data['reln_rank'] = array();
		$this->_data['reln_secret'] = array();
		
		if ($this->_data['female'])
			$gender = 'f';
		else
			$gender = 'm';
		
		$query = "SELECT * FROM AllItems inner join RELA_Record using (ordid) LEFT JOIN ASTP_Record on (RELA_Record.assoc=ASTP_Record.ordid) where parent=".$this->get('ordid')."  and mod_status&0x02>0";
		$res = $this->_parentlist->do_query($query);
		while ($row = $this->_parentlist->row_query($res)) {
			$this->_data['reln_assoc_id'][] = isset($row['assoc'])?$row['assoc']:NULL;
			$this->_data['reln_id'][] = $row['child'];
			$this->_data['reln_type'][] = isset($row[$gender.'_parent'])?$row[$gender.'_parent']:'';
			$this->_data['reln_rank'][] = $this->_parentlist->get_lookup('RELA_rank', $row['rank_lu']);
			$this->_data['reln_secret'][] = $row['secret'];
		}
		$query = "SELECT * FROM AllItems inner join RELA_Record using (ordid) LEFT JOIN ASTP_Record on (RELA_Record.assoc=ASTP_Record.ordid) where child=".$this->get('ordid')."  and mod_status&0x02>0";
		$res = $this->_parentlist->do_query($query);
		while ($row = $this->_parentlist->row_query($res)) {
			$this->_data['reln_assoc_id'][] = isset($row['assoc'])?$row['assoc']:NULL;
			$this->_data['reln_id'][] = $row['parent'];
			$this->_data['reln_type'][] = isset($row[$gender.'_child'])?$row[$gender.'_child']:'';
			$this->_data['reln_rank'][] = $this->_parentlist->get_lookup('RELA_rank', $row['rank_lu']);
			$this->_data['reln_secret'][] = $row['secret'];
		}
	}
	
	public function get_index_reln_assoc_id() {
		return get_index_reln_type();
	}
	
	public function get_index_reln_type() {
		if (!isset($this->_data['reln_type']))
			$this->_setup_reln();
		if (empty($this->_data['reln_type']))
			return FALSE;
		return count($this->_data['reln_type'])-1;
	}
	public function get_index_reln_id() {
		return get_index_reln_type();
	}
	public function get_index_reln_rank() {
		return get_index_reln_type();
	}
	public function get_index_reln_secret() {
		return get_index_reln_type();
	}
	
	public function get_index_templatelist() {
		return $this->_get_index_using_query('templatelist', 'SELECT ordid FROM AllItems inner join NPC__Record using (ordid) WHERE template_id='.$this->ordid().' and mod_status&0x02>0');
	}
	
	public function _get_index_dialogue_shared() {
		if (!isset($this->_data['dialogue'])) {
			$this->_data['dialogue'] = array();
			$this->_data['dialogue_text'] = array();
			
			$query = "SELECT ordid, dialogue FROM AllItems inner join INFO_CTDA_Record using (ordid) INNER JOIN INFO_TRDT_Record using (ordid) WHERE cond_function_lu=".$this->_parentlist->get_lookup_id('CTDA_function', 'GetIsID')." AND cpar1_formid=".$this->get('ordid')." and mod_status&0x02>0";
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$this->_data['dialogue'][] = $row['ordid'];
				$this->_data['dialogue_text'][] = $row['dialogue'];
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
	
	public function level_text() {
		if (!$this->get('offset')) {
			$lvl = $this->get('level_raw');
//			if ($lvl)
				return $lvl;
// this is presumably a bug, but it's more useful than returning 0 as the level
//   			$lvl = $this->get('calcmin');
//			return $lvl;
		}
		$text = 'PC';
		$level_mult = $this->get('level_raw')/1000;
		$text .= "×".$level_mult;
		$cmin = $this->get('calcmin');
		$cmax = $this->get('calcmax');
		if ($cmin>1 && $cmax)
			$text .= ' (range='.$cmin.'-'.$cmax.')';
		elseif ($cmin>1)
			$text .= ' (min='.$cmin.')';
		elseif ($cmax)
			$text .= ' (max='.$cmax.')';
		return $text;
	}
	
	public function level() {
		if (!$this->get('offset')) {
			$lvl = $this->get('level_raw');
			if ($lvl)
				return $lvl;
// this is presumably a bug, but it's more useful than returning 0 as the level
//   			$lvl = $this->get('calcmin');
//			return $lvl;
			return 1;
		}
		
		$pc_level = $this->_parentlist->get_pc_level();
		$level = floor($pc_level * $this->get('level_raw')/1000);
		if ($cmin=$this->get('calcmin'))
			$level = min($level, $cmin);
		if ($cmax=$this->get('calcmax'))
			$level = max($level, $cmax);
		return $level;
	}

	public function leveled_raceclass_data() {
		$level = $this->get('level');
		// calculations already done for requested level; no need to redo calculations
		if (isset($this->_data['_calc_level']) && $this->_data['_calc_level']==$level)
			return;

		$this->_data['_calc_level'] = $level;
		if (!isset($this->_data['_stat_wt_sum']))
			$this->init_raceclass_data();

		$dlevel = $level-1;
		$dstat = $dlevel*10;
		$nfull = floor($dstat/$this->_data['_stat_wt_sum']);
		$dtot = 0;
		$has_extra = array();
		// Can't just do simple rounding here, because it doesn't match Bethesda's rounding
		// Rather, appears to use the same weighting-assignment system as skills
		// For example, if health has weight 4 and magicka weight 2
		// If there are 2 extra points, 1 goes to each
		// If there are 4 extra points, 2 go to each  -- but rounding would suggest 3+1
		foreach (array('fatigue', 'magicka', 'health') as $x => $stat) {
			$delt = $nfull*$this->_data[$stat.'_wt'];
			// -$x*.0.01 is to make sure that in cases of same-weight, fatigue is done before magicka before health
			$has_extra[$stat] = $this->_data[$stat.'_wt'] - $x*0.01;
			$dtot += $delt;
			$this->_data[$stat] = $this->_data[$stat.'0'] + $delt;
			if ($stat=='health')
				$this->_data[$stat] += $dlevel*5;
		}
		arsort($has_extra);
		while ($dtot<$dstat) {
			foreach ($has_extra as $stat => $x) {
				if ($x<=0)
					continue;
				$dtot++;
				$this->_data[$stat]++;
				if ($dtot>=$dstat)
					break;
			}
		}

// Still need to add skill calculations... once I finish figuring out the details
	}

	public function init_raceclass_data() {
		// call has already been done
		// in auto-calc case ... still need to read in spell adjustments
		if (array_key_exists('weight', $this->_data))
			return;
		$race = $this->_parentlist->get_item($this->get('race_id'));
		$class = $this->_parentlist->get_item($this->get('class_id'));
		
		if ($this->get('female'))
			$gender = "_f";
		else
			$gender = "_m";
		foreach (array('weight', 'height') as $attrib) {
			$this->_data[$attrib] = $race->get($attrib.$gender);
		}
		
		foreach (array('fatigue', 'magicka', 'health') as $stat) {
			$this->_data[$stat.'0'] = $race->get($stat.'_raw');
			$this->_data[$stat.'0'] += $this->get($stat.'_raw');
		}

		if ($this->get('autocalc')) {
			$attcount = array();
			$this->_classatt = array();
			$this->_attfac = array();

			$this->_data['_stat_wt_sum'] = 0;
			foreach (array('fatigue', 'magicka', 'health') as $stat) {
				$this->_data[$stat.'_wt'] = $class->get($stat.'_wt');
				$this->_data['_stat_wt_sum'] += $this->_data[$stat.'_wt'];
			}

			$this->_data['_skill_wt_sum'] = 0;
			$this->_data['_skill_wt_max'] = 0;
			$this->_data['_skill_nwts'] = array();
			foreach ($this->_parentlist->get_lookup_list('skilllist') as $skill) {
				$skill = strtolower(str_replace(' ', '_', $skill));
				// this is just the initialization ... will be augmented as appropriate by class and abilities
				$this->_data[$skill."0"] = 15;
				$wt = $class->get($skill.'_wt');
				$this->_data[$skill.'_wt'] = $wt;
				if (!$wt)
					continue;
				$this->_data['_skill_wt_sum'] += $this->_data[$skill.'_wt'];
				$this->_data['_skill_wt_max'] = max($wt, $this->_data['_skill_wt_max']);
				@ $this->_data['_skill_nwts'][$wt]++;
			}
			// racial skill bonuses
			for ($i=1; $i<=7; $i++) {
				$skill = $race->get('skl'.$i);
				if (is_null($skill) || $skill=='' || is_int($skill))
					continue;
				$skill = strtolower(str_replace(' ', '_', $skill));
				$this->_data[$skill."0"] += $race->get('sklbonus'.$i);
			}
		}
		
// Not aware of any permanently-applied spell-derived attribute bonuses in Skyrim

		$race->release();
		$class->release();
	}
	
	public function miscstat_adj($miscstat) {
		if (!isset($this->_data[$miscstat.'0']))
			$this->init_raceclass_data();
		if (isset($this->_data[$miscstat.'_adj']))
			return $this->_getexact($miscstat.'_adj');
		else
			return $this->_getexact($miscstat.'0');
	}
	
	public function miscstat0($miscstat) {
		if (!isset($this->_data[$miscstat.'0']))
			$this->init_raceclass_data();
		return $this->_getexact($miscstat.'0');
	}
	
	public function miscstat($miscstat) {
		if (!isset($this->_data[$miscstat.'0']))
			$this->init_raceclass_data();
		if (!$this->get('autocalc'))
			return $this->_getexact($miscstat.'0');

		$this->leveled_raceclass_data();
		return $this->_getexact($miscstat);
	}
	
	public function miscstat_text($miscstat) {
		if (!$this->leveled())
			return $this->get($miscstat);

		$text = '';
		$val0 = $this->get($miscstat.'0');
		if ($val0>0)
			$text = $val0;
		$level_mult = $this->get('level_raw')/1000;
		$delt = $this->get($miscstat.'_delt');
		if (!$delt) {
			if (!$text)
				$text = '0';
		}
		else {
			if ($text)
				$text .= '+';
			$prt_sub = preg_replace('/\.0$/', '', sprintf('%.1f', 1/$level_mult));
			$text .= '(PC-'.$prt_sub.')';
			$prt_delt = preg_replace('/\.0$/', '', sprintf('%.1f', $level_mult*$delt));
			$text .= "×".$prt_delt;
/*
		$cmin = $this->get('calcmin');
		$cmax = $this->get('calcmax');
		if ($cmin>1 && $cmax)
			$text .= ' (range='.$cmin.'-'.$cmax.')';
		elseif ($cmin>1)
			$text .= ' (min='.$cmin.')';
		elseif ($cmax)
			$text .= ' (max='.$cmax.')';
*/
		}
		return $text;
	}
	
	public function miscstat_delt($miscstat) {
		if (!$this->get('autocalc'))
			return 0;
		if (!isset($this->_data[$miscstat.'_wt']))
			$this->init_raceclass_data();

		$delt = $this->_data[$miscstat.'_wt']*10/$this->_data['_stat_wt_sum'];
		//return sprintf('%0.1f', $delt);
		return $delt;
	}

	// need to override various functions from base class
	public function health() {
		return $this->miscstat('health');
	}
	public function magicka() {
		return $this->miscstat('magicka');
	}
	public function magicka_delt() {
		return $this->miscstat_delt('magicka');
	}
	public function fatigue() {
		return $this->miscstat('fatigue');
	}

	public function health_delt() {
		if (!$this->get('autocalc'))
			return 0;
		if (!isset($this->_data['health_wt']))
			$this->init_raceclass_data();

		// +5 is only difference from miscstat_delt
		$delt = $this->_data['health_wt']*10/$this->_data['_stat_wt_sum'] + 5;
		//return sprintf('%0.1f', $delt);
		return $delt;
	}
	
	public function skill($skill) {
		if (!$this->get('autocalc'))
			return $this->_getexact($skill.'_raw');
		
		$this->leveled_raceclass_data();
		return $this->_getexact($skill);
	}
	
	public function skill_delt($skill) {
		if (!$this->get('autocalc'))
			return 0;

		if (!$this->_data['_skill_wt_sum'])
			$this->init_raceclass_data();
		
		$delt = $this->_data[$skill.'_wt']*8/$this->_data['_skill_wt_sum'];
		return sprintf('%.1f', $delt);
	}
	
	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('RACE', 'CLAS', 'FACT', 'CONT'));
	}
	
	public function get_disambig_type() {
		$dnames = array('Ysgramor' => 'dog',
		                'Paarthurnax' => 'dragon',
		                'Frost' => 'horse');
		$name = $this->get('name');
		if (isset($dnames[$name]))
			return $dnames[$name];
		elseif ($this->is_npc())
			return 'NPC';
		else
			return 'creature';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		$name = $this->get('name');
		if ($name=='Ysgramor')
			return ' (dog)';
		elseif (in_array($name, array('M\'aiq the Liar', 'Cave Bear', 'Chaurus', 'Chicken', 'Elk', 'Goat', 'Horker', 'Rabbit', 'Red Eagle', 'Sibbi Black-Briar', 'Spectral Assassin', 'Spriggan Swarm', 'Alchemist', 'Courier', 'Snow Bear', 'Durnehviir', 'Minorne', 'Vampire Assassin')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
}

class SRItem_MGEF extends GenItem_MGEF {
	protected function _class_setup() {
		$this->_column_data['KWDA'] = array('format' => 'formid');
	}
}

class SRItem_SPEL extends GenItem_SPEL {
	protected function _class_setup() {
		$this->_column_data['ETYP'] = array('display' => 'all', 'format' => 'ordid_as_name');
		$this->_column_data['magic_summ'] = array('display' => 'all', 'title' => 'Description');
	}
	function spellcost() {
		if (!$this->get('autocalc'))
			return $this->_getexact('spellcost_raw');
		else
			return $this->get('auto_spellcost');
	}
	
	function auto_spellcost() {
		$cost = 0;
// based on Turn Greater Undead, Detect Dead, and Circle of Protection, only final total is floor'ed
		foreach ($this->indices('effect_id') as $index) {
			$cost += $this->get('effect_cost', $index);
		}
		return floor($cost);
	}
	
	public function derive_spellschool() {
		$perk = $this->get('perk');
		if (empty($perk))
			return NULL;
		$perk_name = $this->_parentlist->get_name($perk);
		$parts = explode(' ', $perk_name);
		if (count($parts)<2)
			return NULL;
		return $parts[1];
	}
	public function derive_spelllevel() {
		$perk = $this->get('perk');
		if (empty($perk))
			return NULL;
		$perk_name = $this->_parentlist->get_name($perk);
		$parts = explode(' ', $perk_name);
		if (count($parts)<2)
			return NULL;
		return $parts[0];
	}
	// override base function until mercspel equivalent identified
	function source() {
		return NULL;
	}
	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('MGEF', 'ENCH', 'EXPL'));
	}
	public function get_disambig_type() {
		return 'spell';
	}
	public function get_disambig_suffix($showtrigger=FALSE) {
		$name = $this->get('name');
		if (in_array($name, array('Ice Storm')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
	
}

class SRItem_WOOP extends GenItem {
	protected $_column_data =
		array('translate' => array('display' => 'all')
		     );
}

class SRItem_PACK extends GenItem_PACK {
	protected function _class_setup() {
		$this->_column_data['PTDA_type']['title'] = 'Target Type';
		$this->_column_data['activity_name_ref'] = array('format' => 'pack_activity_id', 'title' => 'Activity Name');
	}
}

class SRItem_PERK extends GenItem {
	protected $_column_data =
		array('skill' => array('display' => 'all'),
		      'skill_req' => array('display' => 'all'),
		      'perk_req' => array('display' => 'all', 'format' => 'ordid_as_name'),
		     );
	protected $_display_filters =
		array('skill' => array('title' => 'Skill'));
	// need to tweak this to also find the zero-level entries in each perk tree
	public function derive_skill_lu() {
		foreach ($this->indices('cond_op') as $index) {
			if ($this->get('cond_function', $index)=='GetBaseActorValue') {
				return $this->get('cond_param1',$index);
			}
		}
		return NULL;
	}
	public function skill_req() {
		foreach ($this->indices('cond_op') as $index) {
			if ($this->get('cond_function', $index)=='GetBaseActorValue') {
				return $this->get('cond_value',$index);
			}
		}
		return NULL;
	}
	public function perk_req() {
		foreach ($this->indices('cond_op') as $index) {
			if ($this->get('cond_function', $index)=='HasPerk' && $this->get('cond_value', $index)>0) {
				$formid = $this->get('cond_param1', $index);
				return $this->convert_formid($formid);
			}
		}
		return NULL;
	}
	public function perk_reqb() {
		$first = TRUE;
		foreach ($this->indices('cond_op') as $index) {
			if ($this->get('cond_function', $index)=='HasPerk' && $this->get('cond_value', $index)>0) {
				$formid = $this->get('cond_param1', $index);
				if (empty($formid))
					return NULL;
				if ($first) {
					$first = FALSE;
				}
				else {
					return $this->convert_formid($formid);
				}
			}
		}
	}
	public function perk_rank() {
		$name = trim($this->get('name'));
		if (empty($name))
			return NULL;
		$rank = 0;
		if ($req = $this->get('perk_req')) {
			$reqname = trim($this->_parentlist->get_name($req));
			if ($name==$reqname) {
				$reqitem = $this->_parentlist->get_item($req);
				$rank = $reqitem->get('perk_rank') + 1;
				$reqitem->release();
				return $rank;
			}
		}
		// Check whether it's rank 1 of a set
		$chkval = hexdec(substr($this->get('formid'),2));
		$query = 'SELECT ordid FROM AllItems inner join PERK_CTDA_Record using (ordid) WHERE cond_param1='.$chkval.' AND name like \'%'.addslashes($name).'%\'  and mod_status&0x02>0';
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			$reqitem = $this->_parentlist->get_item($row['ordid']);
			if (trim($reqitem->get('name')==$name && $reqitem->get('perk_req')==$this->get('ordid'))) {
				$reqitem->release();
				return 1;
			}
			$reqitem->release();
		}
		return NULL;
	}
	
	public function get_disambig_type() {
		return 'perk';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		if (in_array(trim($this->get('name')), array('Recovery', 'Wind Walker', 'Custom Fit', 'Well Fitted', 'Power Shot', 'Elemental Protection', 'Magic Resistance', 'Extra Damage')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
}

class SRItem_RCPE extends GenItem {
	protected $_column_data =
		array('skill' => array('display' => 'all'),
		     );
	protected $_display_filters =
		array('skill' => array('title' => 'Skill'),
		     );
}

class SRItem_LVLI extends GenItem {
	protected function _class_setup() {
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
}
class SRItem_LVLN extends GenItem {
	protected function _class_setup() {
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
}
class SRItem_WEAP extends GenItem_WEAP {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
	
	function derive_type() {
		$types = array('ArmorShield' => 'Shield',
		               'ArmorGauntlets' => 'Gauntlets',
		               'ArmorHelmet' => 'Helmet',
		               'ArmorBoots' => 'Boots',
		               'ArmorCuirass' => 'Armor',
		               'ClothingCirclet' => 'Circlet',
		               'ClothingNecklace' => 'Amulet',
		               'ClothingRing' => 'Ring',
		               'ClothingFeet' => 'Shoes',
		               'ClothingHands' => 'Gloves',
		               'ClothingHead' => 'Hood',
		               'ClothingBody' => 'Clothes',
		              );
		foreach ($this->indices('keyword') as $k) {
			$kywd = $this->get('keyword', $k);
			$keyid = $this->_parentlist->get_t_item($kywd)->get('edid');
			if (substr($keyid,0,8)!='WeapType')
				continue;
			$type = substr($keyid,8);
			$type = preg_replace('/(.+)([A-Z].*)/', '$1 $2', $type);
			return $type;
		}
		return 'Other';
	}
	
	protected function derive_skill() {
		if (1) 
			return "One-handed";
		else
			return "Two-handed";
	}
	
	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('ENCH'));
	}
	
	public function get_disambig_type() {
		$dnames = array('Fork' => 'weapon',
		                'Knife' => 'weapon');
		$name = $this->get('name');
		if (isset($dnames[$name]))
			return $dnames[$name];
		else
			return 'item';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		$name = $this->get('name');
		if (in_array($name, array('Wabbajack', 'Briarheart Geis', 'Notched Pickaxe')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
}

class SRItem_LCTN extends GenItem {
	protected $_column_data =
		array(
		       'member' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_member'), 'format' => 'ordid_as_name'),
		     );
	
	public function get_index_member() {
		return $this->_get_index_using_query('member', array("SELECT ordid FROM AllItems inner join CELL_Record using (ordid) WHERE location_id=".$this->get('ordid').' and mod_status&0x02>0', "SELECT ordid FROM AllItems inner join WRLD_Record using (ordid) WHERE location_id=".$this->get('ordid').' and mod_status&0x02>0'));
	}
	
	public function get_city() {
		if ($this->get('name')=='Winterhold College')
			return 'College of Winterhold (place)';
		// don't want to return Dawnstar Sanctuary as city name, esp. because it's the same as the cell's name
		elseif ($this->get('name')=='Dawnstar Sanctuary' || $this->get('name')=='Dark Brotherhood Sanctuary')
			return NULL;
		if ($this->find_keyword('LocTypeCity', TRUE) || $this->find_keyword('LocTypeTown', TRUE))
			return $this->get('name');
		if ($this->get('PNAM')) {
			$tcell = $this->_parentlist->get_t_item($this->get('PNAM'));
			return $tcell->get_city();
		}
		else {
			return NULL;
		}
	}
	
	public function get_hold() {
		if ($this->find_keyword('LocTypeHold', TRUE) || $this->find_keyword('LocTypeHoldMinor', TRUE)) {
			$hold = ucfirst($this->get('name'));
			if ($hold=='Winterhold')
				$hold .= ' (region)';
			elseif ($hold=='Whiterun' || $hold=='Falkreath')
				$hold .= ' Hold';
			return $hold;
		}
		if ($this->get('PNAM')) {
			$tcell = $this->_parentlist->get_t_item($this->get('PNAM'));
			return $tcell->get_hold();
		}
		else {
			return NULL;
		}
	}

	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('REFR', 'CELL', 'WRLD'));
	}
	
	public function get_disambig_type() {
		return 'place';
	}

	// change location name to match marker name if they are different
	// do I also want to check whether there's a mismatch with all cell names? Or just manually add some oddball cases?
	// OR should this be a separate function? (so that I can name of any redirect created for the location name?)
	public function wiki_placename($nospace=FALSE) {
		if ($this->get('marker_id')) {
			$marker = $this->_parentlist->get_item($this->get('marker_id'));
			if ($marker->get('name') && trim($marker->get('name')) != trim($this->get('name'))) {
				$mname = $marker->wiki_pagename($nospace);
				$marker->release();
				return $mname;
			}
		}

		return parent::wiki_pagename($nospace);
	}
}

class SRItem_PGRE extends GenItem_RefItem {};
class SRItem_PHZD extends GenItem_RefItem {};

class SRItem_REFR extends GenItem_REFR {

	protected $_query_filter = '(name IS NOT NULL OR edid IS NOT NULL)';

	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		if ($this->get('map_marker'))
			return array_merge($std, array('LCTN', 'CELL', 'WRLD'));
		else
			return $std;
	}
	
	public function get_disambig_type() {
		if ($this->get('map_marker'))
			return 'place';
		else
			return parent::get_disambig_type();
	}
}

class SRItem_KYWD extends GenItem {
	protected $_column_data =
		array(
		       'used_by' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_used_by')),
		       'member' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_member'), 'format' => 'ordid_as_name'),
		     );
	
	public function get_index_used_by() {
		if (!isset($this->_data['used_by'])) {
			$this->_data['used_by'] = array();
			foreach ($this->_parentlist->get_datatables(NULL,NULL) as $table) {
				if (substr($table,5,4)!='KWDA')
					continue;
				$query = "SELECT count(*) AS count FROM AllItems inner join $table using (ordid) WHERE keyword=".$this->get('ordid').'  and mod_status&0x02>0';
				$row = $this->_parentlist->do_query($query, 'onerow');
				if ($row['count'])
					$this->_data['used_by'][] = substr($table,0,4);
			}
		}
		if (empty($this->_data['used_by']))
			return FALSE;
		return count($this->_data['used_by'])-1;
	}
	
	// this is effectively an initialization function for members
	public function get_index_member() {
		if (!isset($this->_data['member'])) {
			$this->_data['member'] = array();

			// instead just search through all KWDA tables
//			$query = "SHOW tables like '%KWDA_Record'";
//			$tres = $this->_parentlist->do_query($query);
//			while ($row = $this->_parentlist->row_query($tres)) {
//				$rec = reset($row);
			// Is this always filled in before we get here?
			$rectochk =  $this->_data['used_by'];
			foreach ($rectochk as $rec) {
				$query = "SELECT ordid FROM AllItems inner join ${rec}_KWDA_Record using (ordid) WHERE keyword=".$this->get('ordid').'  and mod_status&0x02>0';
//				$query = "SELECT ordid FROM AllItems inner join ${rec} using (ordid) WHERE keyword=".$this->get('ordid').'  and mod_status&0x02>0';
				$res = $this->_parentlist->do_query($query);
				while ($row=$this->_parentlist->row_query($res)) {
					$this->_data['member'][] = $row['ordid'];
				}
			}
		}
		if (empty($this->_data['member']))
			return FALSE;
		return count($this->_data['member'])-1;
	}
}

class SRItem_QUST extends GenItem {
	protected $_column_data =
		array(
		       'dialogue' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name'),
		       'dialogue_view' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue_view'), 'format' => 'ordid_as_name'),
		       'story' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_story'), 'format' => 'ordid_as_name'),
		       'filter' => array('display' => 'all'),
// would rather get this folded in with rest of sids
		       'fragment_sid' => array('title' => 'Fragment Script', 'format' => 'sid'),
		     );
	
	public function get_index_dialogue() {
		if (!isset($this->_data['dialogue'])) {
			$this->_data['dialogue'] = array();
		
			$query = "SELECT ordid FROM AllItems inner join DIAL_Record using (ordid) WHERE quest=".$this->get('ordid').'  and mod_status&0x02>0';
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$this->_data['dialogue'][] = $row['ordid'];
			}
		}
		if (empty($this->_data['dialogue']))
			return FALSE;
		return count($this->_data['dialogue'])-1;
	}
	
	public function get_index_dialogue_view() {
		if (!isset($this->_data['dialogue_view'])) {
			$this->_data['dialogue_view'] = array();
			
			if ($this->_parentlist->hastable('DLVW_Record')) {
				$query = "SELECT ordid FROM AllItems inner join DLVW_Record using (ordid) WHERE quest_id=".$this->get('ordid').'  and mod_status&0x02>0';
				$res = $this->_parentlist->do_query($query);
				while ($row=$this->_parentlist->row_query($res)) {
					$this->_data['dialogue_view'][] = $row['ordid'];
				}
			}
		}
		if (empty($this->_data['dialogue_view']))
			return FALSE;
		return count($this->_data['dialogue_view'])-1;
	}
	
	public function get_index_story() {
		if (!isset($this->_data['story'])) {
			$this->_data['story'] = array();
			
			if ($this->_parentlist->hastable('SMQN_NNAM_Record')) {
				$query = "SELECT ordid FROM AllItems inner join SMQN_NNAM_Record using (ordid) WHERE quest_id=".$this->get('ordid').' and mod_status&0x02>0';
				$res = $this->_parentlist->do_query($query);
				while ($row=$this->_parentlist->row_query($res)) {
					$this->_data['story'][] = $row['ordid'];
				}
			}
		}
		if (empty($this->_data['story']))
			return FALSE;
		return count($this->_data['story'])-1;
	}
	
	public function get_questname() {
		$pagename = $this->get('name');
		
		$dungname = NULL;
		if (empty($pagename) && substr($this->get('FLTR'),0,7)=='Dungeon') {
			if (preg_match('/^dun(.*)QST/', $this->get('edid'), $matches)) {
				$base = $matches[1];
				$base = preg_replace('/([a-z])([A-Z])/', '$1 $2', $base);
				$bsubs = explode(' ', $base);
				for ($nb=count($bsubs); $nb>=1; $nb--) {
					$btest = addslashes(implode(' ', $bsubs));
					$bquery = "SELECT * from AllItems WHERE rectype='CELL' AND name LIKE '".addslashes($btest)."%'  and mod_status&0x02>0 order by length(name)";
					$bres = $this->do_query($bquery);
					while ($brow=$this->row_query($bres)) {
						if (!empty($brow['name'])) {
							$dungname = $brow['name'];
							break;
						}
					}
					if (!empty($dungname))
						break;
					array_pop($bsubs);
				}
			}
			if ($dungname=='Mistwatch East Tower')
				$dungname = 'Mistwatch';
			elseif ($dungname=='Ansilvund Excavation')
				$dungname = 'Ansilvund';
			elseif ($dungname=='Robber\'s Cove')
				$dungname = 'Robber\'s Gorge';
			elseif ($dungname=='Ragnvald Canal')
				$dungname = 'Ragnvald';
		}
		if (empty($pagename) && isset($dungname))
			$pagename = $dungname.' (quest)';
		
		if (empty($pagename)) {
			$qis = $this->indices('NNAM');
			foreach ($qis as $qi) {
				$pagename = $this->get('NNAM', $qi);
				if (!empty($pagename))
					break;
			}
			if (preg_match('/</', $pagename))
				$pagename = NULL;
		}
		
		$pagename = trim($pagename);
		if (substr($pagename,-1,1)=='.')
			$pagename = substr($pagename,0,-1);
		
		if ($pagename=='Delivery') {
			if ($this->get('edid')=='FreeformWindhelmA')
				$pagename = 'Delivery (Hillevi)';
			elseif ($this->get('edid')=='SolitudeFreeform04')
				$pagename = 'Delivery (Sorex)';
		}
		elseif ($pagename=='Special Delivery') {
			if ($this->get('edid')=='FreeformRiften15')
				$pagename .= ' (Bolli)';
			else
				$pagename .= ' (Sylgja)';
		}
		elseif (in_array($pagename, array('Sanuarach Mine', 'Kolskeggr Mine'))) {
			$pagename .= ' (quest)';
		}
		elseif ($pagename=='Steal the Argonian Ale')
			$pagename = 'Argonian Ale Extraction';
		elseif ($pagename=='Recover Andurs\' Amulet of Arkay')
			$pagename = 'Andurs\' Arkay Amulet';
		elseif ($pagename=='Find Derkeethus')
			$pagename = 'Extracting an Argonian';
		
		if (empty($pagename))
			$pagename = $this->get('edid');
		
		return $pagename;
	}
}

class SRItem_LCRT extends GenItem {
	protected $_column_data =
		array(
		       'item_id' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_item'), 'format' => 'ordid_as_name'),
		       'item_base_id' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_item'), 'format' => 'ordid_as_name'),
		     );
	
	public function _setup_item() {
		$this->_data['item_id'] = array();
		$this->_data['item_base_id'] = array();
		
		$query = "SELECT * FROM AllItems inner join REFR_Record using (ordid) where XLRT=".$this->get('ordid').'  and mod_status&0x02>0';
		$res = $this->_parentlist->do_query($query);
		while ($row = $this->_parentlist->row_query($res)) {
			$this->_data['item_id'][] = $row['ordid'];
			$this->_data['item_base_id'][] = $row['base_id'];
		}
	}
	
	public function get_index_item_id() {
		if (!isset($this->_data['item_id']))
			$this->_setup_item();
		if (empty($this->_data['item_id']))
			return FALSE;
		return count($this->_data['item_id'])-1;
	}
	
	public function get_index_item_base_id() {
		if (!isset($this->_data['item_base_id']))
			$this->_setup_item();
		if (empty($this->_data['item_base_id']))
			return FALSE;
		return count($this->_data['item_base_id'])-1;
	}
}

class SRItem_DIAL extends GenItem {
	protected $_column_data =
		array(
		       'dialogue' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name'),
		       'dialogue_text' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue')),
		     );
	
	public function _get_index_dialogue_shared() {
		if (!isset($this->_data['dialogue'])) {
			$this->_data['dialogue'] = array();
			$this->_data['dialogue_text'] = array();
			
			$query = "SELECT ordid, dialogue FROM AllItems inner join INFO_Record using (ordid) INNER JOIN INFO_TRDT_Record using (ordid) WHERE group_id=".$this->get('ordid').' and mod_status&0x02>0';
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$this->_data['dialogue'][] = $row['ordid'];
				$this->_data['dialogue_text'][] = $row['dialogue'];
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

class SRItem_SMEN extends GenItem {
	protected function _class_setup() {
		$this->_column_data['event_type'] = array('display' => 'all');
		$this->_column_data['quest_children'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_children'), 'format' => 'ordid_as_name');
		$this->_column_data['branch_children'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_children2'), 'format' => 'ordid_as_name');
	}
	public function get_index_quest_children() {
		return $this->_get_index_using_query('quest_children', 'SELECT ordid from AllItems inner join SMQN_Record using (ordid) where parent='.$this->ordid().' and mod_status&0x02>0');
	}
	public function get_index_branch_children() {
		return $this->_get_index_using_query('branch_children', 'SELECT ordid from AllItems inner join SMBN_Record using (ordid) where parent='.$this->ordid().' and mod_status&0x02>0');
	}
}


class SRItem_SMBN extends GenItem {
	protected function _class_setup() {
		$this->_column_data['quest_children'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_children'), 'format' => 'ordid_as_name');
		$this->_column_data['branch_children'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_children2'), 'format' => 'ordid_as_name');
		$this->_column_data['event_children'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_children3'), 'format' => 'ordid_as_name');
	}
	public function get_index_quest_children() {
		return $this->_get_index_using_query('quest_children', 'SELECT ordid from AllItems inner join SMQN_Record using (ordid) where parent='.$this->ordid().' and mod_status&0x02>0');
	}
	public function get_index_branch_children() {
		return $this->_get_index_using_query('branch_children', 'SELECT ordid from AllItems inner join SMBN_Record using (ordid) where parent='.$this->ordid().' and mod_status&0x02>0');
	}
	public function get_index_event_children() {
		return $this->_get_index_using_query('event_children', 'SELECT ordid from AllItems inner join SMEN_Record using (ordid) where parent='.$this->ordid().' and mod_status&0x02>0');
	}
}

class SRItem_FACT extends GenItem_FACT {
	protected function _class_setup() {
		$this->_column_data['dialogue'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name');
		$this->_column_data['dialogue_text'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'));
	}
	
	public function _get_index_dialogue_shared() {
		if (!isset($this->_data['dialogue'])) {
			$this->_data['dialogue'] = array();
			$this->_data['dialogue_text'] = array();
			
			$query = "SELECT ordid, dialogue FROM AllItems inner join INFO_CTDA_Record using (ordid) INNER JOIN INFO_TRDT_Record using (ordid) WHERE cond_function_lu=".$this->_parentlist->get_lookup_id('CTDA_function', 'GetInFaction')." AND cpar1_formid=".$this->get('ordid').' and mod_status&0x02>0';
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$this->_data['dialogue'][] = $row['ordid'];
				$this->_data['dialogue_text'][] = $row['dialogue'];
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

	public function get_disambig_type() {
		return 'faction';
	}
	
	// Use edid for faction names in any difficult cases
	public function wiki_pagename($nospace=FALSE) {
		$dquery = "SELECT * from AllItems where rectype='".$this->get('rectype')."' and name='".addslashes($this->get('name'))."' and ordid!=".$this->get('ordid').'  and mod_status&0x02>0';
		$dres = $this->do_query($dquery);
		// Use edid for any cases where multiple factions share the same name
		if (mysql_num_rows($dres)!=0) {
			return $this->get('edid');
		}
		// Use edid if name contains =
		if (strpos($this->get('name'),'=')!==FALSE)
			return $this->get('edid');
		// Special cases where faction name is not identical to other names, but yet is too similar
		// Does not need to include Warmaidens, because multiple-same-name has already picked it up
		if (in_array($this->get('edid'), array('DarkBrotherhoodFaction', 'ThievesGuildFaction', 'CrimeFactionThievesGuild')))
			return $this->get('edid');
		if ($this->get_disambig_suffix())
			return $this->get('edid');

		return parent::wiki_pagename($nospace);
	}
}

class SRItem_VTYP extends GenItem {
	protected function _class_setup() {
		$this->_column_data['dialogue'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name');
		$this->_column_data['dialogue_text'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'));
	}
	
	public function _get_index_dialogue_shared() {
		if (!isset($this->_data['dialogue'])) {
			$this->_data['dialogue'] = array();
			$this->_data['dialogue_text'] = array();
			
			$query = "SELECT ordid, dialogue FROM AllItems inner join INFO_CTDA_Record using (ordid) INNER JOIN INFO_TRDT_Record using (ordid) WHERE cond_function_lu=".$this->_parentlist->get_lookup_id('CTDA_function', 'GetIsVoiceType')." AND cpar1_formid=".$this->get('ordid').' and mod_status&0x02>0';
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$this->_data['dialogue'][] = $row['ordid'];
				$this->_data['dialogue_text'][] = $row['dialogue'];
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

class SRItem_ALCH extends GenItem_ALCH {
	protected function _class_setup() {
		$this->_column_data['levlists'] = array('display' => 'details_html', 'title' => 'Level Lists', 'display_option' => array('details_html' => 'table_levlists'), 'format' => 'ordid_as_name');
	}
}

class SRItem_INGR extends GenItem_INGR {
	public function get_disambig_types() {
		$std = parent::get_disambig_types();
		return array_merge($std, array('FLOR', 'TREE'));
	}
	
	public function get_disambig_type() {
		return 'item';
	}
	
	public function get_disambig_suffix($showtrigger=FALSE) {
		$name = $this->get('name');
		if (in_array($name, array('Briar Heart', 'Jarrin Root')))
			return '';
		return parent::get_disambig_suffix($showtrigger);
	}
}
