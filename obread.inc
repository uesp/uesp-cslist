<?php
require_once 'csread.inc';

class OBRead extends CSRead {
	protected $_files = array("Oblivion.noSI.esm.mod",
	                           "Oblivion.esm.mod",
	                           "DLCThievesDen.esp.mod",
	                           "DLCVileLair.esp.mod",
	                           "DLCFrostcrag.esp.mod",
	                           "DLCHorseArmor.esp.mod",
	                           "Knights.esp.mod",
	                           "DLCMehrunesRazor.esp.mod",
	                           "DLCOrrery.esp.mod",
	                           "DLCSpellTomes.esp.mod",
	                           "DLCBattlehornCastle.esp.mod");
	// NB DLCShiveringIsles.esp does not officially have any FormID items
	protected $_fileofficial = array("Oblivion.esm",
	                                 "DLCShiveringIsles.esp",
	                                 "DLCThievesDen.esp",
	                                 "DLCVileLair.esp",
	                                 "DLCFrostcrag.esp",
	                                 "DLCHorseArmor.esp",
	                                 "Knights.esp",
	                                 "DLCMehrunesRazor.esp",
	                                 "DLCOrrery.esp",
	                                 "DLCSpellTomes.esp",
	                                 "DLCBattlehornCastle.esp");
	protected $_filenamespace = array("Oblivion",
	                                  "Shivering",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion",
	                                  "Oblivion");
	protected $_filemodname = array("",
	                                "",
	                                "Dunbarrow Cove",
	                                "Deepscorn Hollow",
	                                "Frostcrag Spire",
	                                "Horse Armor",
	                                "Knights of the Nine",
	                                "Mehrunes Razor",
	                                "Repairing the Orrery",
	                                "Spell Tomes", 
	                                "Battlehorn Castle");
	protected $_filemodabbrev = array('OB', 'SI', 'DC', 'DH', 'FS', 'HA', 'KN', 'MR', 'OR', 'ST', 'BH');
	
	// Should I manually add a record at 0x00000014 for ACHR to player's NPC_?
	// One simple way:
	// update AllItems set edid='PlayerRef', rectype='ACHR' where formid='0x00000014'
	// insert into ACHR_Record set ordid=xx, base_id=yy
	
	protected function _datadef_setup() {
		if (!empty($this->_datadef))
			return;
		$this->_parentlist->set_lookup('actorlist', array('Strength', 'Intelligence', 'Willpower', 'Agility', 'Speed', 'Endurance', 'Personality', 'Luck', 'Health', 'Magicka', 'Fatigue', 'Encumbrance', 'Armorer', 'Athletics', 'Blade', 'Block', 'Blunt', 'Hand to Hand', 'Heavy Armor', 'Alchemy', 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Mysticism', 'Restoration', 'Acrobatics', 'Light Armor', 'Marksman', 'Mercantile', 'Security', 'Sneak', 'Speechcraft'));
		$this->_parentlist->set_lookup('skilllist', array('Armorer', 'Athletics', 'Blade', 'Block', 'Blunt', 'Hand to Hand', 'Heavy Armor', 'Alchemy', 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Mysticism', 'Restoration', 'Acrobatics', 'Light Armor', 'Marksman', 'Mercantile', 'Security', 'Sneak', 'Speechcraft'));
		$this->_parentlist->set_lookup('attriblist', array('Strength', 'Intelligence', 'Willpower', 'Agility', 'Speed', 'Endurance', 'Personality', 'Luck'));
		$this->_parentlist->set_lookup('speclist', array('Combat', 'Magic', 'Stealth'));
		$this->_parentlist->set_lookup('effect_range', array('Self', 'Touch', 'Target'));
		$this->_parentlist->set_lookup('magic_school', array( 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Mysticism', 'Restoration'));
		$this->_parentlist->set_lookup('soullist', array( 'none', 'Petty', 'Lesser', 'Common', 'Greater', 'Grand'));
		$this->_parentlist->set_lookup('APPA_type', array('Mortar and Pestle', 'Alembic', 'Calcinator', 'Retort'));
		$this->_parentlist->set_lookup('DIAL_type', array('Topic', 'Conversation', 'Combat', 'Persuasion', 'Detection', 'Service', 'Miscellaneous'));
		$this->_parentlist->set_lookup('ENCH_type', array('Scroll', 'Staff', 'Weapon', 'Apparel'));
		$this->_parentlist->set_lookup('INFO_emotion', array('Neutral', 'Anger', 'Disgust', 'Fear', 'Sad', 'Happy', 'Surprise'));
		$this->_parentlist->set_lookup('PACK_type', array('Find', 'Follow', 'Escort', 'Eat', 'Sleep', 'Wander', 'Travel', 'Accompany', 'Use item at', 'Ambush', 'Flee not combat', 'Cast magic'));
		$this->_parentlist->set_lookup('PACK_day', array('Sundas', 'Morndas', 'Tirdas', 'Middas', 'Turdas', 'Fredas', 'Loredas', 'Morndas-Fredas', 'Loredas/Sundas', 'Morndas/Middas/Fredas', 'Tirdas/Turdas', 255=>'Any'));
		$this->_parentlist->set_lookup('PLDT_type', array('Near reference', 'In cell', 'Near current location', 'Near editor location', 'Object ID', 'Object type'));
		$this->_parentlist->set_lookup('PTDT_type', array('Specific reference', 'Object ID', 'Object type'));
		$this->_parentlist->set_lookup('TNAM_type', array('None', 'Camp', 'Cave', 'City', 'Elven Ruin', 'Fort Ruin', 'Mine', 'Landmark', 'Tavern', 'Settlement', 'Daedric Shrine', 'Oblivion Gate', 'Unknown'));
		$this->_parentlist->set_lookup('SPEL_type', array('Spell', 'Disease', 'Power', 'Lesser Power', 'Ability', 'Poison'));
		$this->_parentlist->set_lookup('spelllevels', array('Novice', 'Apprentice', 'Journeyman', 'Expert', 'Master'));
		$this->_parentlist->set_lookup('WEAP_type', array('Blade One Hand', 'Blade Two Hand', 'Blunt One Hand', 'Blunt Two Hand', 'Staff', 'Bow'));
		$this->_parentlist->set_lookup('ARMO_type', array( "Helmet", "Cuirass", "Greaves", "Boots", "Gauntlets", "Shield", "Other"));
		$this->_parentlist->set_lookup('CLOT_type', array( 'Hood', 'Pants/Skirt', 'Shoes', 'Shirt', 'Gloves', 'Ring', 'Amulet', 'Other'));
		$this->_parentlist->set_lookup('CTDA_function', array(
			153 => 'CanHaveFlames',
			127 => 'CanPayCrimeGold',
			14 => 'GetActorValue',
			61 => 'GetAlarmed',
			190 => 'GetAmountSoldStolen',
			8 => 'GetAngle',
			81 => 'GetArmorRating',
			274 => 'GetArmorRatingUpperBody',
			63 => 'GetAttacked',
			264 => 'GetBarterGold',
			277 => 'GetBaseActorValue',
			229 => 'GetClassDefaultMatch',
			41 => 'GetClothingValue',
			122 => 'GetCrime',
			116 => 'GetCrimeGold',
			110 => 'GetCurrentAIPackage',
			143 => 'GetCurrentAIProcedure',
			18 => 'GetCurrentTime',
			148 => 'GetCurrentWeatherPercent',
			170 => 'GetDayOfWeek',
			46 => 'GetDead',
			84 => 'GetDeadCount',
			203 => 'GetDestroyed',
			45 => 'GetDetected',
			180 => 'GetDetectionLevel',
			35 => 'GetDisabled',
			39 => 'GetDisease',
			76 => 'GetDisposition',
			1 => 'GetDistance',
			215 => 'GetDoorDefaultOpen',
			182 => 'GetEquipped',
			73 => 'GetFactionRank',
			60 => 'GetFactionRankDifference',
			128 => 'GetFatiguePercentage',
			288 => 'GetFriendHit',
			160 => 'GetFurnitureMarkerID',
			74 => 'GetGlobalValue',
			48 => 'GetGold',
			99 => 'GetHeadingAngle',
			318 => 'GetIdleDoneOnce',
			338 => 'GetIgnoreFriendlyHits',
			67 => 'GetInCell',
			230 => 'GetInCellParam',
			71 => 'GetInFaction',
			32 => 'GetInSameCell',
			305 => 'GetInvestmentGold',
			310 => 'GetInWorldspace',
			91 => 'GetIsAlerted',
			68 => 'GetIsClass',
			228 => 'GetIsClassDefault',
			64 => 'GetIsCreature',
			161 => 'GetIsCurrentPackage',
			149 => 'GetIsCurrentWeather',
			237 => 'GetIsGhost',
			72 => 'GetIsID',
			254 => 'GetIsPlayableRace',
			224 => 'GetIsPlayerBirthsign',
			69 => 'GetIsRace',
			136 => 'GetIsReference',
			70 => 'GetIsSex',
			246 => 'GetIsUsedItem',
			247 => 'GetIsUsedItemType',
			47 => 'GetItemCount',
			107 => 'GetKnockedState',
			80 => 'GetLevel',
			27 => 'GetLineOfSight',
			5 => 'GetLocked',
			65 => 'GetLockLevel',
			320 => 'GetNoRumors',
			255 => 'GetOffersServicesNow',
			157 => 'GetOpenState',
			193 => 'GetPCExpelled',
			199 => 'GetPCFactionAttack',
			195 => 'GetPCFactionMurder',
			197 => 'GetPCFactionSteal',
			201 => 'GetPCFactionSubmitAuthority',
			249 => 'GetPCFame',
			132 => 'GetPCInFaction',
			251 => 'GetPCInfamy',
			129 => 'GetPCIsClass',
			130 => 'GetPCIsRace',
			131 => 'GetPCIsSex',
			312 => 'GetPCMiscStat',
			225 => 'GetPersuasionNumber',
			98 => 'GetPlayerControlsDisabled',
			365 => 'GetPlayerInSEWorld',
			362 => 'GetPlayerHasLastRiddenHorse',
			6 => 'GetPos',
			56 => 'GetQuestRunning',
			79 => 'GetQuestVariable',
			77 => 'GetRandomPercent',
			244 => 'GetRestrained',
			24 => 'GetScale',
			53 => 'GetScriptVariable',
			12 => 'GetSecondsPassed',
			66 => 'GetShouldAttack',
			159 => 'GetSitting',
			49 => 'GetSleeping',
			58 => 'GetStage',
			59 => 'GetStageDone',
			11 => 'GetStartingAngle',
			10 => 'GetStartingPos',
			50 => 'GetTalkedToPC',
			172 => 'GetTalkedToPCParam',
			361 => 'GetTimeDead',
			315 => 'GetTotalPersuasionNumber',
			144 => 'GetTrespassWarningLevel',
			242 => 'GetUnconscious',
			259 => 'GetUsedItemActivate',
			258 => 'GetUsedItemLevel',
			40 => 'GetVampire',
			142 => 'GetWalkSpeed',
			108 => 'GetWeaponAnimType',
			109 => 'GetWeaponSkillType',
			147 => 'GetWindSpeed',
			154 => 'HasFlames',
			214 => 'HasMagicEffect',
			227 => 'HasVampireFed',
			353 => 'IsActor',
			314 => 'IsActorAVictim',
			313 => 'IsActorEvil',
			306 => 'IsActorUsingATorch',
			280 => 'IsCellOwner',
			267 => 'IsCloudy',
			150 => 'IsContinuingPackagePCNear',
			163 => 'IsCurrentFurnitureObj',
			162 => 'IsCurrentFurnitureRef',
			354 => 'IsEssential',
			106 => 'IsFacingUp',
			125 => 'IsGuard',
			282 => 'IsHorseStolen',
			112 => 'IsIdlePlaying',
			289 => 'IsInCombat',
			332 => 'IsInDangerousWater',
			300 => 'IsInInterior',
			146 => 'IsInMyOwnedCell',
			285 => 'IsLeftUp',
			278 => 'IsOwner',
			176 => 'IsPCAMurderer',
			175 => 'IsPCSleeping',
			171 => 'IsPlayerInJail',
			358 => 'IsPlayerMovingIntoNewSpace',
			339 => 'IsPlayersLastRiddenHorse',
			266 => 'IsPleasant',
			62 => 'IsRaining',
			327 => 'IsRidingHorse',
			287 => 'IsRunning',
			103 => 'IsShieldOut',
			286 => 'IsSneaking',
			75 => 'IsSnowing',
			223 => 'IsSpellTarget',
			185 => 'IsSwimming',
			141 => 'IsTalking',
			265 => 'IsTimePassing',
			102 => 'IsTorchOut',
			145 => 'IsTrespassing',
			329 => 'IsTurnArrest',
			111 => 'IsWaiting',
			101 => 'IsWeaponOut',
			309 => 'IsXBox',
			104 => 'IsYielding',
			36 => 'MenuMode',
			42 => 'SameFaction',
			133 => 'SameFactionAsPC',
			43 => 'SameRace',
			134 => 'SameRaceAsPC',
			44 => 'SameSex',
			135 => 'SameSexAsPC',
			323 => 'WhichServiceMenu',
			));
		
		$this->_parentlist->set_lookup('Worldspace', array('Test', 'Tamriel', 'Shivering Isles', 'Frostfire Glade', 'Pale Pass', 'Oblivion', 'Paradise', 'Other'));
		$this->_parentlist->set_lookup('Worldtype', array('Test', 'Main', 'City', 'Submap', 'Other'));
		
		// Note that the way datadef_alias is implemented, this is only effective if all CTDT subrecords
		// appear in records where CTDA subrecords also appear -- which is true currently for all OB cases
		$this->_datadef_alias['CTDT'] = 'CTDA';
		
		$this->_datadef['any'] = 
			array('EDID' => array( 'keep' => 2, 'type' => 'string', 'desc' => 'Editor ID'),
			      'FULL' => array( 'keep' => 2, 'type' => 'string', 'val' => 'name', 'desc' => 'Full Name'),
			      'SCRI' => array( 'keep' => 0, 'type' => 'formid', 'desc' => 'ScriptID'),
			      'MODL' => array( 'keep' => 0, 'type' => 'string', 'val' => 'model', 'desc' => 'Model Path'),
			      'MOD2' => array( 'keep' => 0, 'type' => 'string', 'val' => 'model_ground', 'desc' => 'Model Path 2'),
			      'MOD3' => array( 'keep' => 0, 'type' => 'string', 'val' => 'model_F', 'desc' => 'Model Path 3'),
			      'MOD4' => array( 'keep' => 0, 'type' => 'string', 'val' => 'model_F_ground', 'desc' => 'Model Path 4'),
			      'MODB' => array( 'keep' => -1),
			      'MO2B' => array( 'keep' => -1),
			      'MO3B' => array( 'keep' => -1),
			      'MO4B' => array( 'keep' => -1),
			      'MODT' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data'),
			      'MO2T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 2'),
			      'MO3T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 3'),
			      'MO4T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 4'),
			      'ICON' => array( 'keep' => 0, 'type' => 'string', 'val' => 'icon', 'desc' => 'Icon Path'),
			      'ICO2' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Icon Path 2'),
			      'ANAM' => array ('keep' => 1, 'val' => 'enchantcharge', 'type' => 'ushort'),
			      'ENAM' => array( 'keep' => 1, 'val' => 'enchant_id', 'type' => 'formid', 'multi' => 'ENAM'), //(except RACE)
			      'SNAM' => array( 'keep' => 0, 'type' => 'formid')
			     );
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
			      'XHRS' => array( 'keep' => 0, 'val' => 'horse_id', 'type' => 'formid'), //horse
			      'XMRC' => array( 'keep' => 0, 'val' => 'merccont_id', 'type' => 'formid'), //merchant container
			      'XRGD' => array( 'keep' => -1 ),
			      'XSCL' => array( 'keep' => 0, 'val' => 'scale', 'type' => 'float'));
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
			      'XGLB' => array( 'keep' => 0, 'type' => 'formid'), //global variable
			      'XOWN' => array( 'keep' => 0, 'val' => 'owner_id', 'type' => 'formid'), //owner, could be NPC or faction
			      'XRGD' => array( 'keep' => -1 ),
			      'XRNK' => array( 'keep' => 0, 'val' => 'owner_rank', 'type' => 'ulong'), //faction rank
			      'XSCL' => array( 'keep' => 0, 'val' => 'scale', 'type' => 'float'));
		/*$this->_datadef['ACTI'] =
			array();*/
		$this->_datadef['ALCH'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'))),
			      'ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'type' => 'ulong', 'flag' => array( '!autocalc', 'food')))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'mgefstr',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'effectb_id', 'type' => 'mgefstr'),
			                                             array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong', 'lookup' => 'actorlist'))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1,
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		$this->_datadef['AMMO'] =
			array('DATA' => array ('keep' => 1,
			                       'data' => array( array( 'val' => 'speed', 'type' => 'float'),
			                                        array( 'type' => 'ulong', 'flag' => array('resistnw')),
			                                        array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'damage', 'type' => 'ushort'))));
		/*$this->_datadef['ANIO'] = array('DATA' => array( 'keep' => -1));*/
		$this->_datadef['APPA'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'byte', 'lookup' => 'APPA_type'),
			                                        array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'quality', 'type' => 'float'))));
		$this->_datadef['ARMO'] =
			array('BMDT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail')),
			                                        array( 'type' => 'ushort', 'flag' => array ('hide_ring', 'hide_amulet', NULL, NULL, NULL, NULL, 'unplayable', 'heavy')),
			                                        array( 'val' => 'type', 'type'=> 'byte', 'lookup' => 'ARMO_type', 'extra' => 1))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'rating', 'type' => 'ushort', 'factor' => 0.01),
			                                        array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'health', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'skill', 'type' => 'ubyte', 'lookup' => 'skilllist', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))));
		$this->_datadef['BOOK'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array( 'scroll', 'cannot_take')),
			                                        array( 'val' => 'skill', 'type' => 'byte', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		$this->_datadef['BSGN'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'SPLO' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'multi' => 'SPLO',
			                       'val' => 'spell_id'));
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
			                                        array( 'val' => 'cell_y', 'type' => 'long'))));
		$this->_datadef['CLAS'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'DATA' => array( 'keep' => 1,
			                       'len1' => 48,
			                       'data1' => array( array( 'val' => 'att1', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'att2', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'spec', 'type' => 'ulong', 'lookup' => 'speclist'),
			                                         array( 'val' => 'skl1', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl2', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl3', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl4', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl5', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl6', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl7', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'type' => 'ulong', 'flag' => array('playable', 'guard'))),
			                       'len2' => 50,
			                       'data2' => array( array( 'val' => 'att1', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'att2', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'spec', 'type' => 'ulong', 'lookup' => 'speclist'),
			                                         array( 'val' => 'skl1', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl2', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl3', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl4', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl5', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl6', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'val' => 'skl7', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                         array( 'type' => 'ulong', 'flag' => array('playable', 'guard')),
			                                         array( 'val' => 'trainskl', 'type' => 'ubyte', 'lookup' => 'skilllist'),
			                                         array( 'val' => 'trainlvl', 'type' => 'ubyte'))));
		/*$this->_datadef['CLMT'] =
			array('WLST' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1),
			      'GNAM' => array( 'keep' => -1),
			      'TNAM' => array( 'keep' => -1));*/
		$this->_datadef['CLOT'] =
			array('BMDT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('hair', 'head', 'upper_body', 'lower_body', 'hand', 'foot', 'right_ring', 'left_ring', 'amulet', 'weapon', 'back_weapon', 'side_weapon', 'quiver', 'shield', 'torch', 'tail')),
			                                        array( 'type' => 'ushort', 'flag' => array ('hide_ring', 'hide_amulet', NULL, NULL, NULL, NULL, 'unplayable')),
			                                        array( 'val' => 'type', 'type'=> 'byte', 'lookup' => 'CLOT_type', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		$this->_datadef['CONT'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array(NULL, 'respawn')),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'QNAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'CNTO' => array( 'keep' => 1,
			                       'multi' => 'CNTO',
			                       'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'long'))));
		$this->_datadef['CREA'] =
			array('NIFZ' => array( 'keep' => -1),
			      'ACBS' => array( 'keep' => 1,
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
			                                        array( 'val' => 'faction_rank', 'type' => 'byte'))),
			      'INAM' => array( 'keep' => 1,
			                       'val' => 'deathitem_id',
			                       'type' => 'formid'),
			      'RNAM' => array( 'keep' => -1),
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
			                                        array( 'val' => 'responsibility', 'type' => 'ubyte'))),
			      'PKID' => array( 'keep' => 0,
			                       'val' => 'package_id',
			                       'multi' => 'PKID',
			                       'type' => 'formid'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => NULL, 'type' => 'ubyte'),
			                                        array( 'val' => 'combat', 'type' => 'ubyte'),
			                                        array( 'val' => 'magic', 'type' => 'ubyte'),
			                                        array( 'val' => 'stealth', 'type' => 'ubyte'),
			                                        array( 'val' => NULL, 'type' => 'ubyte'),
			                                        array( 'val' => 'soul', 'type' => 'ubyte', 'lookup' => 'soullist'),
			                                        array( 'val' => 'health_raw', 'type' => 'ushort'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'damage', 'type' => 'ushort'))),
			      'ZNAM' => array( 'keep' => -1),
			      'CSCR' => array( 'keep' => -1),
			      'CSDC' => array( 'keep' => -1),
			      'CSDI' => array( 'keep' => -1),
			      'CSDT' => array( 'keep' => -1),
			      'BNAM' => array( 'keep' => 0,
			                       'type' => 'float'),
			      'TNAM' => array( 'keep' => -1),
			      'WNAM' => array( 'keep' => -1),
			      'NAM0' => array( 'keep' => -1),
			      'NAM1' => array( 'keep' => -1),
			      'KFFZ' => array( 'keep' => -1));
		/*$this->_datadef['CSTY'] =
			array();*/
		$this->_datadef['DIAL'] =
			array('QSTI' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'multi' => 'QSTI'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ubyte', 'lookup' => 'DIAL_type'))),
			     );
		$this->_datadef['DOOR'] =
			array('ANAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'BNAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'FNAM' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('oblivion_gate', 'autodoor', 'hidden', 'minimal_use')))),
			      'TNAM' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'multi' => 'TNAM'));
		/*$this->_datadef['EFSH'] =
			array();*/
		// Enchantment
		$this->_datadef['ENCH'] =
			array('ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ulong', 'lookup' => 'ENCH_type'),
			                                        // treating first value as gold value added to item because of enchantment
			                                        // whereas second is the magicka cost of the using the enchantment
			                                        // Really based upon a single example (Bands of the Chosen), where
			                                        // enchantvalue=0 is consistent with total item value=0
			                                        array( 'val' => 'enchantvalue_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'enchantcost_raw', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc')))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'mgefstr',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'effectb_id', 'type' => 'mgefstr'),
			                                             array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong', 'lookup' => 'actorlist'))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		/*$this->_datadef['EYES'] =
			array('DATA' => array('keep' => -1));*/
		$this->_datadef['FACT'] =
			array('CNAM' => array('keep' => 1,
			                      'val' => 'crimegold',
			                      'type' => 'float'),
			      // not sure about the ushorts here: info comes from WryeBash, which identifies the param as an int
			      'XNAM' => array('keep' => 1,
			                      'multi' => 'XNAM',
			                      'data' => array( array( 'val' => 'opp_faction_id', 'type' => 'formid'),
			                                       array( 'val' => 'opp_modifier', 'type' => 'short'))),
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
			      'INAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank_icon',
			                       'type' => 'string'));
		$this->_datadef['FLOR'] =
			array('PFIG' => array( 'keep' => 1,
			                       'val' => 'ingr_id',
			                       'type' => 'formid'),
			      'PFPC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'spring', 'type' => 'ubyte'),
			                                        array( 'val' => 'summer', 'type' => 'ubyte'),
			                                        array( 'val' => 'fall', 'type' => 'ubyte'),
			                                        array( 'val' => 'winter', 'type' => 'ubyte'))));
		$this->_datadef['FURN'] =
			array('MNAM' => array( 'keep' => -1));
		// Global variable
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
		/*$this->_datadef['GRAS'] =
			array('DATA' => array( 'keep' => -1));*/
		/*$this->_datadef['HAIR'] =
			array('DATA' => array( 'keep' => -1));*/
		/*$this->_datadef['IDLE'] =
			array();*/
		// why isn't KotN dialoge option about "Legends say...." showing up in database?
		$this->_datadef['INFO'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ubyte', 'lookup' => 'DIAL_type'),
			                                        array( 'type' => 'ushort', 'flag' => array('goodbye', 'random', 'say_once', NULL, 'refusal', 'random_end', 'rumors')),
			                                        array( 'val' => 'group_id', 'type' => 'formid', 'extra' => 1))),
			      'QSTI' => array( 'keep' => 1,
			                       'type' => 'formid'),
			      'PNAM' => array( 'keep' => -1,
			                       'type' => 'formid'),  // not a real formID but an INFO specific value
			      'TRDT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'emotion_type', 'type' => 'ulong', 'lookup' => 'INFO_emotion'),
			                                        array( 'val' => 'emotion_value', 'type' => 'ulong'))),
			      'NAM1' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'NAM2' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ulong', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => 'cond_param3', 'type' => 'long'))),
			      'TCLT' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'SCHR' => array( 'keep' => -1),
			      'SCDA' => array( 'keep' => -1),
			      'SCTX' => array( 'keep' => -1),
			      'SCRO' => array( 'keep' => -1),
			      'CTDT' => array( 'keep' => -1));
		$this->_datadef['INGR'] =
			array('DATA' => array( 'keep' => 1,
			                       'val' => 'weight',
			                       'type' => 'float'),
			      // 'value' is always final value, regardless of effects/autocalc
			      'ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array( '!autocalc', 'food')))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'mgefstr',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'effectb_id', 'type' => 'mgefstr'),
			                                             array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong', 'lookup' => 'actorlist'))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		$this->_datadef['KEYM'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		/* All LAND records deleted when file was compressed
		$this->_datadef['LAND'] = array('VNML' => array( 'keep' => -1),
		                                'VHGT' => array( 'keep' => -1),
		                                'VCLR' => array( 'keep' => -1),
		                                'BTXT' => array( 'keep' => -1),
		                                'ATXT' => array( 'keep' => -1),
		                                'VTXT' => array( 'keep' => -1));
		*/
		$this->_datadef['LIGH'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'time', 'type' => 'long', 'default' => -1),
			                                        array( 'val' => 'radius', 'type' => 'ulong'),
			                                        array( 'val' => 'color', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('dynamic', 'carried', 'negative', 'flicker', NULL, 'default_off', 'flicker slow', 'pulse', 'pulse slow', 'spot light', 'spot shadow')),
			                                        array( 'val' => 'falloff', 'type' => 'float', 'default' => 1),
			                                        array( 'val' => 'fov', 'type' => 'float', 'default' => 90),
			                                        array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'FNAM' => array( 'keep' => -1));
		$this->_datadef['LSCR'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'LNAM' => array( 'keep' => -1));
		/*$this->_datadef['LTEX'] = array('HNAM' => array( 'keep' => -1),
		                                'GNAM' => array( 'keep' => -1));*/
		$this->_datadef['LVLC'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each')),
			      'LVLD' => array( 'keep' => 1,
			                       // special treatment to pull out both chancenone and (for old-style records) the alllvl bit
			                       // set_lvld will also set 'alllvl' record, but no need to specify that here (alllvl existence already specified by LVLF record)
			                       'type' => 'lvld',
			                       'val' => 'chancenone'),
			      'TNAM' => array( 'keep' => 0,
			                       'val' => 'template_id',
			                       'type' => 'formid'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'ushort'),
			                                        array( 'val' => 'item_id', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))));
		$this->_datadef['LVLI'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each')),
			      'LVLD' => array( 'keep' => 1,
			                       // special treatment to pull out both chancenone and (for old-style records) the alllvl bit
			                       // set_lvld will also set 'alllvl' record, but no need to specify that here (alllvl existence already specified by LVLF record)
			                       'type' => 'lvld',
			                       'val' => 'chancenone'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'ushort'),
			                                        array( 'val' => 'item_id', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))),
			      'DATA' => array( 'keep' => -1));
		$this->_datadef['LVSP'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each', 'useall')),
			      'LVLD' => array( 'keep' => 1,
			                       // special treatment to pull out both chancenone and (for old-style records) the alllvl bit
			                       // set_lvld will also set 'alllvl' record, but no need to specify that here (alllvl existence already specified by LVLF record)
			                       'type' => 'lvld',
			                       'val' => 'chancenone'),
			      'LVLO' => array( 'keep' => 1,
			                       'multi' => 'LVLO',
			                       'data' => array( array( 'val' => 'item_lvl', 'type' => 'ushort'),
			                                        array( 'val' => 'item_name', 'index' => 4, 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'short'))));
		// Magic Effect
		$this->_datadef['MGEF'] =
			array('DESC' => array( 'keep' => 0,
			                       'type' => 'blob',
			                       'desc' => 'Description'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('hostile', 'recover', 'detrimental', 'magpct', 'self', 'touch', 'target', 'nodur', 'nomag', 'noarea', 'fxpersist', 'spellmaking', 'enchanting', 'noingr', NULL, NULL, 'use_weapon', 'use_armor', 'use_creature', 'use_skill', 'use_attrib')),
			                                        array( 'val' => 'basecost', 'type' => 'float'),
			                                        array( 'val' => 'school', 'index' => 12, 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                        array( 'val' => 'resist', 'type' => 'long'),
			                                        array( 'val' => 'enchantfactor', 'index' => 56, 'type' => 'float'),
			                                        array( 'val' => 'barterfactor', 'type' => 'float'))),
			      'ESCE' => array( 'keep' => -1));
		$this->_datadef['MISC'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		$this->_datadef['NPC_'] =
			array('NIFZ' => array( 'keep' => -1),
			      'ACBS' => array( 'keep' => 1,
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
			                                        array( 'val' => 'trainskl', 'type' => 'ubyte', 'lookup' => 'skilllist'),
			                                        array( 'val' => 'trainlvl', 'type' => 'ubyte'))),
			      'PKID' => array( 'keep' => 0,
			                       'val' => 'package_id',
			                       'multi' => 'PKID',
			                       'type' => 'formid'),
			      'CNAM' => array( 'keep' => 1,
			                       'val' => 'class_id',
			                       'type' => 'formid'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'vallist' => 'skilllist', 'valsuffix' => '_raw', 'type' => 'ubyte'),
			                                        array( 'val' => 'health_raw', 'type' => 'ulong'),
			                                        array( 'vallist' => 'attriblist', 'valsuffix' => '_raw', 'type' => 'ubyte'))),
			      'HNAM' => array( 'keep' => -1),
			      'LNAM' => array( 'keep' => -1),
			      'ENAM' => array( 'keep' => -1),
			      'HCLR' => array( 'keep' => -1),
			      'ZNAM' => array( 'keep' => -1),
			      'FGGS' => array( 'keep' => -1),
			      'FGGA' => array( 'keep' => -1),
			      'FGTS' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1));
		$this->_datadef['PACK'] =
			array('PKDT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('offer_services', 'must_reach_loc', 'must_complete', 'lock_start', 'lock_end', 'lock_location', 'unlock_start', 'unlock_end', 'unlock_location', 'continue_if_pc_near', 'once_per_day', NULL, 'skip_fallout', 'run', NULL, NULL, NULL, 'sneak', 'swim', 'allow_falls', 'armor_unequip', 'weap_unequip', 'defensive', 'use_horse', 'no_idle')),
			                                        array( 'val' => 'type', 'type' => 'long', 'lookup' => 'PACK_type'))),
			      'PSDT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'month', 'type' => 'ubyte'),
			                                        array( 'val' => 'day', 'type' => 'ubyte', 'lookup' => 'PACK_day'),
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
			      // need to add CTDT -> but store as CTDA
			      // (CTDA replaced CTDT)
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ulong', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => 'cond_param3', 'type' => 'long'))),
			                     );
		/* All PGRD records deleted when file was compressed
		$this->_datadef['PGRD'] =
			array('PGRP' => array( 'keep' => -1),
			      'PGAG' => array( 'keep' => -1),
			      'PGRR' => array( 'keep' => -1),
			      'PGRI' => array( 'keep' => -1),
			      'PGRL' => array( 'keep' => -1));
		*/
		// To work out structure of records, look at details of quests such as
		// Dark02Watery (0x000232cf):
		// * some CTDAs right after DATA; none intermixed with INDX; section at end with QSTAs and CTDAs
		// But then there even uglier ones:
		// DL9MehrunesQuest (0x07000ed9)
		// * CTDA appear within INDX after QSDT
		// * multiple QSDT / INDX and multiple CTDA / QSDT - one CNAM per QSDT?
		// * plus multiple QSTA at end of record, multiple CTDA / QSTA
		// But aren't there also cases of multiple CNAMs for one quest stage
		
		// Need to also have way to say that CTDT is alias for CTDA ... globally or foreach subrec??
		$this->_datadef['QUST'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('enabled', 'repeat_topics', 'repeat_stages')),
			                                        array( 'val' => 'priority', 'type' => 'ubyte'))),
			      // not sure about CTDAs -- is CTDA grouped under INDX? or QSTA?... all of above :(
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ulong', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => 'cond_param3', 'type' => 'long'))),
			      'INDX' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'ushort'),
			      // multiple values possible within one INDX
			      'QSDT' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'ubyte',
			                       'flag' => array('complete')),
			      // technically under INDX-QSDT
			      // SCTX is script source; SCHR/SCDA are results of compiling script
			      // SCRO is a list of global variables within the SCTX
			      'SCHR' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'SCDA' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      'SCTX' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'blob'),
			      // multiple SCRO possible per INDX-QSDT
			      'SCRO' => array( 'keep' => -1,
			                       'multi' => 'INDX'),
			      // technically under INDX-QSDT
			      'CNAM' => array( 'keep' => 0,
			                       'multi' => 'INDX',
			                       'type' => 'blob'),
			      // is QSTA grouped under INDX? No
			      'QSTA' => array( 'keep' => 0,
			                       'multi' => 'QSTA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'qtarget', 'type' => 'formid'))),
			      // not sure about CTDAs -- is CTDA grouped under INDX? or QSTA?
			      // multiple CTDAs possible for one value of QSTA!!
			      'QSTA-CTDA' => array( 'keep' => 0,
			                       'multi' => 'QSTA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ulong', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => 'cond_param3', 'type' => 'long'))),
			     );
		$this->_datadef['RACE'] =
			array('ATTR' => array( 'keep' => 1,
			                       'data' => array(array( 'val' => 'strength_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'intelligence_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'willpower_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'agility_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'speed_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'endurance_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'personality_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'luck_m', 'type' => 'ubyte'),
			                                       array( 'val' => 'strength_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'intelligence_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'willpower_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'agility_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'speed_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'endurance_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'personality_f', 'type' => 'ubyte'),
			                                       array( 'val' => 'luck_f', 'type' => 'ubyte'))),
			      'DATA' => array( 'keep' => 1,
			                       // -1 for sklx when missing
			                       'data' => array( array( 'val' => 'skl1', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus1', 'type' => 'byte'),
			                                        array( 'val' => 'skl2', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus2', 'type' => 'byte'),
			                                        array( 'val' => 'skl3', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus3', 'type' => 'byte'),
			                                        array( 'val' => 'skl4', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus4', 'type' => 'byte'),
			                                        array( 'val' => 'skl5', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus5', 'type' => 'byte'),
			                                        array( 'val' => 'skl6', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus6', 'type' => 'byte'),
			                                        array( 'val' => 'skl7', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus7', 'type' => 'byte'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'height_m', 'type' => 'float'),
			                                        array( 'val' => 'height_f', 'type' => 'float'),
			                                        array( 'val' => 'weight_m', 'type' => 'float'),
			                                        array( 'val' => 'weight_f', 'type' => 'float'),
			                                        array( 'type' => 'ulong', 'flag' => array( 'playable')))),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'SPLO' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'val' => 'spell_id',
			                       'multi' => 'SPLO'),
			      // not sure about the ushorts here: info comes from WryeBash, which identifies the param as an int
			      'XNAM' => array( 'keep' => 0,
			                       'multi' => 'XNAM',
			                       'data' => array( array( 'val' => 'opp_race_id', 'type' => 'formid'),
			                                        array( 'val' => 'opp_modifier', 'type' => 'short'))),
			      'CNAM' => array( 'keep' => -1),
			      'DNAM' => array( 'keep' => -1),
			      'ENAM' => array( 'keep' => -1),
			      'FGGA' => array( 'keep' => -1),
			      'FGGS' => array( 'keep' => -1),
			      'FGTS' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1),
			      'HNAM' => array( 'keep' => -1),
			      'MNAM' => array( 'keep' => -1),
			      'INDX' => array( 'keep' => -1),
			      'NAM0' => array( 'keep' => -1),
			      'NAM1' => array( 'keep' => -1),
			      'PNAM' => array( 'keep' => -1),
			      'UNAM' => array( 'keep' => -1),
			      'VNAM' => array( 'keep' => -1)
			     );
		$this->_datadef['REFR'] =
			array('NAME' => array( 'keep' => 1,
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
			      'XTRG' => array( 'keep' => -1),
			      // this does not appear to be parent_id -- comes out as NULL for every REFR item?
			      /*			      'XTRG' => array( 'keep' => 0,
			                       'val' => 'parent_id',
			                       'type' => 'formid'),*/
			      'XSED' => array( 'keep' => -1),
			      'XLOD' => array( 'keep' => -1),
			      'XPCI' => array( 'keep' => -1),
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
			      /*'XESP' => array( 'keep' => -1),*/
			      'XLCM' => array( 'keep' => -1),
			      'XRTM' => array( 'keep' => -1),
			      'XACT' => array( 'keep' => -1),
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
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'x', 'type' => 'float'),
			                                        array( 'val' => 'y', 'type' => 'float'),
			                                        array( 'val' => 'z', 'type' => 'float'),
			                                        array( 'val' => 'rx', 'type' => 'float'),
			                                        array( 'val' => 'ry', 'type' => 'float'),
			                                        array( 'val' => 'rz', 'type' => 'float')))
			     );
		/*$this->_datadef['REGN'] =
			array();
		$this->_datadef['ROAD'] =
			array();
		$this->_datadef['SBSP'] =
			array('DNAM' => array( 'keep' => -1));*/
		$this->_datadef['SCPT'] =
			array('SCHR' => array( 'keep' => -1),
			      'SCDA' => array( 'keep' => -1),
			      'SCTX' => array( 'keep' => 1,
			                       'type' => 'blob'),
			      'SLSD' => array( 'keep' => -1),
			      'SCVR' => array( 'keep' => -1),
			      'SCRO' => array( 'keep' => -1),
			      'SCRV' => array( 'keep' => -1));
		$this->_datadef['SGST'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'uses', 'type' => 'ubyte'),
			                                        // values all 0 -- and no mention on wiki of any value, so
			                                        // effect doesn't seem to add value
			                                        array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'val' => 'effect_id',
			                       'type' => 'mgefstr',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'effectb_id', 'type' => 'mgefstr'),
			                                             array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong', 'lookup' => 'actorlist'))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		$this->_datadef['SKIL'] =
			array('INDX' => array( 'keep' => 1,
			                       'val' => 'name',
			                       'type' => 'ulong',
			                       'lookup' => 'actorlist'),
			      'DESC' => array( 'keep' => 0,
			                       'type' => 'blob'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'action', 'type' => 'ulong'),
			                                        array( 'val' => 'attrib', 'type' => 'ulong', 'lookup' => 'attriblist'),
			                                        array( 'val' => 'spec', 'type' => 'ulong', 'lookup' => 'speclist'),
			                                        array( 'val' => 'experience1', 'type' => 'float'),
			                                        array( 'val' => 'experience2', 'type' => 'float'))),
			      'ANAM' => array( 'keep' => 0,
			                       'val' => 'appren_text',
			                       'type' => 'blob'),
			      'JNAM' => array( 'keep' => 0,
			                       'val' => 'jman_text',
			                       'type' => 'blob'),
			      'ENAM' => array( 'keep' => 0,
			                       'val' => 'expert_text',
			                       'type' => 'blob'),
			      'MNAM' => array( 'keep' => 0,
			                       'val' => 'master_text',
			                       'type' => 'blob'));
		$this->_datadef['SLGM'] =
			array('DATA' => array( 'keep' => 1,
			                       // value is not affected by presence of soul
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'SOUL' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'soul_contained',
			                       'lookup' => 'soullist'),
			      'SLCP' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'soul_capacity',
			                       'lookup' => 'soullist'));
		/*$this->_datadef['SOUN'] =
			array('SNDX' => array( 'keep' => -1),
			      'SNDD' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1));*/
		$this->_datadef['SPEL'] =
			array('SPIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ulong', 'lookup' => 'SPEL_type'),
			                                        array( 'val' => 'spellcost', 'type' => 'long'),
			                                        array( 'val' => 'spelllevel', 'type' => 'ulong', 'lookup' => 'spelllevels'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc', NULL, 'start_spell', 'silence_immune', 'ignore_los', 'script_always', 'disallow reflect')),
			                                        array( 'val' => 'spellschool', 'type' => 'tinyint', 'lookup' => 'magic_school', 'extra' => 1))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'val' => 'effect_id',
			                       'type' => 'mgefstr',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'effectb_id', 'type' => 'mgefstr'),
			                                             array( 'val' => 'eff_mag', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong', 'lookup' => 'actorlist'))),
//second definition of FULL -- need to accommodate in coding
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      'EFID-SCIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'script_id', 'type' => 'formid'),
			                                             array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'magic_school'),
			                                             array( 'val' => 'visual', 'type' => 'string', 'len' => 4),
			                                             array( 'type' => 'ulong', 'flag' => array( 'hostile')))));
		// Note that I do care about markers (edid like '%Marker%') -- but the only info I need about markers is
		// tracked by AllItems table
		/*$this->_datadef['STAT'] =
			array();*/
		// Main plugin header
		$this->_datadef['TES4'] =
			array('HEDR' => array( 'keep' => -1),
			      'OFST' => array( 'keep' => -1),
			      'DELE' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => -1),
			      'SNAM' => array( 'keep' => -1),
			      'MAST' => array( 'keep' => -1));
		/*$this->_datadef['TREE'] =
			array('SNAM' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => -1),
			      'BNAM' => array( 'keep' => -1));
		$this->_datadef['WATR'] =
			array();*/
		$this->_datadef['WEAP'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ulong', 'lookup' => 'WEAP_type'),
			                                        array( 'val' => 'speed', 'type' => 'float'),
			                                        array( 'val' => 'reach', 'type' => 'float'),
			                                        array( 'type' => 'ulong', 'flag' => array('resistnw')),
			                                        array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'health', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'damage', 'type' => 'short'),
			                                        array( 'val' => 'skill', 'type' => 'ubyte', 'lookup' => 'skilllist', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))));
		$this->_datadef['WRLD'] =
			array('WNAM' => array( 'keep' => 1,
			                       'type' => 'formid'),
			      'SNAM' => array( 'keep' => -1),
			      'CNAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'NAM2' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'DATA' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array( 'small', 'notravel', 'oblivion')));
		/*$this->_datadef['WTHR'] =
			array('CNAM' => array( 'keep' => -1),
			      'DNAM' => array( 'keep' => -1),
			      'NAM0' => array( 'keep' => -1),
			      'FNAM' => array( 'keep' => -1),
			      'HNAM' => array( 'keep' => -1),
			      'DATA' => array( 'keep' => -1),
			      'SNAM' => array( 'keep' => -1));*/
	}

	// cell pairs that should be kept separate (doors between the cells should not be followed when connecting cells together)
	// although connections may still be used to assign a cell to a worldspace (in case of a set of cells that does not itself
	// connect to outside world, e.g., for The Old Way)
	// each entry needs to be put in a separate array so that in cases where multiple separates are provided for a single cell,
	// they don't override each other
	protected function _get_cells_separate() {
		return
			array(array("BrumaCaverns" => "BrumaJearlsHouseBasement"),
			      array("SENSHouseofDementia" => "SENSCastleSheogorath"),
			      array("SENSHouseofMania" => "SENSCastleSheogorath"),
			      array("BravilWizardsGrotto" => "BravilCastlePrivateQuartersNorthWing")
			     );
	}
	
	// names of cells that are to be preferentially used as primary entrance/exit to a set of cells with multiple entrances/exits
	// extra array provided for future possibility of setting x/y/z preferences to choose a single exit 
	
	// eventually may also want a set of secondary cells?  For example, to group merchants and state that a merchant is part of the Underworks rather than Waistworks.
	protected function _get_cells_preferred() {
		// need to get exit and nonexit recognized: checks need to look at both door and door_rev, see whether either one matches
		// should also check that exit connects to specified cell (limit typos)
		// add check that only one acceptable door found leading from cell to outside (because at least one of original doors was incorrect)
		// if more than one acceptable door, be sure to print out all necessary information to fill in this table (edid+formid for cells, doors)
		// but also want a default system for choosing which door (so that these details don't need to be done on first round of data generation)
		return
			array(//"0x00091268" => array('name' => 'Fathis Aren\'s Tower'), this is a world, not a cell
			      "BravilWizardsLair" => array('exit' => 'WizardsTowerDoor'),
			      "WellspringCave" => array('nonexit' => 'WellspringGroveEntrance01'),
			      "SerpentsTrail" => array('nonexit' => '0x0008fe86'),
			      "XPEbrocca01" => array('nonexit' => 'EbroccaTrapBackDoor'),
			      "XPFain01" => array('nonexit' => 'XPFainExteriorbto01bREF'),
			      "XPGardens01" => array('nonexit' => 'DoorFromSETheFringeOrderedToXPGardens01Ref'),
			      "XPXeddefen01" => array('nonexit' => 'SE08XeddefenDoorPreCollapseRef'),
			      "XPXirethard01" => array('nonexit' => 'SE07AEscapeTunnelDoorRef'),
			      "XPSwampgasHole01" => array('nonexit' => '0x00013177'),
			      "SEDeepwallowBeeleisHouse" => array('nonexit' => '0x00059852'),
			      "SENSBooksofBliss" => array('nonexit' => '0x00070502'),
			      "SESplitManiaJastiraNanusHouse" => array('nonexit' => '0x00069939'),
			      "SESplitDementiaJastiraNanusHouse" => array('nonexit' => '0x0006993d'),
			      "SENSMuseumofOddities" => array('nonexit' => '0x000451d4'),
			      "SENSMuurinesHouseUpstairs" => array('nonexit' => '0x000451d6'),
			      "SEPasswallNanettesHouse" => array('nonexit' => '0x000120b2'),
			      "SENSRendilDrararasHouseUpstairs" => array('nonexit' => '0x0005faa3'),
			      "SEHighcrossRunsinCirclesHouse" => array('nonexit' => '0x00069935'),
			      "SENSSicklyBernicesTaphouseGuestRooms" => array('nonexit' => '0x0003bedd'),
			      "SENSTheChoosyBeggar" => array('nonexit' => '0x0005fe81'),
			      "SENSThingsFoundUpstairs" => array('nonexit' => '0x000451d5'),
			      "SENSCastleSheogorath" => array('exit' => 'SEPalaceDoorToExteriorMania'),
			      "SENSHouseofDementia" => array('exit' => '0x0001302a'),
			      "SENSHouseofMania" => array('exit' => '0x00013029')
			     );
	}
	
}

?>
