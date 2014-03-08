<?php
require_once 'mwread.inc';
require_once 'mwdata.inc';

/* Extending OBRead because some records use standard OBRead data definitions */
class MWSaveRead extends MWRead {
	protected _readdata = array();
	function __construct($savefile, $parentlist=NULL) {
		if (!isset($parentlist))
			$parentlist = 'OBData';
		parent::__construct($parentlist);
		$this->_savefile = $savefile;
		$this->_nespfile = NULL;
	}
	
	protected function _datadef_setup() {
		parent::_datadef_setup();
		
		$this->_datadef['NPCC'] =
			array('NPDT' => array( 'keep' => 1,
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
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))),
			      'NPCO-XIDX' => array( 'keep' => 1,
			                            'data' => array(array( 'val' => 'item_index', 'type' => 'long'))),
			      // can actually be float or long depending upon type of item...
			      'NPCO-XHLT' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_health' , 'type' => 'long'))),
			      'NPCO-SCRI'	=> array( 'keep' => 0, 'type' => 'edid'),
			      'NPCO-SLSD' => array( 'keep' => -1),
			      'NPCO-SLCS' => array( 'keep' => -1),
			      'NPCO-XCHG' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_charge', 'type' => 'float'))),
			      'NPCO-XSOL' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_soul', 'type' => 'string'))),
		
			      'WIDX' => array( 'keep' => 0,
			                       'data' => array(array( 'val' => 'wear_index', 'type' => 'long'),
			                                       array( 'val' => 'wear_val', 'type' => 'long'))),
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
			     );
		$this->_datadef['CREC'] =
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
			      // appears to be how multiple instances of same creature are distinguished
			      // NAME in CELL has a ulong appended to the end of the edid -> that ulong is this index
			      // in other words, presence of INDX effectively says this is a ACRE/REFR, not a base object
			      // ... not sure how equivalent is done for NPCs
			      'INDX' => array('keep' => 0,
			                      'data' => array( array( 'val' => 'index', 'type' => 'ulong'))),
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))),
			      'NPCO-XIDX' => array( 'keep' => 1,
			                            'data' => array(array( 'val' => 'item_index', 'type' => 'long'))),
			      // can actually be float or long depending upon type of item...
			      'NPCO-XHLT' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_health' , 'type' => 'long'))),
			      'NPCO-SCRI'	=> array( 'keep' => 0, 'type' => 'edid'),
			      'NPCO-SLSD' => array( 'keep' => -1),
			      'NPCO-SLCS' => array( 'keep' => -1),
			      'NPCO-XCHG' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_charge', 'type' => 'float'))),
			      'NPCO-XSOL' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_soul', 'type' => 'string'))),
			      
			      'WIDX' => array( 'keep' => 0,
			                       'data' => array(array( 'val' => 'wear_index', 'type' => 'long'),
			                                       array( 'val' => 'wear_val', 'type' => 'long'))),
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
			     );
		
		$this->_datadef['CNTC'] =
			array('CNDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'))),
			      'FLAG' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array( 'organic', 'respawn')))),
			      // no idea what this is....
			      'INDX' => array('keep' => 0,
			                      'data' => array( array( 'val' => 'index', 'type' => 'ulong'))),
			      'NPCO' => array( 'keep' => 1,
			                       'multi' => 'NPCO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_count', 'type' => 'long'),
			                                        array( 'val' => 'item_id', 'type' => 'edid'))),
			      'NPCO-XIDX' => array( 'keep' => 1,
			                            'data' => array(array( 'val' => 'item_index', 'type' => 'long'))),
			      // can actually be float or long depending upon type of item...
			      'NPCO-XHLT' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_health' , 'type' => 'long'))),
			      'NPCO-SCRI'	=> array( 'keep' => 0, 'type' => 'edid'),
			      'NPCO-SLSD' => array( 'keep' => -1),
			      'NPCO-SLCS' => array( 'keep' => -1),
			      'NPCO-XCHG' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_charge', 'type' => 'float'))),
			      'NPCO-XSOL' => array( 'keep' => 0,
			                            'data' => array(array( 'val' => 'item_soul', 'type' => 'string')))
			     );
		$this->_datadef['GAME'] =
			array( 'GMDT' => array( 'keep' => -1));
		$this->_datadef['PCDT'] =
			/* there's more data than this in PCDT, but don't know what the rest is */
			array( 'PNAM' => array( 'keep' => 0,
			                        'data' => array(array( 'val' => NULL, 'type' => 'ulong'),
			                                        array( 'val' => 'major_nlevel', 'type' => 'ulong'),
			                                        array( 'vallist' => 'skilllist', 'valsuffix' => '_exp', 'type' => 'float'),
			                                        array( 'vallist' => 'attriblist', 'valsuffix' => '_nlevel', 'type' => 'ubyte'))),
			       'SNAM' => array( 'keep' => -1), /* size 360, floats in repeating pattern of 3 numbers */
			       'KNAM' => array( 'keep' => -1), /* size 400, pattern: (36 0x0, 4 0xff)... repeats 10 times */
			       /* data of following not checked */
			       'FNAM' => array( 'keep' => -1), /* size 44, repeats; not standard name subrecord */
			       'BNAM' => array( 'keep' => -1), /* size 14 */
			       'NAM0' => array( 'keep' => -1), /* size 22 */
			       'ENAM' => array( 'keep' => -1), /* size 8 */
			       'LNAM' => array( 'keep' => -1), /* size 8 */
			       'DNAM' => array( 'keep' => -1) /* lots of entries, each a string: dialogue topic that' been read? */
			     );
		$this->_datadef['STLN'] = array( 'keep' => -1);
		$this->_datadef['SPLM'] =
			array('NAM0' => array( 'keep' => -1),
			      'XNAM' => array( 'keep' => -1),
			      'NPDT' => array( 'keep' => -1),
			      'SPDT' => array( 'keep' => -1)
			     );
		$this->_datadef['KLST'] = array( 'keep' => -1); /* Daveh speculates this is kill stats */
		$this->_datadef['REFR'] =
			array('CHRD' => array( 'keep' => 0,
			                       'data' => array(array( 'vallist' => 'skilllist', 'valsuffix' => array('_curr', '_raw'), 'type' => 'ulong'))),
			      'ACDT' => array( 'keep' => 0,
			                       'data' => array(array( 'val' => NULL, 'type' => 'bytes', 'len' => 40 /*ulongs?*/),
			                                       array( 'val' => 'health_curr', 'type' => 'float'),
			                                       array( 'val' => 'health_raw', 'type' => 'float'),
			                                       array( 'val' => 'fatigue_curr', 'type' => 'float'),
			                                       array( 'val' => 'fatigue_raw', 'type' => 'float'),
			                                       array( 'val' => 'magicka_curr', 'type' => 'float'),
			                                       array( 'val' => 'magicka_raw', 'type' => 'float'),
			                                       array( 'val' => 'maxmagicka_curr', 'type' => 'float'),
			                                       array( 'val' => 'maxmagicka_raw', 'type' => 'float'),
			                                       array( 'val' => 'encumbrance_curr', 'type' => 'float'),
			                                       array( 'val' => 'encumbrance_raw', 'type' => 'float'),
			                                       array( 'valllist' => 'attriblist', 'valsuffix' => array('_curr', '_raw'), 'type' => 'float'),
			                                       array( 'val' => NULL, 'type' => 'long'),
			                                       array( 'val' => NULL, 'type' => 'long'),
			                                       array( 'val' => 'resist_magicka', 'type' => 'long'), /* magicka/fire could be flipped */
			                                       array( 'val' => 'resist_fire', 'type' => 'long'),
			                                       array( 'val' => 'resist_frost', 'type' => 'long'), /* frost/shock could be flipped */
			                                       array( 'val' => 'resist_shock', 'type' => 'long'),
			                                       array( 'val' => 'resist_disease', 'type' => 'long'))),
			      /* other values probably included in ACDT's longs:
			      disposition, flying, pccrimelevel, resistpoison, silence, superjump, castpenalty,
			      invisible, pcvisionbonus, swimspeed, alarm, attackbonus, chameleon, reputation,
			      scale?, blindness, defendbonus, fight, hello, resistblight, resistnormalweapons,
			      waterwalking, armorbonus, flee, paralysis, resistcorprus, resistparalysis
			      also: find out whether values are just from abilities or all current effects
			      */
			      'ACSC' => array( 'keep' => -1), /* size 112 */
			      'ACSL' => array( 'keep' => -1), /* size 112 */
			      // subrecords I haven't looked at yet
			      'FRMR' => array( 'keep' => -1), /* size 4 */
			      'ACTN' => array( 'keep' => -1), /* size 4 */
			      'STPR' => array( 'keep' => -1), /* size 24 */
			      'WNAM' => array( 'keep' => -1), /* size 23 */
			      'ND3D' => array( 'keep' => -1), /* size 1 */
			      'DATA' => array( 'keep' => -1), /* size 24 */
			      
			      
			           );
		
	}
	
	protected function _datadef_check() {
		// need to figure out a way to just do the vallist -> val and other basic processing
		parent::_datadef_check(array('debug' => false, 'cleanup_only' => true));
	}
	
	/* add a way in csread to customize what's done after a set of data has been read
	*/
	
	function read() {
		//$this->request_data('all');
		// BOOK should be added if I want list of skill books that have been read
		// really, the only NPC_ I want is player, and only NPCC is PlayerSaveGame
		// CNTCs are to get inventories... but need to figure out how to know which containers have player contents
		// ... skip organic=1,respawn=1
		// ... ownership is under REFR_Record (but player doesn't own any items)
		// ALCHs to get custom potions (to properly decode inventories)
		$this->request_data(array('TES3', 'GLOB', 'NPC_', 'NPCC', 'CNTC', 'ALCH', 'REFR', 'PCDT'));
		//		$this->_set_datadef_debug();
		$this->_doprint = true;
		$fullfile = "../../charstats/data/".$this->_savefile;
		if (!($this->_fpin = fopen($fullfile, "rb"))) {
			print "Unable to open file $file\n";
			exit();
		}
		$this->_savefilesize = filesize($fullfile);
		
		$this->_grouptype = NULL;
		$this->_ngroup = 0;
		$this->_groupordid = array();
		$this->_groupid = array();
		while (!feof($this->_fpin)) {
			$this->read_next();
		}
	}
	
		// Override by derived classes for any extra processing necessary to handle a sub-record
	protected function _do_special_sub($rectype, $type, $data) {
		if ($rectype=='NPC_' && $type=='FLAG') {
			if ($this->_parentlist->get_active('name')=='Nephele') {
				$item = $this->_parentlist->get_active_item();
				$item->dump_data();
			}
			return false;
		}
		
		if ($rectype!='PCDT' && $rectype!='REFR')
			return false;
		if ($type!='PNAM' && $type!='SNAM' && $type!='KNAM' && $type!='ACDT' && $type!='ACSC' && $type!='ACSL' && $type!='CHRD')
			return false;
		
		print "$rectype $type ".strlen($data)."\n";
		$vals = array();
		for ($i=0; $i<strlen($data); $i+=4) {
			$k = $i/4;
			$sub = substr($data,$i,4);
			print "  ".$k."    ";
			print $this->extract_single('float', $sub)."     ";
			print $this->extract_single('ulong', $sub)."     ";
			for ($j=0; $j<4; $j++)
				print $this->extract_single('ubyte', substr($sub,$j,1))."  ";
			$vals[$k] = $this->extract_single('float', $sub);
			print "\n";
		}
		
		if ($type=='SNAM') {
			for ($k=3; $k<count($vals); $k++) {
				print "      $k ".($vals[$k]-$vals[$k-3])."\n";
			}
		}
		
		return false;
	}
	
	protected function _record_done($rectype) {
		$item = $this->_parentlist->get_active_item();
		$id = $item->get('id');
		// set_modids
		if ($rectype=='TES3') {
			$this->_modids = array();
			foreach ($item->indices('master_file') as $i) {
				$pluginname = strtolower($item->get('master_file', $i));
			// can't just use array_search because cases can mismatch (.ESP vs .esp)
				foreach ($this->_fileofficial as $j => $oname) {
					if ($pluginname==strtolower($oname)) {
						$this->_modids[$i] = $j;
						break;
					}
				}
				if (!isset($this->_modids[$i])) {
					print "Unable to recognize plugin filename ".$name."\n";
				}
			}
			$this->_parentlist->inactive(false);
			return;
		}
		
		elseif ($rectype=='GLOB') {
			$keepvals = array('GameHour', 'Month', 'Day', 'Year', 'PCHasCrimeGold', 'CharGenState', 'PCVampire', 'PCRace', 'VampClan', 'DaysPassed', 'PCWerewolf', 'PCKnownWerewolf');
			if (in_array($id, $keepvals))
				$this->_readdata[$id] = $item->get('FLTV');
			$this->_parentlist->inactive(false);
			return;
		}
		
		elseif ($rectype=='NPC_') {
			if ($id!='player') {
				$this->_parentlist->inactive(false);
				return;
			}
			$playeritem = $this->_parentlist->newitem('NPC_');
			
		}
		
		elseif ($rectype=='ALCH') {
			$this->_parentlist->add_created_item($item);
			$this->_parentlist->inactive(true);
			return;
		}
		
		$this->_parentlist->inactive(false);
		return;
	}
}