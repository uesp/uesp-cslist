<?php
require_once 'csread.inc';

class MWFRRead extends CSRead {
	//protected $_files = array("Morrowind.esm", "Bloodmoon.esm", "Tribunal.esm");
	//protected $_fileofficial = array("Morrowind.esm", "Bloodmoon.esm", "Tribunal.esm");
	//protected $_filenamespace = array('Morrowind', 'Bloodmoon', 'Tribunal');
	//protected $_filemodname = array('', '', '');
	//protected $_filemodabbrev = array('MW', 'BM', 'TR');
	
	protected $_files = array("MorrowindFR.esm");
	protected $_fileofficial = array("Morrowind.esm");
	protected $_filenamespace = array('Morrowind');
	protected $_filemodname = array('');
	protected $_filemodabbrev = array('MW');
	
	// MW-specific values for header_rec and header_sub
	protected $_header_rec = array(
		'type' => 4,
		'size' => 'ulong',
		'unknown' => 4, // unknown header, usually 0
		'flags' => 'ulong',
		// subrecords
	);
	protected $_header_sub = array(
		'type' => 4,
		'datasize' => 'ulong',
		'data' => NULL,
	);
	
	protected function getClassId()
	{
		return substr(get_class($this),0,4);
	}
	
	protected function _datadef_setup() {
		if (!empty($this->_datadef))
			return;
		$this->_parentlist->set_lookup('attriblist', array("Strength", "Intelligence", "Willpower", "Agility", "Speed", "Endurance", "Personality", "Luck"));
		$this->_parentlist->set_lookup('skilllist', array("Block", "Armorer", "Medium Armor", "Heavy Armor", "Blunt Weapon", "Long Blade", "Axe", "Spear", "Athletics", "Enchant", "Destruction", "Alteration", "Illusion", "Conjuration", "Mysticism", "Restoration", "Alchemy", "Unarmored", "Security", "Sneak", "Acrobatics", "Light Armor", "Short Blade", "Marksman", "Mercantile", "Speechcraft", "Hand-to-hand"));
		// these indices aren't used by Bethesda, but useful to have for consistency with OB
		$this->_parentlist->set_lookup('actorlist', array("Strength", "Intelligence", "Willpower", "Agility", "Speed", "Endurance", "Personality", "Luck", 'Health', 'Magicka', 'Fatigue', 'Encumbrance', "Block", "Armorer", "Medium Armor", "Heavy Armor", "Blunt Weapon", "Long Blade", "Axe", "Spear", "Athletics", "Enchant", "Destruction", "Alteration", "Illusion", "Conjuration", "Mysticism", "Restoration", "Alchemy", "Unarmored", "Security", "Sneak", "Acrobatics", "Light Armor", "Short Blade", "Marksman", "Mercantile", "Speechcraft", "Hand-to-hand"));
		$this->_parentlist->set_lookup('skillspec', array("C", "C", "C", "C", "C", "C", "C", "C", "C", "M", "M", "M", "M", "M", "M", "M", "M", "M", "S", "S", "S", "S", "S", "S", "S", "S", "S"));
		$this->_parentlist->set_lookup('effect_range', array('Self', 'Touch', 'Target'));
		$this->_parentlist->set_lookup('APPA_type', array( 'Mortar and Pestle', 'Alembic', 'Calcinator', 'Retort'));
		//		$this->_parentlist->set_lookup('ARMO_type', array( "Helmet", "Cuirass", "L Pauldron", "R Pauldron", "Greaves", "Boots", "L Gauntlet", "R Gauntlet", "Shield", "L Bracer", "R Bracer"));
		$this->_parentlist->set_lookup('ARMO_type', array( "Helmet", "Cuirass", "Left Pauldron", "Right Pauldron", "Greaves", "Boots", "Left Gauntlet", "Right Gauntlet", "Shield", "Left Bracer", "Right Bracer"));
		$this->_parentlist->set_lookup('body_location', array( "Head", "Hair", "Neck", "Cuirass", "Groin", "Skirt", "R Hand", "L Hand", "R Wrist", "L Wrist",
			"Shield", "R Forearm", "L Forearm", "R Upper Arm", "L Upper Arm", "R Foot", "L Foot", "R Ankle", "L Ankle", "R Knee",
			"L Knee", "R Upper Leg", "L Upper Leg", "R Pauldron", "L Pauldron", "Weapon", "Tail" ));
		$this->_parentlist->set_lookup('speclist', array('Combat', 'Magic', 'Stealth'));
		//		$this->_parentlist->set_lookup('CLOT_type', array( 'Pants', 'Shoes', 'Shirt', 'Belt', 'Robe', 'R Glove', 'L Glove', 'Skirt', 'Ring', 'Amulet'));
		$this->_parentlist->set_lookup('CLOT_type', array( 'Pants', 'Shoes', 'Shirt', 'Belt', 'Robe', 'Right Glove', 'Left Glove', 'Skirt', 'Ring', 'Amulet'));
		$this->_parentlist->set_lookup('CREA_type', array( 'Creature', 'Daedra', 'Undead', 'Humanoid'));
		$this->_parentlist->set_lookup('DIAL_type', array( 'regular', 'voice', 'greeting', 'persuasion', 'journal'));
		$this->_parentlist->set_lookup('ENCH_type', array( 'Cast Once', 'Cast When Strikes', 'Cast When Used', 'Constant Effect'));
		$this->_parentlist->set_lookup('genderlist', array( 'M', 'F'));
		$this->_parentlist->set_lookup('SCVR_type', array( '', 'function', 'global', 'local', 'journal', 'item', 'dead', 'not id', 'not faction', 'not class', 'not race', 'not cell', 'not local'));
		$this->_parentlist->set_lookup('SCVR_op', array( '=', '!=', '>', '>=', '<', '<='));
		$this->_parentlist->set_lookup('schools', array( 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Mysticism', 'Restoration'));
		$this->_parentlist->set_lookup('SPEL_type', array( 'Spell', 'Ability', 'Blight', 'Disease', 'Curse', 'Power'));
		//		$this->_parentlist->set_lookup('WEAP_type', array( "ShortBladeOneHand", "LongBladeOneHand", "LongBladeTwoClose", "BluntOneHand", "BluntTwoClose", "BluntTwoWide", "SpearTwoWide", "AxeOneHand", "AxeTwoHand", "MarksmanBow", "MarksmanCrossbow", "MarksmanThrown", "Arrow", "Bolt" ));
		$this->_parentlist->set_lookup('WEAP_type', array( "Short Blade", "Long Blade, 1-Hand", "Long Blade, 2-Hand", "Blunt Weapon, 1-Hand", "Blunt Weapon, 2-Hand Close", "Blunt Weapon, 2-Hand Wide", "Spear", "Axe, 1-Hand", "Axe, 2-Hand", "Marksman, Bows", "Marksman, Crossbows", "Marksman, Throwing Weapons", "Marksman, Arrows", "Marksman, Bolts" ));
		
		// these names are not directly linked to effects in the construction set, so names have to be provided explicitly
		// (specifically, names are specified in a series of GMST string values, presumably for translation purposes.  But
		//  the connection between MGEF and GMST string value is hardwired and not available from CS data)
		$this->_parentlist->set_lookup('MGEF_name', array(0 => "Water Breathing", 1 => "Swift Swim", 2 => "Water Walking", 3 => "Shield", 4 => "Fire Shield", 5 => "Lightning Shield", 6 => "Frost Shield", 7 => "Burden", 8 => "Feather", 9 => "Jump", 10 => "Levitate", 11 => "Slowfall", 12 => "Lock", 13 => "Open", 14 => "Fire Damage", 15 => "Shock Damage", 16 => "Frost Damage", 17 => "Drain Attribute", 18 => "Drain Health", 19 => "Drain Magicka", 20 => "Drain Fatigue", 21 => "Drain Skill", 22 => "Damage Attribute", 23 => "Damage Health", 24 => "Damage Magicka", 25 => "Damage Fatigue", 26 => "Damage Skill", 27 => "Poison", 28 => "Weakness to Fire", 29 => "Weakness to Frost", 30 => "Weakness to Shock", 31 => "Weakness to Magicka", 32 => "Weakness to Common Disease", 33 => "Weakness to Blight Disease", 34 => "Weakness to Corprus Disease", 35 => "Weakness to Poison", 36 => "Weakness to Normal Weapons", 37 => "Disintegrate Weapon", 38 => "Disintegrate Armor", 39 => "Invisibility", 40 => "Chameleon", 41 => "Light", 42 => "Sanctuary", 43 => "Night-Eye", 44 => "Charm", 45 => "Paralyze", 46 => "Silence", 47 => "Blind", 48 => "Sound", 49 => "Calm Humanoid", 50 => "Calm Creature", 51 => "Frenzy Humanoid", 52 => "Frenzy Creature", 53 => "Demoralize Humanoid", 54 => "Demoralize Creature", 55 => "Rally Humanoid", 56 => "Rally Creature", 57 => "Dispel", 58 => "Soultrap", 59 => "Telekinesis", 60 => "Mark", 61 => "Recall", 62 => "Divine Intervention", 63 => "Almsivi Intervention", 64 => "Detect Animal", 65 => "Detect Enchantment", 66 => "Detect Key", 67 => "Spell Absorption", 68 => "Reflect", 69 => "Cure Common Disease", 70 => "Cure Blight Disease", 71 => "Cure Corprus Disease", 72 => "Cure Poison", 73 => "Cure Paralyzation", 74 => "Restore Attribute", 75 => "Restore Health", 76 => "Restore Magicka", 77 => "Restore Fatigue", 78 => "Restore Skill", 79 => "Fortify Attribute", 80 => "Fortify Health", 81 => "Fortify Magicka", 82 => "Fortify Fatigue", 83 => "Fortify Skill", 84 => "Fortify Maximum Magicka", 85 => "Absorb Attribute", 86 => "Absorb Health", 87 => "Absorb Magicka", 88 => "Absorb Fatigue", 89 => "Absorb Skill", 90 => "Resist Fire", 91 => "Resist Frost", 92 => "Resist Shock", 93 => "Resist Magicka", 94 => "Resist Common Disease", 95 => "Resist Blight Disease", 96 => "Resist Corprus Disease", 97 => "Resist Poison", 98 => "Resist Normal Weapons", 99 => "Resist Paralysis", 100 => "Remove Curse", 101 => "Turn Undead", 102 => "Summon Scamp", 103 => "Summon Clannfear", 104 => "Summon Daedroth", 105 => "Summon Dremora", 106 => "Summon Ancestral Ghost", 107 => "Summon Skeletal Minion", 108 => "Summon Bonewalker", 109 => "Summon Greater Bonewalker", 110 => "Summon Bonelord", 111 => "Summon Winged Twilight", 112 => "Summon Hunger", 113 => "Summon Golden Saint", 114 => "Summon Flame Atronach", 115 => "Summon Frost Atronach", 116 => "Summon Storm Atronach", 117 => "Fortify Attack", 118 => "Command Creature", 119 => "Command Humanoid", 120 => "Bound Dagger", 121 => "Bound Longsword", 122 => "Bound Mace", 123 => "Bound Battle-Axe", 124 => "Bound Spear", 125 => "Bound Longbow", 126 => "EXTRA SPELL", 127 => "Bound Cuirass", 128 => "Bound Helm", 129 => "Bound Boots", 130 => "Bound Shield", 131 => "Bound Gloves", 132 => "Corprus", 133 => "Vampirism", 134 => "Summon Centurion Sphere", 135 => "Sun Damage", 136 => "Stunted Magicka", 137 => "Summon Verminous Fabricant"));
		
		$this->_parentlist->set_lookup('Worldspace', array('Test', 'Vvardenfell', 'Solstheim', 'Mournhold'));
		$this->_parentlist->set_lookup('Worldtype', array('Test', 'Main', 'City', 'Submap', 'Other'));
		
# keep:
#  2 - always read this entry if the record is requested (used for ID-related fields without which record is useless)
#  1 - read this data entry by default
#  0 - skip this data entry by default
# -1 - data entry cannot be read because format information has not been added to code
		$this->_datadef['any'] =
			array( 'NAME' => array( 'keep' => 2 ),
			       'FNAM' => array( 'keep' => 2, 'val' => 'name' ),
			       'MODL' => array( 'keep' => 0, 'val' => 'model' ), // model (NIF) file
			       'ITEX' => array( 'keep' => 0, 'val' => 'icon' ), // icon
			       'SCRI' => array( 'keep' => 0, 'type' => 'edid')); // script name
		$this->_datadef['ACTI'] = array();
		$this->_datadef['ALCH'] =
			array('ALDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'autocalc', 'type' => 'boolean4'))),
			      'ENAM' => array( 'keep' => 1,
			                       'multi' => 'ENAM',
			                       'data' => array( array( 'val' => 'effect_id', 'type' => 'mgefid'),
			                                        array( 'val' => 'eff_skill', 'type' => 'byte', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'eff_attrib', 'type' => 'byte', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'eff_range', 'type' => 'long', 'lookup' => 'effect_range'),
			                                        array( 'val' => 'eff_area', 'type' => 'long'),
			                                        array( 'val' => 'eff_dur', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmin', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmax', 'type' => 'long'))),
			      'TEXT' => array( 'keep' => 0));
		$this->_datadef['APPA'] =
			array('AADT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'long', 'lookup' => 'APPA_type'),
			                                        array( 'val' => 'quality', 'type' => 'float'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'))));
		$this->_datadef['ARMO'] =
			// enchantcost here is the value shown as 'Enchant' on unenchanted armor
			array( 'AODT' => array( 'keep' => 1,
			                        'data' => array(
			                                         array( 'val' => 'type', 'type' => "long", 'lookup' => 'ARMO_type' ),
			                                         array( 'val' => "weight", 'type' => "float" ),
			                                         array( 'val' => "value", 'type' => "long" ),
			                                         array( 'val' => "health", 'type' => "long" ),
			                                         array( 'val' => "enchantcost", 'type' => "long", 'factor' => 0.1 ),
			                                         array( 'val' => "rating", 'type' => "long" ),
			                                         array( 'val' => 'skill', 'type' => 'byte', 'lookup' => 'skilllist', 'extra' => 1),
			                                         array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			       'INDX' => array( 'keep' => 0,
			                        'multi' => "INDX",
			                        'val' => 'body_loc',
			                        'type' => "byte",
			                        'lookup' => 'body_location'),
			       'BNAM' => array( 'keep' => 0,
			                        'multi' => "INDX" ),
			       'CNAM' => array( 'keep' => 0,
			                        'multi' => "INDX" ),
			       'ENAM' => array( 'keep' => 1,
			                        'val' => 'enchant_id',
			                        'multi' => "ENAM",
			                        'type' => 'edid'));
		// nothing about BODY records included in file format
		$this->_datadef['BODY'] =
			array('BYDT' => array( 'keep' => -1));
		$this->_datadef['BOOK'] =
			array('BKDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'scroll', 'type' => 'long'),
			                                        array( 'val' => 'skill', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'enchantcost', 'type' => 'long', 'factor' => 0.1))),
			      'TEXT' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      // ENAM not listed in file format; assuming format is same as for other item types
			      'ENAM' => array( 'keep' => 1,
			                       'val' => 'enchant_id',
			                       'multi' => "ENAM",
			                       'type' => 'edid'));
		$this->_datadef['BSGN'] =
			array('TNAM' => array( 'keep' => -1),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'NPCS' => array( 'keep' => 1,
			                       'multi' => 'NPCS',
			                       'val' => 'spell_id',
			                       'type' => 'edid'));
		// must always get RGNN and DATA for CELL records in order to create ID
		$this->_datadef['CELL'] =
			array('DATA' => array( 'keep' => 2,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( '!exterior', 'haswater', 'sleep_illegal', NULL, NULL, NULL, NULL, 'fake exterior')),
			                                        array( 'val' => 'cell_x', 'type' => 'long'),
			                                        array( 'val' => 'cell_y', 'type' => 'long'),
			                                        array( 'val' => 'raw_edid', 'type' => 'string', 'extra' => 1),
			                                        array( 'val' => 'worldtype', 'lookup' => 'Worldtype', 'extra' => 1),
			                                        array( 'val' => 'worldspace', 'type' => 'ubyte', 'lookup' => 'Worldspace', 'extra' => 1),
			                                        array( 'val' => 'parentcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extloc_x', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extloc_y', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extdoor_id', 'type' => 'edid', 'extra' => 1))),
			      'RGNN' => array( 'keep' => 1,
			                       'type' => 'string',
			                       'lookup_create' => 'CELL_RGNN'),
			      'NAM0' => array( 'keep' => -1, 'type' => 'long'),
			      'NAM5' => array( 'keep' => -1, 'type' => 'long'),
			      'WHGT' => array( 'keep' => -1, 'type' => 'float', 'val' => 'water_height'),
			      'AMBI' => array( 'keep' => -1,
			                       'data' => array( array( 'val' => 'ambientcolor', 'type' => 'long' ),
			                                        array( 'val' => 'sunlightcolor', 'type' => 'long' ),
			                                        array( 'val' => 'fogcolor', 'type' => 'long' ),
			                                        array( 'val' => 'fogdensity', 'type' => 'float'))),
			      'FRMR' => array( 'keep' => 1,
			                       'sqltable' => 'REFR',
			                       'multi' => 'FRMR',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'FRMR', 'type' => 'ulong'),
			                                        array( 'val' => 'cell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'door_dup_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'door_rev_id', 'type' => 'edid', 'extra' => 1))),
# these entries are for items within the cell -> code automatically transforms FRMR subrecords into individual
# REFR records
			      'FRMR-NAME' => array( 'keep' => 1, 'val' => 'base_id', 'type' => 'edid'),
			      'FRMR-XSCL' => array( 'keep' => 0, 'type' => 'float', 'val' => 'scale', 'default' => 1),
			      // 'force' is to ensure entry is created, even though it doesn't exist in base files
			      'FRMR-DELE' => array( 'keep' => 0, 'type' => 'boolean4', 'force' => 1),
			      'FRMR-DODT' => array( 'keep' => 0,
			                            'data' => array( array( 'val' => 'door_x', 'type' => 'float'),
			                                             array( 'val' => 'door_y', 'type' => 'float'),
			                                             array( 'val' => 'door_z', 'type' => 'float'),
			                                             array( 'val' => 'door_rx', 'type' => 'float'),
			                                             array( 'val' => 'door_ry', 'type' => 'float'),
			                                             array( 'val' => 'door_rz', 'type' => 'float'))),
			      'FRMR-DNAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'cell_rev_id'), // door exit cell
			      'FRMR-FLTV' => array( 'keep' => 0, 'type' => 'long'),
			      'FRMR-KNAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'key_id'), // key name
			      'FRMR-TNAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'trap_id'), // trap name
			      'FRMR-UNAM' => array( 'keep' => -1),
			      'FRMR-ANAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'owner_id'), // owner ID string
			      'FRMR-BNAM' => array( 'keep' => 0, 'type' => 'edid'), // global variable / rank ID string
			      'FRMR-INTV' => array( 'keep' => -1),
			      'FRMR-NAM9' => array( 'keep' => -1),
			      'FRMR-XSOL' => array( 'keep' => 0, 'type' => 'edid'), // soul extra data (ID string of creature)
			      'FRMR-DATA' => array( 'keep' => 0,
			                            'data' => array( array( 'val' => 'x', 'type' => 'float'),
			                                             array( 'val' => 'y', 'type' => 'float'),
			                                             array( 'val' => 'z', 'type' => 'float'),
			                                             array( 'val' => 'rx', 'type' => 'float'),
			                                             array( 'val' => 'ry', 'type' => 'float'),
			                                             array( 'val' => 'rz', 'type' => 'float'))),
			      // following records are not described in file format
			      // NAM0, CNAM, and INDX all found after FRMR records and were incorrectly unsetting FRMR
			      // since nothing known about XCHG, just set up both ways to be safe
			      'FRMR-NAM0' => array( 'keep' => -1, 'type' => 'long', 'val' => 'FRMR-NAM0'),
			      'FRMR-CNAM' => array( 'keep' => -1, 'type' => 'edid'),
			      'FRMR-INDX' => array( 'keep' => -1),
			      'FRMR-XCHG' => array( 'keep' => -1, 'val' => 'FRMR-XCHG'),
			      'XCHG' => array( 'keep' => -1));
		$this->_datadef['CLAS'] =
			array('CLDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'att1', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'att2', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'spec', 'type' => 'long', 'lookup' => 'speclist'),
			                                        array( 'val' => 'skl1a', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl1b', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl2a', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl2b', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl3a', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl3b', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl4a', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl4b', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl5a', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl5b', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'type' => 'ulong', 'flag' => array( 'playable')),
			                                        array( 'type' => 'ulong', 'flag' => array( 'mercweap', 'mercarmor', 'mercclot', 'mercbook', 'mercingred', 'mercpick', 'mercprobe', 'merclight', 'mercapp', 'mercrep', 'mercmisc', 'mercspel', 'mercmagic', 'mercpot', 'train', 'spellmake', 'enchantmake', 'repair')))),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'));
		$this->_datadef['CLOT'] =
			array('CTDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'long', 'lookup' => 'CLOT_type'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'short'),
			                                        array( 'val' => 'enchantcost', 'type' => 'short', 'factor' => 0.1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'INDX' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'val' => 'body_loc',
			                       'type' => 'byte',
			                       'lookup' => 'body_location'),
			      // model used for item (model for all chars, or else male-specific model)
			      'BNAM' => array( 'keep' => 0,
			                       'multi' => 'INDX'),
			      // model used for female-specific version
			      'CNAM' => array( 'keep' => 0,
			                       'multi' => 'INDX'),
			      'ENAM' => array( 'keep' => 1,
			                       'val' => 'enchant_id',
			                       'type' => 'edid'));
		$this->_datadef['CONT'] =
			array('CNDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'))),
			      'FLAG' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( 'organic', 'respawn')))),
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))));
		$this->_datadef['CREA'] =
			array('NPDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'long', 'lookup' => 'CREA_type'),
			                                        array( 'val' => 'level_raw', 'type' => 'long'),
			                                        array( 'val' => 'strength_raw', 'type' => 'long'),
			                                        array( 'val' => 'intelligence_raw', 'type' => 'long'),
			                                        array( 'val' => 'willpower_raw', 'type' => 'long'),
			                                        array( 'val' => 'agility_raw', 'type' => 'long'),
			                                        array( 'val' => 'speed_raw', 'type' => 'long'),
			                                        array( 'val' => 'endurance_raw', 'type' => 'long'),
			                                        array( 'val' => 'personality_raw', 'type' => 'long'),
			                                        array( 'val' => 'luck_raw', 'type' => 'long'),
			                                        array( 'val' => 'health_raw', 'type' => 'long'),
			                                        array( 'val' => 'magicka_raw', 'type' => 'long'),
			                                        array( 'val' => 'fatigue_raw', 'type' => 'long'),
			                                        array( 'val' => 'soul', 'type' => 'long'),
			                                        array( 'val' => 'combat', 'type' => 'long'),
			                                        array( 'val' => 'magic', 'type' => 'long'),
			                                        array( 'val' => 'stealth', 'type' => 'long'),
			                                        array( 'val' => 'attackmin1', 'type' => 'long'),
			                                        array( 'val' => 'attackmax1', 'type' => 'long'),
			                                        array( 'val' => 'attackmin2', 'type' => 'long'),
			                                        array( 'val' => 'attackmax2', 'type' => 'long'),
			                                        array( 'val' => 'attackmin3', 'type' => 'long'),
			                                        array( 'val' => 'attackmax3', 'type' => 'long'))),
			      'FLAG' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( NULL, NULL, 'respawn', NULL, NULL, NULL, NULL, NULL, 'essential')))),
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))),
			      'NPCS' => array( 'keep' => 0,
			                       'multi' => 'NPCS',
			                       'data' => array( array( 'val' => 'spell_id', 'type' => 'edid'))),
			      'AIDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'hello', 'type' => 'byte'),
			                                        array( 'val' => 'fight', 'type' => 'byte', 'index' => 2),
			                                        array( 'val' => 'flee', 'type' => 'byte'),
			                                        array( 'val' => 'alarm', 'type' => 'byte'),
			                                        array( 'val' => 'flag', 'type' => 'ulong', 'index' => 8))),
			      'AI_W' => array( 'keep' => -1),
			      'AI_T' => array( 'keep' => -1),
			      'AI_F' => array( 'keep' => -1),
			      'AI_E' => array( 'keep' => -1),
			      'AI_A' => array( 'keep' => -1),
			      'XSCL' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'scale', 'type' => 'float', 'default' => 1))),
			      // SNAM not reported in format file
			      'CNAM' => array ( 'keep' => -1));
		$this->_datadef['DIAL'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'byte', 'lookup' => 'DIAL_type'))));
		$this->_datadef['DOOR'] =
			array('SNAM' => array( 'keep' => 0),
			      'ANAM' => array( 'keep' => 0));
		$this->_datadef['ENCH'] =
			array('ENDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'long', 'lookup' => 'ENCH_type'),
			                                        array( 'val' => 'enchantcost_raw', 'type' => 'long'),
			                                        array( 'val' => 'enchantcharge', 'type' => 'long'),
			                       // not sure what the last value is, but it doesn't seem to be an autocalc flag
			                                        array( 'val' => NULL, 'type' => 'ulong'))),
			      'ENAM' => array( 'keep' => 1,
			                       'multi' => 'ENAM',
			                       'data' => array( array( 'val' => 'effect_id', 'type' => 'mgefid'),
			                                        array( 'val' => 'eff_skill', 'type' => 'byte', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'eff_attrib', 'type' => 'byte', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'eff_range', 'type' => 'long', 'lookup' => 'effect_range'),
			                                        array( 'val' => 'eff_area', 'type' => 'long'),
			                                        array( 'val' => 'eff_dur', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmin', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmax', 'type' => 'long'))));
		$this->_datadef['FACT'] =
			array('RNAM' => array( 'keep' => 1,
			                       'multi' => 'RNAM',
			                       'data' => array( array( 'val' => 'rank_name', 'type' => 'string'))),
			      'FADT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'att1', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'att2', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'rank1att1', 'type' => 'long'),
			                                        array( 'val' => 'rank1att2', 'type' => 'long'),
			                                        array( 'val' => 'rank1skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank1skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank1', 'type' => 'long'),
			                                        array( 'val' => 'rank2att1', 'type' => 'long'),
			                                        array( 'val' => 'rank2att2', 'type' => 'long'),
			                                        array( 'val' => 'rank2skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank2skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank2', 'type' => 'long'),
			                                        array( 'val' => 'rank3att1', 'type' => 'long'),
			                                        array( 'val' => 'rank3att2', 'type' => 'long'),
			                                        array( 'val' => 'rank3skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank3skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank3', 'type' => 'long'),
			                                        array( 'val' => 'rank4att1', 'type' => 'long'),
			                                        array( 'val' => 'rank4att2', 'type' => 'long'),
			                                        array( 'val' => 'rank4skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank4skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank4', 'type' => 'long'),
			                                        array( 'val' => 'rank5att1', 'type' => 'long'),
			                                        array( 'val' => 'rank5att2', 'type' => 'long'),
			                                        array( 'val' => 'rank5skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank5skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank5', 'type' => 'long'),
			                                        array( 'val' => 'rank6att1', 'type' => 'long'),
			                                        array( 'val' => 'rank6att2', 'type' => 'long'),
			                                        array( 'val' => 'rank6skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank6skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank6', 'type' => 'long'),
			                                        array( 'val' => 'rank7att1', 'type' => 'long'),
			                                        array( 'val' => 'rank7att2', 'type' => 'long'),
			                                        array( 'val' => 'rank7skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank7skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank7', 'type' => 'long'),
			                                        array( 'val' => 'rank8att1', 'type' => 'long'),
			                                        array( 'val' => 'rank8att2', 'type' => 'long'),
			                                        array( 'val' => 'rank8skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank8skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank8', 'type' => 'long'),
			                                        array( 'val' => 'rank9att1', 'type' => 'long'),
			                                        array( 'val' => 'rank9att2', 'type' => 'long'),
			                                        array( 'val' => 'rank9skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank9skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank9', 'type' => 'long'),
			                                        array( 'val' => 'rank10att1', 'type' => 'long'),
			                                        array( 'val' => 'rank10att2', 'type' => 'long'),
			                                        array( 'val' => 'rank10skill1', 'type' => 'long'),
			                                        array( 'val' => 'rank10skill2', 'type' => 'long'),
			                                        array( 'val' => 'rank10', 'type' => 'long'),
			                                        array( 'val' => 'skill1', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skill2', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skill3', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skill4', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skill5', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skill6', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'type' => 'ulong', 'flag' => array('hidden')))),
			      'ANAM' => array( 'keep' => 0,
			                       'multi' => 'ANAM',
			                       'data' => array( array( 'val' => 'opp_faction_id', 'type' => 'edid'))), // name of faction for which reaction is being defined
			      'INTV' => array( 'keep' => 0,
			                       'multi' => 'ANAM',
			                       'data' => array( array( 'val' => 'opp_modifier', 'type' => 'long'))));
		$this->_datadef['GLOB'] =
			array( 'FNAM' => array( 'keep' => 1,
			                        'type' => "string",
			                        'len' => 1),
			       'FLTV' => array( 'keep' => 1,
			                        'type' => "glob" ));
		$this->_datadef['GMST'] =
			array( 'STRV' => array( 'keep' => 1,
			                        'type' => "blob" ),
			       'INTV' => array( 'keep' => 1,
			                        'type' => "long" ),
			       'FLTV' => array( 'keep' => 1,
			                        'type' => "float" ) );
#INFO records are multiple records all tied to previous DIAL record
		$this->_datadef['INFO'] =
			array('NAME' => array( 'keep' => 1, 'val' => 'infotext', 'type' => 'blob'),
			      'INAM' => array( 'keep' => 1, 'type' => 'infoid'),
			      'PNAM' => array( 'keep' => 1, 'type' => 'infoid'),
			      'NNAM' => array( 'keep' => 1, 'type' => 'infoid'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'infotype', 'type' => 'long'),
			                                        array( 'val' => 'disposition', 'type' => 'long'),
			                                        array( 'val' => 'rank', 'type' => 'byte'),
			                                        array( 'val' => 'gender', 'type' => 'byte', 'lookup' => 'genderlist'),
			                                        array( 'val' => 'pcrank', 'type' => 'byte'),
			                                        array( 'val' => 'group_id', 'type' => 'edid', 'extra' => 1))),
			      'ONAM' => array( 'keep' => 0, 'type' => 'edid'), // actor string
			      'RNAM' => array( 'keep' => 0, 'type' => 'edid'), // race string
			      'CNAM' => array( 'keep' => 0, 'type' => 'edid'), // class string
			      'FNAM' => array( 'keep' => 0, 'type' => 'edid'), // faction string
			      'ANAM' => array( 'keep' => 0, 'type' => 'edid'), // cell string
			      'DNAM' => array( 'keep' => 0, 'type' => 'edid'), // PC faction string
			      'QSTN' => array( 'keep' => 1, 'type' => 'byte'),
			      'QSTF' => array( 'keep' => 1, 'type' => 'byte'),
			      'QSTR' => array( 'keep' => 1, 'type' => 'byte'),
			      'SCVR' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'index', 'type' => 'byte'),
			                                        array( 'val' => 'seltype', 'type' => 'byte', 'lookup' => 'SCVR_type'),
			                                        array( 'val' => 'function', 'type' => 'short'),
			                                        array( 'val' => 'operator', 'type' => 'byte', 'lookup' => 'SCVR_op'),
			                                        array( 'val' => 'selname', 'type' => 'dialid'))),
			      'SNAM' => array( 'keep' => -1),
			      'INTV' => array( 'keep' => -1),
			      'FLTV' => array( 'keep' => -1),
			      'BNAM' => array( 'keep' => -1));
		$this->_datadef['INGR'] =
			array('IRDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'eff1_id', 'type' => 'mgefidl'),
			                                        array( 'val' => 'eff2_id', 'type' => 'mgefidl'),
			                                        array( 'val' => 'eff3_id', 'type' => 'mgefidl'),
			                                        array( 'val' => 'eff4_id', 'type' => 'mgefidl'),
			                                        array( 'val' => 'skl1', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl2', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl3', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skl4', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'att1', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'att2', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'att3', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'att4', 'type' => 'long', 'lookup' => 'attriblist'))));
		$this->_datadef['LEVC'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( 'alllvl')))),
			      'NNAM' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'chancenone'),
			      'INDX' => array( 'keep' => 1,
			                       'val' => 'nitems',
			                       'type' => 'long'),
			      'CNAM' => array( 'keep' => 1,
			                       'multi' => 'CNAM',
			                       'type' => 'edid',
			                       'val' => 'item_id'),
			      'INTV' => array( 'keep' => 1,
			                       'multi' => 'CNAM',
			                       'type' => 'short',
			                       'val' => 'item_lvl'));
		$this->_datadef['LEVI'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( 'alllvl', 'each')))),
			      'NNAM' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'chancenone'),
			      'INDX' => array( 'keep' => 1,
			                       'val' => 'nitems',
			                       'type' => 'long'),
			      'INAM' => array( 'keep' => 1,
			                       'multi' => 'INAM',
			                       'type' => 'edid',
			                       'val' => 'item_id'),
			      'INTV' => array( 'keep' => 1,
			                       'multi' => 'INAM',
			                       'type' => 'short',
			                       'val' => 'item_lvl'));
		// assuming documentation stating that script is at LIGH-SCPT (instead of LIGH-SCRI) is wrong
		$this->_datadef['LIGH'] =
			array('LHDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'time', 'type' => 'long'),
			                                        array( 'val' => 'radius', 'type' => 'long'),
			                                        array( 'type' => 'long', 'index' => 20,
			                                               'flag' => array( 'dynamic', 'cancarry')))),
			      'SNAM' => array( 'keep' => -1));
		$this->_datadef['LOCK'] =
			array('LKDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'quality', 'type' => 'float'),
			                                        array( 'val' => 'uses', 'type' => 'long'))));
		$this->_datadef['LTEX'] =
			array('INTV' => array( 'keep' => -1),
			      'DATA' => array( 'keep' => -1));
		$this->_datadef['MGEF'] =
			array('INDX' => array( 'keep' => 2,
			                       'type' => 'long'),
			      'MEDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'school', 'type' => 'long', 'lookup' => 'schools'),
			                                        array( 'val' => 'basecost', 'type' => 'float'),
# are these flags at 2/4/8 ? or at 200/400/800?
			                                        array( 'type' => 'ulong', 'flag' => array( NULL, 'spellmaking', 'enchanting', 'negative')))),
			      'PTEX' => array( 'keep' => -1),
			      'CVFX' => array( 'keep' => -1),
			      'AVFX' => array( 'keep' => -1),
			      'BVFX' => array( 'keep' => -1),
			      'HVFX' => array( 'keep' => -1),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'CSND' => array( 'keep' => -1),
			      'BSND' => array( 'keep' => -1),
			      'HSND' => array( 'keep' => -1),
			      'ASND' => array( 'keep' => -1));
		$this->_datadef['MISC'] =
			array('MCDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'unsellable', 'type' => 'boolean4'))),
			      'ENAM' => array( 'keep' => 1, 'type' => 'edid', 'val' => 'enchant_id'));
		$this->_datadef['NPC_'] =
			array('RNAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'race_id'), // race name
			      'ANAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'faction_id'), // faction name
			      'CNAM' => array( 'keep' => 0, 'type' => 'edid', 'val' => 'class_id'), // class name
			      'BNAM' => array( 'keep' => -1),
			      'KNAM' => array( 'keep' => -1),
			      'NPDT' => array( 'keep' => 1,
			                       'len1' => 12,
			                       'len2' => 52,
			                       'data1' => array( array( 'val' => 'level_raw', 'type' => 'short'),
			                                         array( 'val' => 'disposition', 'type' => 'byte'),
			                                         array( 'val' => 'reputation', 'type' => 'byte'),
			                                         array( 'val' => 'rank', 'type' => 'byte'),
			                                         array( 'val' => 'gold', 'type' => 'long', 'index' => 8)),
			                       'data2' => array( array( 'val' => 'level_raw', 'type' => 'short'),
			                                         array( 'vallist' => 'attriblist', 'valsuffix' => '_raw', 'type' => 'byte'),
			                                         array( 'vallist' => 'skilllist', 'valsuffix' => '_raw', 'type' => 'byte'),
			                                         array( 'val' => NULL, 'type' => 'byte'),
			                                         array( 'val' => 'health_raw', 'type' => 'short'),
			                                         array( 'val' => 'magicka_raw', 'type' => 'short'),
			                                         array( 'val' => 'fatigue_raw', 'type' => 'short'),
			                                         array( 'val' => 'disposition', 'type' => 'byte'),
			                                         array( 'val' => 'reputation', 'type' => 'byte'),
			                                         array( 'val' => 'rank', 'type' => 'byte'),
			                                         array( 'val' => NULL, 'type' => 'byte'),
			                                         array( 'val' => 'gold', 'type' => 'long'))),
			      'FLAG' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( 'female', 'essential', 'respawn', NULL, 'autocalc')))),
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))),
			      'NPCS' => array( 'keep' => 0,
			                       'multi' => 'NPCS',
			                       'data' => array( array( 'val' => 'spell_id', 'type' => 'edid'))),
			      'AIDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'hello', 'type' => 'byte'),
			                                        array( 'val' => 'fight', 'type' => 'byte', 'index' => 2),
			                                        array( 'val' => 'flee', 'type' => 'byte'),
			                                        array( 'val' => 'alarm', 'type' => 'byte'),
			                                        array( 'type' => 'ulong', 'index' => 8, 'flag' => array( 'mercweap', 'mercarmor', 'mercclot', 'mercbook', 'mercingred', 'mercpick', 'mercprobe', 'merclight', 'mercapp', 'mercrep', 'mercmisc', 'mercspel', 'mercmagic', 'mercpot', 'train', 'spellmake', 'enchantmake', 'repair')))),
			      'AI_W' => array( 'keep' => -1),
			      'AI_T' => array( 'keep' => -1),
			      'AI_F' => array( 'keep' => -1),
			      'AI_E' => array( 'keep' => -1),
			      'CNDT' => array( 'keep' => -1),
			      'AI_A' => array( 'keep' => -1),
			      'DODT' => array( 'keep' => -1),
			      'DNAM' => array( 'keep' => -1),
			      'XSCL' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'scale', 'type' => 'float', 'default' => 1))));
		$this->_datadef['PROB'] =
			array('PBDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'quality', 'type' => 'float'),
			                                        array( 'val' => 'uses', 'type' => 'long'))));
		$this->_datadef['RACE'] =
			array('RADT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'skl1', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus1', 'type' => 'long'),
			                                        array( 'val' => 'skl2', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus2', 'type' => 'long'),
			                                        array( 'val' => 'skl3', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus3', 'type' => 'long'),
			                                        array( 'val' => 'skl4', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus4', 'type' => 'long'),
			                                        array( 'val' => 'skl5', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus5', 'type' => 'long'),
			                                        array( 'val' => 'skl6', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus6', 'type' => 'long'),
			                                        array( 'val' => 'skl7', 'type' => 'long', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'skbonus7', 'type' => 'long'),
			                                        array( 'val' => 'strength_m', 'type' => 'long'),
			                                        array( 'val' => 'strength_f', 'type' => 'long'),
			                                        array( 'val' => 'intelligence_m', 'type' => 'long'),
			                                        array( 'val' => 'intelligence_f', 'type' => 'long'),
			                                        array( 'val' => 'willpower_m', 'type' => 'long'),
			                                        array( 'val' => 'willpower_f', 'type' => 'long'),
			                                        array( 'val' => 'agility_m', 'type' => 'long'),
			                                        array( 'val' => 'agility_f', 'type' => 'long'),
			                                        array( 'val' => 'speed_m', 'type' => 'long'),
			                                        array( 'val' => 'speed_f', 'type' => 'long'),
			                                        array( 'val' => 'endurance_m', 'type' => 'long'),
			                                        array( 'val' => 'endurance_f', 'type' => 'long'),
			                                        array( 'val' => 'personality_m', 'type' => 'long'),
			                                        array( 'val' => 'personality_f', 'type' => 'long'),
			                                        array( 'val' => 'luck_m', 'type' => 'long'),
			                                        array( 'val' => 'luck_f', 'type' => 'long'),
			                                        array( 'val' => 'height_m', 'type' => 'float'), # long instead of float??
			                                        array( 'val' => 'height_f', 'type' => 'float'), # long instead of float??
			                                        array( 'val' => 'weight_m', 'type' => 'float'), # may not exist?? (based on Wrye)
			                                        array( 'val' => 'weight_f', 'type' => 'float'), # may not exist??
			                                        array( 'type' => 'ulong', 'flag' => array( 'playable', 'beast')))),
			      'NPCS' => array( 'keep' => 0,
			                       'multi' => 'NPCS',
			                       'data' => array( array( 'val' => 'spell_id', 'type' => 'edid'))),
			      'DESC' => array( 'keep' => 0, 'type' => 'blob'));
		$this->_datadef['REGN'] =
			array('WEAT' => array( 'keep' => -1),
			      'BNAM' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => -1),
			      'SNAM' => array( 'keep' => -1));
		$this->_datadef['REPA'] =
			array('RIDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => 'uses', 'type' => 'long'),
			                                        array( 'val' => 'quality', 'type' => 'float'))));
		$this->_datadef['SCPT'] =
			array('SCHD' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'script_name', 'type' => 'string', 'len' => 32),
			                                        array( 'val' => 'numshorts', 'type' => 'long'),
			                                        array( 'val' => 'numlongs', 'type' => 'long'),
			                                        array( 'val' => 'numfloats', 'type' => 'long'),
			                                        array( 'val' => 'scriptdatasize', 'type' => 'long'),
			                                        array( 'val' => 'localvarsize', 'type' => 'long'))),
			      'SCVR' => array( 'keep' => -1),
			      'SCDT' => array( 'keep' => -1),
			      'SCTX' => array( 'keep' => 0,
			                       'type' => 'blob'));
		$this->_datadef['SOUN'] =
			array('DATA' => array( 'keep' => -1 ));
		$this->_datadef['SKIL'] =
			array('INDX' => array( 'keep' => 2,
			                       'type' => 'mgefidl'),
			      'SKDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'attrib', 'type' => 'long', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'spec', 'type' => 'long', 'lookup' => 'speclist'),
			                                        array( 'val' => 'keep1', 'type' => 'float'),
			                                        array( 'val' => 'keep2', 'type' => 'float'),
			                                        array( 'val' => 'keep3', 'type' => 'float'),
			                                        array( 'val' => 'keep4', 'type' => 'float'))),
			      'DESC' => array( 'keep' => 0, 'type' => 'blob' ));
		$this->_datadef['SNDG'] =
			array('DATA' => array( 'keep' => -1),
			      'SNAM' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => -1));
		$this->_datadef['SPEL'] =
			array('SPDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'long', 'lookup' => 'SPEL_type'),
			                                        array( 'val' => 'spellcost', 'type' => 'long'),
			                                        array( 'type' => 'ulong', 'flag' => array( 'autocalc', 'pcstart', 'alwayssucc')),
			                                        array( 'val' => 'spellschool', 'type' => 'tinyint', 'lookup' => 'magic_school', 'extra' => 1))),
			      'ENAM' => array( 'keep' => 1,
			                       'multi' => 'ENAM',
			                       'data' => array( array( 'val' => 'effect_id', 'type' => 'mgefid'),
			                                        array( 'val' => 'eff_skill', 'type' => 'byte', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'eff_attrib', 'type' => 'byte', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'eff_range', 'type' => 'long', 'lookup' => 'effect_range'),
			                                        array( 'val' => 'eff_area', 'type' => 'long'),
			                                        array( 'val' => 'eff_dur', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmin', 'type' => 'long'),
			                                        array( 'val' => 'eff_magmax', 'type' => 'long'))));
		$this->_datadef['STAT'] = array();
		$this->_datadef['TES3'] =
			array('HEDR' => array( 'keep' => -1),
			      'MAST' => array( 'keep' => 0,
			                       'multi' => 'MAST',
			                       'subset' => 1,
			                       'data' => array(array( 'val' => 'master_file', 'type' => 'string'))),
			      'MAST-DATA' => array( 'keep' => 0,
			                            'data' => array( array( 'val' => 'master_sza', 'type' => 'ulong'),
			                                             array( 'val' => 'master_szb', 'type' => 'ulong'))));
		$this->_datadef['WEAP'] =
			array( 'WPDT' => array( 'keep' => 1,
			                        'data' => array(
			                                         array( 'val' => "weight", 'type' => "float" ),
			                                         array( 'val' => "value", 'type' => "long" ),
			                                         array( 'val' => "type", 'type' => "short", 'lookup' => 'WEAP_type'),
			                                         array( 'val' => "health", 'type' => "short" ),
			                                         array( 'val' => "speed", 'type' => "float" ),
			                                         array( 'val' => "reach", 'type' => "float" ),
			                                         array( 'val' => "enchantcost", 'type' => "short", 'factor' => 0.1 ),
			                                         array( 'val' => "chopmin", 'type' => "byte" ),
			                                         array( 'val' => "chopmax", 'type' => "byte" ),
			                                         array( 'val' => "slashmin", 'type' => "byte" ),
			                                         array( 'val' => "slashmax", 'type' => "byte" ),
			                                         array( 'val' => "thrustmin", 'type' => "byte" ),
			                                         array( 'val' => "thrustmax", 'type' => "byte" ),
# documentation confusing here....
			                                         array( 'type' => "ulong", 'flag' => array( NULL, "resistnw" )),
			                                         array( 'val' => 'skill', 'type' => 'ubyte', 'lookup' => 'skilllist', 'extra' => 1),
			                                         array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			       'ENAM' => array( 'keep' => 1,
			                        'multi' => "ENAM",
			                        'val' => 'enchant_id',
			                        'type' => 'edid'));
	}
	
	// cell pairs that should be kept separate (doors between the cells should not be followed when connecting cells together)
	// although connections may still be used to assign a cell to a worldspace (in case of a set of cells that does not itself
	// connect to outside world, e.g., for The Old Way)
	// each entry needs to be put in a separate array so that in cases where multiple separates are provided for a single cell,
	// they don't override each other
	protected function _get_cells_separate() {
		return
			array(array("Shallit" => "Drethan Ancestral Tomb"),
			      array("Vivec, Foreign Quarter Upper Waistworks" => "Vivec, Foreign Quarter Lower Waistworks"),
			      array("Vivec, Redoran Prison Cells" => "Vivec, Redoran Waistworks"),
			      array("Vivec, The Abbey of St. Delyn the Wise" => "Vivec, St. Delyn Waistworks"),
			      array("Vivec, St. Delyn Glassworker's Hall" => "Vivec, St. Delyn Waistworks"),
			      array("Ghostgate, Temple" => "Ghostgate, Tower of Dusk"),
			      array("Vivec, Elven Nations Cornerclub" => "Vivec, Hlaalu Waistworks"),
			      array("Ebonheart, Underground Caves" => "Ebonheart, Imperial Commission"),
			      array("Solstheim, Castle Karstaag, Caverns of Karstaag" => "Solstheim, Castle Karstaag, Banquet Hall"),
			      array("Mournhold, Sadri Manor" => "Old Mournhold: Residential Sewers"),
			      array("Old Mournhold: Palace Sewers" => "Old Mournhold: Residential Sewers"),
			      array("Old Mournhold: West Sewers" => "Old Mournhold: Residential Sewers"),
			      array("Old Mournhold: Palace Sewers" => "Old Mournhold: Bazaar Sewers"),
			      array("Old Mournhold: Palace Sewers" => "Old Mournhold: West Sewers"),
			     );
	}
	
	// names of cells that are to be preferentially used as primary entrance/exit to a set of cells with multiple entrances/exits
	// extra array provided for future possibility of setting x/y/z preferences to choose a single exit 
	
	// eventually may also want a set of secondary cells?  For example, to group merchants and state that a merchant is part of the Underworks rather than Waistworks.
	protected function _get_cells_preferred() {
		return
			array("Sadrith Mora, Tel Naga Great Hall" => array(),
			      "Sadrith Mora, Wolverine Hall" => array(),
			      "Sadrith Mora, Gateway Inn" => array(),
			      "Tel Aruhn, Tower Entry" => array(),
			      "Tel Branora, Lower Tower" => array(),
			      "Molag Mar, Waistworks" => array(),
			      "Vos, Varo Tradehouse Entrance" => array(),
			      "Tel Vos, Jail" => array(),
			      "Tel Uvirith, Tower Lower" => array(),
			      "Dagon Fel, The End of the World" => array(),
			      "Vivec, Telvanni Waistworks" => array(),
			      "Marandus, Lower Level" => array(),
			      "Vivec, Arena Waistworks" => array(),
			      "Vivec, High Fane" => array(),
			      "Vivec, St. Olms Waistworks" => array(),
			      "Vivec, Foreign Quarter Lower Waistworks" => array(),
			      "Vivec, Redoran Waistworks" => array(),
			      "Vivec, St. Delyn Waistworks" => array(),
			      "Vivec, Hlaalu Waistworks" => array(),
			      "Ebonheart, Grand Council Chambers" => array(),
			      "Kogoruhn, Hall of the Watchful Touch" => array(),
			      "Pelagiad, Fort Pelagiad" => array(),
			      "Moonmoth Legion Fort, Interior" => array(),
			      "Falasmaryon, Lower Level" => array(),
			      "Buckmoth Legion Fort, Interior" => array(),
			      "Maelkashishi, Shrine" => array(),
			      "Hlormaren, Keep, Bottom Level" => array(),
			      "Andasreth, Lower Level" => array(),
			      "Gnisis, Eggmine" => array(),
			      "Fort Frostmoth, General Quarters" => array(),
			      "Solstheim, Castle Karstaag, Throne Room" => array(),
			      "Solstheim, Mortrag Glacier: Entry" => array("worldspace" => "Solstheim"),
			      "Sotha Sil, Outer Flooded Halls" => array("worldspace" => "Mournhold"),
			      "Magas Volar" => array("worldspace" => "Vvardenfell"),
			      "Old Mournhold: West Sewers" => array("worldspace" => "Mournhold"),
			      "Mournhold, Royal Palace: Reception Area" => array(),
			     );
	}
	
	// Daveh says that 0x2000 is blocked; 0x400 persistent.  But since I'm not sure what those mean, and I'm not
	// necessarily interested in those flags, don't bother to set any flags at this point
	protected function _set_rec_flags($flags) {
		return;
	}
	
	protected function _do_special_sub($rectype, $type, $data) {
		if ($type == "NAME" && $rectype!="INFO") {
			if (is_null($this->_subset)) {
				$data = $this->process_string($data);
				$this->_parentlist->set_id($data);
				return $data;
			}
			else {
				return false;
			}
		}
		else if ($type=="INDX" && $rectype=="MGEF") {
			$this->_parentlist->set_id($this->process_raw_mgefid($data));
			return $data;
		}
		else if ($type=="INDX" && $rectype=="SKIL") {
			$this->_parentlist->set_id(str_replace('MGEF', 'SKIL', $this->process_raw_mgefid($data)));
			return $data;
		}
		// for now, not treating INFO record INAM/PNAM/NNAM records as full IDs (i.e., not looking them up or crosslinking)
		//		elseif ($type == "INAM" && $rectype=="INFO") {
		//		}
		
		return false;
	}
}

?>
