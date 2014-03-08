<?php
require_once 'csdata.inc';
require_once 'ob_genitem.inc';
require_once 'genitem.inc';

class OBData extends CSData {
	protected $_namespace = 'Oblivion';
	function __construct() {
		parent::__construct();
	}
}

/* 
   OBItem
*/

class OBItem_ARMO extends GenItem_ARMO {
	protected function derive_skill() {
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
		}
		return 'Other';
	}
}

class OBItem_CLOT extends GenItem_CLOT {
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

class OBItem_CELL extends GenItem_CELL {
	protected function derive_worldspace() {
		if (!$this->get('exterior'))
			return NULL;
		if (is_null($edid = $this->_parentlist->get_edid($this->get('world_id'))))
			return NULL;
		
		if (preg_match('/Tamriel/', $edid))
			return "Tamriel";
		if ((preg_match('/^IC/', $edid) && !preg_match('/MQ16/', $edid)) ||
		    preg_match('/^(Anvil|Bravil|Bruma|Cheydinhal|Chorrol|Leyawiin|Skingrad).*World$/', $edid) ||
		    preg_match('/Kvatch(Entrance|East|Plaza)/', $edid))
			return "Tamriel";
		if (preg_match('/TGTempleOfTheEmperorZero/', $edid))
			return "Tamriel";
		
		if (preg_match('/SE(World|TheFringe)/', $edid))
			return "Shivering Isles";
		if (preg_match('/SENS(Bliss|Crucible|Palace)/', $edid) ||
		    preg_match('/SE(Mania|Dementia)Garden/', $edid))
			return "Shivering Isles";
		if (preg_match('/SEVitharn/', $edid))
			return "Shivering Isles";
		
		if (preg_match('/ms14|oblivion|peryite|boethia/i', $edid))
			return "Oblivion";
		if (preg_match('/paradise/i', $edid))
			return "Paradise";
		if (preg_match('/palepass/i', $edid))
			return "Pale Pass";
		if (preg_match('/ms37world/i', $edid))
			return "Frostfire Glade";
		
		if (preg_match('/arkvedstower|dreamworld|ms14world/i', $edid))
			return "Other";
		
		return NULL;
	}
	
	protected function derive_worldtype() {
		if (!$this->get('exterior'))
			return NULL;
		if (is_null($edid = $this->_parentlist->get_edid($this->get('world_id'))))
			return NULL;
		
		if (preg_match('/Tamriel/', $edid))
			return "Main";
		if ((preg_match('/^IC/', $edid) && !preg_match('/MQ16/', $edid)) ||
		    preg_match('/^(Anvil|Bravil|Bruma|Cheydinhal|Chorrol|Leyawiin|Skingrad).*World$/', $edid) ||
		    preg_match('/Kvatch(Entrance|East|Plaza)/', $edid))
			return "City";
		if (preg_match('/TGTempleOfTheEmperorZero/', $edid))
			return "Submap";
		
		if (preg_match('/SE(World|TheFringe)/', $edid))
			return "Main";
		if (preg_match('/SENS(Bliss|Crucible|Palace)/', $edid) ||
		    preg_match('/SE(Mania|Dementia)Garden/', $edid))
			return "City";
		if (preg_match('/SEVitharn/', $edid))
			return "Submap";
		
		if (preg_match('/ms14|oblivion|peryite|boethia/i', $edid))
			return "Other";
		if (preg_match('/paradise/i', $edid))
			return "Other";
		if (preg_match('/palepass/i', $edid))
			return "Other";
		if (preg_match('/ms37world/i', $edid))
			return "Other";
		
		if (preg_match('/arkvedstower|dreamworld|ms14world/i', $edid))
			return "Other";
		
		return NULL;
	}
}

class OBItem_DIAL extends GenItem {
        protected $_column_data =
                array(
                       'dialogue' => array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name'),
                     );

        public function _get_index_dialogue_shared() {
                if (!isset($this->_data['dialogue'])) {
                        $this->_data['dialogue'] = array();

                        $query = "SELECT ordid FROM AllItems inner join INFO_Record using (ordid) WHERE group_id=".$this->get('ordid').' and mod_status&0x02>0';
                        $res = $this->do_query($query);
                        while ($row=$this->row_query($res)) {
                                $this->_data['dialogue'][] = $row['ordid'];
                        }
                }
                if (empty($this->_data['dialogue']))
                        return FALSE;
                return count($this->_data['dialogue'])-1;
        }

        public function get_index_dialogue() {
                return $this->_get_index_dialogue_shared();
        }
}


class OBItem_MGEF extends GenItem_MGEF {
	function specific_name($skillname, $attribname) {
		$ename = $this->get('name');
		
		if ($skillname)
			$ename = preg_replace('/\s+Skill/', ' '.$skillname, $ename);
		elseif ($attribname)
			$ename = preg_replace('/\s+Attribute/', ' '.$attribname, $ename);
		
		return $ename;
	}
	
	function units($mag) {
		// most of this has not been tested
		$ename = $this->get('name');
		if (preg_match('/Calm|Command|Demoralize|Frenzy|Undead/', $ename)) {
			$mag = floor($mag/4);
			return "up to level ".$mag;
		}
		elseif (preg_match('/Open|Lock/', $ename)) {
			$locklevels = array('Very Easy' => 7, 'Easy' => 20, 'Average' => 40, 'Hard' => 80, 'Very Hard' => NULL);
			foreach ($locklevels as $lvlname => $lvlmax) {
				if (is_null($lvlmax) || $mag<=$lvlmax)
					return $lvlname." Lock";
			}
		}
		else {
			if ($this->get('magpct'))
				$unit = "%";
			else {
				$unit = " pt";
				if ($mag!='1')
					$unit .= "s";
			}
			return $mag.$unit;
		}
	}
}

class OBItem_NPC_ extends GenItem_NPC_ {
	protected function _class_setup() {
		$this->_column_data['level_text'] = array('title' => 'NPC Level', 'display' => 'details_html');
		$this->_column_data['health_text'] = array('title' => 'NPC Health', 'display' => 'details_html');
		$this->_column_data['magicka_text'] = array('title' => 'NPC Magicka', 'display' => 'details_html');
		$this->_column_data['dialogue'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'), 'format' => 'ordid_as_name');
                $this->_column_data['dialogue_text'] = array('display' => 'details_html', 'display_option' => array('details_html' => 'table_dialogue'));
	}
	
	public function experience_needed($skill) {
		$skilllevel = $this->skill($skill);
		
		$experience = 0.25883 * fpow($skilllevel, 1.5);
		if ($_data[$skill.'_type']=='major')
			$experience *= 0.6;
		if ($_data[$skill.'_spec'])
			$experience *= 0.75;
		
		return $experience;
	}
	
	function persuasion_reactions() {
		// this is a first guess at how the game is calculating reactions, but it doesn't work yet
		$mults = array('boast' => array('aggression' => 10,
		                                'confidence' => -2,
		                                'energy' => -1,
		                                'intelligence' => 2,
		                                'personality' => -2,
		                                'responsibility' => 1,
		                                'strength' => 1,
		                                'willpower' => -1),
		               'coerce' => array('aggression' => -10,
		                                 'confidence' => 2,
		                                 'energy' => 1,
		                                 'intelligence' => -1,
		                                 'personality' => -2,
		                                 'responsibility' => -1,
		                                 'strength' => 2,
		                                 'willpower' => 1),
		               'joke' => array('aggression' => 5,
		                               'confidence' => -1,
		                               'energy' => 2,
		                               'intelligence' => -2,
		                               'personality' => 1,
		                               'responsibility' => -2,
		                               'strength' => 1,
		                               'willpower' => 2),
		               'admire' => array('aggression' => -5,
		                                 'confidence' => 1,
		                                 'energy' => -2,
		                                 'intelligence' => -1,
		                                 'personality' => -1,
		                                 'responsibility' => 2,
		                                 'strength' => 1,
		                                 'willpower' => -2));
		
		$total = array();
		foreach ($mults as $reaction => $rdata) {
			$total[$reaction] = 0;
			foreach ($rdata as $attrib => $mult) {
				if (is_null($aval = $this->get($attrib))) {
					print "unable to get value for $attrib\n";
					continue;
				}
				$total[$reaction] += $aval*$mult;
			}
		}
		asort($total);
		return $total;
	}
	
	public function level_text() {
		if (!$this->leveled())
			return $this->get('level_raw');
		$text = 'PC';
		$level_off = $this->get('level_raw');
		if ($level_off>=0)
			$text .= "+".$level_off;
		else
			$text .= '-'.abs($level_off);
		$cmin = $this->get('calcmin');
		$cmax = $this->get('calcmax');
		if ($cmin>1 && $cmin>$level_off+1 && $cmax)
			$text .= ' (range='.$cmin.'-'.$cmax.')';
		elseif ($cmin>1 && $cmin>$level_off+1)
			$text .= ' (min='.$cmin.')';
		elseif ($cmax)
			$text .= ' (max='.$cmax.')';
		return $text;
	}
	
	// following two functions adapated from cs_pl/npc_esp
	// have not yet been tested in php
	public function magicka_text() {
		if (!$this->leveled())
			return $this->get('magicka');
		$this->init_raceclass_data();
		// -1 because increases are relative to level 1, not level 0
		$level_off = $this->get('level_raw')-1;
		$lvlout = "PC";
		if ($level_off>=0)
			$lvlout .= '+'.$level_off;
		else
			$lvlout .= '-'.abs($level_off);
		
		$cmin = $this->get('calcmin');
		$cmax = $this->get('calcmax');
		
		// intel0 does *not* include intel_adj
		$intel0 = $this->get('intelligence0');
		$intel_delt = $this->get('intelligence_delt');
		$magicka_adj = $this->get('magicka_adj');
		$magickamax = 100*2.5 + $magicka_adj;
		if ($cmax)
			$magickamax = min($magickamax, floor(($intel0+($cmax-1)*$intel_delt)*2.5 + 0.5) + $magicka_adj);
		$magicka0 = floor($intel0*2.5 + 0.5) + $magicka_adj;
		$text = $magicka0.' + '.($intel_delt*2.5).'x('.$lvlout.') (max='.$magickamax.')';
		
		return $text;
	}
	
	public function health_text() {
		if (!$this->leveled())
			return $this->get('health');
		$this->init_raceclass_data();
		$level_off = $this->get('level_raw');
		$lvlout = "PC";
		// -1 because increases are relative to level 1, not level 0
		if (($level_off-1)>=0)
			$lvlout .= '+'.($level_off-1);
		else
			$lvlout .= '-'.abs($level_off-1);
		
		$cmin = $this->get('calcmin');
		$cmax = $this->get('calcmax');
		
		// str0 and end0 do *not* include str_adj or end_adj
		$strength0 = $this->get('strength0');
		$endurance0 = $this->get('endurance0');
		$health0 = $this->get('health_adj') + floor(0.5*($strength0 + $endurance0) + 0.5);

		$health_delt = $this->get('health_delt');
		$strength_delt = $this->get('strength_delt');
		$endurance_delt = $this->get('endurance_delt');
		$class_delt = $this->_classfac;
		
		$lvlcut = min((100-$strength0)/$strength_delt, (100-$endurance0)/$endurance_delt);
		$lvlcut = floor($lvlcut)+1-$level_off;
		$lvl0 = max(1, 5-$level_off);
		
		$text = "$health0 + (".$class_delt."+".(($strength_delt+$endurance_delt)/2).")x($lvlout), PC=$lvl0-$lvlcut";
		return $text;
	}

        public function _get_index_dialogue_shared() {
                if (!isset($this->_data['dialogue'])) {
                        $this->_data['dialogue'] = array();
                        $this->_data['dialogue_text'] = array();

                        $query = "SELECT ordid, NAM1 as dialogue FROM AllItems inner join INFO_CTDA_Record using (ordid) inner join INFO_Record using (ordid) WHERE cond_function_lu=".$this->_parentlist->get_lookup_id('CTDA_function', 'GetIsID')." AND cond_param1=".hexdec(substr($this->get('formid'),2))." and mod_status&0x02>0";
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

class OBItem_LVLC extends GenItem {
// rewrite of perl code; has not yet been tested or even compiled (is get_t_item usage correct?)
// based on previous tests, whether or not a LVLC respawns seems to be determined by the 
// template creature for the leveled list -- but only if the template creature is a creature (not a NPC),
// and only if respawn has been set for that creature
// otherwise, LVLCs always respawn
	function respawn() {
		if (isset($this->_data['respawn']))
			return $this->_data['respawn'];
		if (is_null($this->_data['template_id']))
			$this->_data['respawn'] = true;
		elseif (!is_object($template = $this->_parentlist->get_t_item($this->_data['template_id'])))
			$this->_data['respawn'] = true;
		elseif ($item->get('rectype')=='NPC_')
			$this->_data['respawn'] = true;
		elseif (is_null($respawn=$item->get('respawn')))
			$this->_data['respawn'] = true;
		else
			$this->_data['respawn'] = $respawn;
		return $this->_data['respawn'];
	}
}

class OBItem_SPEL extends GenItem_SPEL {
	function spellcost() {
		if (!$this->get('autocalc'))
			return $this->_getexact('spellcost');
		else
			return $this->get('auto_spellcost');
	}
	
	function auto_spellcost() {
		$cost = 0;
		// not sure effect_cost is the right param name
		foreach ($this->indices('effect_id') as $index) {
			$cost += $this->get('effect_cost', $index);
		}
		return $cost;
	}
	
	function derive_spelllevel() {
		return $this->get('auto_spelllevel');
	}
	
	function auto_spelllevel() {
		$levels = array('Novice' => 25.5, 'Apprentice' => 62.5, 'Journeyman' => 149.5, 'Expert' => 399.5, 'Master' => NULL);
		$cost = $this->auto_spellcost();
		foreach ($levels as $lvlname => $lvlmax) {
			if (is_null($lvlmax) || $cost<$lvlmax)
				return $lvlname;
		}
	}
	
	function set_custom_spell() {
		$this->_data['SPEL_type_lu'] = $this->_parentlist->get_lookup_id('SPEL_type', 'Spell');
		$this->_data['autocalc'] = true;
	}
	
	function add_custom_effect($effname, $mag, $dur=NULL, $area=NULL, $range='Self') {
		if (!array_key_exists('effect_id', $this->_data))
			$index = 0;
		else
			$index = count($this->_data['effect_id']);
		
		$words = preg_split('/\s+/', $effname);
		$actor = NULL;
		if (count($words)==2) {
			// there are two reasons why we can't just do a single check on actorlist
			// 1: need to distinguish skills from attributes (different base effect name)
			// 2: don't want to pull in effects such as Restore Health or Restore Fatigue
			if (!is_null($skill=$this->_parentlist->get_lookup_id('skilllist', $words[1]))) {
				$effname = $words[0]." Skill";
				$actor = $this->_parentlist->get_lookup_id('actorlist', $words[1]);
			}
			elseif (!is_null($attrib=$this->_parentlist->get_lookup_id('attriblist', $words[1]))) {
				$effname = $words[0]." Attribute";
				$actor = $this->_parentlist->get_lookup_id('actorlist', $words[1]);
			}
		}
		$eff_item = $this->_parentlist->get_item_from_name($effname, 'MGEF');
		if (!$eff_item) {
			print "Unable to identify effect {$effname}\n";
			return;
		}
		$this->_data['effect_id'][$index] = $eff_item->get('ordid');
		$this->_data['effectb_id'][$index] = $eff_item->get('ordid');
		$eff_item->release();
		
		$this->_data['eff_mag'][$index] = $mag;
		if (!is_null($dur))
			$this->_data['eff_dur'][$index] = $dur;
		if (!is_null($area))
			$this->_data['eff_area'][$index] = $area;
		$this->_data['eff_range_lu'][$index] = $this->_parentlist->get_lookup_id('effect_range', $range);
		if (!is_null($actor))
			$this->_data['eff_act'][$index] = $actor;
	}
	
	function eff_skill($index) {
		return $this->get('eff_act', $index);
	}
	function eff_attrib($index) {
		return $this->get('eff_act', $index);
	}
}

class OBItem_WEAP extends GenItem_WEAP {
	protected function derive_skill() {
		$type = $this->get('type');
		if (substr($type,0,2)=='Bl')
			return substr($type,0,5);
		elseif ($type=='Bow')
			return "Marksman";
		else
			return NULL;
	}
}

?>
