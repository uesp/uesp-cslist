<?php

require_once 'csread.inc';

// XML
// conditional block
// effect block
// script block
// records

// Notes about FO format:
// extra 4 bytes in header_group, header_rec
// EFID and other references to MGEF use standard type=formid instead of type=mgefstr
//  (i.e., references to mag.effs. use a number, not a four-character string such as FIDG)
//  -> should be able to quickly test in SR based on whether all MGEF EDIDs are four-characters long
// SKIL -> AVIF ?? but then where is action for experience? skill group? 

// Do tests on new header flags?

// actor values are mostly in same order as AVIF records 
//  -- but aggression/confidence/energy/mood completely out of place
// http://falloutmods.wikia.com/wiki/Actor_Value_List

// This now includes both Fallout3 and FalloutNewVegas filedefns
// But not sure about reading both into same database -> don't want FNV records being treated as
// modifiers to FO3 records
// If I really cared about FO3/FNV I would probably want to read each one into a separate DB,
// and have different derived classes for each.  But since this all just for testing purposes, I
// don't want to bother with the extra effort

class FORead extends CSRead {
	//	protected $_files = array("Fallout3.esm");
	//	protected $_fileofficial = array("Fallout3.esm");
	protected $_files = array("FalloutNV.esm");
	protected $_fileofficial = array("FalloutNV.esm");
	protected $_filenamespace = array("Fallout3");
	protected $_filemodname = array("");
	protected $_filemodabbrev = array('FO');
	
	protected $_header_group = array(
		'type' => 4,
		'size' => 'ulong',
		'label' => 4, // L, SS, formid
		'grtypenum' => 'ulong',
		'stamp' => 4, // date stamp?
		'flags_c' => 4,
		// records
	);
	protected $_header_rec = array(
		'type' => 4,
		'size' => 'ulong',
		'flags' => 'ulong',
		'formid' => 'raw_formid',
		'flags_b' => 4,
		'flags_c' => 4,
		// subrecords
	);

	// skip in .mod file:
	// IDLE, IDLM, IMAD, LAND, NAVI, NAVM, STAT, WTHR
	protected function _datadef_setup() {
		if (!empty($this->_datadef))
			return;
		$this->_parentlist->set_lookup('effect_range', array('Self', 'Touch', 'Target'));
		
		$this->_parentlist->set_lookup('PACK_type', array('Find', 'Follow', 'Escort', 'Eat', 'Sleep', 'Wander', 'Travel', 'Accompany', 'Use item at', 'Ambush', 'Flee not combat', 'Cast magic'));
		$this->_parentlist->set_lookup('PLDT_type', array('Near reference', 'In cell', 'Near current location', 'Near editor location', 'Object ID', 'Object type'));
		$this->_parentlist->set_lookup('PTDT_type', array('Specific reference', 'Object ID', 'Object type'));
		$this->_parentlist->set_lookup('ARMO_type', array( "Helmet", "Cuirass", "Greaves", "Boots", "Gauntlets", "Shield", "Other"));
		$this->_parentlist->set_lookup('CLOT_type', array( 'Hood', 'Pants/Skirt', 'Shoes', 'Shirt', 'Gloves', 'Ring', 'Amulet', 'Other'));
		
		//		$this->_parentlist->set_lookup('ENCH_type', array('Scroll', 'Staff', 'Weapon', 'Apparel'));
		$this->_parentlist->set_lookup('Worldspace', array('Test', 'Wasteland', 'DC', 'Other'));
		$this->_parentlist->set_lookup('Worldtype', array('Test', 'Main', 'City', 'Submap', 'Other'));
		
		// ENAM => EITM?
		$this->_datadef['any'] = 
			array('EDID' => array( 'keep' => 2, 'type' => 'string', 'desc' => 'Editor ID'),
			      'FULL' => array( 'keep' => 2, 'type' => 'string', 'val' => 'name', 'desc' => 'Full Name'),
			      'SCRI' => array( 'keep' => 0, 'type' => 'formid', 'desc' => 'ScriptID'),
			      'MODL' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path'),
			      'MOD2' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 2 (ground)'),
			      'MOD3' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 3 (F)'),
			      'MOD4' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 4 (F ground)'),
			      'MODT' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data'),
			      'MO2T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 2'),
			      'MO3T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 3'),
			      'MO4T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 4'),
			      'ICON' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Icon Path'),
			      'ICO2' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Icon Path 2'),
			      
			      'EITM' => array( 'keep' => 1, 'val' => 'enchant_id', 'type' => 'formid', 'multi' => 'EITM'),
			      
			     );
		// I may want to spend time experimenting with these extra values to get a head-start on understanding
		// them if they show up in Skyrim
		// XIBS, XPPA -> empty record: flag?
		// XLCM -> values 0/1/3: flag?
		// XPRD -> float, range 5->21
		$this->_datadef['ACHR'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'),
			                                        array( 'val' => 'rx', 'type' => 'float'),
			                                        array( 'val' => 'ry', 'type' => 'float'),
			                                        array( 'val' => 'rz', 'type' => 'float'))),
			      'NAME' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'base_id', 'type' => 'formid'),
			                                        array( 'val' => 'cell_id', 'type' => 'formid', 'extra' => 1))),
			      'XESP' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'parent_id', 'type' => 'formid'), // parent object/trigger
			                                        array( 'val' => NULL, 'type' => 'long'))),
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XLKR' => array( 'keep' => 0, 'type' => 'formid'), // REFR/ACHR/ACRE
			      'XMRC' => array( 'keep' => 0, 'val' => 'merccont_id', 'type' => 'formid'), //merchant container
			      'XSCL' => array( 'keep' => 0, 'val' => 'scale', 'type' => 'float')
			     );
		$this->_datadef['ACRE'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'),
			                                        array( 'val' => 'rx', 'type' => 'float'),
			                                        array( 'val' => 'ry', 'type' => 'float'),
			                                        array( 'val' => 'rz', 'type' => 'float'))),
			      'NAME' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'base_id', 'type' => 'formid'),
			                                        array( 'val' => 'cell_id', 'type' => 'formid', 'extra' => 1))),
			      'XESP' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'parent_id', 'type' => 'formid'), // parent object/trigger
			                                        array( 'val' => NULL, 'type' => 'long'))),
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XLKR' => array( 'keep' => 0, 'type' => 'formid'), // REFR/ACHR/ACRE
			      'XMRC' => array( 'keep' => 0, 'val' => 'merccont_id', 'type' => 'formid'), //merchant container
			      'XOWN' => array( 'keep' => 0, 'val' => 'owner_id', 'type' => 'formid'), //owner, could be NPC or faction
			      'XRNK' => array( 'keep' => 0, 'val' => 'owner_rank', 'type' => 'ulong'), //faction rank
			      'XSCL' => array( 'keep' => 0, 'val' => 'scale', 'type' => 'float')
			     );
		$this->_datadef['ALCH'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'))),
			      'ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array( '!autocalc', 'food', 'medicine')),
			                                        array( 'val' => 'withdrawal', 'type' => 'formid'),
			                                        array( 'val' => 'addiction_chance', 'type' => 'float'),
			                                        array( 'val' => 'consume_sound', 'type' => 'formid'))),
			      // FO addition -- Not sure what this value is
			      // 0=junk - damage rads; 10=drugs (but not all medicine) -- increase stats, mostly addiction chance; 11=stimpak; 12=meat/food; 13=alcohol
			      'ETYP' => array( 'keep' => 0, 'type' => 'long'),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'formid',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      // in FO effectb_id removed (length just 20 bytes); other values match up
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong'/*, 'lookup' => 'effect_range'*/),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong'/*, 'lookup' => 'actorlist'*/))),
			     );
		$this->_datadef['AMMO'] =
			// FO appears to have done away with weight, turned damage into a byte? (total DATA-size=13)
			// (although FOMM says final byte is unknown; byte value 1-100)
			array('DATA' => array ('keep' => 1,
			                       'data' => array( array( 'val' => 'speed', 'type' => 'float'),
			                                        array( 'type' => 'ulong', 'flag' => array('resistnw')),
			                                        array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'damage', 'type' => 'byte'))),
			      // FO addition
			      'ONAM' => array( 'keep' => 1,
			                       'type' => 'string',
			                       'desc' => 'Short Name - Displayed when viewing weapons'),
			     );
		// Parts of ARMO items?
		$this->_datadef['ARMA'] =
			array('BMDT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail')),
			                                        array( 'type' => 'ushort', 'flag' => array ('hide_ring', 'hide_amulet', NULL, NULL, NULL, NULL, 'unplayable', 'heavy')),
			                                        array( 'val' => 'type', 'type'=> 'byte', 'lookup' => 'ARMO_type', 'extra' => 1))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'health', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      // Equipment type
			      // 0=items that are carried such as launchers, hoses
			      // 3=knuckles/fists
			      // 7=backpack
			      // 8=head gear
			      // 9=globes
			      'ETYP' => array( 'keep' => 0, 'type' => 'long'),
			      'DNAM' => array( 'keep' => 0, 'type' => 'long', 'factor' => 0.01, 'desc' => 'Damage Resistance (DR)'),
			     );
		// meanings of the body location flags seem to have shifted
		// hand -> minimally used; foot / amulet / shield -> not used
		// right_ring -> pip boy?
		// left_ring -> backpack?
		// weapon, torch -> helmet-related
		// back_weapon -> neck?
		// side_weapon -> eyes?
		// quiver -> mouth?
		// tail -> collar?
		$this->_datadef['ARMO'] =
			array('BMDT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail')),
			                                        array( 'type' => 'ushort', 'flag' => array ('hide_ring', 'hide_amulet', NULL, NULL, NULL, NULL, 'unplayable', 'heavy')),
			                                        array( 'val' => 'type', 'type'=> 'byte', 'lookup' => 'ARMO_type', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'health', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      // Following all added by FO
			      'REPL' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'repair'), // repair items
			      'BIPL' => array( 'keep' => 0, 'val' => 'part_id', 'type' => 'formid'), // refs to ARMA entries
			      // Equipment type
			      // 0=items that are carried in hands
			      // 7=full outfits -> only 7s have BIPL entries
			      // 8=head gear
			      'ETYP' => array( 'keep' => 0, 'type' => 'long'),
			      'DNAM' => array( 'keep' => 0, 'type' => 'long', 'factor' => 0.01, 'desc' => 'Damage Resistance (DR)'),
			     );
		// actor value info
		$this->_datadef['AVIF'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob',
			                       'desc' => 'Description'),
			      'ANAM' => array( 'keep' => 0,
			                       'type' => 'string',
			                       'desc' => 'Alternate Name'),
			     );
		$this->_datadef['CELL'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('notravel', 'haswater', NULL, NULL, NULL, 'public', NULL, 'fake exterior')),
			                                        array( 'val' => 'exterior', 'type' => 'boolean', 'extra' => 1),
			                                        array( 'val' => 'worldtype', 'lookup' => 'Worldtype', 'extra' => 1),
			                                        array( 'val' => 'worldspace', 'type' => 'ubyte', 'lookup' => 'Worldspace', 'extra' => 1),
			                                        array( 'val' => 'parentcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'world_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extloc_x', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extloc_y', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extdoor_id', 'type' => 'edid', 'extra' => 1))),
			      'XCLL' => array( 'keep' => -1),
			      'XCMT' => array( 'keep' => 0,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('public_music', 'dungeon_music')))),
			      'XOWN' => array( 'keep' => 1,
			                       'val' => 'owner_id',
			                       'type' => 'formid'),
			      'XGLB' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'XRNK' => array( 'keep' => 1,
			                       'val' => 'owner_rank',
			                       'type' => 'long'),
			      'XCCM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'XCWT' => array( 'keep' => -1),
			      'XCLW' => array( 'keep' => 1,
			                       'val' => 'water_height',
			                       'type' => 'float',
			                       'default' => 0),
			      'XCLR' => array( 'keep' => 0,
			                       'type' => 'formid',
			                       'multi' => 'XCLR'),
			      'XCLC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'cell_x', 'type' => 'long'),
			                                        array( 'val' => 'cell_y', 'type' => 'long'))),
			      // FO additions
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XCIM' => array( 'keep' => 0, 'type' => 'formid'), // image?
			      'XCMO' => array( 'keep' => 0, 'type' => 'formid'), // music
			      'XCAS' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		$this->_datadef['CLAS'] =
			array('DESC' => array( 'keep' => 0,
			                       'desc' => 'Description',
			                       'type' => 'blob'),
			      'DATA' => array( 'keep' => 1,
			                       'desc' => 'Class Data',
			      // values appear to be AVIF-lookups -- but AVIF itself doesn't contain the indices
			                       'data' => array( array( 'val' => 'skl1', 'type' => 'long'),
			                                        array( 'val' => 'skl2', 'type' => 'long'),
			                                        array( 'val' => 'skl3', 'type' => 'long'),
			                                        array( 'val' => 'skl4', 'type' => 'long'),
			                                        array( 'val' => 'skl5', 'type' => 'long'),
			                                        array( 'val' => 'skl6', 'type' => 'long'),
			                                        array( 'val' => 'skl7', 'type' => 'long'))),
			      'ATTR' => array( 'keep' => 1,
			                       'desc' => 'Class SPECIAL Attributes',
			                       'data' => array( array( 'val' => 'att1', 'type' => 'byte'),
			                                        array( 'val' => 'att2', 'type' => 'byte'),
			                                        array( 'val' => 'att3', 'type' => 'byte'),
			                                        array( 'val' => 'att4', 'type' => 'byte'),
			                                        array( 'val' => 'att5', 'type' => 'byte'),
			                                        array( 'val' => 'att6', 'type' => 'byte'),
			                                        array( 'val' => 'att7', 'type' => 'byte')))
			     );
		
		// caravan money
		$this->_datadef['CMNY'] =
			array('DATA' => array('keep' => 1,
			                      'type' => 'long',
			                      'val' => 'value'),
			     );
		
		$this->_datadef['CONT'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array(NULL, 'respawn')),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      // closing sound; SNAM = opening sound
			      //			      'QNAM' => array( 'keep' => 0,
			      //			                       'type' => 'formid'),
			      'CNTO' => array( 'keep' => 1,
			                       'multi' => 'CNTO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'long'))),
			      // added by FO; tests confirm that COED is subset of CNTO
			      'CNTO-COED' => array( 'keep' => 0,
			                            'multi' => 'CNTO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			     );
		
		$this->_datadef['CREA'] =
			// impossible to tell whether  ACBS is similar in meaning
			// flags look similar; offset/calcmin/calcmax make sense
			array('ACBS' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array(NULL, 'essential', NULL, 'respawn', 'autocalc', NULL, NULL, 'offset')),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'fatigue_raw', 'type' => 'ushort'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'level_raw', 'type' => 'short'),
			                                        array( 'val' => 'calcmin', 'type' => 'ushort'),
			                                        array( 'val' => 'calcmax', 'type' => 'ushort'))),
			      'SNAM' => array( 'keep' => 0,
			                       'multi' => 'SNAM',
			                       'data' => array( array( 'val' => 'faction_id', 'type' => 'formid'),
			                                        array( 'val' => 'faction_rank', 'type' => 'byte'),
			                                        array( 'val' => 'faction_a', 'type' => 'byte'),
			                                        array( 'val' => 'faction_b', 'type' => 'byte'),
			                                        array( 'val' => 'faction_c', 'type' => 'byte'))),
			      'INAM' => array( 'keep' => 1,
			                       'val' => 'deathitem_id',
			                       'type' => 'formid'), // LVLI
			      'SPLO' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'val' => 'spell_id',
			                       'multi' => 'SPLO'),
			      'CNTO' => array( 'keep' => 1,
			                       'multi' => 'CNTO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'long'))),
			      // added by FO
			      'CNTO-COED' => array( 'keep' => 0,
			                            'multi' => 'CNTO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			      // aggression 0/1/2
			      'AIDT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'aggression', 'type' => 'ubyte'),
			                                        array( 'val' => 'confidence', 'type' => 'ubyte'),
			                                        array( 'val' => 'energy', 'type' => 'ubyte'),
			                                        array( 'val' => 'responsibility', 'type' => 'ubyte'))),
			      'PKID' => array( 'keep' => 0,
			                       'val' => 'package_id',
			                       'multi' => 'PKID',
			                       'type' => 'formid'),
			      // health_raw nearly always 0... but other values OKish (to extent they're even used)
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => NULL, 'type' => 'ubyte'),
			                                        array( 'val' => 'combat', 'type' => 'ubyte'),
			                                        array( 'val' => 'magic', 'type' => 'ubyte'),
			                                        array( 'val' => 'stealth', 'type' => 'ubyte'),
			                                        array( 'val' => NULL, 'type' => 'ubyte'),
			                                        array( 'val' => 'soul', 'type' => 'ubyte'/*, 'lookup' => 'soullist'*/),
			                                        array( 'val' => 'health_raw', 'type' => 'ushort'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'damage', 'type' => 'ushort'))),
			      // 0.15, 0.2, 0.25, 0.33, 0.5, 0.75, 1, 1.25
			      'BNAM' => array( 'keep' => 0, 'type' => 'float'),
			      // FO additions
			      // unknown decoding: DEST, DSTD, DSTF, EAMT, etc.
			      'CNAM' => array( 'keep' => 0, 'type' => 'formid'), // IPDS
			      // Audio template
			      'CSCR' => array( 'keep' => 0, 'type' => 'formid'), // CREA
			      // Weapon lists
			      'LNAM' => array( 'keep' => 0, 'type' => 'formid'), // FLST
			      // template?  appears to be
			      'TPLT' => array( 'keep' => 0, 'type' => 'formid'),
			      'ZNAM' => array( 'keep' => 0, 'type' => 'formid' /*ECZY*/),
			      // -106 -> 108
			      'RNAM' => array( 'keep' => 0, 'type' => 'byte'),
			      'TNAM' => array( 'keep' => 0, 'type' => 'float'),
			      'WNAM' => array( 'keep' => 0, 'type' => 'float'),
			     );
		
		// dehydration
		$this->_datadef['DEHY'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'mag', 'type' => 'long'),
			                                        array( 'val' => 'effect_id', 'type' => 'formid'))),
			     );
		// Enchantment
		$this->_datadef['ENCH'] =
			array('ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ulong'/*, 'lookup' => 'ENCH_type'*/),
			                                        array( 'val' => 'charge', 'type' => 'ulong'),
			                                        array( 'val' => 'enchantcost', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc')))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       //'type' => 'mgefstr',
			                       'type' => 'formid',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong'/*, 'lookup' => 'actorlist'*/))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong'/*, 'lookup' => 'magic_school'*/),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		// FormID list?? -- Some type of non-leveled list of items?
		// in which case should it be processed by levellist stuff?
		$this->_datadef['FLST'] = 
			array('LNAM' => array( 'keep' => 0,
			                       'type' => 'formid',
			                       'val' => 'item_id',
			                       'multi' => 'LNAM',
			                       'desc' => 'List Item'),
			     );
		// FO and OB; no fomm data
		$this->_datadef['FACT'] =
			array('CNAM' => array('keep' => 1,
			                      'val' => 'crimegold',
			                      'type' => 'float'),
			      // in OB I hvae this as formid+short, but not sure
			      // switch to FO-based format, although I don't know meaning of final val (0/1/2/3)
			      'XNAM' => array('keep' => 1,
			                      'multi' => 'XNAM',
			                      'data' => array( array( 'val' => 'opp_faction_id', 'type' => 'formid'),
			                                       array( 'val' => 'opp_modifier', 'type' => 'long'),
			                                       array( 'val' => 'opp_x', 'type' => 'long'))),
			      // FO also has flags at bits 8 and 9
			      'DATA' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('hidden', 'evil', 'special_combat')),
			      'RNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank',
			                       'type' => 'ushort'),
			      'MNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank_male',
			                       'type' => 'string'),
			      'FNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank_female',
			                       'type' => 'string'),
			     );
		// Global variable
		// According to FOMM, FNAM=115 means FLTV is int; FNAM=102 is float
		$this->_datadef['GLOB'] =
			array('FNAM' => array( 'keep' => 1,
			                       'type' => 'string',
			                       'len' => 1,
			                       'desc' => 'Value type'),
			      'FLTV' => array( 'keep' => 1,
			                       'type' => 'float',
			                       'desc' => 'Value'));
		// Game setting
		$this->_datadef['GMST'] =
			array('DATA' => array( 'keep' => 1,
			                       'type' => 'gmst',
			                       'desc' => 'Setting value'));
		// hunger
		$this->_datadef['HUNG'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'mag', 'type' => 'long'),
			                                        array( 'val' => 'effect_id', 'type' => 'formid'))),
			     );
		// item mod
		// connected to weapons by WMI1, WMI2, WMI3
		$this->_datadef['IMOD'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long' /* always 0 */))),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			     );
		$this->_datadef['LSCR'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'WMI1' => array( 'keep' => 1, 'val' => 'type_id', 'type' => 'formid'),
			     );
		// LSCT has a DATA subrec with lots of long numbers, but no way to guess at meaning
		$this->_datadef['LVLC'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each', 'useall')),
			      'LVLD' => array( 'keep' => 1,
			                       'type' => 'byte',
			                       'val' => 'chancenone'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'short'),
			                                        array( 'val' => NULL, 'type' => 'short'),
			                                        array( 'val' => 'item_id', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))),
			      'LVLO-COED' => array( 'keep' => 0,
			                            'multi' => 'LVLO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			      // chance none coming from a global variable
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		$this->_datadef['LVLI'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each', 'useall')),
			      'LVLD' => array( 'keep' => 1,
			                       'type' => 'byte',
			                       'val' => 'chancenone'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'short'),
			                                        array( 'val' => NULL, 'type' => 'short'),
			                                        array( 'val' => 'item_id', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))),
			      'LVLO-COED' => array( 'keep' => 0,
			                            'multi' => 'LVLO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			      // chance none coming from a global variable
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		$this->_datadef['LVLN'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each', 'useall')),
			      'LVLD' => array( 'keep' => 1,
			                       'type' => 'byte',
			                       'val' => 'chancenone'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'short'),
			                                        array( 'val' => NULL, 'type' => 'short'),
			                                        array( 'val' => 'item_id', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))),
			      'LVLO-COED' => array( 'keep' => 0,
			                            'multi' => 'LVLO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			      // chance none coming from a global variable
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		// Magic Effect
		$this->_datadef['MGEF'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob',
			                       'desc' => 'Description'),
			      // flag values unconfirmed for FO3
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('hostile', 'recover', 'detrimental', 'magpct', 'self', 'touch', 'target', 'nodur', 'nomag', 'noarea', 'fxpersist', 'spellmaking', 'enchanting', 'noingr', NULL, NULL, 'use_weapon', 'use_armor', 'use_creature', 'use_skill', 'use_attrib')),
			                                        array( 'val' => 'basecost', 'type' => 'float'),
			                                        array( 'val' => 'school', 'index' => 12, 'type' => 'ulong'/*, 'lookup' => 'magic_school'*/),
			                                        array( 'val' => 'resist', 'type' => 'long'),
			                                        array( 'val' => 'enchantfactor', 'index' => 56, 'type' => 'float'),
			                                        array( 'val' => 'barterfactor', 'type' => 'float'))));
		
		// Game messages
		$this->_datadef['MESG'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob',
			                       'desc' => 'Description'),
			      // INAM, DNAM, TNAM also exist -- each int with unknown meanin
			      // Each ITXT entry is a possible response to the message
			      // (no ITXTs if message doesn't want a response)
			      'ITXT' => array( 'keep' => 0,
			                       'type' => 'string',
			                       'multi' => 'ITXT',
			                       'desc' => 'Button 1 Text'),
			     );
		$this->_datadef['MISC'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		// FO addition but no fomm data
		// complete guess at DATA -> 1 byte 0->3
		// "scroll" might be holodisk/paper flag; cannot take looks reasonably good
		$this->_datadef['NOTE'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array( 'scroll', 'cannot_take')))),
			      // refQUST
			      'ONAM' => array( 'keep' => 0, 'type' => 'formid'),
			      // refNPC
			      'SNAM' => array( 'keep' => 0, 'type' => 'formid'),
			      // DDS
			      'XNAM' => array( 'keep' => 1, 'type' => 'string'),
			     );
		// none of this data comes from fomm.xml
		// want to (a) check whether it generally still seems to apply
		// (b) look for any sign of perks being assigned to NPCs
		// (c) figure out effective ways to decode/test unknown data
		$this->_datadef['NPC_'] =
			array('ACBS' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('female', 'essential', NULL, 'respawn', 'autocalc', NULL, NULL, 'offset', NULL, 'no_lowlevel', NULL, NULL, NULL, 'no_rumors', 'summonable', 'no_persuasion', NULL, NULL, NULL, NULL, 'corpse_check')),
			                                        array( 'val' => 'magicka_raw', 'type' => 'ushort'),
			                                        array( 'val' => 'fatigue_raw', 'type' => 'ushort'),
			                                        array( 'val' => 'gold', 'type' => 'short'),
			                                        array( 'val' => 'level_raw', 'type' => 'short'),
			                                        array( 'val' => 'calcmin', 'type' => 'ushort'),
			                                        array( 'val' => 'calcmax', 'type' => 'ushort'))),
			      'SNAM' => array( 'keep' => 0,
			                       'multi' => 'SNAM',
			                       'data' => array( array( 'val' => 'faction_id', 'type' => 'formid'),
			                                        array( 'val' => 'faction_rank', 'type' => 'byte'))),
			      'INAM' => array( 'keep' => 1,
			                       'val' => 'deathitem_id',
			                       'type' => 'formid'),
			      'RNAM' => array( 'keep' => 1,
			                       'val' => 'race_id',
			                       'type' => 'formid'),
			      'SPLO' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'val' => 'spell_id',
			                       'multi' => 'SPLO'),
			      'CNTO' => array( 'keep' => 1,
			                       'multi' => 'CNTO',
			                       'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'long'))),
			      'AIDT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'aggression', 'type' => 'ubyte'),
			                                        array( 'val' => 'confidence', 'type' => 'ubyte'),
			                                        array( 'val' => 'energy', 'type' => 'ubyte'),
			                                        array( 'val' => 'responsibility', 'type' => 'ubyte'),
			                                        array( 'type' => 'ulong', 'flag' => array( 'mercweap', 'mercarmor', 'mercclot', 'mercbook', 'mercingred', NULL, NULL, 'merclight', 'mercapp', NULL, 'mercmisc', 'mercspel', 'mercmagic', 'mercpot', 'train', NULL, 'recharge', 'repair')),
			                                        array( 'val' => 'trainskl', 'type' => 'ubyte'/*, 'lookup' => 'skilllist'*/),
			                                        array( 'val' => 'trainlvl', 'type' => 'ubyte'))),
			      'PKID' => array( 'keep' => 0,
			                       'val' => 'package_id',
			                       'multi' => 'PKID',
			                       'type' => 'formid'),
			      'CNAM' => array( 'keep' => 1,
			                       'val' => 'class_id',
			                       'type' => 'formid'),
			      // template?
			      'TPLT' => array( 'keep' => 0, 'type' => 'formid'),
			      /* length ranges from 11 to 25?  -- normally 11
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'vallist' => 'skilllist', 'valsuffix' => '_raw', 'type' => 'ubyte'),
			                                        array( 'val' => 'health_raw', 'type' => 'ulong'),
			                                        array( 'vallist' => 'attriblist', 'valsuffix' => '_raw', 'type' => 'ubyte'))),*/
			      // new subrecs:
			      // COED (12)
			      // EAMT (2)
			      // HCLR (4) -> hair color?
			      // OBND (12)
			      // PNAM (4)
			      // SCRI (4)
			      // TPLT (4)
			      // VTCK (4)
			      // ZNAM (4)
			      'PNAM' => array( 'keep' => -1), // refs to HDPT (head part? eyebrows, etc.)
			      // do I want to check what exactly these NAMs are for -> find out what rectype they reference?
			      'HNAM' => array( 'keep' => -1),
			      'LNAM' => array( 'keep' => -1),
			      'ENAM' => array( 'keep' => -1),
			      'HCLR' => array( 'keep' => -1),
			      'ZNAM' => array( 'keep' => -1),
			      'FGGS' => array( 'keep' => -1),
			      'FGGA' => array( 'keep' => -1),
			      'FGTS' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1)
			     );
		$this->_datadef['PACK'] =
			array('PKDT' => array( 'keep' => -1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('services', 'must_reach', 'must_complete', 'lock_start', 'lock_end', 'lock_location', 'unlock_start', 'unlock_end', 'unlock_location', 'continue_pc', 'once_per_day', NULL, 'skip_fallout', 'run', NULL, NULL, NULL, 'sneak', 'swim', 'fall', 'armor_unequip', 'weap_unequip', 'defense', 'horse', 'no_idle')),
			                                        array( 'val' => 'type', 'type' => 'long', 'lookup' => 'PACK_type'))),
			      'PSDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'month', 'type' => 'ubyte'),
			                                        array( 'val' => 'day', 'type' => 'ubyte'),
			                                        array( 'val' => 'date', 'type' => 'ubyte'),
			                                        array( 'val' => 'time', 'type' => 'ubyte'),
			                                        array( 'val' => 'duration', 'type' => 'ulong'))),
			      'PLDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'PLDT_type', 'type' => 'ulong', 'lookup' => 'PLDT_type', 'extra' => 1),
			                                        array( 'val' => 'location', 'type' => 'packid_PLDT'),
			                                        array( 'val' => 'radius', 'type' => 'ulong'))),
			      'PTDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'PTDT_type', 'type' => 'ulong', 'lookup' => 'PTDT_type', 'extra' => 1),
			                                        array( 'val' => 'target', 'type' => 'packid_PTDT'),
			                                        array( 'val' => 'count', 'type' => 'ulong'))),
			      'CTDA' => array( 'keep' => -1,
			                       'multi' => 'CTDA'));
		// Skill Perks
		$this->_datadef['PERK'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob',
			                       'desc' => 'Description'),
			      'DATA' => array( 'keep' => 1,
			                       // could first byte be placeholder for skill?
			                       // Skyrim version will also need to have prereq perk somehow
			                       'data' => array( array( 'val' => NULL, 'type' => 'byte'), // always 0  
			                                        array( 'val' => 'minlevel', 'type' => 'byte'),
			                                        array( 'val' => 'numranks', 'type' => 'byte'),
			                                        array( 'val' => 'selectable', 'type' => 'boolean'),
			                                        array( 'val' => 'temporary', 'type' => 'boolean'))),
			      'PRKE' => array( 'keep' => 1,
			                       'multi' => 'PRKE',
			                       'subset' => 1,
			                       // 0=Linked Quest Stage; 1=Linked SPEL; 2=Complex
			                       'data' => array( array( 'val' => 'perk_type', 'type' => 'byte'),
			                                        array( 'val' => 'rank_index', 'type' => 'byte'))),
			      //PRKE-DATA format depends upon perk_type
			      //need to have this entry here to ensure that DATA records aren't misread
			      //looks like technically PKRE section extends until a PKRF record is found
			      // plus should also be using PRKC record to get conditions
			      'PRKE-DATA' => array( 'keep' => 0,
			                            'len1' => 3,
			                            // 'Complex' (type=2) Effect Type ([X,3,3] for below affects combat stats depending on X: 0=normal damage  2=critical damage  6==damage to player limbs  8==VATS accuracy  36==critical chance AGAINST player"
			                            // type1=type_code; type2=param_1; type3=param_2
			                            // http://falloutmods.wikia.com/wiki/Perk_Effect_Complex_Codes
			                            'data1' => array( array('val' => 'type1', 'type' => 'byte'),
			                                              array('val' => 'type2', 'type' => 'byte'),
			                                              array('val' => 'type3', 'type' => 'byte')),
			                            //Linked SPEL record (type=1)
			                            'len2' => 4,
			                            'data2' => array( array('val' => 'spell', 'type' => 'formid')),
			                            //Linked QUST+stage (type=3)
			                            'len3' => 8,
			                            'data3' => array( array('val' => 'quest', 'type' => 'formid'),
			                                              // quest stage to trigger
			                                              // three more bytes follow this => stage data 2/3/4?
			                                              array('val' => 'qindx', 'type' => 'byte'))),
			     );
		$this->_datadef['QUST'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('enabled', 'repeat_topics', 'repeat_stages')),
			                                        array( 'val' => 'priority', 'type' => 'ubyte'))),
			      'INDX' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'ushort'),
			      'QSDT' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'ubyte',
			                       'flag' => array('complete')),
			      'SCHR' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'SCDA' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'SCTX' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'blob'),
			      'SCRO' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'QSTA' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'CTDA' => array( 'keep' => -1,
			                       'multi' => 'INDX'), // this actually branches with multiple CTDA for a single INDX
			      'CNAM' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'blob')); // multi on INDX-CTDA
		// radiation stage
		$this->_datadef['RADS'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'mag', 'type' => 'long'),
			                                        array( 'val' => 'effect_id', 'type' => 'formid'))),
			     );
		// recipe category
		$this->_datadef['RCCT'] =
			array('DATA' => array( 'keep' => 0,
			                       'val' => 'val1',
			                       'type' => 'byte'), // no idea what this byte is: -2, -1, 1, 60, 107
			     );
		// recipe
		// no fomm.xml data
		$this->_datadef['RCPE'] =
			array('DATA' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'skill', 'type' => 'long'), // AVIF -1 to 44
			                                        array( 'val' => 'skill_req', 'type' => 'long'),
			                                        array( 'val' => 'cat1', 'type' => 'formid'),
			                                        array( 'val' => 'cat2', 'type' => 'formid'))),
			      'RCIL' => array( 'keep' => 1,
			                       'val' => 'ingredient',
			                       'type' => 'formid',
			                       'multi' => 'RCIL',
			                       'subset' => 1),
			      'RCIL-RCQY' => array( 'keep' => 1,
			                            'val' => 'imult',
			                            'type' => 'long',
			                            'multi' => 'RCIL'),
			      'RCOD' => array( 'keep' => 1,
			                       'val' => 'product',
			                       'type' => 'formid',
			                       'multi' => 'RCOD',
			                       'subset' => 1),
			      'RCOD-RCQY' => array( 'keep' => 1,
			                            'val' => 'pmult',
			                            'type' => 'long',
			                            'multi' => 'RCOD'),
			     );
		$this->_datadef['REFR'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'),
			                                        array( 'val' => 'rx', 'type' => 'float'),
			                                        array( 'val' => 'ry', 'type' => 'float'),
			                                        array( 'val' => 'rz', 'type' => 'float'))),
			      'NAME' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'base_id', 'type' => 'formid'),
			                                        array( 'val' => 'cell_id', 'type' => 'formid', 'extra' => 1))),
			      'XMRK' => array( 'keep' => 1,
			                       'val' => 'map_marker',
			                       'type' => 'boolean'),  // boolean on presence/absence: data is zerolength
			      'FNAM' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('visible', 'travel')),
			      'XOWN' => array( 'keep' => 1,
			                       'val' => 'owner_id',
			                       'type' => 'formid'),
			      'XRNK' => array( 'keep' => 1,
			                       'val' => 'owner_rank',
			                       'type' => 'ulong'),
			      'XGLB' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'XSCL' => array( 'keep' => 1,
			                       'val' => 'scale',
			                       'type' => 'float'),
			      'XTEL' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'door_rev_id', 'type' => 'formid'),
			                                        array( 'val' => 'door_x', 'type' => 'float'),
			                                        array( 'val' => 'door_y', 'type' => 'float'),
			                                        array( 'val' => 'door_z', 'type' => 'float'),
			                                        array( 'val' => 'door_rx', 'type' => 'float'),
			                                        array( 'val' => 'door_ry', 'type' => 'float'),
			                                        array( 'val' => 'door_rz', 'type' => 'float'),
			                                        array( 'val' => 'door_dup_id', 'type' => 'formid', 'extra' => 1),
			                                        array( 'val' => 'cell_rev_id', 'type' => 'edid', 'extra' => 1))),
			      // this does not appear to be parent_id -- comes out as NULL for every REFR item?
			      /*			      'XTRG' => array( 'keep' => 0,
			                       'val' => 'parent_id',
			                       'type' => 'formid'),*/
			      'XLOC' => array( 'keep' => 1,
			                       'len1' => 12,
			                       'data1' => array( array( 'val' => 'lock_level', 'type' => 'ubyte'),
			                                         array( 'val' => 'key_id', 'index' => 4, 'type' => 'formid'),
			                                         array( 'type' => 'ulong', 'flag' => array(NULL, NULL, 'leveled'))),
			                       'len2' => 16,
			                       'data2' => array( array( 'val' => 'lock_level', 'type' => 'ubyte'),
			                                         array( 'val' => 'key_id', 'index' => 4, 'type' => 'formid'),
			                                         array( 'type' => 'ulong', 'index' => 12, 'flag' => array(NULL, NULL, 'leveled')))),
			      // copied from ACHR/ACRE, just blindly assuming it's the same for REFR
			      'XESP' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'parent_id', 'type' => 'formid'), // parent object/trigger
			                                        array( 'val' => NULL, 'type' => 'long'))),
			      'XCNT' => array( 'keep' => 1,
			                       'val' => 'count',
			                       'type' => 'ulong'),
			      'TNAM' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'marker',
			                       'lookup' => 'TNAM_type'),
			      'ONAM' => array( 'keep' => 0,
			                       'val' => 'default_open',
			                       'type' => 'boolean'),
			      // OB entries dropped: XACT, XLCM, XLOD, XPCI, XRTM, XSED, XTRG
			      
			      // Unknown additions from FO... others added, too, but even less idea on those
			      // XNDP(refNAVM), XLTW(refREFR-PWAT), XRDS(float), XEMI(refLIGH,refREGN), XPWR, XPRD, etc.
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XLKR' => array( 'keep' => 0, 'type' => 'formid'), // REFR/ACHR/ACRE
			      // refAMMO? Ammo provided in a weapon? -- and XAMC count of the ammo?
			      'XAMT' => array( 'keep' => 1, 'type' => 'formid'),
			      'XAMC' => array( 'keep' => 1, 'type' => 'formid'),
			     );
		// reputation within a group?
		$this->_datadef['REPU'] =
			array('DATA' => array( 'keep' => 0,
			                       'val' => 'rep',
			                       'type' => 'float'),
			     );
		$this->_datadef['SCPT'] =
			array('SCTX' => array( 'keep' => 1,
			                       'type' => 'blob'),
			     );
		// sleep deprivation
		$this->_datadef['SLPD'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'mag', 'type' => 'long'),
			                                        array( 'val' => 'effect_id', 'type' => 'formid'))),
			     );
		// FO SPEL_type: normal/targeted;0;radiation sickness;1;ability;4;poison;5;withdrawal;10
		// FO adds CTDA
		$this->_datadef['SPEL'] =
			array('SPIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ulong'/*, 'lookup' => 'SPEL_type'*/),
			                                        array( 'val' => 'spellcost', 'type' => 'long'),
			                                        array( 'val' => 'spelllevel', 'type' => 'ulong', 'lookup' => 'spelllevels'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc', NULL, 'start_spell', 'silence_immune', 'ignore_los', 'script_always', 'disallow reflect')),
			                                        array( 'val' => 'spellschool', 'type' => 'tinyint', 'lookup' => 'magic_school', 'extra' => 1))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'val' => 'effect_id',
			                       'type' => 'formid',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong'/*, 'lookup' => 'actorlist'*/))),
			     );
		$this->_datadef['WEAP'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'health', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'damage', 'type' => 'short'),
			                                        array( 'val' => 'capacity', 'type' => 'ubyte'),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'EAMT' => array( 'keep' => 1, 'type' => 'short', 'val' => 'enchantcost', 'desc' => 'Enchantment Charge Amount'),
			      'NAM0' => array( 'keep' => 1, 'type' => 'formid', 'val' => 'ammo', 'desc' => 'Ammo Type Record'),
			      'REPL' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'repair'), // repair items
			      // BIPL looks like link to generic version of a unique item
			      'BIPL' => array( 'keep' => 0, 'val' => 'part_id', 'type' => 'formid'), 
			      // 0: flamer; 1: plasma; 2: gun; 3: sword; 4: fist; 5: bomb; 6: mine
			      'ETYP' => array( 'keep' => 0, 'type' => 'long'),
			      'NNAM' => array( 'keep' => 1, 'type' => 'string', 'desc' => 'Projectile Node'),
			      'DNAM' => array( 'keep' => 1,
			                       'desc' => 'Damage Data',
			                       // anim unarmed;0;1H melee;1;2H melee;2;pistol 1;3;pistol 2;4;rifle 1;5;rifle 2;6;flamer/gatling/minigun;8;launcher;9;grenade;10;mine;12
			                       'data' => array( array( 'val' => 'anim_type', 'type' => 'long', 'desc' => 'Weapon Animation Group'),
			                                        array( 'val' => 'anim_time', 'type' => 'float', 'desc' => 'Animation Timescale'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'min_spread', 'type' => 'float', 'desc' => 'Minimum Spread'),
			                                        array( 'val' => 'max_spread', 'type' => 'float', 'desc' => 'Maximum Spread'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => 'zoom_fov', 'type' => 'float', 'desc' => 'Zoom FOV'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => 'proj', 'type' => 'formid', 'desc' => 'Projectile FormID'),
			                                        array( 'val' => NULL, 'type' => 'byte'),
			                                        array( 'val' => NULL, 'type' => 'byte'),
			                                        array( 'val' => 'proj_num', 'type' => 'byte', 'desc' => 'Number of Projectiles'),
			                                        array( 'val' => NULL, 'type' => 'byte'),
			                                        array( 'val' => 'min_range', 'type' => 'float', 'desc' => 'Minimum Range'),
			                                        array( 'val' => 'max_range', 'type' => 'float', 'desc' => 'Maximum Range'),
			                                        // (0=Normal, 1=Dismember, 2=Explode?, 3=None?)
			                                        // really wrong ... is it just a byte?
			                                        array( 'val' => 'onhit', 'type' => 'long', 'desc' => 'On Hit Dismemberment'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'mult', 'type' => 'float', 'desc' => 'Attack Multiplier'),
			                                        array( 'val' => 'ammo_per', 'type' => 'float', 'desc' => 'Ammo Used Per Shot'),
			                                        array( 'val' => 'ap_per', 'type' => 'float', 'desc' => 'AP Cost Per Shot'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => 'crit_death', 'type' => 'float', 'desc' => 'Critical Death Effect Chance??'),
			                                        array( 'val' => NULL, 'type' => 'float'),
			                                        array( 'val' => 'jam', 'type' => 'float', 'desc' => 'Jam Time'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'skill', 'type' => 'long', 'desc' => 'Weapon Skill Type (AV Code)'))),
			      'CRDT' => array( 'keep' => 0,
			                       'desc' => 'Critical Damage Data',
			                       'data' => array( array( 'val' => 'crit_bonus', 'type' => 'long', 'desc' => 'Critical Damage Bonus??'),
			                                        array( 'val' => 'crit_chance', 'type' => 'float', 'desc' => 'Critical Damage Chance??'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'crit_spell', 'type' => 'formid', 'desc' => 'Critical SPEL Effect'))),
			     );
		$this->_datadef['WRLD'] =
			array('WNAM' => array( 'keep' => 1,
			                       'type' => 'formid'),
			      'CNAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'NAM2' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'DATA' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array( 'small', 'notravel', 'oblivion')));
		
// Main plugin header
		$this->_datadef['TES4'] =
			array('HEDR' => array( 'keep' => -1),
			      'SCRN' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => -1),
			      'SNAM' => array( 'keep' => -1),
			      'MAST' => array( 'keep' => -1));
	}
}