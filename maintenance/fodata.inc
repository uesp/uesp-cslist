<?php
require_once 'csdata.inc';
require_once 'fo_genitem.inc';
require_once 'genitem.inc';

class FOData extends CSData {
	protected $_namespace = 'Fallout3';
	function __construct() {
		parent::__construct();
	}
}

/* 
   FOItem
*/

class FOItem_AMMO extends GenItem_AMMO {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
	}
}
class FOItem_ARMO extends GenItem_ARMO {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
	}
	
	protected function derive_skill() {
		/*	if ($this->_data['heavy'])
			return "Heavy Armor";
		else
			return "Light Armor";*/
		return '';
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
		}
		return 'Other';
	}
}

// GenItem_ARMA doesn't exist ... but basically want to treat ARMA with same functions as ARMO
class FOItem_ARMA extends GenItem_ARMO {
	protected function derive_skill() {
		/*if ($this->_data['heavy'])
			return "Heavy Armor";
		else
			return "Light Armor";*/
		return '';
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
		}
		return 'Other';
	}
}

class FOItem_BOOK extends GenItem_BOOK {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
	}
}

class FOItem_CLOT extends GenItem_CLOT {
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

class FOItem_CELL extends GenItem_CELL {
	// Main map name (Tamriel, Shivering Isles, Pale Pass, Other)
	protected function derive_worldspace() {
		if (is_null(parent::derive_worldspace()))
			return NULL;
		$wid = $this->get('world_id');
		$edid = $this->_parentlist->get_edid($wid);
		$name = $this->_parentlist->get_name($wid);
		
		// This needs to be checked!!
		if (preg_match('/Test/', $edid) || preg_match('/Test/', $name))
			return NULL;
		if (preg_match('/Encounter|Inverted/', $edid))
			return NULL;
		
		if (preg_match('/Wasteland|Megaton/', $edid))
			return "Wasteland";
		
		if (preg_match('/^DC/', $edid))
			return "DC";
		 if (preg_match('/GNR|Mon|Statesman/', $edid))
			return "DC";
		
		return "Other";
	}
	
	// Main/City/Submap/Other
	protected function derive_worldtype() {
		if (is_null(parent::derive_worldtype()))
			return NULL;
		$wid = $this->get('world_id');
		$edid = $this->_parentlist->get_edid($wid);
		$name = $this->_parentlist->get_name($wid);
		
		// This needs to be checked!!
		if (preg_match('/Test/', $edid) || preg_match('/Test/', $name))
			return NULL;
		if (preg_match('/Encounter|Inverted/', $edid))
		    return NULL;
		 
		if (preg_match('/Wasteland/', $edid))
			return "Main";
		if (preg_match('/Megaton/', $edid))
			return "City";
		
		if (preg_match('/^DC/', $edid))
			return "City";
		if (preg_match('/GNR|Mon|Statesman/', $edid))
		    return "Submap";
		
		return "Other";
	}
}

class FOItem_NPC_ extends GenItem_NPC_ {
	public function experience_needed($skill) {
		return 0;
	}
}

class FOItem_WEAP extends GenItem_WEAP {
	protected function _class_setup() {
		$this->_display_filters['countEITM'] = $this->_display_filters['countENAM'];
		unset($this->_display_filters['countENAM']);
	}
	protected function derive_skill() {
		if (1) 
			return "One-handed";
		else
			return "Two-handed";
	}
}

