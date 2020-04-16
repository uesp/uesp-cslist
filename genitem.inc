<?php

/* Functions common to REFR, ACHR, and ACRE classes */
class GenItem_RefItem extends GenItem {
	protected $_column_data =
		array('base_rectype' => array('title' => 'Base Type',
		                              'display' => 'all',
		                             ),
		      'base_id' => array('title' => 'Base Record',
		                         'display' => 'all',
		                         'format' => 'ordid_as_name',
		                          ),
		     );
	// transform group_id into cell_id
	function set_group_id($value, $index=NULL) {
		$this->_data['cell_id'] = $value;
	}
	
	protected function add_base_data() {
		if (array_key_exists('base_rectype', $this->_data))
			return;
		$base_id=$this->_getexact('base_id');
		if (empty($base_id)) {
			$this->_data['base_rectype'] = NULL;
			return;
		}
		$base_query = 'SELECT * from AllItems where ordid='.$base_id;
		$base_row = $this->_parentlist->do_query($base_query, 'onerow');
		if (empty($base_row))
			$this->_data['base_rectype'];
		else {
			foreach (array('rectype', 'formid', 'edid', 'name') as $valname) {
				if (isset($base_row[$valname]))
					$this->_data['base_'.$valname] = $base_row[$valname];
			}
		}
	}
	function base_rectype() {
		$this->add_base_data();
		return $this->_getexact('base_rectype');
	}
	function base_formid() {
		$this->add_base_data();
		return $this->_getexact('base_formid');
	}
	function base_edid() {
		$this->add_base_data();
		return $this->_getexact('base_edid');
	}
	function base_name() {
		$this->add_base_data();
		return $this->_getexact('base_name');
	}
	
	function get_dist($refpoint, &$angle=NULL, $units='ft') {
		$dx = $dy = NULL;
		if (!is_object($refpoint)) {
			$tref = $this->_parentlist->get_item($refpoint);
			$dx = $this->get('x')-$tref->get('x');
			$dy = $this->get('y')-$tref->get('y');
			$tref->release();
		}
		else {
			$dx = $this->get('x')-$refpoint->get('x');
			$dy = $this->get('y')-$refpoint->get('y');
		}
		
		$length = sqrt($dx*$dx + $dy*$dy);
		// convert cell units to feet
		if ($units=='ft')
			$length *= 192/4096;
		
		$angle = atan2($dy, $dx);
		
		return $length;
	}
	
	function formatted_direction($refpoint) {
		//$orient = array('north', 'north-northeast', 'northeast', 'east-northeast', 'east', 'east-southeast', 'southeast', 'south-southeast', 'south', 'south-southwest', 'southwest', 'west-southwest', 'west', 'west-northwest', 'northwest', 'north-northwest', 'north');
		$orient = array('N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW', 'N');
		$length = $this->get_dist($refpoint, $dir);
		
		//  change from angle in radians (ccl) to degrees from north (cl);
		$dir = 90-$dir*180/pi();
		while ($dir<0)
			$dir+=360;
		$printdir = $orient[floor($dir/360*16+0.5)];
		$length = floor($length/10+0.5)*10;
		
		return "$length ft $printdir";
	}
	
	function formatted_location() {
		$cell = $this->_parentlist->get_item($this->get('cell_id'));
		if ($cell->get('worldtype')=='Test' || strpos($cell->get('name'), "Not A Test Cell")!==FALSE || substr($cell->get('edid'),0,9)=='Warehouse') {
			//if (!$cell->get('worldtype') || $cell->get('worldtype')=='Test') {
			$cell->release();
			return NULL;
		}
		
		$text = '';
		if ($this->get('base_edid')=='MapMarker' && $this->get('name')) {
			$name = $this->get('name');
		}
		else
			$name = $cell->get('name');
		if (empty($name) && $cell->get('worldtype')=='Other') {
			$name = $this->_parentlist->get_name($cell->get('world_id'));
		}
		if (empty($name)) {
			$marker_id = $cell->find_nearest_marker($this->ordid());
			
			if (!empty($marker_id)) {
				$marker = $this->_parentlist->get_item($marker_id);
				$text = $marker->formatted_location();
				if ($this->get_dist($marker)>75) {
					$text .= ", ".$this->formatted_direction($marker).' of marker';
					$text .= " (x=".round($this->get('x')).", y=".round($this->get('y')).")";
				}
			}
			else {
				$text = "At x=".round($this->get('x')).", y=".round($this->get('y'));
				print "No marker found for ".$this->get('ordid')." x=".$this->get('x')." y=".$this->get('y')."\n";
			}
		}
		else {
			$text = "[[$name]]";
			$ex = NULL;
			if ($this->get('base_edid')=='MapMarker') {
				$query = 'SELECT ordid FROM AllItems inner join LCTN_Record using (ordid) WHERE marker_id='.$this->ordid().' and mod_status&0x02>0';
				$res = $this->do_query($query);
				$lctn_id = NULL;
				while ($row=$this->row_query($res)) {
					$lctn_id = $row['ordid'];
				}
				if (!$lctn_id) {
					$query = "SELECT ordid FROM AllItems where rectype='LCTN' and name='".addslashes($this->get('name'))."' and mod_status&0x02>0";
					$res = $this->do_query($query);
					if ($res->num_rows==1) {
						$row = $this->row_query($res);
						$lctn_id = $row['ordid'];
					}
				}
				if ($lctn_id) {
					$lctn = $this->_parentlist->get_item($lctn_id);
					$ex = $lctn->get_city();
					if (!$ex || $ex==$name)
						$ex = $lctn->get_hold();
				}
			}
			if (empty($ex)) {
				$ex = $cell->get_city();
				if (!$ex || $ex==$name)
					$ex = $cell->get_hold();
			}
			if ($ex)
				$text .= ' ([['.$ex.']])';
		}
		
		$cell->release();
		return $text;
	}
}

class GenItem_ACHR extends GenItem_RefItem {}
class GenItem_ACRE extends GenItem_RefItem {}
class GenItem_REFR extends GenItem_RefItem {
	protected $_query_filter = 'name IS NOT NULL';
}

/* Basically just OB cell-related functions on assumption that FO/SR will behave similarly to OB
   MW overwrites many of these functions */
class GenItem_CELL extends GenItem {
	//	protected $_query_filter = 'name IS NOT NULL';
	
	function set_group_id($value) {
		$this->_data['parentcell_id'] = $value;
	}	
	
	public function exterior_any() {
		if ($this->get('exterior') || $this->get('fake exterior'))
			return true;
		else
			return false;
	}
	
	protected function derive_edid() {
		if (!is_null($this->get('group_id'))) {
			return $this->_parentlist->get_edid($this->get('group_id'));
		}
		return $this->_edid;
	}
	
	protected function derive_name() {
		if (!is_null($this->get('group_id'))) {
			return $this->_parentlist->get_name($this->get('group_id'));
		}
		// cannot use get('name') here!! recursively calls itself indefinitely.  But likely that there's no data if derive_name was even called
		if (array_key_exists('name', $this->_data))
			return $this->_data['name'];
		else
			return NULL;
	}
	
	protected function derive_exterior() {
		if (!($world=$this->get('world_id')) || is_null($world)) {
			return false;
		}
		else {
			return true;
		}
	}
	
	public function water_height() {
		if (!$this->get('haswater'))
			return NULL;
		else
			return $this->_data['water_height'];
	}
	
	/* Very generic version of this function
	   Will need to be customized for each game once worldspace names known */
	protected function derive_worldspace() {
		// Skipping interior cells because their data ends up being set by setup_cells based on
		// knowledge of the connected exterior cell
		// Trying to do it without knowing the exterior cell is pointless
		if (!$this->get('exterior'))
			return NULL;
		if (is_null($edid = $this->_parentlist->get_edid($this->get('world_id'))))
			return NULL;
		
		return "Other";
		
		return NULL;
	}
	
	/* Very generic version of this function
	   Will need to be customized for each game once worldspace names known */
	protected function derive_worldtype() {
		if (!$this->get('exterior'))
			return NULL;
		if (is_null($edid = $this->_parentlist->get_edid($this->get('world_id'))))
			return NULL;
		
		return "Other";
		
		return NULL;
	}
}


/* Functions common to NPC_ and CREA classes */
/* Eventually, many of the GenItem_NPC_ functions should probably be moved here */
class GenItem_Actor extends GenItem {
	public function set_pc_level($pc_level) {
		$this->_parentlist->set_pc_level($pc_level);
	}
	
	public function level() {
		if (!$this->get('offset'))
			return $this->get('level_raw');
		
		$pc_level = $this->_parentlist->get_pc_level();
		$level = $pc_level + $this->get('level_raw');
		if ($cmin=$this->get('calcmin'))
			$level = min($level, $cmin);
		if ($cmax=$this->get('calcmax'))
			$level = max($level, $cmax);
		return $level;
	}
}

class GenItem_CREA extends GenItem_Actor {
	public function refr_rectype() {
		return 'ACRE';
	}
}

// skill_raw, attrib_raw: values read from game data file.  Only relevant for non-autocalc NPCs
//   may be added to adj-values (for health, magicka), or otherwise manipulated to get final values
// skill0, attrib0: initial (level 0) values based on race, gender, class
// skill_adj, attrib_adj: adjustments based on abilities
//   can't be added into 0 values because magicka calc wants intell0 *without* intell_adj
// skill, attrib: base values for NPC (e.g., current values if not damaged, etc)

// skill_curr, attrib_curr: current values taking into damage, enchantments
//  (same as skill/attrib unless reading from save data... i.e., unless dealing with player stats)
// skill_exp: experience gained in skill (player only)

class GenItem_NPC_ extends GenItem_Actor {
	protected $_column_data =
		array('race_id' => array('display' => 'all', 'title' => 'Race', 'format' => 'ordid_as_name'),
		      'female' => array('display' => 'all', 'title' => 'Gender', 'format' => 'gender'),
		      'class_id' => array('display' => 'all', 'title' => 'Class', 'format' => 'ordid_as_name'),
		      'numACHR' => array('display' => 'details_html'),
		      'ACHR' => array('display' => 'all', 'title' => 'ACHR (if unique)', 'format' => 'ordid_as_name'),
		      'numLevel' => array('display' => 'details_html'),
		     );
	protected $_display_filters =
		array('race' => array('title' => 'Race', 'rectype' => 'RACE'),
		      'female' => array('title' => 'Gender', 'options' => array(0 => 'Male', 1 => 'Female')),
		      'class' => array('title' => 'Class', 'title_pl' => 'Classes', 'rectype' => 'CLAS'),
		     );
	protected $_classatt;
	protected $_attfac;
	protected $_classfac;
	protected $_is_player = false;
	
	public function refr_rectype() {
		return 'ACHR';
	}
	public function set_player($is_player) {
		$this->_is_player=$is_player;
	}
	public function is_player() {
		return $this->_is_player;
	}
	public function leveled() {
		return $this->_data['offset'] && $this->_data['autocalc'];
	}
	
	protected function _getderived($valname, $index=NULL) {
		// check whether param is an attrib, a skill, [attrib0], [skill0]
		// if so, call appropriate function
		if (substr($valname,-1,1)=='0') {
			$basename = substr($valname,0,-1);
			$exname = '0';
		}
		elseif (substr($valname,-4,4)=='_adj') {
			$basename = substr($valname,0,-4);
			$exname = '_adj';
		}
		elseif (substr($valname,-5,5)=='_delt') {
			$basename = substr($valname,0,-5);
			$exname = '_delt';
		}
		elseif (substr($valname,-5,5)=='_text') {
			$basename = substr($valname,0,-5);
			$exname = '_text';
		}
		elseif (substr($valname,-5,5)=='_exp_pct') {
			$basename = substr($valname,0,-5);
			$exname = '_exp_pct';
		}
		else {
			$basename = $valname;
			$exname = '';
		}
		$function = NULL;
		if (!isset($this->_parentlist)) {
			print "parentlist not set\n";
			var_dump($this->_edid);
			var_dump($this->_formid);
			var_dump($this->_ordid);
			var_dump($this->_rectype);
			var_dump($this->_data);
		}
		if (!is_null($this->_parentlist->get_lookup_id('attriblist', $basename, false))) {
			$function = 'attrib'.$exname;
		}
		elseif (!is_null($this->_parentlist->get_lookup_id('skilllist', $basename, false))) {
			$function = 'skill'.$exname;
		}
		elseif ($basename=='health' || $basename=='magicka' || $basename=='fatigue') {
			if (method_exists($this, $valname))
				return $this->$valname();
			$function = 'miscstat'.$exname;
		}
		if (isset($function)) {
			if (method_exists($this, $function))
				return $this->$function($basename);
			$function = 'miscstat'.$exname;
			if (method_exists($this, $function))
				return $this->$function($basename);
		}
		return parent::_getderived($valname, $index);
	}
	
	public function gender() {
		if ($this->get('female'))
			return 'Female';
		else
			return 'Male';
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
		
		$skill_items = array();
		if ($this->get('autocalc')) {
			$attcount = array();
			$this->_classatt = array();
			$this->_attfac = array();
			
			foreach ($this->_parentlist->get_lookup_list('attriblist') as $attrib) {
				$attrib = strtolower(str_replace(' ', '_', $attrib));
				// this is just the initialization ... will be augmented as appropriate by class and abilities
				$this->_data[$attrib."0"] = $race->get($attrib.$gender);
				$attcount[$attrib] = 0;
				$this->_classatt[$attrib] = false;
			}
			for ($i=1; $i<=2; $i++) {
				$attrib = strtolower(str_replace(' ', '_', $class->get('att'.$i)));
				$this->_classatt[$attrib] = true;
				if ($attrib != 'personality')
					$this->_data[$attrib."0"] += 5;
			}
			
			$spec = $class->get('spec');
			foreach ($this->_parentlist->get_lookup_list('skilllist') as $skill) {
				$item = $this->_parentlist->get_item_from_name($skill, 'SKIL');
				$skill = strtolower(str_replace(' ', '_', $skill));
				$skill_items[$skill] = $item;
				// this is just the initialization ... will be augmented as appropriate by class and abilities
				$this->_data[$skill."0"] = 5;
				$this->_data[$skill."_type"] = "misc";
				$this->_data[$skill."_spec"] = false;
				if ($skill_items[$skill]->get('spec')==$spec) {
					$this->_data[$skill."0"] += 5;
					$this->_data[$skill."_spec"] = true;
				}
			}
		// racial skill bonuses
			for ($i=1; $i<=7; $i++) {
				$skill = $race->get('skl'.$i);
				if (is_null($skill) || $skill=='' || is_int($skill))
					continue;
				$skill = strtolower(str_replace(' ', '_', $skill));
				$this->_data[$skill."0"] += $race->get('sklbonus'.$i);
			}
		// class skill bonuses
		// OB-style skills
			for ($i=1; $i<=7; $i++) {
				$skill = $class->get('skl'.$i);
				if (is_null($skill) || $skill=='' || is_int($skill))
					continue;
				$skill = strtolower(str_replace(' ', '_', $skill));
				
				$skattrib = strtolower(str_replace(' ', '_', $skill_items[$skill]->get('attrib')));
				$attcount[$skattrib]++;
				
				$this->_data[$skill."0"] += 20;
				$this->_data[$skill."_type"] = "major";
			}
		// MW-style skills
			foreach (array('a', 'b') as $let) {
				for ($i=1; $i<=5; $i++) {
					$skill = $class->get('skl'.$i.$let);
					if (is_null($skill) || $skill=='' || is_int($skill))
						continue;
					$skill = strtolower(str_replace(' ', '_', $skill));
					if ($let=='a') {
						$this->_data[$skill."0"] += 25;
						$this->_data[$skill."_type"] = "major";
					}
					else {
						$this->_data[$skill."0"] += 10;
						$this->_data[$skill."_type"] = "minor";
					}
				}
			}
		}
		else {
			foreach ($this->_parentlist->get_lookup_list('attriblist') as $attrib) {
				$attrib = strtolower(str_replace(' ', '_', $attrib));
				// this is just the initialization ... will be augmented as appropriate by class and abilities
				$this->_data[$attrib."0"] = 0;
			}
		}
		
		$this->_data['fmm'] = 0;
		
		foreach (array($this, $race) as $base) {
			foreach ($base->indices('spell_id') as $index) {
				$spell = $this->_parentlist->get_item($base->get('spell_id', $index));
				if ($spell->get('type')!='Ability')
					continue;
				foreach ($spell->indices('effect_id') as $spindex) {
					$mgef = $spell->get('mgef_specific_name', $spindex);
					if (preg_match('/Fortify\s+(.*)/', $mgef, $matches)) {
						$attrib = strtolower(str_replace(' ', '_', $matches[1]));
						$mag = $spell->get('eff_mag', $spindex);
						//						print "mgef=$mgef attrib=$attrib mag=$mag spell=".$spell->get('ordid')."<br />\n";
						if ($mgef=='Fortify Maximum Magicka')
							$this->_data['fmm'] += $mag;
						else
							@ $this->_data[$attrib."_adj"] += $mag;
					}
				}
				$spell->release();
			}
		}
		
		if ($this->get('autocalc')) {
			foreach ($this->_parentlist->get_lookup_list('attriblist') as $attrib) {
				$attrib = strtolower(str_replace(' ', '_', $attrib));
				$this->_attfac[$attrib] = 0.6 + $attcount[$attrib]*0.8;
			}
			// always 0 for luck and personality
			$this->_attfac['luck'] = 0;
			$this->_attfac['personality'] = 0;
			if ($spec=='Combat')
				$this->_classfac = 5;
			elseif ($spec=='Stealth')
				$this->_classfac = 4;
			else
				$this->_classfac = 3;
			$this->_classfac += ($this->_classatt['endurance'] ? 1 : 0);
		}
		
		$race->release();
		$class->release();
		$this->_release_items($skill_items);
	}
	
	public function fatigue() {
		// don't even check autocalc here: need to include any possible attribute bonuses from class/race/whatever, so easiest to just calculate the value
		// BUT probably should *not* include any attrib_adj adjustments!
		$value = $this->get('fatigue_adj');
		foreach (array('endurance', 'agility', 'strength', 'willpower') as $attrib) {
			$value += $this->get($attrib) - $this->get($attrib.'_adj');
		}
		return $value;
	}
	
	public function miscstat_adj($miscstat) {
		if (!isset($this->_data[$miscstat.'_adj']))
			$this->init_raceclass_data();
		return $this->_getexact($miscstat.'_adj');
	}
	
	public function miscstat0($miscstat) {
		if (!isset($this->_data[$miscstat.'0']))
			$this->init_raceclass_data();
		return $this->_getexact($miscstat.'0');
	}
	
	public function height() {
		if (!array_key_exists('height', $this->_data))
			$this->init_raceclass_data();
		return $this->_getexact('height');
	}
	public function weight() {
		if (!array_key_exists('weight', $this->_data))
			$this->init_raceclass_data();
		return $this->_getexact('weight');
	}
	
	public function skill($skill) {
		if (!$this->get('autocalc'))
			return $this->_getexact($skill.'_raw');
		
		$value = $this->get($skill.'0') + $this->get($skill.'_adj');
		$level = $this->get('level');
		$delt = $this->skill_delt($skill);
		// add processing
		$value = min(100, floor($value + ($level-1)*$delt + 0.5));
		return $value;
	}
	
	public function skill_delt($skill) {
		if (!$this->get('autocalc'))
			return 0;
		if ($this->_data[$skill."_type"]=='misc')
			$delt = 0.1;
		else
			$delt = 1;
		if ($this->_data[$skill."_spec"])
			$delt += 0.5;
		return $delt;
	}
	
	// OB-specific (or likely to be)
	public function magicka() {
		if (!$this->get('autocalc'))
			return $this->_getexact('magicka_raw') + $this->get('magicka_adj');
		
		// subtract out any adjustments -> they're not taken into account in magicka calc
		$intel = $this->get('intelligence') - $this->get('intelligence_adj');
		// Confirmed (specifically for Puny Ancus, 0x000165ed) that this is *floor* (don't want to add 0.5 first)
		$magicka = floor(2.5*$intel) + $this->get('magicka_adj');
		return $magicka;
	}
	
	public function magicka_delt() {
		if (!$this->get('autocalc'))
			return 0;
		return 2.5*$this->attrib_delt('intelligence');
	}
	
	public function health() {
		if (!$this->get('autocalc'))
			return $this->_getexact('health_raw') + $this->get('health_adj');
		
		$levelfac = array(NULL, 0.4, 0.55, 0.7, 0.85);
		$level = $this->get('level');
		
		// subtract out any adjustments -> they're not taken into account in magicka calc
		$asum = $this->get('strength') + $this->get('endurance') - $this->get('strength_adj') - $this->get('endurance_adj');
		$health = floor($asum/2) + ($level-1)*$this->_classfac;
		if (array_key_exists($level, $levelfac) && !is_null($levelfac[$level]))
			$health = floor($health*$levelfac[$level]);
		$health += $this->get('health_adj');
		return $health;
	}
	
	public function health_delt() {
		if (!$this->get('autocalc'))
			return 0;
		return ($this->attrib_delt('strength')+$this->attrib_delt('endurance'))/2 + $this->_classfac;
	}
	
	public function attrib($attrib) {
		if (!$this->get('autocalc'))
			return $this->_getexact($attrib."_raw") + $this->get($attrib."0");
		
		$value = $this->get($attrib.'0') + $this->get($attrib.'_adj');
		if ($attrib=='personality')
			$value;
		
		$level = $this->get('level');
		$value = min(100, floor($value + ($level-1)*$this->_attfac[$attrib] + 0.5));
		return $value;
	}
	
	public function attrib_delt($attrib) {
		if (!$this->get('autocalc'))
			return 0;
		if ($attrib=='personality')
			return 0;
		return $this->_attfac[$attrib];
	}
	
	public function skill_exp_pct($skill) {
		$exp = $this->_getexact($skill.'_exp');
		$exp_pct = $exp / $this->experience_needed($skill) * 100;
		return $exp_pct;
	}
	
	public function experience_needed($skill) {
		return NULL;
	}
	
	public function numACHR() {
		$query = "SELECT COUNT(*) AS count FROM ACHR_Record WHERE base_id=".$this->get('ordid');
		$row = $this->_parentlist->do_query($query, 'onerow');
		return $row['count'];
	}
	
	public function ACHR() {
		$query = "SELECT ordid FROM ACHR_Record WHERE base_id=".$this->get('ordid');
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
	
	public function numLevel() {
		if (!$this->_parentlist->hastable('LevelList'))
			return NULL;
		$query = "SELECT COUNT(*) AS count FROM LevelList WHERE item_ordid=".$this->get('ordid');
		$row = $this->_parentlist->do_query($query, 'onerow');
		return $row['count'];
	}
}

/* 
   ENCHANTMENTS
*/
class GenItem_Magic extends GenItem_GenMagic {
	function get_isenchanted() {
		// enchant_id used to be ENAM
		$nench = 0;
		$eid = $this->get('enchant_id');
		if (!empty($eid))
			$nench = 1;
		if (stristr($this->edid(), 'unique'))
			$nench += 100;
		return $nench;
	}
	
	// $name_only option is used by effect_desc
	function magic_desc($name_only=FALSE) {
		$iemax = $this->get_index('effect_id');
		$enchitems = array();
		if ($iemax!==FALSE)
			$enchitems = array($this);
		else {
			$eids = $this->get('enchant_id');
			if (!empty($eids)) {
				if (is_array($eids)) {
					$enchitems = array();
					foreach ($eids as $eid) 
						$enchitems[] = $this->_parentlist->get_item($eid);
				}
				else {
					$enchitems = array($this->get_as_item('enchant_id'));
				}
			}
		}
		
		if (empty($enchitems))
			return '';
		
		$output = '';
		foreach ($enchitems as $eitem) {
			if ($output != '')
				$output .= "\n";
			if (!$name_only) {
				$t = $eitem->get('type');
				if (!empty($t)) {
					$output .= $eitem->get('type').": ";
				}
			}
			$first = TRUE;
			foreach ($eitem->indices('effect_id') as $ie) {
				if (!$first)
					$output .= "; ";
				else
					$first = FALSE;
				if (!$name_only) {
					$output .= $eitem->get('magic_desc_line', $ie);
				}
				else {
					$output .= $this->get('mgef_specific_name', $ie);
				}
			}
		}
		$this->_release_items($enchitems);
		return $output;
	}
	
	function magic_desc_line($index) {
		$mgef = $this->get_as_item('effect_id', $index);
		$ename = $this->get('mgef_specific_name', $index);
		$lineout = $ename.",";
		$mag = $this->get('eff_mag', $index);
		if ($mag && $mgef) {
			$lineout .= " ".$mgef->units($mag);
			if ($mag=='1' && substr($lineout,-1)=='s')
				$lineout = substr($lineout,0,-1);
		}
		
		if (($dur=$this->get('eff_dur', $index))) {
			$lineout .= " for ".$dur." sec";
			if ($dur>1)
				$lineout .= "s";
		}
		if (($area=$this->get('eff_area', $index)))
			$lineout .= " in ".$area." ft";
		$range = $this->get('eff_range', $index);
		if (!empty($range))
			$lineout .= " on ".$range;
		
		if (substr($lineout,-1)==',')
			$lineout = substr($lineout,0,-1);
		
		if ($mgef)
			$mgef->release();
		
		// game specific extra details (from parent class, though, because of funky inheritance order)
		$lineout = $this->magic_desc_line_extra($index, $lineout);
		return $lineout;
	}
	
	// Provide just a list of effect names -- without mag/duration/etc.
	// In particular, written for ingredients
	// But needs special treatment in MW where each effect is a separate column
	function effect_desc() {
		return $this->magic_desc(TRUE);
	}
	
	function units($mag) {
		if ($this->get('magpct'))
			$unit = "%";
		else {
			$unit = " pt";
			if ($mag!='1')
				$unit .= "s";
		}
		return $mag.$unit;
	}
	
	public function charge_uses() {
		// for MW records, enchantcharge is part of individual ENCH record --> $chargeb
		// for OB records, enchantcharge is part of WEAP record -> $charge
		$charge = $this->get('enchantcharge');
		$enchants = $this->get('enchant_id');
		if (empty($enchants))
			return NULL;
		if (!is_array($enchants))
			$enchants = array($enchants);
		$cost = $chargeb = 0;
		foreach ($enchants as $eid) {
			$titem = $this->_parentlist->get_t_item($eid);
			$chargeb += $titem->get('enchantcharge');
			$cost += $titem->get('enchantcost');
		}
		if (!$charge)
			$charge = $chargeb;
		if (!$cost || !$charge)
			return NULL;
		$nuse = floor($charge/$cost);
		$prt_cost = preg_replace('/\.0$/', '', sprintf('%.1f', $cost));
		
		return $charge.'/'.$prt_cost.'='.$nuse;
	}
}

/* functions in classes are here to remind myself what functions I'm expecting to be present
   in various rectype-specific classes */
class GenItem_MGEF extends GenItem_Magic {
	protected $_column_data =
		array('school' => array('display' => 'all',
		                        ),
		      'basecost' => array('title' => 'Base Cost',
		                          'display' => 'all',
		                         ),
		     );
	protected $_display_filters =
		array('school' => array('title' => 'School'));
}
class GenItem_ENCH extends GenItem_Magic {
	// enchantcost is the cost in magicka of using the enchantment
	// enchantvalue is the gold value added because of the enchantment
	protected $_column_data =
		array('enchantcost' => array('display' => 'all', 'title' => 'Enchant Cost'),
		      'charge' => array('display' => 'all'),
		      'magic_desc' => array('display' => 'all'),
		     );
	protected $_display_filters =
		array('type' => array('title' => 'Type'));
	
	public function enchantcost() {
		if (!$this->get('autocalc'))
			return $this->get('enchantcost_raw');
		if (!array_key_exists('enchantcost_raw', $this->_data) && $this->_getexact('enchantcost'))
	      return $this->_getexact('enchantcost');
		
		$cost = 0;
		foreach ($this->indices('effect_id') as $i) {
			$cost += $this->get('effect_cost', $i);
		}
		return $cost;
	}
	
	// Gold value added by enchantment
	// Note: only relevant for constant-value (clothing) enchantments
	// Weapon values are based on enchantcost
	public function enchantvalue() {
		if (!$this->get('autocalc')) {
			return $this->get('enchantvalue_raw');
		}
		$value = 0;
		foreach ($this->indices('effect_id') as $i) {
			$value += $this->get('effect_value', $i);
		}
		return $value;
	}
}

class GenItem_ALCH extends GenItem_Magic {
	protected $_column_data =
		array('magic_desc' => array('display' => 'all'),
		     );
	protected $_display_filters =
		array('food' => array('title' => 'Type', 'options' => array(0 => 'Potion', 1 => 'Food'))
		);
	
	// Override derive_value
	// Needs to do calc by summing over effect_id *not* enchant_id
	function derive_value() {
		// short-circuit for sake of MW
		$value = $this->_getexact('value');
		if (!empty($value))
			return $value;
		
		$auto = $this->get('autocalc');
		// if autocalc set do *not* include value_raw
		if (!empty($auto))
			$base_value = 0;
		else
			return $this->get('value_raw');
		
		$effects = $this->get('effect_id');
		if (empty($effects))
			return $base_value;
		if (!is_array($effects))
			$effects = array($effects);
		foreach ($effects as $i => $eid) {
			$base_value += $this->get('effect_cost', $i);
		}
		return floor($base_value);
	}
}
// Note that AMMO does not exist in MW -> all listed as WEAP
class GenItem_AMMO extends GenItem_Magic {
	protected $_column_data =
		array('damage' => array('display' => 'all'),
		      'magic_desc' => array('display' => 'all'),
		      'plain_item_ordid' => array('display' => array('list_wiki', 'list_csv', 'details_html')),
		     );
	protected $_display_filters =
		array('countENAM' => array('title' => 'Enchanted', 'title_pl' => 'Ench. Statuses', 'options' => array(0 => 'Unenchanted Items', 1 => 'Enchanted Items'), 'sql'=> array(0 => '=0', 1 => '> 0')));
}
class GenItem_ARMO extends GenItem_Magic {
	// This is added to by MWItem_ARMO
	protected $_column_data =
		array('rating' => array('display' => 'all'),
		      'type' => array('display' => array('list_html', 'list_csv', 'details_html')),
		      'skill' => array('display' => array('list_html', 'list_csv', 'details_html')),
		      'magic_desc' => array('display' => 'all'),
		      'plain_item_ordid' => array('display' => array('list_wiki', 'list_csv', 'details_html')),
		     );
	// add enchanted/unenchanted flag as filter
	protected $_display_filters =
		array('skill' => array('title' => 'Skill'),
		      'type' => array('title' => 'Type'),
		      'countENAM' => array('title' => 'Enchanted', 'title_pl' => 'Ench. Statuses', 'options' => array(0 => 'Unenchanted Items', 1 => 'Enchanted Items'), 'sql'=> array(0 => '=0', 1 => '> 0')),
		      'levellist_id' => array('title' => 'Leveled', 'title_pl' => 'Leveled Statuses', 'options' => array(0 => 'Non-leveled Items', 1 => 'Leveled Items'), 'sql' => array(0 => 'IS NULL', 1 => 'IS NOT NULL')),
		     );
	protected function derive_skill() {}
}
class GenItem_BOOK extends GenItem_Magic {
	protected $_column_data =
		array('skill' => array('display' => 'all'),
		      'magic_desc' => array('display' => 'all'),
		      'DESC' => array('format' => 'pre_tags'),
		     );
	protected $_display_filters =
		array('skill' => array('title' => 'Skill'),
		      'countENAM' => array('title' => 'Enchanted', 'title_pl' => 'Ench. Statuses', 'options' => array(0 => 'Unenchanted Items', 1 => 'Enchanted Items'), 'sql'=> array(0 => '=0', 1 => '> 0')),
		      'scroll' => array('title' => 'Type', 'options' => array(0 => 'Non-scrolls', 1 => 'Scrolls')));
}
class GenItem_CLOT extends GenItem_Magic {
	protected $_column_data =
		array('magic_desc' => array('display' => 'all'),
		      'type' => array('display' => array('list_html', 'list_csv', 'details_html')),
		      'plain_item_ordid' => array('display' => array('list_wiki', 'list_csv', 'details_html')),
		     );
	protected $_display_filters =
		array('type' => array('title' => 'Type'),
		      'countENAM' => array('title' => 'Enchanted', 'title_pl' => 'Ench. Statuses', 'options' => array(0 => 'Unenchanted Items', 1 => 'Enchanted Items'), 'sql'=> array(0 => '=0', 1 => '>0')),
		      'levellist_id' => array('title' => 'Leveled', 'title_pl' => 'Leveled Statuses', 'options' => array(0 => 'Non-leveled Items', 1 => 'Leveled Items'), 'sql' => array(0 => 'IS NULL', 1 => 'IS NOT NULL')),
		     );
}
// Don't want to do standard magic_desc for INGR
// Results in full mag/duration/etc being printed for each effect, even though those values are meaningless
// *BUT* I do still want Magic's merge-skill/attrib-into-effect-name feature
class GenItem_INGR extends GenItem_Magic {
	protected $_column_data =
		array('effect_desc' => array('display' => array('list_html', 'list_csv', 'details_html'),
		                             'title' => 'Effects',
		                             /*'vars' => array('eff1', 'eff2', 'eff3', 'eff4', 'skl1', 'skl2', 'skl3', 'skl4', 'att1', 'att2', 'att3', 'att4', 'effect_id', 'eff_mag', 'eff_magmin', 'eff_magmax', 'eff_dur', 'eff_area', 'eff_range', 'eff_act', 'effectb_id')*/),
		      'eff1' => array('display' => 'list_wiki', 'title' => 'Primary Effect', 'format' => 'ingr_eff', 'display_option' => array('list_wiki' => array('location' => 2))),
		      'eff2' => array('display' => 'list_wiki', 'title' => 'Secondary Effect', 'format' => 'ingr_eff', 'display_option' => array('list_wiki' => array('location' => 3))),
		      'eff3' => array('display' => 'list_wiki', 'title' => 'Tertiary Effect', 'format' => 'ingr_eff', 'display_option' => array('list_wiki' => array('location' => 4))),
		      'eff4' => array('display' => 'list_wiki', 'title' => 'Quaternary Effect', 'format' => 'ingr_eff', 'display_option' => array('list_wiki' => array('location' => 5))),
		     );
	public function eff1() {
		return $this->get('effect_id', 0);
	}
	public function eff2() {
		return $this->get('effect_id', 1);
	}
	public function eff3() {
		return $this->get('effect_id', 2);
	}
	public function eff4() {
		return $this->get('effect_id', 3);
	}
}
class GenItem_MISC extends GenItem_Magic {
	protected $_column_data =
		array('magic_desc' => array('display' => 'all'),
		     );
}

class GenItem_WEAP extends GenItem_Magic {
	// MW adds to this column_data
	protected $_column_data =
		array('damage' => array('display' => 'all'),
		      'type' => array('display' => array('list_html', 'list_csv', 'details_html')),
		      'magic_desc' => array('display' => 'all'),
		      'plain_item_ordid' => array('display' => array('list_wiki', 'list_csv', 'details_html')),
		      'speed' => array('display' => array('list_wiki', 'list_csv'),
		                       'display_option' => array('list_wiki' => array('location' => 6))),
		      'reach' => array('display' => array('list_wiki', 'list_csv'),
		                       'display_option' => array('list_wiki' => array('location' => 7))),
		      'charge_uses' => array('display' => array('list_wiki', 'list_csv', 'details_html')),
		     );
	// SR replaces countENAM with countEITM
	protected $_display_filters =
		array('type' => array('title' => 'Type'),
		      'countENAM' => array('title' => 'Enchanted', 'title_pl' => 'Ench. Statuses', 'options' => array(0 => 'Unenchanted Items', 1 => 'Enchanted Items'), 'sql'=> array(0 => '=0', 1 => '>0')),
		      'levellist_id' => array('title' => 'Leveled', 'title_pl' => 'Leveled Statuses', 'options' => array(0 => 'Non-leveled Items', 1 => 'Leveled Items'), 'sql' => array(0 => 'IS NULL', 1 => 'IS NOT NULL')),
		     );
	protected function derive_skill() {}
}

class GenItem_SGST extends GenItem_Magic {
	protected $_column_data =
		array('magic_desc' => array('display' => 'all'),
		     );
}
class GenItem_SPEL extends GenItem_Magic {
	protected $_column_data =
		array('spellcost' => array('display' => 'all',
		                           'display_option' => array('list_wiki' => array('location' => -3)),
		                           'title' => 'Magicka Cost',
					   'vars' => array('spellcost_raw')),
		      // spelllevel doesn't exist for MW
		      'spelllevel' => array('display' => array('all'),
		                            'display_option' => array('list_wiki' => array('location' => -1)),
		                            'title' => 'Skill level'),
		      'spellschool' => array('display' => array('list_html', 'list_csv', 'details_html', 'details_csv'),
		                             'title' => 'School'),
		      'magic_desc' => array('display' => 'all',
		                            'title' => 'Effects',
		                            'display_option' => array('list_wiki' => array('location' => -4))),
		      'source' => array('display' => array('details_html', 'list_wiki'),
		                        'display_option' => array('list_wiki' => array('location' => -2)),
		                        'title' => 'Where to get',
		                        'format' => 'spell_source'),
		     );
	protected $_display_filters =
		array('type' => array('title' => 'Type'),
		      'spellschool' => array('title' => 'School'));
	
	// Another OB-specific school, being put here for lack of anything better for MW
	function derive_spellschool() {
		$school_costs = array();
		foreach ($this->indices('effect_id') as $index) {
			$scriptschool = $this->get('school', $index);
			if (!empty($scriptschool)) {
				@ $school_costs[$scriptschool] += 0;
				continue;
			}
			$eid = $this->get('effect_id', $index);
			if (!is_object($effect = $this->_parentlist->get_item($eid)))
				continue;
			$school = $effect->get('school');
			$cost = $this->get('effect_cost', $index);
			@ $school_costs[$school] += $cost;
			$effect->release($effect);
		}
		$retval = array_shift(array_keys($school_costs));
		return $retval;
	}
	
	// Get list of all vendors for this spell
	// Would be good to also include any spell tomes that provide spell, except in OB spell tomes are done
	// by script, making a search somewhat difficult
	// Hopefully SR will add a built-in mechanism for supporting spell tomes
	// MW overrides this function, because record is NPCS instead of SPLO
	function source() {
		$query = 'SELECT ordid, name FROM AllItems INNER JOIN NPC__Record USING (ordid) INNER JOIN NPC__SPLO_Record USING (ordid) WHERE mercspel=1 AND spell_id='.$this->get('ordid').'  and mod_status&0x02>0 ORDER BY name';
		$res = $this->_parentlist->do_query($query);
		$ordids = array();
		while ($row=$this->_parentlist->row_query($res)) {
			$ordids[] = $row['ordid'];
		}
		return implode(' ', $ordids);
	}
}

/* Non-magic */
class GenItem_APPA extends GenItem {
	protected $_column_data =
		array('quality' => array('display' => 'all'),
		      );
}
class GenItem_CONT extends GenItem {
	protected $_column_data =
		array('respawn' => array('display' => 'all'),
		     );
}
class GenItem_FACT extends GenItem {
	// evil entries are removed by MWItem_FACT
	protected $_column_data =
		array('hidden' => array('display' => 'all'),
		      'evil' => array('display' => 'all'),
		      'member' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_member'), 'format' => 'ordid_as_name'),
		      'member_rank' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_member')),
		     );
	protected $_display_filters =
		array('hidden' => array('title' => 'Hidden', 'title_pl' => 'Hidden Values', 'options' => array(0 => 'Not Hidden', 1 => 'Hidden')),
		      'evil' => array('title' => 'Evil', 'title_pl' => 'Evil Values', 'options' => array(0 => 'Not Evil', 1 => 'Evil')));
	
	// this is effectively an initialization function for members
	public function get_index_member() {
		// doesn't exist in MW
		if (!$this->_parentlist->hastable('NPC__SNAM_Record'))
			return FALSE;
		if (!isset($this->_data['member'])) {
			$this->_data['member'] = array();
			$this->_data['member_rank'] = array();
			$query = 'SELECT ordid, faction_rank FROM AllItems inner join NPC__SNAM_Record using (ordid) WHERE faction_id='.$this->get('ordid').' and mod_status&0x02>0';
			$res = $this->_parentlist->do_query($query);
			while ($row=$this->_parentlist->row_query($res)) {
				$this->_data['member'][] = $row['ordid'];
				$this->_data['member_rank'][] = $row['faction_rank'];
			}
		}
		if (empty($this->_data['member']))
			return FALSE;
		return count($this->_data['member'])-1;
	}
	
}
class GenItem_FLOR extends GenItem {
	protected $_column_data =
		array('ingr_id' => array('display' => 'all', 'title' => 'Ingredient', 'format' => array('ordid_as_name')),
		     );
}
class GenItem_GLOB extends GenItem {
	protected $_column_data =
		array('FLTV' => array('display' => 'all', 'title' => 'Value'),
		     );
}
class GenItem_GMST extends GenItem {
	protected $_column_data =
		array('DATA' => array('display' => 'all', 'title' => 'Value'),
		     );
}
class GenItem_SLGM extends GenItem {
	protected $_column_data =
		array('soul_contained' => array('title' => 'Soul',
		                                'display' => 'all',
		                                'display_option' => array('list_wiki' => array('location' => 1))),
		      'soul_capacity' => array('title' => 'Capacity',
		                               'display' => 'all',
		                               'display_option' => array('list_wiki' => array('location' => -1))),
		     );
}

// Special functions to handle PLDT/PTDT values that are either formids OR object types
// 
class GenItem_PACK extends GenItem {
	protected $_column_data =
		array('PLDT_type' => array('title' => 'Location Type'),
		      'location' => array('format' => array('ordid_or_type')),
		      'PTDT_type' => array('title' => 'Target Type'),
		      'target' => array('format' => array('ordid_or_type')),
		      'month' => array('format' => array('pack_timeval')),
		      'date' => array('format' => array('pack_date')),
		      'time' => array('format' => array('pack_timeval')),
		      'hour' => array('format' => array('pack_timeval')),
		      'minute' => array('format' => array('pack_timeval')),
		     );
}
