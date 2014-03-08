<?php
require_once 'obread.inc';
require_once 'obdata.inc';

/* Extending OBRead because some records use standard OBRead data definitions */
class OBSaveRead extends OBRead {
	protected $_headerdef = array();
	protected $_readdata = array();
	protected $_changedef = array();
	protected $_changeorder = array();
	protected $_propertydef = array();
	protected $_ireflist = array();
	protected $_createdlist = array();
	protected $_change_endloc = NULL;
	// sdebug (save_debug) instead of just debug to avoid triggering debug messages in parent classes
	// eventual idea is use sdebug to switch entire class from operation mode to full test mode
	// 0 = operational, mode for running live on web site (pretty messages, keep running after error, etc)
	// 1 = almost operational -- command-line mode equivalent of web-mode
	// 10 = large scale debugging
	protected $_sdebug = 10;
	protected $_usetext = true;
	protected $_fptext = NULL;
	protected $_textreclocs = array();
	protected $_errorstats = array();
	protected $_currerrors = array();
	protected $_savefile = NULL;
	protected $_savefilesize = NULL;
	
	protected $_propstats = array();
	
	function __construct($savefile, $parentlist=NULL) {
		if (!isset($parentlist))
			$parentlist = 'OBData';
		parent::__construct($parentlist);
		$this->_savefile = $savefile;
		$this->_nespfile = NULL;
	}
	
	protected function _datadef_setup() {
		parent::_datadef_setup();
		$this->_headerdef =
			array('fileheader' => array(array('val' => 'fileId', 'type' => 'string', 'len' => 12),
			                            array('val' => 'majorVersion', 'type' => 'ubyte', 'skip' => true),
			                            array('val' => 'minorVersion', 'type' => 'ubyte'), // keep to check for ref bug
			                            array('val' => 'saveTime', 'type' => 'systemtime')),
			      'saveheader' => array(array('val' => 'headerVersion', 'type' => 'ulong', 'skip' => true),
			                            array('val' => '_datasize', 'type' => 'ulong'),
			                            array('val' => 'saveNum', 'type' => 'ulong'),
			                            array('val' => 'pcName', 'type' => 'string'),
			                            array('val' => 'pcLevel', 'type' => 'ushort'),
			                            array('val' => 'pcLocation', 'type' => 'string'),
			                            array('val' => 'gameDays', 'type' => 'float'),
			                            array('val' => 'gameTicks', 'type' => 'ulong'),
			                            array('val' => 'gameTime', 'type' => 'systemtime')),
			      'global' => array(array('val' => 'formIdsOffset', 'type' => 'ulong'),
			                        array('val' => 'recordsCount', 'type' => 'ulong'),
			                        array('val' => 'nextObjectId', 'type' => 'raw_formid'),
			                        array('val' => 'worldId', 'type' => 'formid'),
			                        array('val' => 'worldX', 'type' => 'ulong'),
			                        array('val' => 'worldY', 'type' => 'ulong'),
			                        array('val' => 'pcLoc_cell', 'type' => 'formid'),
			                        array('val' => 'pcLoc_x', 'type' => 'float'),
			                        array('val' => 'pcLoc_y', 'type' => 'float'),
			                        array('val' => 'pcLoc_z', 'type' => 'float'),
			                        array('val' => 'globalsCount', 'type' => 'ushort'),
			                        array('val' => 'globals',
			                              'skip' => true,
			                              'len' => 8,
			                              'data' => array(array('val' => 'iref', 'type' => 'ulong', 'skip' => true),
			                                              array('val' => 'value', 'type' => 'float', 'skip' => true))),
			                        array('val' => 'tesClassCount', 'type' => 'ushort'),
			                        array('val' => 'tesClass', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'processesCount', 'type' => 'ushort'),
			                        array('val' => 'processes', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'specEventCount', 'type' => 'ushort'),
			                        array('val' => 'specEvent', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'weatherCount', 'type' => 'ushort'),
			                        array('val' => 'weather', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => NULL, 'type' => 'ulong'),
			                        array('val' => 'createdCount', 'type' => 'ulong'),
			                        array('val' => 'created', 'sub' => 'read_created_rec'),
			                        array('val' => 'quickKeysCount', 'type' => 'ushort'),
			                        array('val' => 'quickKeys', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'reticuleCount', 'type' => 'ushort'),
			                        array('val' => 'reticule', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'interfaceCount', 'type' => 'ushort'),
			                        array('val' => 'interface', 'type' => 'ubyte', 'skip' => true),
			                        array('val' => 'regionsSize', 'type' => 'ushort'),
			                        array('val' => 'regionsCount', 'type' => 'ushort'),
			                        array('val' => 'regions',
			                              'skip' => true,
			                              'len' => 8,
			                              'data' => array(array('val' => 'iref', 'type' => 'ulong', 'skip' => true),
			                                              array('val' => 'value', 'type' => 'ulong', 'skip' => true)))),
			                             );
		// Array keys are the values used in primary flag to signal which changeorder subrecords are present
		// Order in which keys appear in these arrays are the order in which the subrecords appear
		// QUST data sizes presumably controlled by information specific to each quest
		// (because there's nothing in save file data to signal how large quest
		$this->_changeorder =
			// 3->4->6->5->2->28->9 from player NPC_ data
			// 9->10 
			// 6->8->2
			// 2->7->9
			// 5->8
			// 28 -> 7 -> 9
			// 0->3->4->6->7
			array('NPC_' => array( 'Form Flags' => 0,
			                       'Base Attributes' => 3,
			                       'Base Data' => 4,
			                       'Factions' => 6,
			                       'Spell List' => 5,
			                       'AI Data' => 8,
			                       'Base Health' => 2,
			                       'Base Modifiers' => 28,
			                       'Full Name' => 7,
			                       'Skills' => 9,
			                       'Combat Style' => 10),
			      'CREA' => array( 'Form Flags' => 0,
			                       'Base Attributes' => 3,
			                       'Base Data' => 4,
			                       'Factions' => 6,
			                       'Spell List' => 5,
			                       'AI Data' => 8,
			                       'Base Health' => 2,
			                       'Base Modifiers' => 28,
			                       'Full Name' => 7,
			                       'Skills' => 9,
			                       'Combat Style' => 10),			      // would be nice to also pick up
			      //  Life State (dead npcs)
			      //  Enabled/disabled (removed dead npcs)
			      'ACHR' => array( 'Created' => 1,
			                       'Moved' => 2,
			                       'Havok Moved' => 3,
			                       'Oblivion Flag' =>23,
			                       'Unknown876' => '$data[\'is_pc\']==true',
			                       'Actor Flag' => true,
			                       'Form Flags' => 0,
			                       'Inventory' => 27,
			                       'Actor States' => true,
			                       'Follow' => 17,
			                       // Follow is a 32? byte section directory following actor states
			                       // There are alot of other records between Inventory and PC Data,
			                       // but I'm skipping them all because figuring out their lengths is 
			                       // more trouble than it's worth -- and I'm not interested in any
			                       // of the data
			                       // Instead, PC Data just scans for some known bytes and then starts reading
			                       //'Havok Moved' => 3,
			                       //'Scale' => 4,
			                       //'AI Ref' => '$data[\'actor_flag\']>=0',
			                       // 'Dialogue' => ?? // triggered, but trigger value unknown
			                       'PC Data' => '$data[\'actor_flag\']==0'
			                     ),
			      // 26 -> 27 (assuming 27 is ulong, and value<60000 more likely... and value>0 more likely)
			      // 27 -> 3 -> 28
			      // 1, 2 are zero-size
			      // 4 -> 5 -> 24 confirmed in test saves
			      'CELL' => array( 'Cell Created' => 1,
			                       'UnkCELL2' => 2,
			                       'UnkCELL26' => 26,
			                       'Detach Time' => 27,
			                       'Cell Flags' => 3, 
			                       // whether size starts at 0/32/36 unknown -- coming from somewhere other than save file
			                       'Seen Data' => 28,
			                       // relative position of 24/4/5 unknown
	 		                       'Full Name' => 4,
			                       'Ownership' => 5,
			                       'Pathgrid Modified' => 24),
			      'REFR' => array( 'Cell Changed' => 31,
			                       // 2 and 3 seem to lead to same single record
			                       'Created' => 1,
			                       'Moved' => 2,
			                       'Havok Moved' => 3,
			                       'Oblivion Flag' => 23,
			                       'Form Flags' => 0,
			                       'Inventory' => 27,
			                       /* rest needs to be handled some other way */
			                     ),
			      'INFO' => array( 'Topic Said Once' => 28 ),
			      'KEYM' => array( 'Form Flags' => 0,
			                       'Value' =>3 ),
			      'SLGM' => array( 'Form Flags' => 0,
			                       'Value' => 3 ),
			      'INGR' => array( 'Form Flags' => 0,
			                       'Value' => 3 ),
			      // relative order confirmed 3->2
			      'FACT' => array( 'Faction Reactions' => 3,
			                       'Faction Flags' => 2),
			      'ARMO' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'AMMO' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'ALCH' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'MISC' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'WEAP' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'BOOK' => array( 'Form Flags' => 0,
			                       'Value' => 3,
			                       'Book Skill' => 2),
			      'CLOT' => array( 'Form Flags' => 0,
			                       'Value' => 3),
			      'PACK' => array( 'Never Run Flag' => 28),
			      'QUST' => array( 'Quest Flags' => 2,
			                       'Quest Stages' => 28,
			                       'Quest Script' => 27),
			      'generic' => array( 'Form Flags' => 0,
			                          'Value' =>3 )
			     );
		
		$this->_changedef =
			array('Form Flags' => array('keep' => 0,
			                            'val' => 'form_flags',
			                            'type' => 'ulong'),
			      'Base Health' => array('keep' => 1,
			                             'val' => 'health_base',
			                             'type' => 'ulong'),
			      'Base Attributes' => array('keep' => 1,
			                                 'len' => 8,
			                                 'data' => array( array( 'val' => 'strength_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'intelligence_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'willpower_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'agility_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'speed_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'endurance_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'personality_base', 'type' => 'ubyte'),
				                                 array( 'val' => 'luck_base', 'type' => 'ubyte'))),
			      'Base Data' => array('keep' => 1,
			                           'len' => 16,
			                           'data' => array( array( 'type' => 'ulong', 'flag' => array('female', 'essential', NULL, 'respawn', 'autocalc', NULL, NULL, 'offset', NULL, 'no_lowlevel', NULL, NULL, NULL, 'no_rumors', 'summonable', 'no_persuasion', NULL, NULL, NULL, NULL, 'corpse_check')),
			                                            array( 'val' => 'magicka_base', 'type' => 'ushort'),
			                                            array( 'val' => 'fatigue_base', 'type' => 'ushort'),
			                                            array( 'val' => 'gold', 'type' => 'short'),
			                                            array( 'val' => 'level_base', 'type' => 'short'),
			                                            array( 'val' => 'calcmin', 'type' => 'ushort'),
			                                            array( 'val' => 'calcmax', 'type' => 'ushort'))),
			      'Spell List' => array('keep' => 0,
			                            'data' => array( array( 'val' => 'spell_irefCount', 'type' => 'ushort'),
			                                             array( 'val' => 'spell_iref', 'type' => 'ulong'))), // irefs
			      'Factions' => array('keep' => 0,
			                          'data' => array( array( 'val' => 'factionCount', 'type' => 'ushort'),
			                                           array( 'val' => 'faction',
				                                               'len' => 5,
				                                           'data' => array ( array ( 'val' => 'iref', 'type' => 'ulong'),
					                                           array ( 'val' => 'rank', 'type' => 'byte' ))))),
			      'Full Name' => array('keep' => 1,
			                           'data' => array( array( 'val' => 'fullName', 'type' => 'string'))),
			      'AI Data' => array('keep' => -1,
			                         'val' => NULL,
			                         'type' => 'ulong'),
			      'Skills' => array('keep' => 1,
			                        'len' => 21,
			                        'data' => array( array( 'val' => 'armorer_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'athletics_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'blade_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'block_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'blunt_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'hand_to_hand_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'heavy_armor_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'alchemy_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'alteration_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'conjuration_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'destruction_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'illusion_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'mysticism_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'restoration_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'acrobatics_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'light_armor_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'marksman_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'mercantile_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'security_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'sneak_base', 'type' => 'ubyte'),
			                                         array( 'val' => 'speechcraft_base', 'type' => 'ubyte'))),
			      'Base Modifiers' => array('keep' => -1,
			                                'data' => array( array( 'val' => 'modCount', 'type' => 'ushort' ),
				                                array( 'val' => 'mod',
				                                       'len' => 5,
				                                       'data' => array( array( 'val' => 'mod_index', 'type' => 'ubyte'),
					                                       array( 'val' => 'mod_val', 'type' => 'float'))))),
			      'Cell Changed' => array('keep' => 0,
			                       'len' => 16,
			                       // moved_cell=0 if cell hasn't changed?
			                       // or else why is moved_cell 0 so often?
			                       'data' => array( array( 'val' => 'cell', 'type' => 'ulong' /* iref */),
			                                        array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'))),
			      'Moved' => array('keep' => 0,
			                       'len' => 28,
			                       // moved_cell=0 if cell hasn't changed?
			                       // or else why is moved_cell 0 so often?
			                       'data' => array( array( 'val' => 'cell', 'type' => 'ulong' /* iref */),
			                                        array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'),
			                                        array( 'val' => 'rx', 'type' => 'float'),
			                                        array( 'val' => 'ry', 'type' => 'float'),
			                                        array( 'val' => 'rz', 'type' => 'float'))),
			      'Created' => array('keep' => 0,
			                         'len' => 36,
			                         'data' => array( array( 'val' => 'create_flag', 'type' => 'ulong'),
			                                          array( 'val' => 'base_iref', 'type' => 'ulong' /*iref*/),
				                                       array( 'val' => 'cell', 'type' => 'ulong' /* iref */),
			                                          array( 'val' => 'x', 'type' => 'float'),
			                                          array( 'val' => 'y', 'type' => 'float'),
			                                          array( 'val' => 'z', 'type' => 'float'),
			                                          array( 'val' => 'rx', 'type' => 'float'),
			                                          array( 'val' => 'ry', 'type' => 'float'),
			                                          array( 'val' => 'rz', 'type' => 'float'))),
			      'Actor Flag' => array('keep' => 10,
			                            'len' => 1,
			                            'type' => 'ubyte',
			                            'val' => 'actor_flag'),
			      'Unknown876' => array('val' => 'damage',
			                            'count' => 219,
			                            'type' => 'float'),
			      'Inventory' => array('keep' => 1,
			                           'data' => array( array( 'val' => 'itemCount', 'type' => 'ushort'),
			                                            array( 'val' => 'item', 'sub' => 'read_inventory_record'))),
			      'PC Data' => array('keep' => 1,
			                         'sub' => 'read_pcdata'),
			      'Actor States' => array('keep' => 1,
			                              'sub' => 'read_property_structure'),
			      'Cell Created' => array('val' => NULL,
			                              'len' => 0,
			                              'type' => 'bytes'),
				   'UnkCELL2' => array('val' => NULL,
				                       'len' => 0,
				                       'type' => 'bytes'),
				   'UnkCELL26' => array('val' => NULL,
				                        'len' => 4,
				                        'type' => 'bytes'),
			      'Detach Time' => array('val' => NULL,
			                             'type' => 'ulong'),
			      'Cell Flags' => array('val' => NULL,
			                            'type' => 'ubyte'),
			      'Had Havok Move Flag' => array('val' => NULL,
			                             'data' => array( array( 'val' => 'havokCount', 'type' => 'ushort'),
			                                              array( 'val' => 'havok', 'type' => 'ubyte'))),
			      'Animation' => array('val' => NULL,
			                           'data' => array( array( 'val' => 'animCount', 'type' => 'ushort'),
				                                         array( 'val' => 'anim', 'type' => 'ubyte'))),
			      'Oblivion Flag' => array('val' => 'oblivion_flag',
			                               'type' => 'ulong' /* iref or formid? */),
			      'Topic Said Once' => array('val' => NULL,
			                                 'len' => 0,
			                                 'type' => 'bytes'),
			      'Seen Data' => array('val' => NULL,
			                           'sub' => 'read_seen_data'),
			      'Faction Flags' => array('val' => NULL,
			                               'type' => 'ubyte'),
			      'Faction Reactions' => array('val' => NULL,
			                                   'data' => array( array( 'val' => 'reactionCount', 'type' => 'ushort'),
				                                   array('val' => 'reaction', 'type' => 'bytes', 'len' => 8))), /* FACT_iref, ubyte_reaction, ubyte[3]_flag*/
			'Book Skill' => array('val' => NULL,
			                         'type' => 'ubyte'),
			'Never Run Flag' => array('val' => NULL,
			                        'len' => 0,
			                        'type' => 'bytes'),
			      /* This works in 90% of cases... but occasionally it reads too much
			         No explanation for shorter cases found yet */
			'Quest Script' => array('val' => NULL,
			                        'data' => array( array( 'val' => 'qscptCount', 'type' => 'ushort'),
			                                         array( 'val' => NULL, 'type' => 'byte'),
			                                         array( 'val' => 'qscpt', 'type' => 'bytes', 'len' => 12))),
			'Quest Stages' => array('val' => NULL,
			                        'data' => array( array( 'val' => 'qstageCount', 'type' => 'ubyte'),
			                                         array( 'val' => 'qstage',
				                                         'data' => array(array( 'val' => 'qstg_indx', 'type' => 'ubyte'),
					                                         array('val' => 'qstg_flag', 'type' => 'ubyte'), // 0/1
					                                         array('val' => 'qstg_subCount', 'type' => 'ubyte'),
					                                         array('val' => 'qstg_sub', 'type' => 'bytes', 'len' => 5))))), // index(ubyte, counts up from start of qstage), float
			'Quest Flags' => array('val' => NULL,
			                       'type' => 'ubyte'),
			      'Ownership' => array('val' => 'owner_iref',
			                           'type' => 'ulong'),
			      'Value' => array('val' => 'value',
			                           'len' => 4,
			                           'type' => 'ulong'),
			      'Pathgrid Modified' => array('val' => NULL,
			                                   'data' => array(array('val' => 'pgridCount', 'type' => 'ushort'),
				                                                array('val' => 'pgrid', 'type' => 'ushort'))),
			      'Combat Style' => array('val' => NULL, 'len' => 4, 'type' => 'ulong'), /* iref to CSTY object */
			      // text dumps always show 32 bytes
			      // from identifiable cases it's possible that format is variable length
			      // (id, ushort, ushortCount, byte[ushortCount])
			      'Follow' => array('val' => NULL,
			                        'data' => array(array('val' => 'id', 'type' => 'ulong'),
			                                        array('val' => NULL, 'type' => 'bytes', 'len' => 28)))
			);
		
		// Sequences in which rare/questioned properties have appeared
		// 0x25 confirmed:
		// ACHR: 0x11 -> 0x12 -> 0x1e -> *0x25* (subsequent float,0x00,0x00) ... used for Soul of Kewan and other Souls from Peryite's Realm (all with same data structure)... in file that has completed Peryite's quest; textdumps state "Disabled"
		// ACHR: 0x11 -> 0x12 -> *0x25* (0 byte) -> 0x1e (0x1e iref confirmed)... used for Forlorn Watchman after quest done.  Also textdump lists state as "Disabled"
			                                                                            
		// 0x3d confirmed (crimeGold)
		// ACHR: 0x11 -> 0x4f -> 0x1e -> *0x3d* -> 0x1f -> 0x4b (0x1e's iref confirmed; size of 0x4b confirmed) -> City-Swimmer (known thief)
			                                                                            
		// 0x52 confirmed
		// ACHR: 0x12 -> 0x4f -> 0x5c -> 0x1e -> *0x52* (subsq. 1 iref) -> 0x59 (0x1e's iref confirmed; 0x59's string confirmed) -> Thoronir
		// ACHR: 0x12 -> 0x5c -> 0x1e -> *0x52* (subsequent data is two irefs); 0x1e's iref confirmed (i.e., 0x5c looks good)
		
		// ACRE (Unicorn): 0x1e -> *0x4e* (2 byte) -> 0x4b (length of 0x4b confirmed)
		// ACHR/Inv 0x1c: ring  (plain ACHR cases probably mis-identified PC data)
		// REFR: *0x2e* alone, for Staff of Burden
		// ACHR/Inv 0x1c: but points to gold???... In next file same NPC (Farwil) has Knights of Thorn Medallion
		// ACHR/Inv 0x1c: points to cowl of nocturnal; another case to Ancotar's ring; order priest's robe
		// ACHR: 0x11 -> 0x12 -> 0x4f -> 0x1e ->  (0x1e's iref confirmed, i.e. 0x4f looks good) -> *0x4e* (0x00,0x00)
		// ACRE: 0x11 -> 0x12 -> 0x27 -> 0x4f -> *0x4e* (0x00,0x00) -> 0x1f -> 0x1e (0x1e's iref confirmed) -> Maborel's Horse
		// Also have *0x4e* -> 0x4b, where size of 0x4b confirmed
		// ACRE: 0x11 -> 0x4f -> 0x1e -> *0x4e* (0x1,0x0,0x63,0x1,0x0,0x0,0x4,0x0,0x6f,0xe7,0xbb,0x40) -> 0x4b (0x1e's iref confirmed; 0x4b size confirmed) -> Starving Mountain Lion; 0x4e looks like iref,short,float
		// Another 0x4e: 0x1,0x0,0x63,0x1,0x0,0x0,0x1,0x0,0x4c,0x0c,0xfa,0x40
		//   other values for middle ushort: 0x3,
		// REFR: *0x32* -> 0x4a -> 0x4b (0x4a string confirmed; 0x4b size consistent)
		// REFR: 0x11 -> 0x12 -> 0x4f -> 0x1e -> 0x4e -> *0x5a* (1byte) -> 0x23 (tough to confirm 0x23..)
		$this->_propertydef =
			// first definitions are ones taken from old UESP documentation
			array(0x1b => array('val' => 'equipped', 'len' => 0),
			      0x1c => array('val' => 'equipped', 'len' => 0),
			      0x2a => array('val' => 'affectedItemsNum', 'type' => 'ushort',
			                    'section' => 'All Extra'),
			      0x2b => array('val' => 'itemHealth', 'type' => 'float', 'skip' => true,
			                    'section' => 'All Extra'),
			      0x2d => array('val' => 'time', 'type' => 'float', 'skip' => true),
			      0x2e => array('val' => 'enchPoints', 'type' => 'float', 'skip' => true, 'section' => 'All Extra'),
			      0x2f => array('val' => 'soul', 'type' => 'byte', 'skip' => true),
			      0x12 => array('val' => 'scriptdata', 'skip' => true,
			                    'section' => 'Script',
			                    'data' => array(array('val' => 'SCPT_iref', 'type' => 'ulong'),
			                                    array('val' => 'scriptvarCount', 'type' => 'ushort'),
			                                    array('val' => 'scriptvar', 'sub' => 'read_script_varindex'),
			                                    array('val' => NULL, 'type' => 'byte'))),
			      // placedObject
			      0x22 => array('val' => 'REFRACHRACRE_iref', 'type' => 'ulong' /* iref */, 'skip' => true,
			                   'section' => 'All Extra'),
			      // globVar
			      0x28 => array('val' => 'GLOB_iref', 'type' => 'ulong' /* iref */, 'skip' => true),
			      0x29 => array('val' => 'factRank', 'type' => 'ulong', 'skip' => true),
			      0x36 => array('val' => NULL, 'len' => 5, 'type' => 'bytes'),
			      0x37 => array('val' => 'scale', 'type' => 'float', 'skip' => true),
			      0x55 => array('val' => 'shortcutkey', 'type' => 'byte', 'skip' => true),
			      
			      // entries from ACHR actor states section
			      0x11 => array('val' => 'WRLD_iref', 'type' => 'ulong', 'skip' => true), // worldspace
			      0x1e => array('val' => 'xMarkerHeadingRef', 'len' => 20, 'skip' => true,
			                    'data' => array(array('val' => 'WRLDCELL_iref', 'type' => 'ulong'),
			                                    array('val' => 'x', 'type' => 'float'),
			                                    array('val' => 'y', 'type' => 'float'),
			                                    array('val' => 'z', 'type' => 'float'),
			                                    array('val' => 'flags', 'type' => 'ulong'))),
			      // originally listed as 8 bytes..
			      // but clearly is 14 bytes (are 6 additional bytes always blank?)
			      0x1f => array('val' => 'AIPack', 'len' => 14, 'skip' => true,
			                    'data' => array(array('val' => 'PACK_iref', 'type' => 'ulong'),
			                                    array('val' => 'num', 'type' => 'ulong'),
			                                    array('val' => 'unknown', 'type' => 'bytes', 'len' => 6))),
			      // Trespass: first four bytes clearly have some significance (identifier)
			      // since they're repeated in text dump
			      0x20 => array('val' => 'Trespass', 'len' => 63, 'type' => 'bytes', 'section' => 'Trespass', 'skip' => true),
			      // 0x21 -> mostly unkCount=1, but one case of unkCount=2 confirmed
			      0x21 => array('val' => NULL, 'skip' => true,
			                    'data' => array(array('val' => 'unkCount', 'type' => 'ushort'),
			                                    array('val' => 'unk', 'len' => 5,
			                                          'data' => array(array('val' => 'PACK_iref', 'type' => 'ulong'),
			                                                          array('val' => 'unk2', 'type' => 'ubyte'))))),
			      // fairly confident: testable cases with count=1,2,4
			      0x23 => array('val' => NULL, 'skip' => true,
			                    'data' => array(array('val' => 'ACHR_irefCount', 'type' => 'ushort'),
			                                    array('val' => 'ACHR_iref', 'type' => 'ulong' /*iref*/))),
		                  // 0x23 => array('val' => NULL, 'len' => 20, 'type' => 'bytes', 'skip' => true), // length unknown
			      0x25 => array('val' => 'disabled', 'len' => 0, 'skip' => true),
			      0x35 => array('val' => 'leveledCreature', 'len' => 0, 'skip' => true,
			                    'section' => 'Leveled Creature'),
			      0x3d => array('val' => 'crimeGold', 'type' => 'float', 'section' => 'Crime Gold'),
			      // Movement Extra
			      0x4b => array('val' => NULL,
			                    'section' => 'Movement Extra',
			                    'data' => array(array('val' => 'move_long', 'type' => 'ulong'),
			                                    array('val' => 'moveExCount', 'type' => 'ushort'),
			                                    array('val' => 'moveEx', 'type' => 'ubyte'))),
			      // have one clear case of ushort=0, and total length=2
			      // other clear cases of ushort=1, and total length=12
			      // but if len=2, then 0x5a (90) is another flag....
			      0x4e => array('val' => NULL,
			                    'data' => array(array('val' => 'unkCount', 'type' => 'ushort'),
			                                    array('val' => 'unk', 'type' => 'bytes', 'len' => 10))),
			      0x4f => array('val' => NULL, 'len' => 4, 'type' => 'bytes'),
			      // appears on merchant records, values 0-37
			      // but NOT investment gold
			      0x5c => array('val' => NULL, 'type' => 'float'),
			      // removing 0x6d for now because it has not been confirmed, and gap from next-highest flag is 17
			      //			      0x6d => array('val' => NULL, 'len' => 24, 'type' => 'bytes'),
			      // conversation
			      0x59 => array('val' => NULL, 'skip' => true,
			                    'data' => array(array('val' => 'conv_topic', 'type' => 'string'),
			                                    array('val' => NULL, 'type' => 'ubyte' /* zero to end string*/),
			                                    array('val' => 'byte1', 'type' => 'ubyte'), /* not!! a ushort to provide iref count */
			                                    array('val' => 'byte2', 'type' => 'ubyte'),
			                                    array('val' => 'QUST_iref', 'type' => 'ulong' /*quest iref*/),
			                                    array('val' => 'DIAL_iref', 'type' => 'ulong' /* DIAL iref*/),
			                                    array('val' => 'INFO_iref', 'type' => 'ulong' /* INFO iref*/))),
			      
			      // remaining definitions are ones I have added
			      // seen in inventory substructures
			      0x27 => array('val' => 'FACTNPC__iref',
			                    'type' => 'ulong' /*iref*/,
			                    'section' => 'Owner'),
			      // poison
			      0x48 => array('val' => 'ALCH_iref', // poisoned weapons
			                    'type' => 'ulong' /*iref*/),
			      // rest have only been seen as REFR subrecords (not part of inventory)
			      // but format is same, and other values (0x12, 0x27) overlap with inventory flags or ACHR flags (0x4b)
			      0x31 => array('val' => NULL, 'len' => 6, 'skip' => true,
			                    'section' => 'Lock',
			                    'data' => array(array('val' => 'lock_level', 'type' => 'ubyte'),
			                                    array('val' => 'KEYM_iref', 'type' => 'ulong'),
			                                    array('val' => 'key_flag', 'type' => 'ubyte'))),
		         // section called Extra Magic, but really just looks like an iref pointing to this formid
			      // extra 61 bytes only seen a few times, but some cases are pretty definitive
			      // its possible extra is variable length: ? ushort,byte*ushort,byte,iref ? (i.e., ushort=54 in all examined cases)
			      0x3a => array('val' => NULL, 'len' => 6, 'skip' => true,
			                    'section' => 'Extra Magic',
			                    'data' => array(array('val' => 'REFR_iref', 'type' => 'ulong'),
			                                    array('val' => 'extraCount', 'type' => 'ushort'),
			                                    array('val' => 'extra', 'type' => 'bytes', 'len' => 61))),
			      // no idea what this is, whether length is fixed or variable....
			      // seen as a REFR subrecord
			      // from size of REFR's Extra Magic in text dump, it's likely part of Extra Magic
			      // (but even so, text dump's Extra Magic total is 2 too large)
			      // contains self-iref again as last 4 bytes ... seen on Dark Fissure Altar (necromancer moon?)
			      0x39 => array('val' => NULL, 'len' => 12, 'type' => 'bytes', 'skip' => true,
			                    'section' => 'Extra Magic'),
			      // Another mystery record, with unknown length
			      // part of Animation
			      0x4a => array('val' => NULL, 'skip' => true,
			                    'section' => 'Animation',  // actually a string??
			                    'type' => 'string'),
			                    /*              'data' => array(array('val' => 'mysteryCount', 'type' => 'ubyte'),
			                                    array('val' => 'mystery', 'type' => 'ubyte'))),*/
			      // complete guess.... 
			      0x41 => array('val' => NULL, 'skip' => true,
			                    'type' => 'float',
			                    'section' => 'All Extra'),
			      // complete guess (but based on a few cases)
			      0x53 => array('val' => NULL, 'skip' => true, 'type' => 'ulong',
			                   'section' => 'All Extra'),
			      // iref points to a weapon? NO!!! it's ulong to investment gold
					0x52 => array('val' => 'investGold', 'skip' => true, 'section' => 'Investment Gold',
					              'type' => 'ulong'),
			      // appears on door with Teleport flag set: is this Teleport info?
			      // length same as REFR:XTEL, but order looks like x,y,z,rx,ry,rz,iref
			      0x32 => array('val' => 'Teleport', 'len' => 28, 'section' => 'Teleport', 'skip' => true,
			                    'data' => array(array('val' => 'x', 'type' => 'float'),
			                                    array('val' => 'y', 'type' => 'float'),
			                                    array('val' => 'z', 'type' => 'float'),
			                                    array('val' => 'rx', 'type' => 'float'),
			                                    array('val' => 'ry', 'type' => 'float'),
			                                    array('val' => 'rz', 'type' => 'float'),
			                                    array('val' => 'DOOR_REFR_iref', 'type' => 'ulong'))),
			      // ACHR property
			      // set on Rythe ... unusual entries set for Rythe: SetNoRumors 0; Set Essential 0
			      // value appears to be 0/1 value
			      // Gukimir=1; Geimund=1; Lathon=1; AvitaVesnia=1; Farwil=0; BremmanSenyan=0; Areldur=1; Rythe=0
			      0x5a => array('val' => NULL, 'len' => 1, 'type' => 'ubyte'),
			      0x3e => array('val' => 'Oblivion Entry', 'len' => 16, 'section' => 'Oblivion Entry', 'skip' => true,
			                    'data' => array(array('val' => 'DOOR_REFR_iref', 'type' => 'ulong'),
			                                    array('val' => 'x', 'type' => 'float'),
			                                    array('val' => 'y', 'type' => 'float'),
			                                    array('val' => 'z', 'type' => 'float'))),
			      // appears on Mythic Dawn Assassin's Bound Mace, Armor, and Helmet
			      0x47 => array('val' => NULL, 'len' => 0, 'type' => 'bytes'),
			      // appears on Mythic Dawn Assassin's Bound Mace, Armor, and Helmet
			      0x50 => array('val' => NULL, 'len' => 0, 'type' => 'bytes'),
			      // Map Marker Flags
			      0x33 => array('val' => 'mm_flag', 'type' => 'ubyte', 'section' => 'Map Marker Flags')
			     );
		
	}
	
	protected function _datadef_check() {
		// need to figure out a way to just do the vallist -> val and other basic processing
		parent::_datadef_check(array('debug' => false, 'cleanup_only' => true));
	}
	
	// Need to figure out how to handle shivering isles....
	// update.esp listed on Xbox 360... presumably just how patch done?
	protected function process_modids($plugins) {
		$this->_modids = array();
		foreach ($plugins as $i => $name) {
			$name = strtolower($name);
			// can't just use array_search because cases can mismatch (.ESP vs .esp)
			foreach ($this->_fileofficial as $j => $oname) {
				if ($name==strtolower($oname)) {
					$this->_modids[$i] = $j;
					break;
				}
			}
			if (!isset($this->_modids[$i])) {
				print "Unable to recognize plugin filename ".$name."\n";
			}
		}
	}
	
	// override skips, nulls in data defns for testing
	protected function _set_datadef_debug() {
		if ($this->_sdebug<10)
			return;
		foreach ($this->_propertydef as $flag => $defn) {
			if (isset($defn['skip']))
				unset($this->_propertydef[$flag]['skip']);
			if (!isset($defn['val'])) {
				$val = "Unk0x".sprintf('%02x', $flag);
				$this->_propertydef[$flag]['val'] = $val;
			}
		}
	}
	
	function read() {
		$this->request_data('all');
		$this->_set_datadef_debug();
		
		$fullfile = "../../charstats/data/".$this->_savefile;
		if (!($this->_fpin = fopen($fullfile, "rb"))) {
			print "Unable to open file $file\n";
			exit();
		}
		$this->_savefilesize = filesize($fullfile);
		if ($this->_usetext) {
			if (!file_exists($fullfile.".txt") || !$this->_fptext = fopen($fullfile.".txt", "rb"))
				$this->_usetext = false;
		}
		
		$this->_readdata = $this->extract_generic_data($this->_headerdef['fileheader']);
		if (!isset($this->_readdata['fileId']) || $this->_readdata['fileId']!='TES4SAVEGAME')
			return false;
		$d = $this->extract_generic_data($this->_headerdef['saveheader']);
		$this->_readdata = array_merge($this->_readdata, $d);
		// plugins
		if (($this->_readdata['plugins'] = $this->read_array('string', NULL, 'ubyte'))===false)
			return false;
		$this->process_modids($this->_readdata['plugins']);
		$d = $this->extract_generic_data($this->_headerdef['global']);
		$this->_readdata = array_merge($this->_readdata, $d);
		
		var_dump($this->_readdata);
		
		// skip to formids and read them before processing changes
		$chgstart = ftell($this->_fpin);
		fseek($this->_fpin, $this->_readdata['formIdsOffset']);
		$this->read_formids();
		$formidend = ftell($this->_fpin);
		
		fseek($this->_fpin, $chgstart);
		if ($this->_sdebug>=10) {
			print "------------ Start of change record section ---------------\n";
		}
		$this->anal_changes();
		//$this->read_changes();
		
		// temp effects
		// irefs to EFSH (effect shaders)
		$tempeffects = $this->read_array('ubyte');
		if ($this->_sdebug>=20) {
			print "Size of tempeffects = ".count($tempeffects)."\n";
			for ($i=0; $i<count($tempeffects); $i++) {
				printf("  %3d   %3d  0x%02x\n", $i, $tempeffects[$i], $tempeffects[$i]);
			}
			print "\n";
		}
		
		// next is formids, but they've already been read
		fseek($this->_fpin, $formidend);
		// world spaces -- not confirmed that these are formids
		// not being used, so skipping 
		$worldspaces = $this->read_array('ulong');
		if ($this->_sdebug>=20) {
			print "Size of worldspaces = ".count($worldspaces)."\n";
			if (count($worldspaces)<100) {
				for ($i=0; $i<20 && $i<count($worldspaces); $i++) {
					print sprintf("%3d   %8ld   0x%08lx\n", $i, $worldspaces[$i], $worldspaces[$i]);
				}
			}
		}
	}
	
	function read_text_line($formid, $rectype) {
		if (empty($this->_textreclocs)) {
			$currrec = NULL;
			while ($line = fgets($this->_fptext)) {
				if (substr($line,4,2)=='s:') {
					if (strtoupper(substr($line,0,4))==substr($line,0,4)) {
						$currrec = substr($line,0,4);
						$this->_textreclocs[$currrec]['start'] = ftell($this->_fptext);
						continue;
					}
				}
				if (isset($currrec) && substr($line,0,5)=='Total') {
					$this->_textreclocs[$currrec]['end'] = $lastloc;
					$currrec = NULL;
				}
				$lastloc = ftell($this->_fptext);
			}
		}
		
		$formid = substr(strtoupper($formid),2);
		$startloc = ftell($this->_fptext);
		$keepline = false;
		$testline = false;
		
		if (!isset($this->_textreclocs[$rectype]))
			return false;
		fseek($this->_fptext, $this->_textreclocs[$rectype]['start']);
		while ($line = fgets($this->_fptext)) {
			if (ftell($this->_fptext)>=$this->_textreclocs[$rectype]['end'])
				break;
				// === important to ensure text strings like 5a19 aren't converted to 5... or that 5e10 isn't interpreted as 5.10^10
			if (substr($line,2,8)===$formid) {
				if (substr($formid,0,2)=='FF' && $testline===false) {
					$testline = $line;
				}
				else {
					$keepline = $line;
					break;
				}
			}
		}
		
		if ($testline!==false) {
			// deal with newly created items that might have also have a changed record
			if ($keepline===false || preg_match('/^\s+([^\s]{8})\s+(\d+)\s+00000000\s+0\s+/', $keepline))
				$keepline = $testline;
		}
		if ($keepline===false)
			return false;
		
		if (!preg_match('/^\s+([^\s]{8})\s+(\d+)\s+([^\s]+)\s+(\d+)\s+(.*)\s*-\s+(.*?)\s*$/', $keepline, $matches)) {
			$this->__add_error("Unable to process text_line:\n".$keepline);
			return false;
		}
		list($x, $newformid, $size, $flags, $vers, $name, $changetext) = $matches;
		$name = trim($name);
		$newformid = '0x'.strtolower($newformid);
		$changes = array();
		$changetext = trim($changetext);
		$chgtot = 0;
		while (($paren=strpos($changetext,')'))!==false) {
			$entry = substr($changetext,0,$paren+1);
			$changetext = trim(substr($changetext,$paren+1));
			$p0 = strpos($entry,'(');
			$cname = substr($entry,0,$p0);
			$csize = substr($entry,$p0+1,$paren-$p0-1);
			$changes[$cname] = $csize;
			$chgtot += $csize;
		}
		if (trim($name)=='NOT LOADED')
			$loaded = false;
		else
			$loaded = true;
		return array('formid' => $newformid,
		             'size' => $size,
		             'flags' => $flags,
		             'vers' => $vers,
		             'name' => $name,
		             'loaded' => $loaded,
		             'changes' => $changes,
		             'chgtot' => $chgtot,
		             'text' => $keepline);
	}
	
	function read_formids() {
		$formids = $this->read_array('raw_formid');
		foreach ($formids as $iref => $formid) {
			$item = $this->_parentlist->get_item($formid);
			if ($this->_sdebug>=10) {
				print "iref $iref ";
				printf("0x%08x  ", $iref);
				print $formid;
			}
			if (!is_object($item)) {
				if ($this->_sdebug>=10) {
					print "\n";
				}
				continue;
			}
			
			$rectype = $item->get('rectype');
			if ($this->keep_rectype($rectype, $formid)) {
				$this->_ireflist[$iref] = $item->get('ordid');
				if ($this->_sdebug>=10) {
					if (is_object($item)) {
						print "   ".$item->get('rectype');
						print " ordid ".$item->ordid();
						print " formid ".$item->formid();	
						print " edid ".$item->edid();
				// really should check whether item is a ref item, and then get base name
						print " name ".$item->get('name');
					}
					print "\n";
				}
			}
			
			$item->release();
		}
	}
	
	function convert_iref($iref) {
		if (isset($this->_ireflist[$iref]))
			return $this->_ireflist[$iref];
		$formid = $this->process_raw_formid($iref);
		if (substr($formid,2,2)=='ff') {
			foreach ($this->_createdlist as $icr => $item) {
				if ($formid==$item->get('formid')) {
					// if it's in the _createdlist, then it should be stored in _parentlist->_data using formid instead of ordid
					return $formid;
					//					return -1*$icr;
				}
			}
		}
		else {
			// trying to look up formid otherwise is just going to get the wrong value
			// (it's just the hex'd version of iref)
			return false;
		}
		return false;
	}
	
	function get_iref_item($ordid) {
		/*		if ($ordid<0)
			$item = $this->_createdlist[-1*$ordid];
		  		else*/
			$item = $this->_parentlist->get_item($ordid);
		return $item;
	}
	
	function skip_changes() {
		$origloc = ftell($this->_fpin);
		for ($ichg=0; $ichg<$this->_readdata['recordsCount']; $ichg++) {
			$recstart = ftell($this->_fpin);
			fseek($this->_fpin, $recstart+10);
			$size = $this->extract_single('ushort');
			fseek($this->_fpin, $recstart+10+2+$size);
		}
		return $origloc;
	}
	
	function anal_changes() {
		$typenames = array(6 => 'FACT',
		                   19 => 'APPA',
		                   20 => 'ARMO', 
		                   21 => 'BOOK',
		                   22 => 'CLOT',
		                   25 => 'INGR',
		                   26 => 'LIGH',
		                   27 => 'MISC',
		                   33 => 'WEAP',
		                   34 => 'AMMO',
		                   35 => 'NPC_',
		                   36 => 'CREA',
		                   38 => 'SLGM',
		                   39 => 'KEYM',
		                   40 => 'ALCH',
		                   48 => 'CELL',
		                   49 => 'REFR',
		                   50 => 'ACHR',
		                   51 => 'ACRE',
		                   58 => 'INFO',
		                   59 => 'QUST',
		                   61 => 'PACK');
		
		// Sections that I'm declaring unknown: 29(Equipment), 6(Life State), 19(Run Once)
		// Doing stats on before declaring unknown; 15, 20, 21, 22
		// should also track whether there are flags that only appear in NOT LOADED
		
		$flagnames =
			array(0 => 'Form Flags',
			      1 => array('Created' => 'R',
			                 'Cell Created' => 'CELL' /* might also be 2.. so far 1/2 always together */),
			      2 => array('Moved' => 'R',
			                 'Faction Flags' => 'FACT',
			                 'Book Skill' => 'BOOK',
			                 'Base Health' => 'NPC_',
			                 'Quest Flags' => 'QUST',
			                 '' => 'CELL'),
			      3 => array('Havok Moved' => 'R',
			                 'Faction Reactions' => 'FACT',
			                 'Base Attributes' => 'C',
			                 'Cell Flags' => 'CELL',
			                 '' => NULL),
			      4 => array('Scale' => 'R',
			                 'Base Data' => 'C',
			                 'Full Name' => 'CELL'),
			      5 => array('All Extra' => 'R',
			                 'Spell List' => 'C',
			                 'Ownership' => 'CELL'),
			      6 => array('Lock' => 'REFR',
			                 'Life State' => 'AC',
			                 'Factions' => 'C'),
			      // '' to indicate that it's name is left out of text dump
			      7 => array('Owner' => 'REFR',
			                 'Crime Gold' => 'ACHR',
			                 '' => 'ACRE',
			                 'Full Name' => 'C'),
			      8 => array('AI Data' => 'C'),
			      9 => array('Skills' => 'NPC_'),
			      10 => array('Map Marker Flags' => 'REFR',
			                  'Combat Style' => 'NPC_'),
			      11 => array('Had Havok Move Flag' => 'R'),
			      13 => array('Investment Gold' => 'ACHR'),
			      14 => array('Oblivion Entry' => 'AC'),
			      15 => array('Disp Modifiers' => 'AC'),
			      16 => array('Empty Flag' => 'REFR', 'Non-saved Package' => 'ACHR'),
			      17 => array('Dropped Item Flag' => 'REFR', 'Interrupt' => 'AC', 'Follow' => 'AC', 'Dialogue' => 'AC'),
			      18 => array('Door Default State' => 'REFR', 'Trespass' => 'AC'),
			      19 => array('Door State' => 'REFR', 'Run Once' => 'AC' /* not handling*/),
			      20 => array('Magic Modifiers' => 'AC', 'Teleport' => 'R'),
			      21 => array('Extra Magic' => 'REFR', 'Script Modifiers' => 'AC'),
			      22 => array('Furniture Markers' => 'REFR', 'Game Modifiers' => 'AC'),
			      23 => array('Oblivion Flag' => 'R'),
			      24 => array('Movement Extra' => 'R',
			                  'Pathgrid Modified' => 'CELL'),
			      25 => array('Animation' => 'R'),
			      26 => array('Script' => 'R',
			                  '' => 'CELL'),
			      27 => array('Inventory' => 'R',
			                  'Detach Time' => 'CELL',
			                  'Quest Script' => 'QUST'),
			      28 => array('Leveled Creature' => 'R',
			                  'Base Modifiers' => 'C',
			                  'Seen Data' => 'CELL',
			                  'Topic Said Once' => 'INFO',
			                  'Quest Stages' => 'QUST',
			                  'Never Run Flag' => 'PACK'),
			      29 => array('Equipment' => 'AC'),
			      30 => array('Enabled' => NULL, 'Disabled' => NULL, 'Enabled/Disabled' => NULL),
			      31 => array('Cell Changed' => 'R'));
		
		$progress = 0;
		$nproc = 0;
		$nprocerr = 0;
		$nzero = 0;
		$testdata = array();
		
		$nendis = array();
		$combos = array();
		$lfinals = array();
		$lorder = array();
		$nfs = array();
		$cells = array();
		$ncr = array();
		$textsizes = array();
		$textstats = array();
		$formflags = array();
		$celltypes = array();
		
		$ntype = array();
		for ($ichg=0; $ichg<$this->_readdata['recordsCount']; $ichg++) {
			if (feof($this->_fpin)) {
				print "out of data at $ichg of ".$this->_readdata['recordsCount']." records\n";
				break;
			}
			
			$data = array();
			// probably only want to get raw_formid here -- no need to look up the value
			// until type known and I've decided whether it's change data I'm interested in
			$orig_formval = $this->extract_single('ulong');
			// formid as read in
			$orig_formid = sprintf("0x%08lx", $orig_formval);
			// formid after mod numbers are translated from save file list to DB list
			$data['formid'] = $this->process_raw_formid($orig_formval);
			$data['typenum'] = $this->extract_single('ubyte');
			if (isset($typenames[$data['typenum']]))
				$data['type'] = $typenames[$data['typenum']];
			else {
				print "invalid entry (".$data['typenum'].") for type at $ichg of ".$this->_readdata['recordsCount']." records, formid = ".$data['formid']."\n";
				break;
			}
			$data['flags'] = $this->extract_single('ulong');
			$data['hex_flags'] = sprintf('%08lx', $data['flags']);
			$data['version'] = $this->extract_single('ubyte');
			$data['size'] = $this->extract_single('ushort');
			
			if (!isset($ntype[$data['type']]))
				$ntype[$data['type']] = 0;
			$ntype[$data['type']]++;
			
			$startRec = ftell($this->_fpin);
			$nextRec = $startRec + $data['size'];
			$this->_change_endloc = $nextRec;
			
			$this->__set_error_stats(array('formid'=> $data['formid'], 'rectype' => $data['type'], 'version' => $data['version'], 'startRec' => $startRec, 'nextRec' => $nextRec));
			if ($data['version']!=125)
				$this->__add_error("Unexpected version number for record: ".$data['version']."\n");
			
			if ($this->_savefilesize!==false) {
				$progfrac = floor($startRec/$this->_savefilesize*100);
				if ($progfrac>$progress) {
					print "*********** Progress: $progfrac%\n";
					$progress = $progfrac;
				}
			}
			
			// test whether I want to analyze this line
			$fs = array();
			for ($i=0; $i<32; $i++) {
				$fi = (($data['flags']>>$i)&1) ? true : false;
				if ($fi)
					$fs[] = $i;
			}
			$fsstring = implode($fs, " ");
			
			$pcdata = false;
			if ($data['formid']=='0x00000007' || $data['formid']=='0x00000014') {
				$data['is_pc'] = true;
				$pcdata = true;
			}
			
			$process = false;
			//			if ($data['type']=='REFR' && (in_array(27, $fs) || in_array(25, $fs)))
			if ($data['type']=='REFR' || $data['type']=='ACRE' || $data['type']=='ACHR')
				$process = true;
			
			//			if (!$pcdata && !$process) {
			if (false) {
				fseek($this->_fpin, $nextRec);
				continue;
			}
			
			$doprint = false;
			$doscan = false;
			$dodump = false;
			$skipproc = false;
			$exdata = false;
			$domisc = false;
			
			$fids = array();
			$errors = array();
			$this->_currerrors = array();
			
			if (!$data['size'])
				$nzero++;
			
			$item = $this->_parentlist->get_item($data['formid']);
			$item_disabled = NULL;
			$base_rec = NULL;
			$base_formid = NULL;
			$item_cell = NULL;
			if (is_object($item)) {
				$item_disabled = $item->get('disabled');
				$item_ordid = $item->get('ordid');
				$item_cell = $item->get('cell_id');
				$base_id = $item->get('base_id');
				$base_item = $this->_parentlist->get_item($base_id);
				if (is_object($base_item)) {
					$base_rec = $base_item->get('rectype');
					$base_formid = $base_item->get('formid');
					$base_item->release();
				}
				$item->release();
			}
			
			// There are a lot of REFR items with unknown base items
			// Plus, I haven't retrieved base info yet for newly-created objects
			/*
			if (!isset($base_rec) && ($data['type']=='REFR' || $data['type']=='ACHR' || $data['type']=='ACRE')) {
				$errors[] = "unable to get base item";
			}*/
			
			$summstring = $data['type']."  ".$data['formid']."  (orig=$orig_formid)";
			if (isset($base_rec))
				$summstring .= " $base_rec  $base_formid";
			$summstring .= "	size=".$data['size']."     ".$fsstring."\n";
			
			/**
				************************** Read text data
				*/
			$textline = false;
			if ($this->_usetext)
				$textline = $this->read_text_line($orig_formid, $data['type']);
			if ($textline!==false) {
				if ($textline['formid']!=$orig_formid)
					$errors[] = "binary/text formid mismatch text=".$textline['formid']." data=".$orig_formid." (converted to ".$data['formid'].")";
				if ($textline['size']!=$data['size'])
					$errors[] = "binary/text size mismatch text=".$textline['size']." data=".$data['size'];
				if ($textline['flags']!=strtoupper($data['hex_flags']))
					$errors[] = "binary/text flags mismatch text=".$textline['flags']." data=".$data['hex_flags']." numeric=".$data['flags'];
				
				// check Enabled/Disabled vals
				// they don't make any sense  text['Enabled'] much larger than text['Disabled'] regardless of
				// value of item_disabled
				if (in_array(30,$fs) && !is_null($item_disabled) && $textline['loaded']) {
					if (($item_disabled && !isset($textline['changes']['Disabled'])) || (!$item_disabled && !isset($textline['changes']['Enabled']))) {
						$text = "enabled/disabled mismatch item_disabled=$item_disabled  text.disabled=";
						if (isset($textline['changes']['Disabled']))
							$text .= "yes";
						$text .= "  text.enabled=";
						if (isset($textline['changes']['Enabled']))
							$text .= "yes";
						$text .= "  text.disabled/enabled=";
						if (isset($textline['changes']['Disabled/Enabled']))
							$text .= "yes";
				// turn off this test for now
						//						$errors[] = $text;
					}
					if ($item_disabled) {
						if (isset($textline['changes']['Disabled']))
							@ $nendis['disdis']++;
						elseif (isset($textline['changes']['Enabled']))
							@ $nendis['disen']++;
						else
							@ $nendis['disoth']++;
					}
					else {
						if (isset($textline['changes']['Enabled']))
							@ $nendis['enen']++;
						elseif (isset($textline['changes']['Disabled']))
							@ $nendis['endis']++;
						else
							@ $nendis['enoth']++;
					}
				}
				if (isset($textline['changes']['Disabled/Enabled']) && $textline['loaded']) {
					$errors[] = "Disabled/Enabled set on a loaded object\n";
					var_dump($textline);
				}
			}
			elseif ($this->_usetext) {
				$errors[] = "unable to read text line for formid=".$data['formid'];
			}
			
			// Stats on textline flags
			if ($textline && !empty($textline['loaded'])) {
				foreach ($textline['changes'] as $flag => $sz) {
					if (!isset($textsizes[$flag]))
						$textsizes[$flag] = array('min' => $sz, 'max' => $sz, 'num' => 0, 'sizes' => array());
					if (!isset($textsizes[$flag][$data['type']]))
					    $textsizes[$flag][$data['type']] = 0;
					$textsizes[$flag]['num']++;
					$textsizes[$flag][$data['type']]++;
					if ($sz<$textsizes[$flag]['min'])
						$textsizes[$flag]['min'] = $sz;
					if ($sz>$textsizes[$flag]['max'])
						$textsizes[$flag]['max'] = $sz;
					if (!isset($textsizes[$flag]['sizes'][$sz]))
						$textsizes[$flag]['sizes'][$sz] = 0;
					$textsizes[$flag]['sizes'][$sz]++;
				}
			}
			
			$sizes = array();
			
			/**
				***************************** pcdata
				*/
			if ($pcdata && $data['type']=='NPC_') {
				$this->read_change_record($data);
				
				print "processing player NPC\n";
				unset($data['spell_id']);
				var_dump($data);
			}
			elseif ($data['formid']=='0xfeffffff') {
				fseek($this->_fpin, $nextRec);
				$errors = array();
				$doprint = false;
				$doscan = false;
				continue;
			}
			/**
				************************** REFR / ACHR / ACRE
				*/
			elseif ($data['type']=='REFR' || $data['type']=='ACHR' || $data['type']=='ACRE') {
				$nproc++;
				
				$locs = array();
				
				if (in_array(31,$fs)) {
					$cdata = $this->extract_generic_data(array($this->_changedef['Cell Changed']));
					$cdata['subrectype'] = 31;
					$locs[] = $cdata;
					$sizes['Cell Changed'] = 16;
					$this->__set_error_stats(array('locChanged' => ftell($this->_fpin)));
					if ($textline && $textline['loaded']) {
						$this->__add_error("Cell Changed data found for a loaded record");
					}
				}
				
				if (in_array(2,$fs) || (in_array(3,$fs) && !in_array(1,$fs))) {
					$cdata = $this->extract_generic_data(array($this->_changedef['Moved']));
					if (in_array(2,$fs)) {
						$sizes['Moved'] = 28;
						$cdata['subrectype'] = 2;
					}
					else {
						$sizes['Havok Moved'] = 28;
						$cdata['subrectype'] = 3;
					}
					$locs[] = $cdata;
					$this->__set_error_stats(array('locMoved' => ftell($this->_fpin)));
				}
				
				// 1->0
				if (in_array(1,$fs)) {
					if (!empty($locs)) {
						$errors[] = "Both moved and created set in same record";
					}
					$created = $this->extract_generic_data(array($this->_changedef['Created']));
					$created['subrectype'] = 1;
					$locs[] = $created;
					$sizes['Created'] = 36;
					$this->__set_error_stats(array('locCreated' => ftell($this->_fpin)));
					if (!empty($created)) {
						$id = $this->convert_iref($created['base_iref']);
						if ($id===false)
							$id = $this->_parentlist->get_ordid($this->process_raw_formid($created['base_iref']));
						if ($id===false || !isset($id)) {
							$errors[] = "Unable to convert created base_iref ".$created['base_iref']." (0x".sprintf("%08lx", $created['base_iref']).")";
						}
						else {
							$item = $this->_parentlist->get_item($id);
							if (!is_object($item)) {
								$errors[] = "Unable to get item on created base_iref ".$created['base_iref'].", id $id";
							}
							else {
								$item_cell = $item->get('cell_id');
								$base_rec = $rt = $item->get('rectype');
								
								if ($rt=='REFR' || $rt=='ACHR' || $rt=='ACRE' || $rt=='CELL' || $rt=='WRLD' || $rt=='INFO' || $rt=='PACK')
									//if ($rt!='WEAP' && $rt!='CLOT' && $rt!='ARMO' && $rt!='MISC' && $rt!='BOOK')
									$errors[] = "Created cell base_iref ".$created['base_iref']." (id=$id) is unexpected rectype (rectype=$rt)";
								$item->release();
							}
						}
						
						$extrasize = NULL;
						$crflag = $created['create_flag'];
						if ($created['create_flag']==0 || $created['create_flag']==3) {
							// AMMO can have create_flag=0
						}
						elseif ($created['create_flag']==1) {
							if (isset($base_rec) && $base_rec!='AMMO')
								$this->__add_error("Created record with create_flag=1 and base_rec!=AMMO ($base_rec)");
							if (count($fs)!=1) {
								$this->__add_error("Created record with create_flag=1 and multiple subrecords");
							}
							else {
								$extrasize = $nextRec-ftell($this->_fpin);
								if ($extrasize!=531 && $extrasize!=467)
									$this->__add_error("Created record with create_flag=1 and extrasize!=531 ($extrasize)");
								fseek($this->_fpin, $nextRec);
							}
						}
						else {
							$this->__add_error("Unexpected value for create_flag [".$created['create_flag']."]");
						}
						
						@ $testdata['create'][$crflag]['count']++;
						if (isset($base_rec)) {
							@ $testdata['create'][$crflag]['recs'][$base_rec]++;
							
							if (isset($extrasize)) {
								@ $testdata['create'][$crflag]['sizes'][$extrasize][$base_rec]++;
							}
						}
					}
				}
				
				// location relative to 0/2/3 unknown .. and to each other
				
				if (in_array(23,$fs)) {
					if (empty($locs)) {
						$obliv = $this->extract_single('ulong');
						$this->__set_error_stats(array('locOblivFlag' => ftell($this->_fpin)));
						$id = $this->convert_iref($obliv);
						// both irefs and ordids seen here
						if ($id===false) {
							$id = $this->_parentlist->get_ordid($this->process_raw_formid($obliv));
							@ $celltypes[23]['formid']++;
						}
						else {
							if ($obliv>0xff000000)
								@ $celltypes[23]['ffiref']++;
							else
								@ $celltypes[23]['iref']++;
						}
							
						if ($id===false || !isset($id)) {
							$errors[] = "Unable to convert oblivion flag iref $obliv (0x".sprintf("%08lx", $obliv).")";
						}
						else {
							$item = $this->_parentlist->get_item($id);
							if (!is_object($item)) {
								$errors[] = "Unable to get item on oblivion flag iref $obliv, id $id";
							}
							else {
								// have confirmed both CELLs and WRLDs appear here
								if ($item->get('rectype')!='CELL' && $item->get('rectype')!='WRLD')
									$errors[] = "Oblivion flag iref $obliv (id=$id) is not a CELL/WRLD (rectype=".$item->get('rectype').")";
								$item->release();
							}
						}
						$sizes['Oblivion Flag'] = 4;
						
					}
					else
						$sizes['Oblivion Flag'] = 0;
				}
				
				if ($pcdata && $data['type']=='ACHR') {
					$damage = $this->extract_generic_data(array($this->_changedef['Unknown876']));
					print "unknown 876 section:\n";
					for ($i=0; $i<count($damage['damage']); $i++) {
						if (!$damage['damage'][$i])
							continue;
						print "  ";
						if (($i>=72 && $i<144) || ($i>=152 && $i<216)) {
							print "* ";
						}
						else {
							print "  ";
						}
						print " ".sprintf("%3d",$i)."  ".$damage['damage'][$i]."\n";
					}
				}
				
				// after 1,2,23; before 0
				$actor_flag = NULL;
				if ($data['type']=='ACHR' || $data['type']=='ACRE') {
					// actor flag is after 1/2 (despite UESP docs to contrary on Created section)
					$actor_flag = $this->extract_single('ubyte');
					$actorvals = array(0 => 'High',
					                   1 => 'Mid High',
					                   2 => 'Mid Low',
					                   3 => 'Low',
					                   255 => 'None');
					$this->__set_error_stats(array('locActorFlag' => ftell($this->_fpin), 'valActorFlag' => (isset($actorvals[$actor_flag])?$actorvals[$actor_flag]:$actor_flag)));
					if (!isset($actorvals[$actor_flag])) {
						$errors[] = "Invalid value for actor flag ($actor_flag), loc=".(ftell($this->_fpin)-1)."\n";
						$doprint = true;
						$doscan = true;
					}
					elseif ($textline && $textline['loaded'] && !isset($textline['changes'][$actorvals[$actor_flag]])) {
						$errors[] = "Value for actor flag does not match textline value (flag=$actor_flag;  loc=".(ftell($this->_fpin)-1).")\n";
						$doprint = true;
						$doscan = true;
					}
				}
				
				// cases with 0->23 don't work, so presumably order is 23->0?
				if (in_array(0,$fs)) {
					$form = $this->extract_generic_data(array($this->_changedef['Form Flags']));
					$sizes['Form Flags'] = 4;
					$this->__set_error_stats(array('locFormFlags' => ftell($this->_fpin)));
					@ $formflags[sprintf("%08lx", $form['form_flags'])]++;
					@ $formflags[$data['type']][sprintf("%08lx", $form['form_flags'])]++;
				}
				
				// testing created, moved, and cell changed
				foreach ($locs as $move) {
					if (empty($move))
						continue;
					if ($move['cell']!=0) {
						$id = $this->convert_iref($move['cell']);
						if ($id===false) {
							$id = $this->_parentlist->get_ordid($this->process_raw_formid($move['cell']));
							$errors[] = "Moved/Created cell is a formid, not an iref";
							@ $celltypes[$move['subrectype']]['formid']++;
						}
						elseif ($move['cell']>=0xff000000)
							@ $celltypes[$move['subrectype']]['ffiref']++;
						else
							@ $celltypes[$move['subrectype']]['iref']++;
						if ($id===false || !isset($id)) {
							$this->__add_error("Unable to convert moved cell (iref=".$move['cell']."; 0x".sprintf("%08lx", $move['cell']).")");
						}
						else {
							$item = $this->_parentlist->get_item($id);
							$item_cell = $item->ordid();
							if (!is_object($item)) {
								$errors[] = "Unable to get item on moved cell (iref=".$move['cell'].", id $id)";
							}
							else {
								if ($item->get('rectype')!='CELL' && $item->get('rectype')!='WRLD')
									$this->__add_error("Moved cell iref is not a CELL/WRLD (iref=".$move['cell']."; id=$id; rectype=".$item->get('rectype').")");
								$item->release();
							}
						}
					}
					else {
						@ $celltypes[$move['subrectype']]['zero']++;
					}
					// Skipping x/y/z/rx/ry/rz checks for now
					// Too many false positives; confident in Created/Moved records; iref check more definitive
					if (false) {
					// z=0 not uncommon with ACHR/ACRE; occasional x=0, y=0 values elsewhere
					if (($move['x']==0 && $move['y']==0) || ($move['z']==0 && $data['type']!='ACHR' && $data['type']!='ACRE')) {
						$this->__add_error("Unlikely coordinates for moved (x=".$move['x']." y=".$move['y']." z=".$move['z'].")");
					}
					if (isset($move['rx'])) {
						// There can be strange values in rotations... rx=3.4e38??
						// in particular, ACHRs/ACREs always seem to have flags in rx (0xff, 0xff, 0x7f, 0x7f)
						if (/*(abs($move['rx'])>8 && $data['type']!='ACHR' && $data['type']!='ACRE') ||*/ abs($move['ry'])>8 || abs($move['rz'])>8) {
							$this->__add_error("Unlikely rotations for moved (rx=".$move['rx']." ry=".$move['ry']." rz=".$move['rz'].")");
						}
					}
					}
				}
				
				/**
				********************** Inventory
				*/
				$this->_proptype = 'Inv';
				if (in_array(27, $fs)) {
					$invloc = ftell($this->_fpin);
					$nitems = $this->extract_single('ushort');
					fseek($this->_fpin, $invloc);
					$nmax = $data['size']/12;
				
					if ($nitems<0 || $nitems>$nmax) {
						$doprint = true;
						$skipproc = true;
						$doscan = true;
						$this->__add_error("bad number of inventory items ($nitems)");
					}
				
					if (!$skipproc) {
						$chgdata = $this->extract_generic_data(array($this->_changedef['Inventory']));
						$this->__set_error_stats(array('locInv' => ftell($this->_fpin)));
						if (ftell($this->_fpin)>$nextRec) {
							$this->__add_error("too many inventory items read");
							$doprint = true;
							$skipproc = true;
						}
						if ($chgdata===false) {
							$doprint = true;
							$skipproc = true;
						}
					}
					$sizes['Inventory'] = ftell($this->_fpin)-$invloc;
				}
				$invend = ftell($this->_fpin);
				$this->_proptype = $data['type'];
				$exdata = NULL;
				
				/**
					*************************** Properties (domisc; exdata)
					*/
				// NB it's possible for flag to be 5(All Extra), but for misc section to include
				// Owner(7) information -- in which case textdump adds Owner size to All Extra
				if (!$skipproc) {	
					$nnext = $this->extract_single('ushort');
					fseek($this->_fpin, $invend);
					
					
					$domisc = true;
					if ($data['type']=='REFR') {
						if (!(in_array(6,$fs) || in_array(7,$fs) || in_array(10, $fs) || in_array(21,$fs) || in_array(26,$fs) || in_array(25, $fs) || in_array(28, $fs) || in_array(5, $fs) || in_array(24,$fs)))
							$domisc = false;
					}
					
					// I've now commented this out... which may lead to new problems cropping up
					//   (in particular when nnext==0 is taken by properties instead of being left for finals)
					// But trying to establish what is the correct processing
					//if ($nnext<=0 || $nnext>10)
					//	$domisc = false;
					if ($data['type']=='REFR') {
						// for now, assume propNums OK even if 0 (there are clearly some cases where properties section exists but propNums is 0)
						if ($domisc && $nnext==0) {
							//							$this->__add_error("properties section has propNums=0");
						}
						elseif ($domisc && $nnext>10) {
							$this->__add_error("properties section has propNums=$nnext");
						}
					}
					
					if ($domisc) {
						$exdata = $this->read_property_structure();
						$this->__set_error_stats(array('locPropEnd' => ftell($this->_fpin)));
						if ($exdata===false) {
							$doprint = true;
							$doscan = true;
							$dodump = true;
							$skipproc = true;
						}
						elseif ($data['type']=='REFR') {
							if (in_array(10, $fs) !== isset($exdata['mm_flag'])) {
								$this->__add_error("Mismatch between flag 10 and Map Marker Flags property");
							}
							if (in_array(20, $fs) !== isset($exdata['Teleport'])) {
								$this->__add_error("Mismatch between flag 20 and Teleport property");
							}
							if (in_array(28, $fs) !== isset($exdata['leveledCreature'])) {
								$this->__add_error("Mismatch between flag 28 and Leveled Creature property");
							}
							if (in_array(10, $fs)) {
								@ $testdata['mm_flags'][$exdata['mm_flag']]++;
						// 18652 is my ordid for STAT 0x10 -> MapMarker
								if (isset($base_id) && $base_id!=18652)
									$this->__add_error("Map Marker Flags set on non-MapMarker object ($base_id)");
							}
						}
						else {
							if (in_array(18, $fs) !== isset($exdata['Trespass'])) {
								$this->__add_error("Mismatch between flag 18 and Trespass property");
							}
							if (in_array(14, $fs) !== isset($exdata['Oblivion Entry'])) {
								$this->__add_error("Mismatch between flag 14 and Oblivion Entry property");
							}
							if (in_array(28, $fs) !== isset($exdata['leveledCreature'])) {
								$this->__add_error("Mismatch between flag 28 and Leveled Creature property");
							}
							if (in_array(13, $fs) !== isset($exdata['investGold'])) {
								$this->__add_error("Mismatch between flag 13 and Investment Gold property");
							}
							if (isset($exdata['Unk0x5c'])) {
								if (!isset($testdata['data0x5c']['min']))
									$testdata['data0x5c']['min'] = $exdata['Unk0x5c'];
								else
									$testdata['data0x5c']['min'] = min($testdata['data0x5c']['min'], $exdata['Unk0x5c']);
								@ $testdata['data0x5c']['max'] = min($testdata['data0x5c']['max'], $exdata['Unk0x5c']);
							}
						}
					}
				}
					
				if ($textline && !empty($textline['loaded'])) {
					$exsizes = array();
					if (in_array(24, $fs)) {
						if (isset($exdata['sections']['Movement Extra']))
							$diff = $textline['changes']['Movement Extra'] - $exdata['sections']['Movement Extra'] - 2;
						else
							$diff = $textline['changes']['Movement Extra'];
						if ($diff>=0)
							$exsizes['Movement Extra misc'] = $diff;
					}
					if (in_array(25, $fs)) {
						if (isset($exdata['sections']['Animation']))
							$diff = $textline['changes']['Animation'] - $exdata['sections']['Animation'] - 2;
						else
							$diff = $textline['changes']['Animation'];
						if ($diff>=0)
							$exsizes['Animation misc'] = $diff;
					}
					if (in_array(3, $fs)) {
						if (isset($sizes['Havok Moved']))
							$diff = $textline['changes']['Havok Moved'] - $sizes['Havok Moved'];
						else
							$diff = $textline['changes']['Havok Moved'];
						if ($textline['chgtot']==$textline['size']+28)
							$diff -= 28;
						if ($diff>=0)
							$exsizes['Havok Moved misc'] = $diff;
					}
					
					foreach ($exsizes as $flag => $sz) {
						if ($sz==0)
							continue;
						if (!isset($textsizes[$flag]))
							$textsizes[$flag] = array('min' => $sz, 'max' => $sz, 'num' => 0, 'sizes' => array());
						if (!isset($textsizes[$flag][$data['type']]))
							$textsizes[$flag][$data['type']] = 0;
						$textsizes[$flag]['num']++;
						$textsizes[$flag][$data['type']]++;
						if ($sz<$textsizes[$flag]['min'])
							$textsizes[$flag]['min'] = $sz;
						if ($sz>$textsizes[$flag]['max'])
							$textsizes[$flag]['max'] = $sz;
						if (!isset($textsizes[$flag]['sizes'][$sz]))
							$textsizes[$flag]['sizes'][$sz] = 0;
						$textsizes[$flag]['sizes'][$sz]++;
					}
				}
				$miscend = ftell($this->_fpin);
				$nfinal = 0;				
				
				/**
					************************ nfinal / Miscellaneous section
					*/
				
				$effend = $nextRec;
				$nfinal = 0;
				if ($data['type']=='REFR') {
					if (in_array(4, $fs))
						$effend -= 4;
				}
				$fsizes = array();
				if (in_array(25, $fs)) {
					$addorder = array(24 => 'Movement Extra', 25 => 'Animation', 3 => 'Havok Moved Plus',  11 => 'Had Havok Move Flag', 16 => 'Empty Flag', 22 => 'Furniture Markers', 18 => 'Door Default State');
				}
				else {
					$addorder = array(24 => 'Movement Extra', 25 => 'Animation', 16 => 'Empty Flag', 3 => 'Havok Moved Plus',  11 => 'Had Havok Move Flag', 22 => 'Furniture Markers', 18 => 'Door Default State');
				}
				if ($data['type']!='REFR') {
					unset($addorder[22]);
					unset($addorder[16]);
					unset($addorder[18]);
				}
				
				// overreads seen for following combos of 24/25/3/16/11
				// 24
				// 16
				// 24 3 -> with 4 afterwards
				// possibility: data skipped if only one present? what about when there's 4/22 after?
				
				// assuming 16 is 0/2 bytes, and 11 can be >2 bytes, there's two different orders:
				// 16-11   and   25-11-16
				// also
				//  16-3   vs    25-3-16  
				if ($data['type']=='REFR') {
					// relative position of 11 is unknown, because 11 never appears in loaded records
					// but 16->11 known (assuming 16 is 2 bytes)
					foreach (array_merge(array_keys($addorder), array(NULL, NULL, NULL, NULL, NULL)) as $f) {
						if (isset($f) && !in_array($f, $fs))
							continue;
						$tloc = ftell($this->_fpin);
						$datalen = $this->extract_single('ushort') + 2;
						if ($tloc+$datalen>$effend) {
							fseek($this->_fpin,$tloc);
							break;
							//$this->__add_error("Attempt to read past end of record in misc section (datalen=$datalen, f=$f)");
						}
						else {
							$nfinal++;
							if (!isset($f)) {
								$this->__add_error("more final sections than available flags");
							}
							fseek($this->_fpin, $tloc+$datalen);
							if (isset($f))
								$sizes[$addorder[$f]] = $datalen;
							$fsizes[] = $datalen;
							$this->__set_error_stats(array('miscLoc' => ftell($this->_fpin)));
							if (ftell($this->_fpin)==$effend)
								break;
						}
					}
					
					$tagstr = '';
					$szstr = '';
					foreach ($addorder as $f => $tag) {
						if (!in_array($f, $fs))
							continue;
						if (empty($sizes[$tag]))
							$sz = '0';
						elseif ($sizes[$tag]==2)
							$sz = '2';
						else {
							$sz = 'm';
							@ $testdata['analfinal'][$f]['allsizes'][$sizes[$tag]]++;
						}
						if ($tagstr!='') {
							$tagstr .= ' ';
							$szstr .= ' ';
						}
						$tagstr .= $f;
						$szstr .= $sz;
						@ $testdata['analfinal'][$f]['tot']++;
						@ $testdata['analfinal'][$f]['sz'.$sz]++;
						if (($f==24 || $f==25)) {
							if (isset($exdata['sections'][$tag])) {
								@ $testdata['analfinal'][$f]['prop1sz'.$sz]++;
							}
							else {
								@ $testdata['analfinal'][$f]['prop0sz'.$sz]++;
							}
						}
					}
					
					if ($tagstr!='') {
						if ($effend<$nextRec)
							$tagstr .= ' ex';
						@ $testdata['finalstr']['nfinal'][$nfinal][$tagstr]++;
						@ $testdata['finalstr'][$tagstr][$szstr]++;
					}
					if (isset($sizes['Empty Flag']) && $sizes['Empty Flag']>2) {
						$this->__add_error("Confirm Empty Flag with size>2 (size=".$sizes['Empty Flag'].")");
					}
				}
				else if (in_array(3, $fs)) {
					$tloc = ftell($this->_fpin);
					$datalen = $this->extract_single('ushort') + 2;
					if ($tloc+$datalen>$nextRec) {
						$this->__add_error("Attempt to read past end of record in misc section (datalen=$datalen)");
					}
					else {
						fseek($this->_fpin, $tloc+$datalen);
						$sizes['Havok Moved Plus'] = $datalen;
						$this->__set_error_stats(array('miscLoc' => ftell($this->_fpin)));
					}
				}
				$animend = ftell($this->_fpin);
				
				// Run Once notes:
				// assuming Run Once follows same pattern as Trespass/Follow, where it has a FFxxxxxx identifier,
				// it comes after the properties section, but with a significant amount of data between
				// properties and Run Once (>80 bytes)
				
				// Final order:
				// 24 3 25 ???
				// * 24 never seen with anything in front, but can have up to two x2s after it
				// * 25 seen with one x2 in front
				// * 3 either seen on its own, or as an x2 when 24 has two following x2s (and where 25 also present)
				//   But one case seen where data has to be 3->25 (24 not set in this case; non-zero properties)
				// ... but clear cases of 25->3
				// So 24 25 3
				// But ACHR only looks for 3?
				// And there are REFR 24/25 cases where there's only 1 nfinal (then hit end of record)
							
				// is it possible 24 doesn't have a nfinal option for ACHR/ACRE?
				//  (another case where 24 is set, but textsize=0)
				// and maybe nfinal=1 limit only true for ACHR/ACRE
				// (have a REFR case with 2/24/25 where nfinal=3 (all empty; non-zero props), and data has to bee
				//  read somehow to get to Scale; no 18 or 22 in case)
				//				if (!$skipproc && $data['type']=='REFR') {
				if (!$skipproc) {
					// finals -> 4
					// Scale can be set for ACHR/ACRE, in which case data
					// is possibly right after finals (assuming only one value found in finals)
					if (in_array(4, $fs)) {
						$scale = $this->extract_single('float');
						$sizes['Scale'] = 4;
						if ($scale<0.3 || $scale > 3) {
							$this->__add_error("Unexpected value for Scale ($scale)");
						}
						$this->__set_error_stats(array('locScale' => ftell($this->_fpin)));
						
						$nother = 0;
						foreach ($fs as $f) {
							if ($f==4 || $f==22 || $f==18 || $f==16 || $f==11)
								$nother++;
						}
						if ($data['type']=='REFR' && $nother>1)
							$this->__add_error("Confirm relative position of flag 4 (Scale value = $scale)");
					}
				}
				
				if (!$skipproc) {
					if (($data['type']=='ACHR' || $data['type']=='ACRE') && in_array(17, $fs)) {
						$floc = ftell($this->_fpin);
						$follow = $this->extract_generic_data(array($this->_changedef['Follow']));
						$sizes['Follow'] = ftell($this->_fpin)-$floc;
						$this->__set_error_stats(array('locFollow' => ftell($this->_fpin)));
						$fhex = sprintf("0x%08lx", $follow['id']);
						if (substr($fhex,0,4)!='0xff')
							$this->__add_error("Check follow id (".$follow['id'].", ".$fhex.")");
						
						if ($textline) {
							$found_fol = $found_int = $found_dial = false;
							$tag_fol = NULL;
							$tag_int = NULL;
							$tag_dial = NULL;
							foreach ($textline['changes'] as $flag => $size) {
								if (substr($flag,0,6)=='Follow') {
									$found_fol = true;
									if (strlen($flag)>6)
										$tag_fol = substr($flag,7);
								}
								elseif (substr($flag,0,9)=='Interrupt') {
									$found_int = true;
									if (strlen($flag)>9)
										$tag_int = substr($flag,10);
								}
								elseif (substr($flag,0,8)=='Dialogue') {
									$this->__add_error("Confirm dialogue record");
									$found_dial = true;
									if (strlen($flag)>8)
										$tag_dial = substr($flag,9);
								}
							}
							if ($textline['loaded']) {
								if ((!$found_fol && !$found_dial) || $found_int) {
									$this->__add_error("Unexpected Follow/Interrupt flags on loaded record");
								}
								if (isset($tag_fol) || isset($tag_dial)) {
									if (isset($tag_dial))
										$tag_fol = $tag_dial;
									if ($tag_fol=='00000000') {
										$this->__add_error("Zero-value tag for Follow flag on loaded record");
									}
									else {
										$fid = strtoupper(sprintf("%08lx", $follow['id']));
										if ($fid!=$tag_fol) {
											$this->_add_error("Unexpected value ($tag_fol) for Follow flag on loaded record; expected ".$fid." (".$follow['id'].")");
										}
									}
								}
							}
							else {
								$sizes['Interrupt'] = $sizes['Follow'];
								$sizes['Dialogue'] = $sizes['Follow'];
								if ($found_fol || !$found_int) {
									$this->__add_error("Unexpected Follow/Interrupt flags on non-loaded record");
								}
							}
						}
					}
				}
				
				if ($data['type']=='ACHR' && $pcdata) {
					$data_pc = $this->extract_generic_data(array($this->_changedef['PC Data']));
				}
				
				$dataend = ftell($this->_fpin);
				
				/**
					************************** End of reading data
					*/
				
				if (!$skipproc && $data['type']=='REFR') {
					if (ftell($this->_fpin)!=$nextRec) {
						$left = $nextRec-ftell($this->_fpin);
						// another case of a serious problem that's not being printed just to shorten output.  Needs fixing!!
						if ($left>0 || $nfinal<2) {
							$this->__add_error("Wrong amount of data read, available=$left (known data read to ".ftell($this->_fpin)."; dataend=$dataend, nfinal=$nfinal; end at $nextRec)");
							$doprint = true;
							$doscan = true;
							$skipproc = true;
						}
					}
				}
				elseif (!$skipproc) {
					if (ftell($this->_fpin)>$nextRec) {
						$extra = ftell($this->_fpin)-$nextRec;
						$this->__add_error("Too much data read, extra=$extra (known data read to ".ftell($this->_fpin)."; dataend=$dataend, nfinal=$nfinal; end at $nextRec)");
					}
				}
				
				// Figure out order of nfinal sections
				// Assuming (for now) only relevant for REFR... ACHR/ACRE only do 3 as final section
				if (!$skipproc && $data['type']=='REFR') {
					$addorder = array(24 => 'Movement Extra', 25 => 'Animation', 3 => 'Havok Moved', 11 => 'Had Havok Move Flag', 16 => 'Empty Flag', 22 => 'Furniture Markers', 18 => 'Door Default State');
					if ($data['type']!='REFR') {
						unset($addorder[22]);
						unset($addorder[16]);
						unset($addorder[18]);
					}
					
					$fids = array();
					if ($nfinal && $textline && !empty($textline['loaded'])) {
						$left = $effend - $miscend;
						foreach ($addorder as $f => $name) {
							if (!isset($textline['changes'][$name]))
								continue;
							if (!isset($lfinals[$name]))
								$lfinals[$name] = array('tot' => 0, 'add' => 0);
							$lfinals[$name]['tot']++;
							$tot = 0;
							if (isset($exdata['sections'][$name]))
								$tot += $exdata['sections'][$name];
							if ($name=='Havok Moved' && isset($sizes[$name]))
								$tot += $sizes[$name];
							if ($textline['changes'][$name]>$tot) {
								$diff = 0;
								$fkeep = NULL;
								$fwhich = 0;
								if (($fwhich=array_search($textline['changes'][$name]-$tot,$fsizes))!==false && $fsizes[$fwhich]>2 && !isset($fids[$fwhich])) {
									$fkeep = $fwhich;
									$diff = $textline['changes'][$name]-$tot;
								}
								if (($tot || $name=='Animation') && ($fwhich=array_search($textline['changes'][$name]-$tot-2,$fsizes))!==false && $fsizes[$fwhich]>2 && !isset($fids[$fwhich])) {
									if (!isset($fkeep) || $fwhich<$fkeep) {
										$fkeep = $fwhich;
										$diff = $textline['changes'][$name]-$tot-2;
									}
								}
								if ($name=='Havok Moved' && $textline['changes'][$name]>28 && $textline['chgtot']==$textline['size']+28 && ($fwhich=array_search($textline['changes'][$name]-$tot-28, $fsizes))!==false && $fsizes[$fwhich]>2 && !isset($fids[$fwhich])) {
									if (!isset($fkeep) || $fwhich<$fkeep) {
										$fkeep = $fwhich;
										$diff = $textline['changes'][$name]-$tot-28;
									}
								}
								if ($diff) {
									$left -= $diff;
									$lfinals[$name]['add']++;
									$fids[$fkeep] = $f;
								}
							}
						}
						
						$stop = false;
						foreach ($fsizes as $fwhich => $size) {
							if (!isset($fids[$fwhich]) && $size>2)
								$stop = true;
						}
						
						if (count($fids)) {
							ksort($fids);
							$lstring = implode($fids, ' ');
							// 25_3 is correct, not 3_25
							// have one crystal-clear case of 25_3 (size 25=6; size 3=2823)
							// have several other good cases of 25_3 (size 25=38, size 3=33; anim text total = 40, havok text total=61)
							// cases of 3_25 were all cases where sizes=31,33 -- order could go either way;
							// originally being assigned to 25_3 despite addorder; now following addorder
							if (!isset($lorder[$lstring]))
								$lorder[$lstring] = 0;
							$lorder[$lstring]++;
							foreach ($fsizes as $fwhich => $sz) {
								if (isset($fids[$fwhich]))
									continue;
								$fids[$fwhich] = "x($sz)";
								if ($sz>2)
									$stop = true;
							}
							ksort($fids);
							$lstrb = implode($fids, ' ');
							$lunused = '';
							foreach ($addorder as $f => $name) {
								if (!isset($textline['changes'][$name]))
									continue;
								if (in_array($f, $fids))
									continue;
								if ($lunused!='')
									$lunused .= ' ';
								if (isset($exsizes[$name.' misc']))
									$unused = $exsizes[$name.' misc'];
								else
									$unused = $textline['changes'][$name];
								$lunused .= $f.'('.$unused.')';
							}
							if ($lunused!='')
								$lstrb .= ' + '.$lunused;
							@ $lorder['all'][$lstring][$lstrb]++;
						}
						
						if ($stop) {
							print "fsizes:\n";
							var_dump($fsizes);
							print "fids:\n";
							var_dump($fids);
							print "textline:\n";
							var_dump($textline['changes']);
							if (!empty($exdata['sections'])) {
								print "exdata:\n";
								var_dump($exdata['sections']);
							}
							$this->__add_error("Bad lfinal analysis");
						}
					}
				}
					
				// Stats on textline flags
				if ($textline && !empty($textline['loaded'])) {
					$currloc = ftell($this->_fpin);
					$chkvals = array('Magic Modifiers' => 20, 'Script Modifiers' => 21, 'Game Modifiers' => 22, 'Disp Modifiers' => 15);
					foreach ($textline['changes'] as $flag => $sz) {
						if (!isset($chkvals[$flag]))
							continue;
						$textdata = array();
						
						$textdata['vals'] = $sz;
						if ($flag=='Disp Modifiers')
							$div = 8;
						else
							$div = 5;
						if ($sz==0 || $sz<=2) {
						}
						elseif ((($sz-2)%$div)!=0) {
							$textdata['badvals'] = $sz;
						}
						else {
							$chknum = ($sz-2)/$div;
							fseek($this->_fpin, $miscend);
							$nscan = $nextRec-$miscend-$sz+1;
							$lastloc = NULL;
							$found = array();
							for ($n=0; $n<$nscan; $n++) {
								if (isset($lastloc))
									fseek($this->_fpin, $lastloc);
								$lastloc = NULL;
								// assuming chknum always < 256
								$test = $this->extract_single('ubyte');
								if ($test!=$chknum)
									continue;
								$lastloc = ftell($this->_fpin);
								$test = $this->extract_single('ubyte');
								if ($test!=0)
									continue;
								$ok = true;
								for ($chk=0; $chk<$chknum; $chk++) {
									if ($div==5) {
										$this->extract_single('ubyte');
										$test = $this->extract_single('float');
										if ((abs($test)>0 && abs($test)<1.e-5) || abs($test)>1.e8) {
											$ok = false;
											break;
										}
									}
									else {
										$test = $this->extract_single('ulong');
										if ($test>count($this->_ireflist) && $test<0xff000000) {
											$ok = false;
											break;
										}
										$this->extract_single('ulong');
									}
								}
								if (!$ok)
									continue;
								$found[] = $miscend + $n;
							}
							
							$nfound = count($found);
							$textdata['nfound'] = $nfound;
							$textdata['forward'] = array();
							$textdata['backward'] = array();
							foreach ($found as $loc) {
								$textdata['forward'][] = $loc-$miscend;
								$textdata['forward'][] = ($loc-$miscend)." x".count($found);
								$textdata['backward'][] = $nextRec-$sz-$loc;
								$textdata['backward'][] = ($nextRec-$sz-$loc)." x".count($found);
							}
						}
						
						foreach ($textdata as $tsname => $tsarray) {
							if (!is_array($tsarray))
								$tsarray = array($tsarray);
							foreach ($tsarray as $tsval) {
								if (!isset($textstats[$flag][$tsname][$tsval]))
									$textstats[$flag][$tsname][$tsval] = 0;
								$textstats[$flag][$tsname][$tsval]++;
							}
						}
					}
					fseek($this->_fpin, $currloc);
				}
				
				// Do sizes of sections match expected
				// I shouldn't check ... or at least shouldn't worry about check results ...
				// if textline['size']!=sum($textline['changes'])
				// Havok Moved total seems particularly prone to >20 byte surpluses
				if ($textline && !empty($textline['loaded']) && $data['type']=='REFR') {
						// sizes listed for Furniture Markers and Door Default State seem bogus
					foreach (array('Furniture Markers', 'Door Default State') as $flag) {
						if (isset($textline['changes'][$flag]) && $textline['changes'][$flag]==2 && $textline['chgtot']>=$textline['size']+2) {
							$textline['changes'][$flag] -= 2;
							$textline['chgtot'] -= 2;
						}
					}
					foreach ($textline['changes'] as $flag => $sz) {
						if (!$sz)
							continue;
						$tot = 0;
						if (isset($sizes[$flag]))
							$tot += $sizes[$flag];
						if (isset($exdata['sections'][$flag]))
							$tot += $exdata['sections'][$flag];
						if ($flag=='All Extra' && isset($exdata['sections']['Owner']) && !in_array(7, $fs))
							$tot += $exdata['sections']['Owner'];
						// 75-2-2 is Movement Extra, not Animation (although Animation always seems to be next prop)
						//						if ($flag=='Animation' && isset($exdata['sections']['75-2-2']))
						//	$tot += $exdata['sections']['75-2-2'];
						if ($flag=='Havok Moved' && isset($sizes['Havok Moved Plus']))
							$tot += $sizes['Havok Moved Plus'];
						if ($tot==$sz)
							continue;
						if (isset($exdata['sections'][$flag]) && ($tot+2)==$sz)
							continue;
						if ($miscend > $invend && ($tot+2)==$sz && (!isset($sizes[$flag]) || $sizes[$flag]==$tot))
							continue;
						
						// exceptions from here on are cases that I still need to figure out
						// .... but in the meantime I don't want to be swamped with error messages about these cases
						if ($flag=='Havok Moved' && ($tot+28)==$sz && $textline['chgtot']==$textline['size']+28)
							continue;
						// Old 75 processing... but I think it's being handled now (and not Animation anyway??)
						/*						if ($flag=='Animation' && isset($exdata['sections']['75-last']) && ($tot+4)==$sz) {
							continue;
						}*/
						if ($flag=='Havok Moved' && ($tot+2)==$sz && ($nextRec-$animend)==2)
							continue;
						if ($flag=='Animation' && ($tot+4)==$sz)
							continue;
						if ($flag=='Animation' && count($fsizes)==2 && ($fsizes[0]+$fsizes[1])==$sz)
							continue;
						
						$this->__add_error("Mismatch in section sizes for $flag.  textdump=".$textline['changes'][$flag]."; my total=$tot");
						if (abs($tot-$sz)>4) {
							$dodump = true;
						}
					}
				}
				elseif ($textline && !empty($textline['loaded'])) {
					foreach ($textline['changes'] as $flag => $sz) {
						if (!$sz)
							continue;
						$tot = 0;
						if (isset($sizes[$flag]))
							$tot += $sizes[$flag];
						if (isset($exdata['sections'][$flag]))
							$tot += $exdata['sections'][$flag];
						if ($flag=='Havok Moved' && isset($sizes['Havok Moved Plus']))
							$tot += $sizes['Havok Moved Plus'];
						if (!$tot)
							continue;
						if ($tot==$sz)
							continue;
						if ($flag=='Havok Moved' && ($tot+28)==$sz && $textline['chgtot']==$textline['size']+28)
							continue;
						
						$this->__add_error("Mismatch in section sizes for $flag.  textdump=".$textline['changes'][$flag]."; my total=$tot");
					}
				}
				
				if ($textline && $textline['loaded'] && ($data['type']=='ACHR' || $data['type']=='ACRE') && in_array(19, $fs)) {
					$this->__add_error("Run Once set for loaded record");
				}
				if ($textline && $textline['loaded'] && in_array(11, $fs)) {
					$this->__add_error("Had Havok Move Flag set for loaded record");
				}
				if (in_array(11, $fs) && in_array(3, $fs)) {
					$this->__add_error("Had Havok Move Flag and Havok Moved both set for same record");
				}
				
				if ($data['type']!='REFR' && !$pcdata) {
					$done = ftell($this->_fpin)-$startRec;
					$tot = $nextRec-$startRec;
					
					@ $testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['done'] += $done;
					@ $testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['tot'] += $tot;
					if (!isset($testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['min']))
						$testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['min'] = $done/$tot;
					else
						$testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['min'] = min($testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['min'], $done/$tot);
					@ $testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['max'] = max($testdata['fracdone'][$data['type']][$actorvals[$actor_flag]]['max'], $done/$tot);
				}
			}
			else {
				$this->read_change_record($data);
				if (isset($data['subrec_size'])) {
					foreach ($data['subrec_size'] as $subrec => $sz) {
						$sizes[$subrec] = $sz;
					}
				}
				if (in_array(0,$fs)) {
					@ $formflags[sprintf("%08lx", $data['form_flags'])]++;
					@ $formflags[$data['type']][sprintf("%08lx", $data['form_flags'])]++;
				}
				// Do sizes of sections match expected
				if ($textline && !empty($textline['loaded'])) {
					foreach ($textline['changes'] as $flag => $sz) {
						$tot = 0;
						if (isset($sizes[$flag]))
							$tot += $sizes[$flag];
						if ($tot==$sz)
							continue;
						if ($flag=='Quest Script' && $tot>$sz)
							continue;
						$this->__add_error("Mismatch in section sizes for $flag.  textdump=".$textline['changes'][$flag]."; my total=$tot");
						$doscan = true;
					}
				}
				// not trying to handle quest scripts length mismatches
				if ($data['type']=='QUST' && isset($sizes['Quest Script']) && ftell($this->_fpin)>$nextRec && (ftell($this->_fpin)-$nextRec)<$sizes['Quest Script']) {
					fseek($this->_fpin, $nextRec);
				}
				if (ftell($this->_fpin)!=$nextRec) {
					$this->__add_error("Wrong amount of data read; available=".($nextRec-ftell($this->_fpin))." (known data read to ".ftell($this->_fpin).";nfinal=$nfinal)");
				}
				
				if ($data['type']=='NPC_' || $data['type']=='CREA') {
					foreach ($data as $valname => $value) {
						if ($valname=='health_base' || $valname=='magicka_base' || $valname=='fatigue_base') {
							// game likes to overkill quest NPCs, e.g. Baenlin and Francois Motierre
							$valmin = -100;
							$valmax = 1000;
						}
						elseif ($valname=='level_base') {
							$valmin = -10;
							$valmax = 100;
						}
						elseif ($valname=='speed_base' && $data['type']=='CREA') {
							$valmin = 0;
							$valmax = 255;
						}
						elseif (substr($valname,-5)=='_base') {
							$valmin = 0;
							$valmax = 100;
						}
						else
							continue;
						if ($value<$valmin)
							$this->__add_error("Value of $valname less than expected minimum ($valmin vs $value)");
						elseif ($value>$valmax)
							$this->__add_error("Value of $valname greater than expected maximum ($valmax vs $value)");
					}
				}
				if ($data['type']=='NPC_' || $data['type']=='CREA') {
					if (isset($data['mod'])) {
						foreach (array(9,10) as $chkval) {
							for ($im=0; $im<count($data['mod']); $im++) {
								if ($data['mod'][$im]['mod_index']==$chkval && $data['mod'][$im]['mod_val']!=0) {
									$errors[] = "Non-zero value (".$data['mod'][$im]['mod_val'].") for mod index $chkval";
								}
							}
						}
					}
					if (isset($data['autocalc']) && $data['autocalc']==0 && (isset($data['fatigue_base']) && $data['fatigue_base']==0)) {
						$errors[] = "Zero value of magicka (".$data['magicka_base'].") or fatigue (".$data['fatigue_base'].") for non-autocalc NPC";
					}
				}
			}
			
			// Match numeric flags to textline flags
			$foundfs = array();
			$foundtext = array();
			foreach ($fs as $f) {
				if (!isset($flagnames[$f])) {
					$this->__add_error("No flagnames provided for flag $f");
					continue;
				}
				$testnames = array();
				if (!is_array($flagnames[$f])) {
					$testnames[] = $flagnames[$f];
				}
				else {
					foreach ($flagnames[$f] as $name => $when) {
						if (is_null($when)) {
							$testnames[] = $name;
							continue;
						}
						elseif ($when=='R' && ($data['type']=='REFR' || $data['type']=='ACHR' || $data['type']=='ACRE')) {
							$testnames[] = $name;
							continue;
						}
						elseif ($when=='AC' && ($data['type']=='ACHR' || $data['type']=='ACRE')) {
							$testnames[] = $name;
							continue;
						}
						elseif ($when=='C' && ($data['type']=='NPC_' || $data['type']=='CREA')) {
							$testnames[] = $name;
							continue;
						}
						elseif ($when==$data['type']) {
							$testnames[] = $name;
							continue;
						}
					}
				}
				if (!count($testnames)) {
					$this->__add_error("No flagnames provided for flag $f in rectype ".$data['type']);
					continue;
				}
				if ($textline) {
					foreach ($textline['changes'] as $flag => $sz) {
						foreach ($testnames as $name) {
							$nlen = strlen(trim($name));
							if ($nlen && (substr($flag,0,$nlen)==$name)) {
								$foundfs[$f] = true;
								$foundtext[$flag] = true;
								break 2;
							}
						}
					}
					if (!empty($foundfs[$f]))
						continue;
					foreach ($testnames as $name) {
						$nlen = strlen(trim($name));
						if ($nlen==0) {
							$foundfs[$f] = true;
							break;
						}
					}
				}
			}
			if ($textline) {
				foreach ($fs as $f) {
					if (!isset($foundfs[$f]))
						$this->__add_error( "No match for flag $f found in text dump");
				}
				foreach ($textline['changes'] as $flag => $sz) {
					if ($flag=='Low' || $flag=='Mid Low' || $flag=='Mid High' || $flag=='High' || $flag=='None')
						continue;
					if (!isset($foundtext[$flag]))
						$this->__add_error("No match for text dump value $flag found in fs array");
				}
			}
			
			if (count($errors) || count($this->_currerrors)) {
				$doprint = true;
				$doscan = true;
				$nprocerr++;
				$qual = 'bad';
			}
			else {
				$qual = 'good';
			}
			if (!isset($combos[$qual][$data['type']][$fsstring]))
				$combos[$qual][$data['type']][$fsstring] = 0;
			$combos[$qual][$data['type']][$fsstring]++;
			
			foreach ($fs as $f) {
				if (!isset($nfs[$f][$data['type']]))
					$nfs[$f][$data['type']] = array('tot' => 0, 'good' => 0, 'bad' => 0);
				$nfs[$f][$data['type']]['tot']++;
				$nfs[$f][$data['type']][$qual]++;
			}
			
			if ($doprint || $doscan) {
				print "-----\n";
				print $summstring;
				if ($textline)
					print $textline['text'];
				foreach ($errors as $err) {
					print $err."\n";
				}
				foreach ($this->_currerrors as $err) {
					print $err['error']."\n";
					print $err['where'];
					print "file loc = ".$err['loc']."\n";
				}
				if ($doscan || $combos[$qual][$data['type']][$fsstring]==1) {
					fseek($this->_fpin, $startRec);
					print "full scan:\n";
					$this->scan_raw_data($data['size']);
				}
				if ($dodump || $combos[$qual][$data['type']][$fsstring]==1) {
					print "nfinal=$nfinal\n";
					print "fsizes\n";
					var_dump($fsizes);
					print "sizes\n";
					var_dump($sizes);
					print "exdata.sections\n";
					if (isset($exdata['sections']))
						var_dump($exdata['sections']);
					print "text.changes\n";
					var_dump($textline['changes']);
					if (!empty($fids)) {
						print "fids\n";
						var_dump($fids);
						print "lstring=$lstring, lstrb=$lstrb\n";
					}
				}
			}
		
			fseek($this->_fpin, $nextRec);
			$this->__unset_error_stats();
		}
		
		print "----------------------------  End ----------------------------------\n";
		print "Processed records: $nproc\nProcessed errors: $nprocerr\n";
		print "Zero-size records; $nzero\n";
		print "nendis:\n";
		var_dump($nendis);
		print "lfinals\n";
		var_dump($lfinals);
		print "lorder\n";
		var_dump($lorder);
		print "ntype\n";
		var_dump($ntype);
		
		foreach (array('good', 'bad') as $qual) {
			print ucfirst($qual)." combos:\n";
			if (!isset($combos[$qual]))
				continue;
			ksort($combos[$qual], SORT_NUMERIC);
			foreach ($combos[$qual] as $rectype => $cdata) {
				print "  $rectype (".count($cdata)." combos):\n";
				ksort($cdata, SORT_NUMERIC);
				foreach ($cdata as $combo => $num) {
					print "       \"$combo\" ($num)\n";
				}
			}
		}
		ksort($nfs);
		foreach ($nfs as $f => $rectypes) {
			$first = true;
			ksort($rectypes);
			foreach ($rectypes as $type => $fdata) {
				if ($first) {
					$first = false;
					printf("  %4d  ", $f);
				}
				else {
					print "        ";
				}
				print $type;
				printf("  %6d  %6d  %6d    %5.1f\n", $fdata['tot'], $fdata['good'], $fdata['bad'], $fdata['bad']/$fdata['tot']*100);
			}
		}
		
		print "propstats:\n";
		ksort($this->_propstats);
		foreach ($this->_propstats as $prop => $pdata) {
			print "  \"$prop\":  ".$pdata['total']." total\n";
			ksort($pdata);
			if (isset($pdata['Inv']))
				print "        Inv   ".$pdata['Inv']."\n";
			foreach ($pdata as $rectype => $num) {
				if (strlen($rectype)!=4 || strtoupper($rectype)!=$rectype)
					continue;
				print "        $rectype ".$pdata[$rectype]."\n";
			}
			if (isset($pdata['follow1'])) {
				print "        follow:";
				for ($i=1; ;$i++) {
					if (!isset($pdata['follow'.$i]))
						break;
					print " $i=".$pdata['follow'.$i];
				}
				print "\n";
			}
		}
		if (isset($this->_propstats['0x4b']))
			var_dump($this->_propstats['0x4b']);
			
		print "textsizes:\n";
		ksort($textsizes);
		foreach ($textsizes as $section => $sdata) {
			print "  $section: ".$sdata['num']." total\n";
			ksort($sdata);
			print "        size range: ".$sdata['min']."-".$sdata['max']."\n";
			ksort($sdata['sizes']);
			print "             vals: ".implode(array_keys($sdata['sizes'])," ")."\n";
			arsort($sdata['sizes']);
			if (count($sdata['sizes'])>1) {
				print "             common: ";
				$i=0;
				foreach ($sdata['sizes'] as $sz => $num) {
					if ($i>6)
						break;
					$i++;
					print "$sz($num) ";
				}
				print "\n";
			}
			foreach ($sdata as $rectype => $num) {
				if (strlen($rectype)!=4 || strtoupper($rectype)!=$rectype)
					continue;
				print "        $rectype ".$sdata[$rectype]."\n";
			}
		}
		
		print "textstats:\n";
		foreach ($textstats as $section => $sdata) {
			print "  $section: \n";
			ksort($sdata['vals']);
			print "       datalen=";
			foreach($sdata['vals'] as $val => $num) {
				print " $val($num)";
			}
			print "\n";
			ksort($sdata['nfound']);
			print "       nfound=";
			$nfmax = NULL;
			foreach($sdata['nfound'] as $val => $num) {
				print " $val($num)";
				$nfmax = $val;
			}
			print "\n";
			foreach (array('forward', 'backward') as $dir) {
				arsort($sdata[$dir]);
				print "       $dir\n";
				foreach ($sdata[$dir] as $val => $num) {
					if (strpos($val,'x')!==false)
						continue;
					print "            $val($num)";
					for ($j=1; $j<=$nfmax; $j++) {
						if (!isset($sdata[$dir][$val.' x'.$j]))
							continue;
						print "     x$j(".$sdata[$dir][$val.' x'.$j].")";
					}
					print "\n";
				}
			}
		}
		print "formflags:\n";
		var_dump($formflags);
		print "celltypes:\n";
		var_dump($celltypes);
		
		var_dump($testdata);
		
		print "\n--------------------------------------Serialized Stats-------------------------------\n";
		// + propstats
		foreach (array('nproc', 'nprocerr', 'nzero', 'nendis', 'lfinals', 'lorder', 'ntype', 'combos', 'nfs', 'textsizes', 'textstats', 'formflags', 'celltypes') as $statname) {
			if (empty($$statname))
				continue;
			print "$statname ".serialize($$statname)."\n";
		}
		foreach ($testdata as $statname => $sdata) {
			print "$statname ".serialize($sdata)."\n";
		}
		print "propstats ".serialize($this->_propstats)."\n";
	}
	
	// For now, just a copy of anal_changes
	// Eventually, have this be the cleaned-up function
	function read_changes() {
		$typenames = array(6 => 'FACT',
		                   19 => 'APPA',
		                   20 => 'ARMO', 
		                   21 => 'BOOK',
		                   22 => 'CLOT',
		                   25 => 'INGR',
		                   26 => 'LIGH',
		                   27 => 'MISC',
		                   33 => 'WEAP',
		                   35 => 'NPC_',
		                   36 => 'CREA',
		                   38 => 'SLGM',
		                   39 => 'KEYM',
		                   40 => 'ALCH',
		                   48 => 'CELL',
		                   49 => 'REFR',
		                   50 => 'ACHR',
		                   51 => 'ACRE',
		                   58 => 'INFO',
		                   59 => 'QUST',
		                   61 => 'PACK');
		
		$post = array();
		$zero = array();
		
		$ntype = array();
		$typesize = array();
		for ($ichg=0; $ichg<$this->_readdata['recordsCount']; $ichg++) {
			if (feof($this->_fpin)) {
				print "out of data at $ichg of ".$this->_readdata['recordsCount']." records\n";
				break;
			}
			$data = array();
			// probably only want to get raw_formid here -- no need to look up the value
			// until type known and I've decided whether it's change data I'm interested in
			$data['formid'] = $this->extract_single('raw_formid');
			$data['typenum'] = $this->extract_single('ubyte');
			if (isset($typenames[$data['typenum']]))
				$data['type'] = $typenames[$data['typenum']];
			else {
				print "invalid entry (".$data['typenum'].") for type at $ichg of ".$this->_readdata['recordsCount']." records, formid = ".$data['formid']."\n";
				break;
			}
			$data['flags'] = $this->extract_single('ulong');
			$data['hex_flags'] = sprintf('%08lx', $data['flags']);
			$data['version'] = $this->extract_single('ubyte');
			$data['size'] = $this->extract_single('ushort');
			
			if (!isset($ntype[$data['type']]))
				$ntype[$data['type']] = 0;
			$ntype[$data['type']]++;
			
			if (!isset($typesize[$data['type']][$data['flags']][$data['size']]))
				$typesize[$data['type']][$data['flags']][$data['size']] = 0;
			$typesize[$data['type']][$data['flags']][$data['size']]++;
			$nextRec = ftell($this->_fpin) + $data['size'];
			$this->_change_endloc = $nextRec;
			if ($data['formid']=='0x00000007' || $data['formid']=='0x00000014') {
				$data['is_pc'] = true;
				
				for ($i=0; $i<32; $i++) {
					$fi = (($data['flags']>>$i)&1) ? true : false;
					if ($fi)
						print $i."\n";
				}
				
				//				if ($data['type']=='NPC_')
				$this->read_change_record($data);
				
				// THIS IS THE IMPORTANT PART TO KEEP of read_changes
				if ($data['formid']=='0x00000007') {
					$item = $this->_parentlist->get_item($data['formid']);
				}
				else {
					$base = $this->_parentlist->get_item('0x00000007');
					$item = $this->_parentlist->create_ref_item($base, 'ACHR', '0x00000014');
				}	
				$item->add_change_data($data);
					// don't release the item!!
			}
			fseek($this->_fpin, $nextRec);
		}
	}
	
	function read_change_record(&$data) {
		if (!isset($this->_changeorder[$data['type']])) {
			$this->__add_error("Unrecognized rectype entry ".$data['type']." for changeorder.  Flags=".sprintf("0x%02lx", $data['flags']));
			$usetype = 'generic';
		}
		else
			$usetype = $data['type'];
		foreach ($this->_changeorder[$usetype] as $chgtype => $chgtest) {
			$startloc = ftell($this->_fpin);
			$dochg = false;
			if ($chgtest===true) {
				$dochg = true;
			}
			elseif (is_int($chgtest)) {
				$dochg = (($data['flags']>>$chgtest)&1) ? true : false;
			}
			elseif (is_string($chgtest)) {
				$chgtest = '$dochg = '.$chgtest.';';
				eval($chgtest);
			}
			if ($this->_sdebug>=20)
				print "in changeorder, rectype=".$data['type']." (usetype=$usetype) flags=".$data['flags']." startloc=$startloc chgtype=$chgtype chgtest=$chgtest dochg=$dochg\n";
			if (!$dochg)
				continue;
			$this->__set_error_stats(array('currChg' => $chgtype));
			$this->__set_error_stats(array('locChg'.$chgtype => $startloc), 1);
			$chgdata = $this->extract_generic_data(array($this->_changedef[$chgtype]));
			$chgdata = $this->convert_array_irefs($chgdata);
			$data = array_merge($data, $chgdata);
			if ($this->_sdebug>=10) {
				$data['subrec_size'][$chgtype] = ftell($this->_fpin)-$startloc;
			}
		}
		$this->__unset_error_stats();
	}
	
	function convert_array_irefs($data) {
		foreach ($data as $key => $value) {
			if (substr($key,-4,4)=='iref') {
				$newkey = substr($key,0,-4).'id';
				if (is_array($value)) {
					foreach ($value as $subkey => $subvalue) {
						$iref = $this->convert_iref($subvalue);
						if ($iref!==false)
							$data[$newkey][$subkey] = $iref;
					}
				}
				else {
					$iref = $this->convert_iref($value);
					if ($iref!==false)
						$data[$newkey] = $iref;
				}
				unset($data[$key]);
			}
			elseif (is_array($value)) {
				$data[$key] = $this->convert_array_irefs($value);
			}
		}
		return $data;
	}
	
	function read_activeEffect($ddef=NULL, $input=NULL) {
		$data = array();
		$data['size'] = $this->extract_single('ushort');
		$data['iref'] = $this->extract_single('ulong');
		$data['byte'] = $this->extract_single('ubyte');
		$data['struct'] = $this->extract_single(array('type' => 'bytes', 'len' => $data['size']));
		return $data;
	}
	
	function read_pcdata($ddef=NULL, $input=NULL) {
		$ddef = array(array('val' => 'statistics', 'len' => 34*4, 'type' => 'string' /* ulong[34] */, 'skip' => true),
		              array('val' => NULL, 'len' => 10),
		              array('val' => 'unk1Count', 'type' => 'ushort'),
		              array('val' => 'unk1', 'len' => 4, 'type' => 'bytes' /* ubyte[4] */ /*, 'skip' => true*/),
		              array('val' => NULL, 'len' => 22),
		              array('val' => 'pcBirthsign', 'type' => 'ulong' /* iref */),
		              array('val' => NULL, 'len' => 4*13),
		              // Unk2 not ID'd in UESP doc -> rolled into previous 13 longs
		              array('val' => 'unk2Count', 'type' => 'ushort'),
		              array('val' => NULL, 'len' => 2),
		              array('val' => 'unk2', 'len' => 4, 'type' => 'bytes'),
		              array('val' => NULL, 'len' => 2),
		              array('val' => 'randODoorsCount', 'type' => 'ushort'),
		              array('val' => 'randODoors', 'len' => 5, 'type' => 'string' /*iref, ubyte*/, 'skip' => true),
		              array('val' => NULL, 'len' => 2),
		              array('val' => 'activeEffectCount', 'type' => 'ushort'),
		              array('val' => 'activeEffect', 'sub' => 'read_activeEffect'),
		              array('val' => 'armorer_exp', 'type' => 'float'),
		              array('val' => 'athletics_exp', 'type' => 'float'),
		              array('val' => 'blade_exp', 'type' => 'float'),
		              array('val' => 'block_exp', 'type' => 'float'),
		              array('val' => 'blunt_exp', 'type' => 'float'),
		              array('val' => 'hand_to_hand_exp', 'type' => 'float'),
		              array('val' => 'heavy_armor_exp', 'type' => 'float'),
		              array('val' => 'alchemy_exp', 'type' => 'float'),
		              array('val' => 'alteration_exp', 'type' => 'float'),
		              array('val' => 'conjuration_exp', 'type' => 'float'),
		              array('val' => 'destruction_exp', 'type' => 'float'),
		              array('val' => 'illusion_exp', 'type' => 'float'),
		              array('val' => 'mysticism_exp', 'type' => 'float'),
		              array('val' => 'restoration_exp', 'type' => 'float'),
		              array('val' => 'acrobatics_exp', 'type' => 'float'),
		              array('val' => 'light_armor_exp', 'type' => 'float'),
		              array('val' => 'marksman_exp', 'type' => 'float'),
		              array('val' => 'mercantile_exp', 'type' => 'float'),
		              array('val' => 'security_exp', 'type' => 'float'),
		              array('val' => 'sneak_exp', 'type' => 'float'),
		              array('val' => 'speechcraft_exp', 'type' => 'float'),
		              array('val' => 'advancementCount', 'type' => 'ulong'),
		              array('val' => 'advancement',
		                    'data' => array(array('val' => 'strength_nadv', 'type' => 'byte'),
		                                    array('val' => 'intelligence_nadv', 'type' => 'byte'),
		                                    array('val' => 'willpower_nadv', 'type' => 'byte'),
		                                    array('val' => 'agility_nadv', 'type' => 'byte'),
		                                    array('val' => 'speed_nadv', 'type' => 'byte'),
		                                    array('val' => 'endurance_nadv', 'type' => 'byte'),
		                                    array('val' => 'personality_nadv', 'type' => 'byte'),
		                                    array('val' => 'luck_nadv', 'type' => 'byte'))),
		              array('val' => 'combat_nadv', 'type' => 'byte'),
		              array('val' => 'magic_nadv', 'type' => 'byte'),
		              array('val' => 'stealth_nadv', 'type' => 'byte'),
		              array('val' => 'armorer_nadv', 'type' => 'ulong'),
		              array('val' => 'athletics_nadv', 'type' => 'ulong'),
		              array('val' => 'blade_nadv', 'type' => 'ulong'),
		              array('val' => 'block_nadv', 'type' => 'ulong'),
		              array('val' => 'blunt_nadv', 'type' => 'ulong'),
		              array('val' => 'hand_to_hand_nadv', 'type' => 'ulong'),
		              array('val' => 'heavy_armor_nadv', 'type' => 'ulong'),
		              array('val' => 'alchemy_nadv', 'type' => 'ulong'),
		              array('val' => 'alteration_nadv', 'type' => 'ulong'),
		              array('val' => 'conjuration_nadv', 'type' => 'ulong'),
		              array('val' => 'destruction_nadv', 'type' => 'ulong'),
		              array('val' => 'illusion_nadv', 'type' => 'ulong'),
		              array('val' => 'mysticism_nadv', 'type' => 'ulong'),
		              array('val' => 'restoration_nadv', 'type' => 'ulong'),
		              array('val' => 'acrobatics_nadv', 'type' => 'ulong'),
		              array('val' => 'light_armor_nadv', 'type' => 'ulong'),
		              array('val' => 'marksman_nadv', 'type' => 'ulong'),
		              array('val' => 'mercantile_nadv', 'type' => 'ulong'),
		              array('val' => 'security_nadv', 'type' => 'ulong'),
		              array('val' => 'sneak_nadv', 'type' => 'ulong'),
		              array('val' => 'speechcraft_nadv', 'type' => 'ulong'),
		              array('val' => 'major_nadv', 'type' => 'ulong'),
		              array('val' => NULL, 'type' => 'byte'),
		              array('val' => 'activeQuest', 'type' => 'ulong' /*iref*/),
		              array('val' => 'knownTopicsCount', 'type' => 'ushort'),
		              array('val' => 'knownTopics', 'type' => 'ulong' /*iref*/, 'skip' => true),
		              array('val' => 'openQuestsCount', 'type' => 'ushort'),
		              // this contains an entry for each completed stage of every quest
		              // (for reviewing all the log entries you've been shown)
		              array('val' => 'openQuests', 'len' => 6, 'skip' => true,
		                    'data' => array(array('val' => 'quest_iref', 'type' => 'ulong' /*iref*/),
		                                    array('val' => 'questStage', 'type' => 'ubyte'),
		                                    array('val' => 'logEntry', 'type' => 'ubyte'))),
		              // probably the character's known magic effects for custom spellmaking
		              array('val' => 'magEffectCount', 'type' => 'ulong'),
		              array('val' => 'magEffect', 'len' => 4, 'type' => 'mgefstr', 'skip' => true),
		              array('val' => 'fgGeoSym', 'len' => 200, 'type' => 'string', 'skip' => true),
		              array('val' => 'fgGeoAsym', 'len' => 120, 'type' => 'string', 'skip' => true),
		              array('val' => 'fgTexSym', 'len' => 200, 'type' => 'string', 'skip' => true),
		              array('val' => 'race', 'type' => 'ulong' /*iref*/, 'skip' => true),
		              array('val' => 'hair', 'type' => 'ulong' /*iref*/, 'skip' => true),
		              array('val' => 'eyes', 'type' => 'ulong' /*iref*/, 'skip' => true),
		              array('val' => 'hairLength', 'type' => 'float' /*iref*/, 'skip' => true),
		              array('val' => 'hairColor', 'len' => 3, 'type' => 'string' /*ubyte[3]*/, 'skip' => true),
		              array('val' => 'hairFlag', 'type' => 'ubyte', 'skip' => true),
		              array('val' => 'gender', 'type' => 'ubyte'),
		              array('val' => 'pcName', 'type' => 'string'),
		              array('val' => 'pcClass', 'type' => 'ulong' /*iref*/));
		// customclass values are only present if pcClass iref points to the player custom class
		$ddef_cc = array(array('val' => 'ccAtt1', 'type' => 'ulong', 'lookup_conv' => 'attriblist'),
		                 array('val' => 'ccAtt2', 'type' => 'ulong', 'lookup_conv' => 'attriblist'),
		                 array('val' => 'ccSpec', 'type' => 'ulong', 'lookup_conv' => 'speclist'),
		                 array('val' => 'ccSkl1', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl2', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl3', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl4', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl5', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl6', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccSkl7', 'type' => 'ulong', 'lookup_conv' => 'actorlist'),
		                 array('val' => 'ccUnk1', 'type' => 'ulong', 'skip' => true),
		                 array('val' => 'ccUnk2', 'type' => 'ulong', 'skip' => true),
		                 array('val' => 'ccUnk3', 'type' => 'ulong', 'skip' => true),
		                 array('val' => 'ccName', 'type' => 'string'),
		                 array('val' => 'ccIcon', 'type' => 'string'),
		                 array('val' => NULL, 'type' => 'ulong' /*iref*/));
		
		if (!isset($this->_change_endloc))
			return;
		$data = array();
		$startloc = ftell($this->_fpin);
		
		$endloc = $this->_change_endloc;
		// simply scan until end of change record looking for certain markers to find the pc data
		$alocs = array();
		$imax = $this->_change_endloc - ftell($this->_fpin);
		$found = false;
		$last = NULL;
		for ($i=0; $i<$imax; $i++) {
			$test = $this->extract_single('ubyte');
			if ($test==0x42) {
				if ($last==0xec)
					$alocs[$i] = true;
				elseif ($last==0x96) {
					$ichk = $i-21;
					if (isset($alocs[$ichk])) {
						// skip rest of record; +28 instead +30 because two bytes of record (0x42, 0x96) already read
						fread($this->_fpin, 28);
						$found = true;
						break;
					}
				}
			}
			$last = $test;
		}
		if (!$found)
			return;
		
		$dataloc = ftell($this->_fpin);
		
		print "startloc=$startloc, proploc=".ftell($this->_fpin).", dataloc=$dataloc, endloc=$endloc\n";
		if ($this->_sdebug>=20) {
			fseek($this->_fpin, $startloc);
			$this->scan_raw_data($endloc-$startloc);
		}
		
		$this->_proptype = 'ACHR';
		fseek($this->_fpin, $startloc);
		$exdata = $this->read_property_structure();
		if (isset($exdata['sections'])) {
			print "sections:\n";
			var_dump($exdata['sections']);
		}
		elseif (count($this->_currerrors)) {
			print "Property errors:\n";
			foreach ($this->_currerrors as $err) {
				print $err['error']."\n";
				print $err['where'];
				print "file loc = ".$err['loc']."\n";
			}
		}
		
		fseek($this->_fpin, $dataloc);
		$data = $this->extract_generic_data($ddef);
		
		// are previous bytes health/magicka/etc.?? except more than 1 byte necessary, right?
		// is unk1 always 21??
		print "unk1 (".$data['unk1Count'].")\n";
		//		if ($data['unk1Count']!=21)
		//			$this->__add_error("value other than 21 for unk1Count (".$data['unk1Count'].")");
		foreach ($data['unk1'] as $i => $u) {
			if ($u!='00.00.00.00')
				print "  $i  $u\n";
		}
		print "unk2count =".$data['unk2Count']."\n";
		print "randODoorsCount = ".$data['randODoorsCount']."\n";
		
		$bsign_iref = $data['pcBirthsign'];
		$ordid = $this->convert_iref($bsign_iref);
		$item = $this->_parentlist->get_item($ordid);
		if (is_object($item)) {
			print "birthsign edid=".$item->edid()." ordid=".$item->ordid()." formid=".$item->formid()." name=".$item->get('name')."\n";
			$item->release();
			$dataok = true;
		}
		else {
			print "Unable to process birthsign info";
			$dataok = false;
		}
		
		if ($dataok) {
		if (isset($data['pcClass'])) {
			$class_iref = $data['pcClass'];
			$ordid = $this->convert_iref($class_iref);
			$item = $this->_parentlist->get_item($ordid);
			if (is_object($item)) {
				print "pcClass formid=".$item->formid()." name=".$item->get('name')." edid=".$item->edid()."\n";
				if ($item->formid()=='0x00022843') {
					$ccdata = $this->extract_generic_data($ddef_cc);
					var_dump($ccdata);
					$data = array_merge($data, $ccdata);
				}
				$item->release();
			}
		}
		
		// definitely a list of active effects (including racial, birthsign)
		// iref -> spell
		// byte: spell EFID index
		// values that should be present:
		//   magnitude (not nec. same as default mag, given spell eff, etc.)
		//   duration
		//   dispel-able and dispel magnitude necessary
		// some values are obviously irefs -- to caster?  to target?
		print "activeEffectNum (".$data['activeEffectCount'].")\n";
			if (!empty($data['activeEffect'])) {
		foreach ($data['activeEffect'] as $i => $odata) {
			print "  $i  size=".$odata['size']."  iref=".$odata['iref']." (0x".sprintf('%08lx', $odata['iref']).") byte=".$odata['byte']."  struct=".$odata['struct']."\n";
			$id = $this->convert_iref($odata['iref']);
			$item = $this->_parentlist->get_item($id);
			if (is_object($item)) {
				print "  name=".$item->get('name')."  edid=".$item->get('edid')."\n";
				$item->release();
			}
			for ($j0=0; $j0<(strlen($odata['struct'])/3-3); $j0++) {
				$tstr = '';
				for ($jd=0; $jd<4; $jd++) {
					$jnum = hexdec(substr($odata['struct'],($j0+$jd)*3,2));
					$tstr .= pack('C', $jnum);
				}
				$vals = unpack('f', $tstr);
				$val = $vals[1];
				if (abs($val)>0.01 && abs($val)<1.e5) {
					printf("%d=%.1f ", $j0, $val);
				}
				elseif ($j0==0) {
					print "$j0=$val ";
				}
			}
			print "\n";
		}
			}
				
		print "advancements (count=".$data['advancementCount']."):\n";
		var_dump($data['advancement']);
		print "post-adv numbers: activeQuest=".$data['activeQuest']."; knownTopicsCount=".$data['knownTopicsCount']."; openQuestsCount=".$data['openQuestsCount']."; pcName=".$data['pcName']."\n";
			
			foreach ($data as $valname => $value) {
				if (substr($valname,-4,4)!='_exp')
					continue;
				print "$valname = $value\n";
			}
		}
		print "end of pcdata at ".ftell($this->_fpin)."  end of ACHR record at $endloc\n";
		
		fseek($this->_fpin, $endloc);
		return $data;
	}
	
	function __get_curr_sub($use_parent=NULL) {
		if (!isset($use_parent))
			$use_parent=0;
		$btrace = debug_backtrace();
		$subs = array();
		$currsub = NULL;
		$anysub = NULL;
		foreach ($btrace as $btline) {
			$sub = $btline['function'];
			if (substr($sub,0,2)=='__')
				continue;
			if (!isset($currsub)) {
				$anysub = $sub;
				if ($use_parent)
					$use_parent--;
				else
					$currsub = $sub;
			}
			$subs[$sub] = true;
		}
		if (!isset($currsub) && isset($anysub))
			$currsub = $anysub;
		
		foreach ($this->_errorstats as $sub => $stats) {
			if (!isset($subs[$sub]))
				unset ($this->_errorstats[$sub]);
		}
		
		return $currsub;
	}
	
	function __add_error($error, $severity=0) {
		$currsub = $this->__get_curr_sub();
		// eventually track even more info? record being processed? property?
		$edata = array('error' => "Error in $currsub: ".$error);
		$error_where = '';
		foreach ($this->_errorstats as $sub => $stats) {
			$error_where .= "$sub:";
			foreach ($stats as $statname => $statval) {
				$error_where .= "  $statname=$statval";
			}
			$error_where .= "\n";
		}
		$edata['where'] = $error_where;
		$edata['loc'] = ftell($this->_fpin);
		$edata['sev'] = $severity;
		$this->_currerrors[] = $edata;
	}
	
	function __set_error_stats($stats, $use_parent=NULL) {
		$sub = $this->__get_curr_sub($use_parent);
		if (!isset($this->_errorstats[$sub]))
			$this->_errorstats[$sub] = $stats;
		else
			$this->_errorstats[$sub] = array_merge($this->_errorstats[$sub], $stats);
	}
	
	function __unset_error_stats() {
		$sub = $this->__get_curr_sub();
		unset($this->_errorstats[$sub]);
	}
	
	function read_property_structure($ddef=NULL, $input=NULL) {
		$data = array();
		
		$propnum = $this->extract_single('short');
		$prop_last = NULL;
		$this->__set_error_stats(array('propnum' => $propnum, 'locPropStart' => ftell($this->_fpin)));
		if ($propnum<0) {
			$this->__add_error("negative value for propnum ($propnum)");
			return false;
		}
		// 10 is arbitrary
		elseif ($propnum>10) {
			$this->__add_error("excessively large value for propnum ($propnum)");
			return false;
		}
		
		for ($iprop=0; $iprop<$propnum; $iprop++) {
			$loc0 = ftell($this->_fpin);
			$prop_flag = $this->extract_single('byte');
			$this->__set_error_stats(array('prop_flag' => sprintf("0x%02x",$prop_flag), 'locProp'.$iprop => $loc0, 'iprop' => $iprop));
			
			if (!isset($this->_propertydef[$prop_flag])) {
				/*				if ($prop_flag==0 && isset($prop_last) && $prop_last==75) {
					$test = $this->extract_single('byte');
					if ($test==0) {
						// continue is wrong here ... want to restart the loop
						continue;
					}
					fseek($this->_fpin, ftell($this->_fpin)-1);
				}*/
				$this->__add_error("unrecognized prop_flag ".sprintf("0x%02x",$prop_flag)." ($prop_flag)", 10);
				return false;
			}
			$pdef = $this->_propertydef[$prop_flag];
			$value = NULL;
			if (isset($pdef['len']) && !$pdef['len'])
				$value = true;
			elseif (isset($pdef['type']))
				$value = $this->extract_single($pdef);
			elseif (isset($pdef['data']))
				$value = $this->extract_generic_data($pdef['data']);
			
			if (isset($value) && isset($pdef['val']) && empty($pdef['skip']))
				$data[$pdef['val']] = $value;
			
			$locmid = ftell($this->_fpin);
			if ($prop_flag==0x4b && $iprop<$propnum-1) {
				$extra = $this->extract_single('ushort');
				if ($extra!=0) {
					$this->__add_error("extra bytes following 0x4b property were not 0x00.0x00 ($extra)");
					return false;
				}
			}
			$loc = ftell($this->_fpin);
			
			if ($loc>$this->_change_endloc) {
				$this->__add_error("read past end of change record (loc=$loc endloc=".$this->_change_endloc.")");
				return false;
			}
			
			if ($this->_sdebug>=10) {
				if (isset($pdef['section']))
					$section = $pdef['section'];
				else
					$section = sprintf('0x%02x', $prop_flag);
				if (!isset($data['sections'][$section]))
					$data['sections'][$section] = 0;
				$data['sections'][$section] += $loc-$loc0;
				$section = sprintf('0x%02x', $prop_flag);
				$ptype = $this->_proptype;
				if (!isset($this->_propstats[$section]['total']))
					$this->_propstats[$section]['total'] = 0;
				$this->_propstats[$section]['total']++;
				if (!isset($this->_propstats[$section][$ptype]))
					$this->_propstats[$section][$ptype] = 0;
				$this->_propstats[$section][$ptype]++;
				$nfollow = $propnum-$iprop-1;
				if ($nfollow) {
					if (!isset($this->_propstats[$section]['follow'.$nfollow]))
						$this->_propstats[$section]['follow'.$nfollow] = 0;
					$this->_propstats[$section]['follow'.$nfollow]++;
				}
			}
			if ($this->_sdebug>=10) {
				$prop_hex = sprintf("0x%02x", $prop_flag);
				if (in_array($prop_flag, array(0x28,0x29,0x6d)) || ($prop_flag==0x4e && ($loc-$loc0)>14)) {
					$this->__add_error("Property $prop_hex [$prop_flag] needs confirmation");
				}
				if ($prop_flag==0x3a && $value['extraCount']!=0) {
					if ($value['extraCount']>1 || ($loc-$loc0)!=68) {
						$this->__add_error("Property $prop_hex [$prop_flag] needs confirmation for extraCount=".$value['extraCount']." (locdiff=".($loc-$loc0)."; expect 1+4+2+61*eC=".(7+61*$value['extraCount']).")");
					}
				}
				if ($this->_proptype=='ACHR' && in_array($prop_flag,array(0x1c,0x22,0x37))) {
					$this->__add_error("Property $prop_hex [$prop_flag] needs confirmation for rectype ".$this->_proptype);
				}
				if ($this->_proptype=='REFR' && in_array($prop_flag,array(0x36))) {
					$this->__add_error("Property $prop_hex [$prop_flag] needs confirmation for rectype ".$this->_proptype);
				}
				
				if (is_array($value))
					$this->test_prop_irefs($value);
				elseif (isset($pdef['val']))
					$this->test_prop_irefs(array($pdef['val'] => $value));
			}
			
			$this->__set_error_stats(array('prop_last' => sprintf("0x%02x", $prop_flag), 'last_loc' => $loc0));
			$prop_last = $prop_flag;
			
			if ($prop_flag==0x4b) {
				$size = $locmid-$loc0;
				fseek($this->_fpin, $locmid);
				$ex = NULL;
				if ($loc<$this->_change_endloc) {
					$test = $this->extract_single('ushort');
					if ($test!=0) {
						fseek($this->_fpin, $loc);
						if ($iprop<$propnum-1)
							$ex = 'none-mid';
						else
							$ex = 'none-end';
					}
					elseif ($iprop<$propnum-1) {
						$ex = 'add2';
					}
					else {
						fseek($this->_fpin, $loc);
						$ex = 'last';
					}
				}
				else {
					$ex = 'end';
				}
				if ($iprop<$propnum-1) {
					$loctest = ftell($this->_fpin);
					$nextprop = sprintf("0x%02x", $this->extract_single('ubyte'));
					//					if (!isset($this->_propstats['0x4b']['next'][$nextprop]))
					//						$this->_propstats['0x4b']['next'][$nextprop] = 0;
					@ $this->_propstats['0x4b']['next'][$nextprop]++;
					fseek($this->_fpin, $loctest);
				}
				if (isset($ex)) {
					if (!isset($this->_propstats['0x4b'][$ex][$size]))
						$this->_propstats['0x4b'][$ex][$size] = 0;
					$this->_propstats['0x4b'][$ex][$size]++;
					
					if ($size==11) {
						fseek($this->_fpin, $loc0);
						$bytes = $this->extract_single(array('type' => 'bytes', 'len' => 11));
						if ($ex=='add2')
							$this->extract_single('ushort');
						if (!isset($this->_propstats['0x4b']['11data'][$bytes][$ex]))
							$this->_propstats['0x4b']['11data'][$bytes][$ex] = 0;
						$this->_propstats['0x4b']['11data'][$bytes][$ex]++;
					}
				}
				fseek($this->_fpin, $loc);
			}
		}
		$this->__unset_error_stats();
		return $data;
	}
	
	function test_prop_irefs($test) {
		foreach ($test as $tname => $tval) {
			if (is_array($tval)) {
				$this->test_prop_irefs($tval);
				continue;
			}
			if (!isset($tname))
				continue;
			if (substr($tname,-4,4)!='iref')
				continue;
			if ($tval==0)
				continue;
			$rectype = NULL;
			$base_id = NULL;
			// more generally if (!isset($this->_ireflist[$tval])) ?? assuming that player is only missing iref?
			// .... but only if tval<count($this->_ireflist)!!
			// and there are some other missing irefs...
			// could have a list of missingirefs when in debug mode
			if (!isset($this->_ireflist[$tval]) && in_array($tval,array(0x0163,0x0146,0x0142))) {
				$rectype = 'ACHR';
			}
			else {
				$id = $this->convert_iref($tval);
				if ($id===false)
					$id = $this->_parentlist->get_ordid($this->process_raw_formid($tval));
				if ($id===false || !isset($id)) {
					$this->__add_error("Unable to convert iref $tname ($tval 0x".sprintf("%08lx", $tval).") into formid");
					return false;
				}
				else {
					$item = $this->_parentlist->get_item($id);
					if (!is_object($item)) {
						$this->__add_error("Unable to obtain item for iref $tname ($tval 0x".sprintf("%08lx", $tval).")");
					}
					else {
						// I don't know why iref includes 0-values for formid, but there's no
						// point checking rectype in these cases
						if ($item->get('formid')!='0x00000000') {
							$rectype = $item->get('rectype');
							$base_id = $item->get('base_id');
						}
						$item->release();
					}
				}
			}
			if (is_null($rectype))
				continue;
			if (substr($tname,-5,1)!='_')
				continue;
			
			$testtypes = array();
			if (strlen($tname)>10 && substr($tname,-10)=='_REFR_iref')
				$testtype = 'REFR';
			else
				$testtype = substr($tname,0,-5);
			$result = $this->compare_rectypes($rectype, $testtype);
			if (is_null($result))
				continue;
			elseif ($result===false) {
						// not returning false here; safer to keep going
				$this->__add_error("Rectype mismatch for item obtained from iref $tname ($tval 0x".sprintf("%08lx", $tval).").  Got $rectype, expected $testtype");
			}
			elseif (strlen($tname)<10 || substr($tname,-10)!='_REFR_iref') {
				continue;
			}
			
			// test base object for REFRS
			// be more selective about warnings at this point
			// for example, REFR might be a new object, or point to a new object, in which case
			// dereferencing is going to fail
			if (empty($base_id))
				continue;
			$base_item = $this->_parentlist->get_item($base_id);
			if (!is_object($base_item))
				continue;
			if ($base_item->get('formid')!='0x00000000') {
				$basetype = $base_item->get('rectype');
				$result = $this->compare_rectypes($basetype, substr($tname,0,-10));
				if ($result===false) {
					$this->__add_error("Rectype mismatch for base item obtained from iref $tname ($tval 0x".sprintf("%08lx", $tval)."), base_id=$base_id.  Got $basetype, expected ".substr($tname,10));
				}
			}
			$base_item->release();
		}
	}
	
	function compare_rectypes($rectype, $testtype) {
		$extypes =
			// items that could be found in an inventory
			array('ITEM' => array('ALCH', 'AMMO', 'APPA', 'ARMO', 'BOOK', 'CLOT', 'INGR', 'LIGH', 'MISC', 'SGST', 'SLGM', 'WEAP'));
		
		$testtypes = array();
		for ($itest=0; $itest<=strlen($testtype); $itest+=4) {
			$ttype = substr($testtype,$itest,4);
			if (strlen($ttype)!=4)
				break;
			if (strtoupper($ttype)!=$ttype)
				break;
			$testtypes[] = $ttype;
		}
		if (empty($testtypes))
			return NULL;
		
		if (in_array($rectype, $testtypes))
			return true;
		
		foreach ($extypes as $ex => $tlist) {
			if (in_array($ex, $testtypes)) {
				if (in_array($rectype, $tlist))
					return true;
			}
		}
		
		return false;
	}
	
	function read_seen_data($ddef=NULL, $input=NULL) {
		$data = array();
		$availdata = $this->_change_endloc-ftell($this->_fpin);
		if ($availdata<32)
			return $data;
		$data['seen_header'] = $this->extract_single(array('type' => 'bytes', 'len' => 32));
		if ($availdata<36)
			return $data;
		$data['seen_nset'] = $this->extract_single('ushort');
		$data['seen_flags'] = $this->extract_single('ushort');
		$toread = ($data['seen_nset']-1)*34;
		if (($toread+36)>$availdata) {
			unset($data['seen_nset']);
			unset($data['seen_flags']);
			// too much data; rewind 4 bytes and assume seen_data is 32 bytes long
			fseek($this->_fpin, ftell($this->_fpin)-4);
		}
		else {
			fseek($this->_fpin, ftell($this->_fpin) + $toread);
		}
		return $data;
	}
	
	function read_remaining_data($ddef=NULL, $input=NULL) {
		$endloc = $this->_change_endloc;
		fseek($this->_fpin, $endloc);
	}
	
	function read_script_varindex($ddef=NULL, $input=NULL) {
		$data = array();
		$data['varindex'] = $this->extract_single('ushort');
		$data['vartype'] = $this->extract_single('ushort');
		if (!$data['vartype'])
			$data['locvar'] = $this->extract_single('double');
		else
			$data['refvar'] = $this->extract_single('ulong' /*iref*/);
		return;
	}
	
	function read_inventory_record($ddef=NULL, $input=NULL) {
		$data = array();
		
		$startloc = ftell($this->_fpin);
		$iref_orig = $this->extract_single('ulong');
		$data['id'] = $this->convert_iref($iref_orig);
		// -1 values for stackedNum are possible -> indicating an item listed in .esm file's version of inventory
		// has been removed
		$data['count'] = $this->extract_single('ulong');
		$chgnum = $this->extract_single('ulong');
		$this->__set_error_stats(array('iref_orig' => $iref_orig, 'id' => $data['id'], 'chgnum' => $chgnum, 'locInvRecStart'=>$startloc));
		$this->test_prop_irefs(array('iref_ITEM' => $iref_orig));
		
		for ($ichg=0; $ichg<$chgnum; $ichg++) {
			$this->__set_error_stats(array('ichg' => $ichg));
			$chgdata = $this->read_property_structure();
			if ($chgdata===false)
				return false;
			
			if (isset($chgdata['equipped'])) {
				$data['equipped'] = true;
				unset($chgdata['equipped']);
			}
			if (count($chgdata))
				$data['changed'][$ichg] = $chgdata;
		}
		$this->__unset_error_stats();
		
		if ($data['id']!==false)
			return $data;
		else
			return false;
	}
	
	function scan_raw_data($nbytes) {
		for ($n=0; $n<$nbytes; $n++) {
			$tell = ftell($this->_fpin);
			$value = $this->extract_single('ubyte');
			printf("%4d  %8d  %4d  0x%02x", $n, $tell, $value, $value);
			if ($value>32 && $value<125)
				print "     ".chr($value);
			print "\n";
		}
	}
	
	// use for effects, world spaces, etc.
	function read_array($datatype, $skip=NULL, $sizetype=NULL) {
		if (feof($this->_fpin))
			return false;
		if (!isset($sizetype))
			$sizetype = 'ulong';
		$narray = $this->extract_single($sizetype);
		if (!empty($skip)) {
			$len = NULL;
			if (is_array($datatype) && isset($datatype['len']))
				$len = $datatype['len'];
			elseif (isset($this->_datatype[$datatype]['len']))
				$len = $this->_datatype[$datatype]['len'];
			if (isset($len)) {
				$loc = ftell($this->_fpin) + $len*$narray;
				fseek($this->_fpin, $loc);
				return array();
			}
		}
		
		$data = array();
		for ($ival=0; $ival<$narray; $ival++) {
			if (feof($this->_fpin))
				return false;
			$data[$ival] = $this->extract_single($datatype);
		}
		return $data;
	}
	
	function read_created_rec() {
		$rectype = fread($this->_fpin, 4);
		if (!$this->_do_read_rec($size, $flags, $formid)) {
			print "Error reading item $rectype formid=".$formid."\n";
			return;
		}
		$loc = ftell($this->_fpin);
		$nextrec = $loc + $size;
		
		$keep = $this->keep_rectype($rectype);
		if (!$keep) {
			print "Skipping item $rectype formid=".$formid."\n";
			fseek($this->_fpin, $nextrec);
			return;
		}
		
		$newitem = $this->_parentlist->newitem($rectype);
		if (!is_null($formid))
			$this->_parentlist->set_id($formid);
		$this->_set_rec_flags($flags);
		$this->_subset = NULL;
		$this->_xxxxsize = NULL;
		while (ftell($this->_fpin)<$nextrec) {
			$this->read_sub($rectype, 0);
		}
		
		$this->_createdlist[] = $newitem;
		$this->_parentlist->add_created_item($newitem);
		
		if ($this->_sdebug>=10) {
			print "Created item $rectype formid=".$newitem->formid()." name=".$newitem->get('name')."\n";
		}
		return;
	}
	
	function read_rec($nest, $rectype) {
		if (!$this->_do_read_rec($size, $flags, $formid))
			return;
		
		$loc = ftell($this->_fpin);
		$nextrec = $loc + $size;
	}
	
	function keep_rectype($rectype, $formid=NULL) {
		// known created types that are being skipped
		// ALCH (but may need ALCH later to translate tempeffects)
		// SPEL (ditto)
		// AMMO, WEAP (until I have some reason to look at weapons)
		
		if ($this->_sdebug>=10)
			return true;
		
		if ($rectype=='REFR' && isset($formid)) {
			$item = $this->_parentlist->get_item($formid);
			if (is_object($item)) {
				$base_id = $item->get('base_id');
				$base_item = $this->_parentlist->get_item($base_id);
				if (is_object($base_item)) {
					$rectype = $base_item->get('rectype');
					$formid = $base_id;
					$base_item->release();
				}
				$item->release();
			}
		}
			
		// need CLAS for custom class, probably
		if ($rectype=='CLAS')
			return true;
		// need BSGN for PC's birthsign
		if ($rectype=='BSGN')
			return true;
		// ingredients for alc_calc counts
		if ($rectype=='INGR')
			return true;
		// alchemy apparatus for alc_calc initialization
		if ($rectype=='APPA')
			return true;
		// containers to get player's stored inventory
		if ($rectype=='CONT')
			return true;
		// enchantments for custom enchanted items
		if ($rectype=='ENCH')
			return true;
		// spells so player's spell list is complete ... for testing as much as anything right now
		if ($rectype=='SPEL')
			return true;
		// factions also for testing primarily right now
		if ($rectype=='FACT')
			return true;
		// get enchanted items that might be equipped to know about skill/attribute fortifications
		// only MISC items I really want are ones that fortify -- which is only Scales of Pitiless Justice and Skeleton Key
		// .... but actually, Scales directly mod the attributes, so I don't care about them
		if ($rectype=='ARMO' || $rectype=='CLOT') {
			$keep = true;
			if (isset($formid)) {
				$keep = false;
				$item = $this->_parentlist->get_item($formid);
				if (is_object($item)) {
					if ($item->get_isenchanted()) {
						$keep = true;
					}
					$item->release();
				}
			}
			return $keep;
		}
		if ($rectype=='NPC_' && isset($formid) && $formid=='0x00000007')
			return true;
		if ($rectype=='ACHR' && isset($formid) && $formid=='0x00000014')
			return true;
		
		return false;
	}
}