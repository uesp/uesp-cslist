<?php

/**
 * Pre-assembled for Skyrim
 * Will need a ton of tweaks

* Setting up datadefs with a list of expected/possible rectypes/subrecs
* Especially for REFR -- would prefer to not have to re-read REFRs just to fill in a subrec that
*  I should have known would be there (esp. NAME!!)
* Note that reading REFR without doing NAME is basically useless

* Need to scan final list for lookups and either provide input for lookup, or comment out
* Need to scan for mgef
* Update with any extra subrecs decoded from FO
* Do runs using this to read in FO files -> check that there aren't syntax errors

* Note that most formids aren't being processed as formids -- wait until initial scan has been done to
* confirm record format, instead of creating tons of fake ordid entries
* (especially given how every AllItem entry contributes to slower indexing/etc)
*/

require_once 'csread.inc';

class SRRead extends CSRead {
// Note that _files also determines name of any string files added by the update
// When I copied SR_update_14.esm I did not check for new string files -- but it is very likely that it included some
// Note that the way the string files work they can *NOT* affect any entries that are not otherwise modified by
//  the update (indices can be altered, so the indices cannot be used with earlier esm files)

// The only reason why SR_update_14.esm is larger than SR_update_15.esm is 3 WRLD records (3c, 1691d, 34240)
//  specifically within the (unknown) RNAM subrecord
// Those RNAM subrecords were removed in update_15
	protected $_files = array("Skyrim.esm", "SR_update_11.esm", "SR_update_14.esm", "SR_update_15.esm", "SR_update_16.esm", "Dawnguard.esm", "HearthFires.esm", "SR_update_18.esm", "Dragonborn.esm", "SR_update_19.esm");
	protected $_fileofficial = array("Skyrim.esm", "Update.esm", "Update.esm", "Update.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Update.esm", "Dragonborn.esm", "Update.esm");
	protected $_filenamespace = array("Skyrim", "Skyrim", "Skyrim", "Skyrim", "Skyrim", "Skyrim", "Skyrim", "Skyrim", "Dragonborn", "Skyrim");
	protected $_filemodname = array('', '', '', '', '', 'Dawnguard', 'Hearthfire', '', 'Dragonborn', '');
	protected $_filemodabbrev = array('SR', 'SR', 'SR', 'SR', 'SR', 'DG', 'HF', 'SR', 'DB', 'SR');

	protected $_fileidprefix = array(0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x02, 0x01);
	
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
		'revision' => 4,
		'version' => 2,
		'flags_b' => 2,
		// subrecords
	);

// SR-specific PLDT processing
// * uses ANAM's index
// * different set of possible type values
// * save non-formids to CNAM instead of using negative numbers
// Skyrim: 0/1/9 are formids; 6 appears to be too (keywords)
//   2/3/12 are not (empty); 8 is not (integer)
	function process_packid_PLDT($input) {
		$index = $this->_parentlist->get_active_index('ANAM');
		$type = $this->csunpack("L",substr($input,0,4));

		if (in_array($type, array(0,1,6,9))) {
			$ordid = $this->process_packid(substr($input,4,4), TRUE);
			if (isset($ordid))
				$this->_parentlist->set_active($ordid, 'activity_formid', $index);
		}
		else {
			$raw = $this->csunpack('L', substr($input,4,4));
			if (isset($raw))
				$this->_parentlist->set_active($raw, 'activity_value', $index);
		}
		return $type;
	}

	// Note that CTDAs increased from 24 to 28 bytes in FO (probably added a 4th param)
	// No evidence from FO that params in CTDAs are ever floats

	protected function _datadef_setup() {
		if (!empty($this->_datadef))
			return;
		$this->_parentlist->set_lookup('Worldspace', array('Test', 'Skyrim', 'Other'));
		$this->_parentlist->set_lookup('Worldtype', array('Test', 'Main', 'City', 'Submap', 'Other'));
		
		$this->_parentlist->set_lookup('speclist', array('Combat', 'Magic', 'Stealth'));
                $this->_parentlist->set_lookup('ENCH_type', array('Scroll', 'Staff', 'Weapon', 'Apparel'));
		$this->_parentlist->set_lookup('effect_range', array('Self', 'Touch', 'Target'));
		$this->_parentlist->set_lookup('soullist', array( 'none', 'Petty', 'Lesser', 'Common', 'Greater', 'Grand'));
		$this->_parentlist->set_lookup('DIAL_type', array('Topic', 'Conversation', 'Combat', 'Persuasion', 'Detection', 'Service', 'Miscellaneous'));
		$this->_parentlist->set_lookup('SPEL_type', array('Spell', 'Disease', 'Power', 'Lesser Power', 'Ability', 'Poison', 10 => 'Addiction', 11 => 'Voice Power'));
		$this->_parentlist->set_lookup('casting_type', array('Constant Effect', 'Fire and Forget', 'Concentration'));
		$this->_parentlist->set_lookup('delivery_type', array('Self', 'Contact', 'Aimed', 'Target Actor', 'Target Location'));
		$this->_parentlist->set_lookup('BOOK_type', array('Book', 'Skill Book', 'Spell Tome', 'Note'));
		$this->_parentlist->set_lookup('spelllevels', array('Novice', 'Apprentice', 'Adept', 'Expert', 'Master'));
		// Leaving Mysticism in list -- Bethesda likely to just not use the number instead of redefining the whole list
		// use same lookup for skills and magic schools?
		// 13 = Pickpocket
		// 22 = Restoration?
		$this->_parentlist->set_lookup('ARMO_skill', array('Clothing', 'Light Armor', 'Heavy Armor', 'Jewelry', 'Other'));
		$this->_parentlist->set_lookup('magic_school', array( 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Mysticism', 'Restoration'));
		$this->_parentlist->set_lookup('PLDT_type', array('Near reference', 'In cell', 'Near package start location', 'Near editor location', 'Object ID', 'Object type', 'Near linked reference', 8 => 'Alias (reference)', 9 => 'Alias (location)', 12 => 'Near Self'));
		$this->_parentlist->set_lookup('PTDA_type', array('Specific reference', 'Object ID', 'Object type', 'Linked Reference', 'Ref Alias', 6=>'Self'));
		$this->_parentlist->set_lookup('PACK_day', array('Sundas', 'Morndas', 'Tirdas', 'Middas', 'Turdas', 'Fredas', 'Loredas', 'Morndas-Fredas', 'Loredas/Sundas', 'Morndas/Middas/Fredas', 'Tirdas/Turdas', 255=>'Any'));
		$this->_parentlist->set_lookup('PACK_speed', array('Walk', 'Jog', 'Run', 'Fast Walk'));
		$this->_parentlist->set_lookup('ARMO_type', array( 'Armor', 'Helmet', 'Boots', 'Gauntlets', 'Shield', 'Clothes', 'Hood', 'Shoes', 'Gloves', 'Ring', 'Amulet', 'Circlet', 'Other'));
		$this->_parentlist->set_lookup('WEAP_type', array( 'Bow', 'Staff', 'Dagger', 'Sword', 'Mace', 'War Axe', 'Warhammer', 'Greatsword', 'Battleaxe', 'Bound Arrow', 'Other'));
// This is the lookup using for WEAP typeb (first 4 bytes of DNAM field)
// However, I'm guessing it's not used -- not visible in CK, plus Battleaxe and Warhammer are lumped together even though game treats them differently
//		$this->_parentlist->set_lookup('WEAP_typeb', array( 'Projectile', 'Sword', 'Dagger', 'War Axe', 'Mace', 'Greatsword', 'Battleaxe/Warhammer', 'Bow', 'Staff'));
		
		$this->_parentlist->set_lookup('actorlist', array(6 => 'One-handed', 7 => 'Two-handed', 8 => 'Archery', 9 => 'Block', 10 => 'Smithing', 11 => 'Heavy Armor', 12 => 'Light Armor', 13 => 'Pickpocket', 14 => 'Lockpicking', 15 => 'Sneak', 16 => 'Alchemy', 17 => 'Speech', 18 => 'Alteration', 19 => 'Conjuration', 20 => 'Destruction', 21 => 'Illusion', 22 => 'Restoration', 23 => 'Enchanting'));
		$this->_parentlist->set_lookup('skilllist', array('One-handed', 'Two-handed', 'Archery', 'Block', 'Smithing', 'Heavy Armor', 'Light Armor', 'Pickpocket', 'Lockpicking', 'Sneak', 'Alchemy', 'Speech', 'Alteration', 'Conjuration', 'Destruction', 'Illusion', 'Restoration', 'Enchanting'));
		$this->_parentlist->set_lookup('FACT_combat', array('Neutral', 'Enemy', 'Ally', 'Friend'));
		$this->_parentlist->set_lookup('NPC_aggression', array('Unaggressive', 'Aggressive', 'Very Aggressive', 'Frenzied'));
		$this->_parentlist->set_lookup('NPC_confidence', array('Cowardly', 'Cautious', 'Average', 'Brave', 'Foolhardy'));
		$this->_parentlist->set_lookup('NPC_morality', array('Any Crime', 'Violence Against Enemies', 'Property Crime Only', 'No Crime'));
		$this->_parentlist->set_lookup('NPC_assistance', array('Helps Nobody', 'Helps Allies', 'Helps Friends and Allies'));
		$this->_parentlist->set_lookup('QUST_type', array('None', 'Main Quest', 'Mages\' Guild', 'Thieves\' Guild', 'Dark Brotherhood', 'Companion Quests', 'Miscellaneous', 'Daedric Quests', 'Side Quests', 'Civil War'));
		$this->_parentlist->set_lookup('RACE_size', array('Small', 'Medium', 'Large', 'Extra Large'));
		$this->_parentlist->set_lookup('RELA_rank', array('Lover (4)', 'Ally (3)', 'Confidant (2)', 'Friend (1)', 'Acquaintance (0)', 'Rival (-1)', 'Foe (-2)', 'Enemy (-3)', 'Archnemesis (-4)'));
		 $this->_parentlist->set_lookup('ARCH_lvlmod', array('Easy', 'Medium', 'Hard', 'Very Hard'));
		 $this->_parentlist->set_lookup('SMEN_event_type', array(
			'ADCR' => 'Crime Gold Event',
			'ADIA' => 'Actor Dialogue Event',
			'AFAV' => 'Player Activate Actor',
			'AHEL' => 'Actor Hello Event',
			'AIPL' => 'Player Add Item',
			'ARRT' => 'Arrest Event',
			'ASSU' => 'Assault Actor Event',
			'BRIB' => 'Bribe',
			'CAST' => 'Cast Magic Event',
			'CHRR' => 'Change Relationship Rank',
			'CLOC' => 'Change Location Event',
			'CRFT' => 'Craft Item',
			'CURE' => 'Player Cured',
			'DEAD' => 'Dead Body',
			'ESJA' => 'Escaped Jail',
			'FLAT' => 'Flatter',
			'INFC' => 'Player Infected',
			'INTM' => 'Intimidate',
			'JAIL' => 'Jail Event',
			'KILL' => 'Kill Actor Event',
			'LEVL' => 'Increase Level',
			'LOCK' => 'Lock Pick',
			'NVPE' => 'New Voice Power',
			'PFIN' => 'Pay Fine Event',
			'PRFV' => 'Player Receives Favor',
			'REMP' => 'Player Remove Item',
			'QSTR' => 'Quest Start',
			'SCPT' => 'Script Event',
			'SKIL' => 'Skill Increase',
			'STIJ' => 'Served Time',
			'TRES' => 'Trespass Actor Event'));
		
		$rawCTDA = array(
			0 => array('GetWantBlocking', null, true, array()),
			1 => array('GetDistance', null, true, array('ObjectReferenceID')),
			2 => array('AddItem', null, true, array('ObjectID', 'Count', 'Flag')),
			3 => array('SetEssential', null, false, array('Actor Base', 'Integer')),
			4 => array('Rotate', null, true, array('Axis', 'Speed')),
			5 => array('GetLocked', null, true, array()),
			6 => array('GetPos', null, true, array('Axis')),
			7 => array('SetPos', null, true, array('Axis', 'Float')),
			8 => array('GetAngle', null, true, array('Axis')),
			9 => array('SetAngle', null, true, array('Axis', 'Float')),
			10 => array('GetStartingPos', null, true, array('Axis')),
			11 => array('GetStartingAngle', null, true, array('Axis')),
			12 => array('GetSecondsPassed', null, false, array()),
			13 => array('Activate', null, true, array('ObjectReferenceID', 'Integer')),
			14 => array('GetActorValue', 'GetAV', true, array('Actor Value')),
			15 => array('SetActorValue', 'SetAV', true, array('Actor Value', 'Amount')),
			16 => array('ModActorValue', 'ModAV', true, array('Actor Value', 'Amount')),
			17 => array('SetAtStart', null, true, array()),
			18 => array('GetCurrentTime', null, false, array()),
			19 => array('PlayGroup', null, true, array('Animation Group', 'Flags')),
			20 => array('LoopGroup', null, true, array('Animation Group', 'Integer', 'Flags')),
			21 => array('SkipAnim', null, true, array()),
			22 => array('StartCombat', null, true, array('Actor')),
			23 => array('StopCombat', null, true, array()),
			24 => array('GetScale', null, true, array()),
			25 => array('IsMoving', null, true, array()),
			26 => array('IsTurning', null, true, array()),
			27 => array('GetLineOfSight', 'GetLOS', true, array('ObjectReferenceID')),
			28 => array('AddSpell', null, true, array('Spell Item')),
			29 => array('RemoveSpell', null, true, array('Spell Item')),
			30 => array('Cast', null, true, array('Magic Item', 'ObjectReferenceID', 'Source')),
			31 => array('GetButtonPressed', null, false, array()),
			32 => array('GetInSameCell', null, true, array('ObjectReferenceID')),
			33 => array('Enable', null, true, array('Integer')),
			34 => array('Disable', null, true, array('Integer')),
			35 => array('GetDisabled', null, true, array()),
			36 => array('MenuMode', null, false, array('Integer')),
			37 => array('PlaceAtMe', null, true, array('ObjectID', 'Count', 'Distance', 'Direction')),
			38 => array('PlaySound', null, false, array('Sound', 'Integer')),
			39 => array('GetDisease', null, true, array()),
			40 => array('FailAllObjectives', null, false, array('Quest')),
			41 => array('GetClothingValue', null, true, array()),
			42 => array('SameFaction', null, true, array('Actor')),
			43 => array('SameRace', null, true, array('Actor')),
			44 => array('SameSex', null, true, array('Actor')),
			45 => array('GetDetected', null, true, array('Actor')),
			46 => array('GetDead', null, true, array()),
			47 => array('GetItemCount', null, true, array('ObjectID')),
			48 => array('GetGold', null, true, array()),
			49 => array('GetSleeping', null, true, array()),
			50 => array('GetTalkedToPC', null, true, array()),
			51 => array('Say', null, true, array('Topic', 'Integer', 'Actor Base', 'Integer', 'Integer')),
			52 => array('SayTo', null, true, array('Actor', 'Topic', 'Integer', 'Integer')),
			53 => array('GetScriptVariable', null, false, array('ObjectReferenceID', 'Variable Name')),
			54 => array('StartQuest', null, false, array('Quest')),
			55 => array('StopQuest', null, false, array('Quest')),
			56 => array('GetQuestRunning', 'GetQR', false, array('Quest')),
			57 => array('SetStage', null, false, array('Quest', 'Stage')),
			58 => array('GetStage', null, false, array('Quest')),
			59 => array('GetStageDone', null, false, array('Quest', 'Stage')),
			60 => array('GetFactionRankDifference', null, true, array('Faction', 'Actor')),
			61 => array('GetAlarmed', null, true, array()),
			62 => array('IsRaining', null, false, array()),
			63 => array('GetAttacked', null, true, array()),
			64 => array('GetIsCreature', null, true, array()),
			65 => array('GetLockLevel', null, true, array()),
			66 => array('GetShouldAttack', null, true, array('Actor')),
			67 => array('GetInCell', null, true, array('Cell')),
			68 => array('GetIsClass', null, true, array('Class')),
			69 => array('GetIsRace', null, true, array('Race')),
			70 => array('GetIsSex', null, true, array('Sex')),
			71 => array('GetInFaction', null, true, array('Faction')),
			72 => array('GetIsID', null, true, array('ObjectID')),
			73 => array('GetFactionRank', null, true, array('Faction')),
			74 => array('GetGlobalValue', null, false, array('Global')),
			75 => array('IsSnowing', null, false, array()),
			76 => array('FastTravel', 'ft', false, array('TESObjectREFR')),
			77 => array('GetRandomPercent', null, false, array()),
			78 => array('RemoveMusic', null, false, array('Music')),
			79 => array('GetQuestVariable', null, false, array('Quest', 'Variable Name')),
			80 => array('GetLevel', null, true, array()),
			81 => array('IsRotating', null, true, array()),
			82 => array('RemoveItem', null, true, array('ObjectID', 'Count', 'Flag')),
			83 => array('GetLeveledEncounterValue', null, true, array('Integer')),
			84 => array('GetDeadCount', null, false, array('Actor Base')),
			85 => array('AddToMap', 'ShowMap', false, array('Map Marker', 'Integer')),
			86 => array('StartConversation', null, true, array('TESObjectREFR', 'Topic', 'TESObjectREFR', 'TESObjectREFR', 'Integer', 'Integer')),
			87 => array('Drop', null, true, array('ObjectID', 'Count')),
			88 => array('AddTopic', null, false, array('Topic')),
			89 => array('ShowMessage', null, false, array('Message')),
			90 => array('SetAlert', null, true, array('Integer')),
			91 => array('GetIsAlerted', null, true, array()),
			92 => array('Look', null, true, array('ObjectReferenceID', 'Integer')),
			93 => array('StopLook', null, true, array()),
			94 => array('EvaluatePackage', 'evp', true, array()),
			95 => array('SendAssaultAlarm', null, false, array('Actor', 'Faction')),
			96 => array('EnablePlayerControls', 'epc', false, array('Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer')),
			97 => array('DisablePlayerControls', 'dpc', false, array('Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer')),
			98 => array('GetPlayerControlsDisabled', 'gpc', false, array('Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer', 'Integer')),
			99 => array('GetHeadingAngle', null, true, array('ObjectReferenceID')),
			100 => array('PickIdle', null, true, array()),
			101 => array('IsWeaponMagicOut', null, true, array()),
			102 => array('IsTorchOut', null, true, array()),
			103 => array('IsShieldOut', null, true, array()),
			104 => array('CreateDetectionEvent', null, true, array('TESObjectREFR', 'Integer ')),
			105 => array('IsActionRef', null, true, array('ObjectReferenceID')),
			106 => array('IsFacingUp', null, true, array()),
			107 => array('GetKnockedState', null, true, array()),
			108 => array('GetWeaponAnimType', null, true, array()),
			109 => array('IsWeaponSkillType', null, true, array('Actor Value')),
			110 => array('GetCurrentAIPackage', null, true, array()),
			111 => array('IsWaiting', null, true, array()),
			112 => array('IsIdlePlaying', null, true, array()),
			113 => array('CompleteQuest', null, false, array('Quest')),
			114 => array('Lock', null, true, array('Integer', 'Integer')),
			115 => array('UnLock', null, true, array('Integer')),
			116 => array('IsIntimidatedbyPlayer', null, true, array()),
			117 => array('IsPlayerInRegion', null, false, array('Region')),
			118 => array('GetActorAggroRadiusViolated', null, true, array()),
			119 => array('GetCrimeKnown', null, true, array('Crime Type', 'Criminal', 'Victim')),
			120 => array('SetEnemy', null, false, array('Faction', 'Faction', 'Value', 'Value')),
			121 => array('SetAlly', null, false, array('Faction', 'Faction', 'Value', 'Value')),
			122 => array('GetCrime', null, true, array('Actor', 'Crime Type')),
			123 => array('IsGreetingPlayer', null, true, array()),
			124 => array('StartMisterSandMan', null, true, array('Actor')),
			125 => array('IsGuard', null, true, array()),
			126 => array('StartCannibal', null, true, array('Actor')),
			127 => array('HasBeenEaten', null, true, array()),
			128 => array('GetStaminaPercentage', 'GetStamina', true, array()),
			129 => array('GetPCIsClass', null, false, array('Class')),
			130 => array('GetPCIsRace', null, false, array('Race')),
			131 => array('GetPCIsSex', null, false, array('Sex')),
			132 => array('GetPCInFaction', null, false, array('Faction')),
			133 => array('SameFactionAsPC', null, true, array()),
			134 => array('SameRaceAsPC', null, true, array()),
			135 => array('SameSexAsPC', null, true, array()),
			136 => array('GetIsReference', null, true, array('ObjectReferenceID')),
			137 => array('SetFactionRank', null, true, array('Faction', 'Value')),
			138 => array('ModFactionRank', null, true, array('Faction', 'Value')),
			139 => array('KillActor', 'kill', true, array('Actor', 'Integer', 'Integer')),
			140 => array('ResurrectActor', 'resurrect', true, array('Integer')),
			141 => array('IsTalking', null, true, array()),
			142 => array('GetWalkSpeed', 'GetWalk', true, array()),
			143 => array('GetCurrentAIProcedure', null, true, array()),
			144 => array('GetTrespassWarningLevel', null, true, array()),
			145 => array('IsTrespassing', null, true, array()),
			146 => array('IsInMyOwnedCell', null, true, array()),
			147 => array('GetWindSpeed', null, false, array()),
			148 => array('GetCurrentWeatherPercent', 'getweatherpct', false, array()),
			149 => array('GetIsCurrentWeather', 'getweather', false, array('Weather ID')),
			150 => array('IsContinuingPackagePCNear', null, true, array()),
			151 => array('SetCrimeFaction', null, true, array('Faction')),
			152 => array('GetIsCrimeFaction', null, true, array('Faction')),
			153 => array('CanHaveFlames', null, true, array()),
			154 => array('HasFlames', null, true, array()),
			155 => array('AddFlames', null, true, array()),
			156 => array('RemoveFlames', null, true, array()),
			157 => array('GetOpenState', null, true, array()),
			158 => array('MoveToMarker', 'MoveTo', true, array('ObjectReferenceID', 'Float', 'Float', 'Float')),
			159 => array('GetSitting', null, true, array()),
			160 => array('GetFurnitureMarkerID', null, true, array()),
			161 => array('GetIsCurrentPackage', null, true, array('Package')),
			162 => array('IsCurrentFurnitureRef', null, true, array('ObjectReferenceID')),
			163 => array('IsCurrentFurnitureObj', null, true, array('Furniture')),
			164 => array('SetSize', 'CSize', true, array('Float')),
			165 => array('RemoveMe', null, true, array('Container')),
			166 => array('DropMe', null, true, array()),
			167 => array('GetFactionReaction', null, false, array('Faction', 'Faction')),
			168 => array('SetFactionReaction', null, false, array('Faction', 'Faction', 'Value')),
			169 => array('ModFactionReaction', null, false, array('Faction', 'Faction', 'Value')),
			170 => array('GetDayOfWeek', null, false, array()),
			171 => array('IgnoreCrime', null, true, array('Integer')),
			172 => array('GetTalkedToPCParam', null, false, array('Actor')),
			173 => array('RemoveAllItems', null, true, array('Container', 'Ownership Added', 'No Message Flag')),
			174 => array('WakeUpPC', null, false, array('Integer')),
			175 => array('IsPCSleeping', null, false, array()),
			176 => array('IsPCAMurderer', null, false, array()),
			177 => array('SetCombatStyle', 'setcs', true, array('Combat Style')),
			178 => array('PlaySound3D', null, false, array('Sound')),
			179 => array('SelectPlayerSpell', 'spspell', false, array('Spell Item', 'Casting Source')),
			180 => array('HasSameEditorLocAsRef', null, true, array('TESObjectREFR', 'Keyword')),
			181 => array('HasSameEditorLocAsRefAlias', null, true, array('QuestAlias', 'Keyword')),
			182 => array('GetEquipped', null, true, array('ObjectID')),
			183 => array('Wait', null, true, array('Package')),
			184 => array('StopWaiting', null, true, array('Package')),
			185 => array('IsSwimming', null, true, array()),
			186 => array('ScriptEffectElapsedSeconds', null, false, array()),
			187 => array('SetCellPublicFlag', 'setpublic', false, array('Cell', 'Flag')),
			188 => array('GetPCSleepHours', null, false, array()),
			189 => array('SetPCSleepHours', null, false, array('Integer')),
			190 => array('GetAmountSoldStolen', null, false, array()),
			191 => array('ModAmountSoldStolen', null, false, array('Integer')),
			192 => array('GetIgnoreCrime', null, false, array()),
			193 => array('GetPCExpelled', null, false, array('Faction')),
			194 => array('SetPCExpelled', null, false, array('Faction', 'Value')),
			195 => array('GetPCFactionMurder', null, false, array('Faction')),
			196 => array('SetPCFactionMurder', null, false, array('Faction', 'Value')),
			197 => array('GetPCEnemyofFaction', null, false, array('Faction')),
			198 => array('SetPCEnemyofFaction', null, false, array('Faction', 'Value')),
			199 => array('GetPCFactionAttack', null, false, array('Faction')),
			200 => array('SetPCFactionAttack', null, false, array('Faction', 'Value')),
			201 => array('StartScene', null, false, array('Scene ')),
			202 => array('StopScene', null, false, array('Scene ')),
			203 => array('GetDestroyed', null, true, array()),
			204 => array('SetDestroyed', null, true, array('Integer')),
			205 => array('GetActionRef', 'getAR', true, array()),
			206 => array('GetSelf', 'this', true, array()),
			207 => array('GetContainer', null, true, array()),
			208 => array('GetForceRun', null, true, array()),
			209 => array('SetForceRun', null, true, array('Integer')),
			210 => array('GetForceSneak', null, true, array()),
			211 => array('SetForceSneak', null, true, array('Integer')),
			212 => array('AdvancePCSkill', 'AdvSkill', false, array('Actor Value', 'Amount')),
			213 => array('AdvancePCLevel', 'AdvLevel', false, array()),
			214 => array('HasMagicEffect', null, true, array('Magic Effect')),
			215 => array('GetDefaultOpen', null, true, array()),
			216 => array('SetDefaultOpen', null, true, array('Integer')),
			217 => array('ShowClassMenu', null, false, array()),
			218 => array('ShowRaceMenu', 'SetPlayerRace', false, array('Race')),
			219 => array('GetAnimAction', null, true, array()),
			220 => array('ShowNameMenu', null, false, array()),
			221 => array('SetOpenState', null, true, array('Integer')),
			222 => array('ResetReference', 'RecycleActor', true, array('ObjectReferenceID')),
			223 => array('IsSpellTarget', null, true, array('Magic Item')),
			224 => array('GetVATSMode', null, false, array()),
			225 => array('GetPersuasionNumber', null, false, array()),
			226 => array('GetVampireFeed', null, true, array()),
			227 => array('GetCannibal', null, true, array()),
			228 => array('GetIsClassDefault', null, false, array('Class')),
			229 => array('GetClassDefaultMatch', null, false, array()),
			230 => array('GetInCellParam', null, false, array('Cell', 'ObjectReferenceID')),
			231 => array('UnusedFunction1', null, false, array()),
			232 => array('GetCombatTarget', null, true, array()),
			233 => array('GetPackageTarget', null, true, array()),
			234 => array('ShowSpellMaking', null, false, array()),
			235 => array('GetVatsTargetHeight', null, false, array()),
			236 => array('SetGhost', null, true, array('Integer')),
			237 => array('GetIsGhost', null, true, array()),
			238 => array('EquipItem', 'EquipObject', true, array('ObjectID', 'Flag', 'Hand')),
			239 => array('UnequipItem', 'UnEquipObject', true, array('ObjectID', 'Flag', 'Hand')),
			240 => array('SetClass', null, true, array('Class')),
			241 => array('SetUnconscious', null, false, array('Integer')),
			242 => array('GetUnconscious', null, true, array()),
			243 => array('SetRestrained', null, false, array('Integer')),
			244 => array('GetRestrained', null, true, array()),
			245 => array('ForceFlee', 'Flee', true, array('Cell', 'ObjectReferenceID')),
			246 => array('GetIsUsedItem', null, false, array('ObjectID')),
			247 => array('GetIsUsedItemType', null, false, array('Form Type')),
			248 => array('IsScenePlaying', null, false, array('Scene ')),
			249 => array('IsInDialogueWithPlayer', null, true, array()),
			250 => array('GetLocationCleared', null, false, array('Location')),
			251 => array('SetLocationCleared', null, false, array('Location', 'Integer')),
			252 => array('ForceRefIntoAlias', null, true, array('QuestAlias')),
			253 => array('EmptyRefAlias', null, false, array('QuestAlias')),
			254 => array('GetIsPlayableRace', null, true, array()),
			255 => array('GetOffersServicesNow', null, true, array()),
			256 => array('GetGameSetting', 'GetGS', false, array('String')),
			257 => array('StopCombatAlarmOnActor', 'SCAOnActor', true, array()),
			258 => array('HasAssociationType', null, true, array('Actor', 'AssociationType')),
			259 => array('HasFamilyRelationship', 'Family', true, array('Actor')),
			260 => array('SetWeather', 'sw', false, array('Weather ID', 'Integer')),
			261 => array('HasParentRelationship', 'IsParent', true, array('Actor')),
			262 => array('IsWarningAbout', null, true, array('Form List')),
			263 => array('IsWeaponOut', null, true, array()),
			264 => array('HasSpell', null, true, array('Spell Item')),
			265 => array('IsTimePassing', null, false, array()),
			266 => array('IsPleasant', null, false, array()),
			267 => array('IsCloudy', null, false, array()),
			268 => array('TrapUpdate', null, false, array()),
			269 => array('ShowQuestObjectives', 'SQO', false, array('Quest')),
			270 => array('ForceActorValue', 'ForceAV', true, array('Actor Value', 'Amount')),
			271 => array('IncrementPCSkill', 'IncPCS', false, array('Actor Value')),
			272 => array('DoTrap', null, true, array('Trap', 'Damage', 'Lev. Damage', 'Pushback', 'Death Pushback', 'Min. Velocity', 'Continuous', 'Stagger')),
			273 => array('EnableFastTravel', 'EnableFast', false, array('Integer')),
			274 => array('IsSmallBump', null, true, array()),
			275 => array('GetParentRef', null, true, array()),
			276 => array('PlayBink', null, false, array('String', 'Integer', 'Integer', 'Integer', 'Integer')),
			277 => array('GetBaseActorValue', 'GetBaseAV', true, array('Actor Value')),
			278 => array('IsOwner', null, true, array('Owner')),
			279 => array('SetOwnership', null, true, array('Owner')),
			280 => array('IsCellOwner', null, true, array('Cell', 'Owner')),
			281 => array('SetCellOwnership', null, false, array('Cell', 'Owner')),
			282 => array('IsHorseStolen', null, true, array()),
			283 => array('SetCellFullName', null, false, array('Cell', 'Message')),
			284 => array('SetActorFullName', null, true, array('Message')),
			285 => array('IsLeftUp', null, true, array()),
			286 => array('IsSneaking', null, true, array()),
			287 => array('IsRunning', null, true, array()),
			288 => array('GetFriendHit', null, true, array()),
			289 => array('IsInCombat', null, true, array('Integer')),
			290 => array('SetPackDuration', 'SPDur', true, array('Float')),
			291 => array('PlayMagicShaderVisuals', 'PMS', true, array('Effect Shader ID', 'Time')),
			292 => array('PlayMagicEffectVisuals', 'PME', true, array('Magic Effect', 'Time')),
			293 => array('StopMagicShaderVisuals', 'SMS', true, array('Effect Shader ID')),
			294 => array('StopMagicEffectVisuals', 'SME', true, array('Magic Effect')),
			295 => array('ResetInterior', null, false, array('Cell')),
			296 => array('IsAnimPlaying', null, true, array('Animation Group')),
			297 => array('SetActorAlpha', 'SAA', true, array('Float', 'Int')),
			298 => array('EnableLinkedPathPoints', null, true, array()),
			299 => array('DisableLinkedPathPoints', null, true, array()),
			300 => array('IsInInterior', null, true, array()),
			301 => array('ForceWeather', 'fw', false, array('Weather ID', 'Integer')),
			302 => array('ToggleActorsAI', null, true, array()),
			303 => array('IsActorsAIOff', null, true, array()),
			304 => array('IsWaterObject', null, true, array()),
			305 => array('GetPlayerAction', null, true, array()),
			306 => array('IsActorUsingATorch', null, true, array()),
			307 => array('SetLevel', null, true, array('Integer', 'Integer', 'Integer', 'Integer')),
			308 => array('ResetFallDamageTimer', null, true, array()),
			309 => array('IsXBox', null, false, array()),
			310 => array('GetInWorldspace', null, true, array('WorldSpace')),
			311 => array('ModPCMiscStat', 'ModPCMS', false, array('Miscellaneous Stat', 'Integer')),
			312 => array('GetPCMiscStat', 'GetPCMS', false, array('Miscellaneous Stat')),
			313 => array('GetPairedAnimation', 'GPA', true, array()),
			314 => array('IsActorAVictim', null, true, array()),
			315 => array('GetTotalPersuasionNumber', null, false, array()),
			316 => array('SetScale', null, true, array('Float')),
			317 => array('ModScale', null, true, array('Float')),
			318 => array('GetIdleDoneOnce', null, false, array()),
			319 => array('KillAllActors', 'killall', false, array('Actor')),
			320 => array('GetNoRumors', null, true, array()),
			321 => array('SetNoRumors', null, true, array('Integer')),
			322 => array('Dispel', null, true, array('Magic Item')),
			323 => array('GetCombatState', null, true, array()),
			324 => array('TriggerHitShader', 'ths', false, array('Float')),
			325 => array('GetWithinPackageLocation', null, true, array('PackageData (Location)')),
			326 => array('Reset3DState', null, false, array()),
			327 => array('IsRidingHorse', null, true, array()),
			328 => array('DispelAllSpells', null, true, array()),
			329 => array('IsFleeing', null, true, array()),
			330 => array('AddAchievement', null, false, array('Integer')),
			331 => array('DuplicateAllItems', null, true, array('Container', 'Ownership Added')),
			332 => array('IsInDangerousWater', null, true, array()),
			333 => array('EssentialDeathReload', null, false, array('String')),
			334 => array('SetShowQuestItems', null, false, array('Integer')),
			335 => array('DuplicateNPCStats', null, true, array('Actor')),
			336 => array('ResetHealth', null, true, array()),
			337 => array('SetIgnoreFriendlyHits', 'sifh', true, array('Integer')),
			338 => array('GetIgnoreFriendlyHits', 'gifh', true, array()),
			339 => array('IsPlayersLastRiddenHorse', null, true, array()),
			340 => array('SetActorRefraction', 'sar', true, array('Float')),
			341 => array('SetItemValue', null, true, array('Integer')),
			342 => array('SetRigidBodyMass', null, false, array('Float')),
			343 => array('ShowViewerStrings', 'svs', true, array()),
			344 => array('ReleaseWeatherOverride', 'rwo', false, array()),
			345 => array('SetAllReachable', null, false, array('Integer')),
			346 => array('SetAllVisible', null, false, array('Integer')),
			347 => array('SetNoAvoidance', null, false, array('Integer')),
			348 => array('SendTrespassAlarm', null, true, array('Actor')),
			349 => array('SetSceneIsComplex', null, false, array('Integer')),
			350 => array('Autosave', null, false, array()),
			351 => array('StartMasterFileSeekData', null, false, array()),
			352 => array('DumpMasterFileSeekData', null, false, array()),
			353 => array('IsActor', null, true, array()),
			354 => array('IsEssential', null, true, array()),
			355 => array('PreloadMagicEffect', null, true, array('Magic Effect')),
			356 => array('ShowDialogSubtitles', null, false, array('Integer')),
			357 => array('SetPlayerResistingArrest', null, true, array()),
			358 => array('IsPlayerMovingIntoNewSpace', null, false, array()),
			359 => array('GetInCurrentLoc', null, true, array('Location')),
			360 => array('GetInCurrentLocAlias', null, true, array('QuestAlias')),
			361 => array('GetTimeDead', null, true, array()),
			362 => array('HasLinkedRef', null, true, array('Keyword')),
			363 => array('GetLinkedRef', null, true, array('Keyword')),
			364 => array('DamageObject', 'do', true, array('Float')),
			365 => array('IsChild', null, true, array()),
			366 => array('GetStolenItemValueNoCrime', null, false, array('Faction')),
			367 => array('GetLastPlayerAction', null, false, array()),
			368 => array('IsPlayerActionActive', null, false, array('Integer')),
			369 => array('SetTalkingActivatorActor', null, true, array('Actor')),
			370 => array('IsTalkingActivatorActor', null, true, array('Actor')),
			371 => array('ShowBarterMenu', 'sbm', true, array('Integer')),
			372 => array('IsInList', null, true, array('Form List')),
			373 => array('GetStolenItemValue', null, false, array('Faction')),
			374 => array('AddPerk', null, true, array('Perk')),
			375 => array('GetCrimeGoldViolent', 'getviolent', true, array('Faction')),
			376 => array('GetCrimeGoldNonviolent', 'getnonviolent', true, array('Faction')),
			377 => array('ShowRepairMenu', 'srm', false, array()),
			378 => array('HasShout', null, true, array('Shout')),
			379 => array('AddNote', 'AN', false, array('Note')),
			380 => array('RemoveNote', 'RN', false, array('Note')),
			381 => array('GetHasNote', 'GetN', false, array('Note')),
			382 => array('AddToFaction', 'Addfac', true, array('Faction', 'Value')),
			383 => array('RemoveFromFaction', 'Removefac', true, array('Faction')),
			384 => array('DamageActorValue', 'DamageAV', true, array('Actor Value', 'Amount')),
			385 => array('RestoreActorValue', 'RestoreAV', true, array('Actor Value', 'Amount')),
			386 => array('TriggerHUDShudder', 'hudsh', false, array('Float', 'Float', 'Float', 'Float')),
			387 => array('GetObjectiveFailed', null, false, array('Quest', 'Integer')),
			388 => array('SetObjectiveFailed', null, false, array('Quest', 'Integer', 'State')),
			389 => array('SetGlobalTimeMultiplier', 'sgtm', false, array('Float')),
			390 => array('GetHitLocation', null, true, array()),
			391 => array('IsPC1stPerson', 'pc1st', false, array()),
			392 => array('PurgeCellBuffers', 'pcb', false, array()),
			393 => array('PushActorAway', null, true, array('ObjectReferenceID', 'Integer')),
			394 => array('SetActorsAI', null, true, array('Integer')),
			395 => array('ClearOwnership', null, true, array()),
			396 => array('GetCauseofDeath', null, true, array()),
			397 => array('IsLimbGone', null, true, array('Integer')),
			398 => array('IsWeaponInList', null, true, array('Form List')),
			399 => array('PlayIdle', null, true, array('String')),
			400 => array('ApplyImageSpaceModifier', 'imod', false, array('Imagespace Modifier ID', 'Float')),
			401 => array('RemoveImageSpaceModifier', 'rimod', false, array('Imagespace Modifier ID', 'Float')),
			402 => array('IsBribedbyPlayer', null, true, array()),
			403 => array('GetRelationshipRank', null, true, array('Actor')),
			404 => array('SetRelationshipRank', null, true, array('Actor', 'Value')),
			405 => array('SetCellImageSpace', null, false, array('Cell', 'ImageSpace')),
			406 => array('ShowChargenMenu', 'scgm', false, array()),
			407 => array('GetVATSValue', null, false, array('Integer', 'Integer')),
			408 => array('IsKiller', null, true, array('Actor')),
			409 => array('IsKillerObject', null, true, array('Form List')),
			410 => array('GetFactionCombatReaction', null, false, array('Faction', 'Faction')),
			411 => array('UseWeapon', null, true, array('Weapon', 'Location', 'Target', 'NumAttacks', 'CrouchToReload', 'HoldFire', 'AlwaysHit', 'DoNoDamage', 'TargetLocation')),
			412 => array('EvaluateSpellConditions', 'esc', true, array()),
			413 => array('ToggleMotionBlur', 'tmb', false, array()),
			414 => array('Exists', null, true, array('ObjectReferenceID')),
			415 => array('GetGroupMemberCount', null, true, array()),
			416 => array('GetGroupTargetCount', null, true, array()),
			417 => array('SetObjectiveCompleted', null, false, array('Quest', 'Integer', 'State')),
			418 => array('SetObjectiveDisplayed', null, false, array('Quest', 'Integer', 'State')),
			419 => array('GetObjectiveCompleted', null, false, array('Quest', 'Integer')),
			420 => array('GetObjectiveDisplayed', null, false, array('Quest', 'Integer')),
			421 => array('SetImageSpace', null, false, array('ImageSpace')),
			422 => array('PipboyRadio', 'prad', false, array('String', 'ObjectReferenceID')),
			423 => array('RemovePerk', null, true, array('Perk')),
			424 => array('DisableAllActors', 'DisAA', true, array()),
			425 => array('GetIsFormType', null, true, array('String')),
			426 => array('GetIsVoiceType', null, true, array('VoiceType')),
			427 => array('GetPlantedExplosive', null, true, array()),
			428 => array('CompleteAllObjectives', null, false, array('Quest')),
			429 => array('IsScenePackageRunning', null, true, array()),
			430 => array('GetHealthPercentage', null, true, array()),
			431 => array('SetAudioMultithreading', 'SAM', false, array('Integer')),
			432 => array('GetIsObjectType', null, true, array('Form Type')),
			433 => array('ShowChargenMenuParams', 'scgmp', false, array('Integer', 'Integer', 'Integer')),
			434 => array('GetDialogueEmotion', null, true, array()),
			435 => array('GetDialogueEmotionValue', null, true, array()),
			436 => array('ExitGame', 'exit', false, array()),
			437 => array('GetIsCreatureType', null, true, array('Integer')),
			438 => array('PlayerCreatePotion', null, false, array('Magic Effect', 'Magic Effect', 'Magic Effect')),
			439 => array('PlayerEnchantObject', null, false, array('Object', 'Magic Effect', 'Magic Effect')),
			440 => array('ShowWarning', null, false, array('String')),
			441 => array('EnterTrigger', null, true, array('ObjectReferenceID')),
			442 => array('MarkForDelete', null, true, array()),
			443 => array('SetPlayerAIDriven', null, false, array('Integer')),
			444 => array('GetInCurrentLocFormList', null, true, array('Form List')),
			445 => array('GetInZone', null, true, array('EncounterZone')),
			446 => array('GetVelocity', null, true, array('Axis')),
			447 => array('GetGraphVariableFloat', null, true, array('String')),
			448 => array('HasPerk', null, true, array('Perk')),
			449 => array('GetFactionRelation', null, true, array('Actor')),
			450 => array('IsLastIdlePlayed', null, true, array('Idle Form')),
			451 => array('SetNPCRadio', 'snr', false, array('Integer', 'ObjectReferenceID')),
			452 => array('SetPlayerTeammate', null, true, array('Integer')),
			453 => array('GetPlayerTeammate', null, true, array()),
			454 => array('GetPlayerTeammateCount', null, false, array()),
			455 => array('OpenActorContainer', null, true, array('Integer')),
			456 => array('ClearFactionPlayerEnemyFlag', null, false, array('Faction')),
			457 => array('ClearActorsFactionsPlayerEnemyFlag', null, true, array()),
			458 => array('GetActorCrimePlayerEnemy', null, true, array()),
			459 => array('GetCrimeGold', null, true, array('Faction')),
			460 => array('SetCrimeGold', null, true, array('Integer ', 'Faction')),
			461 => array('ModCrimeGold', null, true, array('Integer ', 'Integer', 'Faction')),
			462 => array('GetPlayerGrabbedRef', null, false, array()),
			463 => array('IsPlayerGrabbedRef', null, false, array('TESObjectREFR')),
			464 => array('PlaceLeveledActorAtMe', null, false, array('ObjectID', 'LevelModifier', 'ForcedZone')),
			465 => array('GetKeywordItemCount', null, true, array('Keyword')),
			466 => array('ShowLockpickMenu', 'slpm', false, array('Integer')),
			467 => array('GetBroadcastState', null, true, array()),
			468 => array('SetBroadcastState', null, true, array('Integer')),
			469 => array('StartRadioConversation', null, true, array('Topic')),
			470 => array('GetDestructionStage', null, true, array()),
			471 => array('ClearDestruction', null, true, array()),
			472 => array('CastImmediateOnSelf', 'cios', true, array('Magic Item')),
			473 => array('GetIsAlignment', null, true, array('Alignment')),
			474 => array('ResetQuest', null, false, array('Quest')),
			475 => array('SetQuestDelay', null, false, array('Quest', 'Float')),
			476 => array('IsProtected', null, true, array()),
			477 => array('GetThreatRatio', null, true, array('Actor')),
			478 => array('MatchFaceGeometry', null, true, array('Actor', 'Value')),
			479 => array('GetIsUsedItemEquipType', null, true, array('EquipType')),
			480 => array('GetPlayerName', null, false, array()),
			481 => array('FireWeapon', null, true, array('Weapon')),
			482 => array('PayCrimeGold', null, true, array('Integer', 'Integer', 'Faction')),
			483 => array('UnusedFunction2', null, true, array('Float')),
			484 => array('MatchRace', null, true, array('Actor')),
			485 => array('SetPCYoung', null, false, array('Integer')),
			486 => array('SexChange', null, false, array('Sex', 'Integer')),
			487 => array('IsCarryable', null, true, array()),
			488 => array('GetConcussed', null, true, array()),
			489 => array('SetZoneRespawns', null, false, array('EncounterZone', 'Integer')),
			490 => array('SetVATSTarget', null, true, array('Integer')),
			491 => array('GetMapMarkerVisible', null, true, array()),
			492 => array('ResetInventory', null, true, array()),
			493 => array('PlayerKnows', null, false, array('Knowable Form')),
			494 => array('GetPermanentActorValue', 'GetPermAV', true, array('Actor Value')),
			495 => array('GetKillingBlowLimb', null, true, array()),
			496 => array('GoToJail', null, true, array('Faction', 'Integer', 'Integer')),
			497 => array('CanPayCrimeGold', null, true, array('Faction')),
			498 => array('ServeTime', null, false, array()),
			499 => array('GetDaysInJail', null, true, array()),
			500 => array('EPAlchemyGetMakingPoison', null, false, array()),
			501 => array('EPAlchemyEffectHasKeyword', null, false, array('Keyword')),
			502 => array('ShowAllMapMarkers', 'tmm', false, array('Integer', 'Integer', 'Integer')),
			503 => array('GetAllowWorldInteractions', null, true, array()),
			504 => array('ResetAI', null, true, array()),
			505 => array('SetRumble', null, false, array('Float', 'Float', 'Float')),
			506 => array('SetNoActivationSound', null, false, array('Sound')),
			507 => array('ClearNoActivationSound', null, false, array()),
			508 => array('GetLastHitCritical', null, true, array()),
			509 => array('AddMusic', null, false, array('Music')),
			510 => array('UnusedFunction3', null, false, array()),
			511 => array('UnusedFunction4', null, true, array()),
			512 => array('SetPCToddler', null, false, array('Integer')),
			513 => array('IsCombatTarget', null, true, array('Actor')),
			514 => array('TriggerScreenBlood', 'tsb', false, array('Integer')),
			515 => array('GetVATSRightAreaFree', null, true, array('TESObjectREFR')),
			516 => array('GetVATSLeftAreaFree', null, true, array('TESObjectREFR')),
			517 => array('GetVATSBackAreaFree', null, true, array('TESObjectREFR')),
			518 => array('GetVATSFrontAreaFree', null, true, array('TESObjectREFR')),
			519 => array('GetIsLockBroken', null, true, array()),
			520 => array('IsPS3', null, false, array()),
			521 => array('IsWin32', null, false, array()),
			522 => array('GetVATSRightTargetVisible', null, true, array('TESObjectREFR')),
			523 => array('GetVATSLeftTargetVisible', null, true, array('TESObjectREFR')),
			524 => array('GetVATSBackTargetVisible', null, true, array('TESObjectREFR')),
			525 => array('GetVATSFrontTargetVisible', null, true, array('TESObjectREFR')),
			526 => array('AttachAshPile', null, true, array('Integer')),
			527 => array('SetCriticalStage', null, true, array('CriticalStage')),
			528 => array('IsInCriticalStage', null, true, array('CriticalStage')),
			529 => array('RemoveFromAllFactions', null, true, array()),
			530 => array('GetXPForNextLevel', null, false, array()),
			531 => array('ShowLockpickMenuDebug', 'slpmd', false, array('Integer')),
			532 => array('ForceSave', null, false, array()),
			533 => array('GetInfamy', null, false, array('Faction')),
			534 => array('GetInfamyViolent', null, false, array('Faction')),
			535 => array('GetInfamyNonViolent', null, false, array('Faction')),
			536 => array('UnusedFunction5', null, true, array()),
			537 => array('Sin', null, false, array('Float', 'Int')),
			538 => array('Cos', null, false, array('Float', 'Int')),
			539 => array('Tan', null, false, array('Float', 'Int')),
			540 => array('Sqrt', null, false, array('Float')),
			541 => array('Log', null, false, array('Float', 'Float')),
			542 => array('Abs', null, false, array('Float')),
			543 => array('GetQuestCompleted', 'GetQC', false, array('Quest')),
			544 => array('UnusedFunction6', null, true, array()),
			545 => array('PipBoyRadioOff', null, false, array()),
			546 => array('AutoDisplayObjectives', null, false, array('Integer')),
			547 => array('IsGoreDisabled', null, false, array()),
			548 => array('FadeSFX', 'FSFX', false, array('Float')),
			549 => array('SetMinimalUse', null, true, array('Integer')),
			550 => array('IsSceneActionComplete', null, false, array('Scene ', 'Integer')),
			551 => array('ShowQuestStages', 'SQS', false, array('Quest')),
			552 => array('GetSpellUsageNum', null, true, array('Magic Item')),
			553 => array('ForceRadioStationUpdate', 'FRSU', false, array()),
			554 => array('GetActorsInHigh', null, false, array()),
			555 => array('HasLoaded3D', null, false, array()),
			556 => array('DisableAllMines', null, false, array()),
			557 => array('SetLastExtDoorActivated', null, false, array('ObjectReferenceID')),
			558 => array('KillQuestUpdates', 'KQU', false, array()),
			559 => array('IsImageSpaceActive', null, false, array('Imagespace Modifier ID')),
			560 => array('HasKeyword', null, true, array('Keyword')),
			561 => array('HasRefType', null, true, array('RefType')),
			562 => array('LocationHasKeyword', null, true, array('Keyword')),
			563 => array('LocationHasRefType', null, true, array('RefType')),
			564 => array('CreateEvent', null, false, array('Keyword', 'Location', 'Ref 1', 'Ref 2', 'Value 1', 'Value 2')),
			565 => array('GetIsEditorLocation', null, true, array('Location')),
			566 => array('GetIsAliasRef', null, true, array('QuestAlias')),
			567 => array('GetIsEditorLocAlias', null, true, array('QuestAlias')),
			568 => array('IsSprinting', null, true, array()),
			569 => array('IsBlocking', null, true, array()),
			570 => array('HasEquippedSpell', 'hasspell', true, array('Casting Source')),
			571 => array('GetCurrentCastingType', 'getcasting', true, array('Casting Source')),
			572 => array('GetCurrentDeliveryType', 'getdelivery', true, array('Casting Source')),
			573 => array('EquipSpell', null, true, array('Spell Item', 'Casting Source')),
			574 => array('GetAttackState', null, true, array()),
			575 => array('GetAliasedRef', null, false, array('QuestAlias')),
			576 => array('GetEventData', null, false, array('Event Function', 'Event Member', 'Data')),
			577 => array('IsCloserToAThanB', null, true, array('TESObjectREFR', 'TESObjectREFR')),
			578 => array('EquipShout', null, true, array('Shout')),
			579 => array('GetEquippedShout', null, true, array('Shout')),
			580 => array('IsBleedingOut', null, true, array()),
			581 => array('UnlockWord', null, false, array('Word Of Power')),
			582 => array('TeachWord', null, false, array('Word Of Power')),
			583 => array('AddToContainer', null, true, array('TESObjectREFR', 'Flag')),
			584 => array('GetRelativeAngle', null, true, array('ObjectReferenceID', 'Axis')),
			585 => array('SendAnimEvent', 'sae', true, array('String')),
			586 => array('Shout', null, true, array('Shout', 'Integer', 'ObjectReferenceID')),
			587 => array('AddShout', null, true, array('Shout')),
			588 => array('RemoveShout', null, true, array('Shout')),
			589 => array('GetMovementDirection', null, true, array()),
			590 => array('IsInScene', null, true, array()),
			591 => array('GetRefTypeDeadCount', null, false, array('Location', 'RefType')),
			592 => array('GetRefTypeAliveCount', null, false, array('Location', 'RefType')),
			593 => array('ApplyHavokImpulse', null, true, array('Float', 'Float', 'Float', 'Float')),
			594 => array('GetIsFlying', null, true, array()),
			595 => array('IsCurrentSpell', null, true, array('Spell Item', 'Casting Source')),
			596 => array('SpellHasKeyword', null, true, array('Casting Source', 'Keyword')),
			597 => array('GetEquippedItemType', null, true, array('Casting Source')),
			598 => array('GetLocationAliasCleared', null, false, array('QuestAlias')),
			599 => array('SetLocationAliasCleared', null, false, array('QuestAlias')),
			600 => array('GetLocAliasRefTypeDeadCount', null, false, array('QuestAlias', 'RefType')),
			601=> array('GetLocAliasRefTypeAliveCount', null, false, array('QuestAlias', 'RefType')),
			602 => array('IsWardState', null, true, array('Ward State')),
			603 => array('IsInSameCurrentLocAsRef', null, true, array('TESObjectREFR', 'Keyword')),
			604 => array('IsInSameCurrentLocAsRefAlias', null, true, array('QuestAlias', 'Keyword')),
			605 => array('LocAliasIsLocation', null, false, array('QuestAlias', 'Location')),
			606 => array('GetKeywordDataForLocation', null, false, array('Location', 'Keyword')),
			607 => array('SetKeywordDataForLocation', null, false, array('Location', 'Keyword', 'Value')),
			608 => array('GetKeywordDataForAlias', null, false, array('QuestAlias', 'Keyword')),
			609 => array('SetKeywordDataForAlias', null, false, array('QuestAlias', 'Keyword', 'Value')),
			610 => array('LocAliasHasKeyword', null, false, array('QuestAlias', 'Keyword')),
			611 => array('IsNullPackageData', null, true, array('PackageData (Possibly Null)')),
			612 => array('GetNumericPackageData', null, true, array('PackageData (Numeric)')),
			613 => array('IsFurnitureAnimType', null, true, array('Furniture Anim Type')),
			614 => array('IsFurnitureEntryType', null, true, array('Furniture Entry Type')),
			615 => array('GetHighestRelationshipRank', null, true, array()),
			616 => array('GetLowestRelationshipRank', null, true, array()),
			617 => array('HasAssociationTypeAny', null, true, array('AssociationType')),
			618 => array('HasFamilyRelationshipAny', null, true, array()),
			619 => array('GetPathingTargetOffset', null, true, array('Axis')),
			620 => array('GetPathingTargetAngleOffset', null, true, array('Axis')),
			621 => array('GetPathingTargetSpeed', null, true, array()),
			622 => array('GetPathingTargetSpeedAngle', null, true, array('Axis')),
			623 => array('GetMovementSpeed', null, true, array()),
			624 => array('GetInContainer', null, true, array('TESObjectREFR')),
			625 => array('IsLocationLoaded', null, false, array('Location')),
			626 => array('IsLocAliasLoaded', null, false, array('QuestAlias')),
			627 => array('IsDualCasting', null, true, array()),
			628 => array('DualCast', null, true, array('Magic Item', 'ObjectReferenceID')),
			629 => array('GetVMQuestVariable', null, false, array('Quest', 'VM Variable Name')),
			630 => array('GetVMScriptVariable', null, false, array('ObjectReferenceID', 'VM Variable Name')),
			631 => array('IsEnteringInteractionQuick', null, true, array()),
			632 => array('IsCasting', null, true, array()),
			633 => array('GetFlyingState', null, true, array()),
			634 => array('SetFavorState', null, true, array('Integer')),
			635 => array('IsInFavorState', null, true, array()),
			636 => array('HasTwoHandedWeaponEquipped', null, true, array()),
			637 => array('IsExitingInstant', null, true, array()),
			638 => array('IsInFriendStatewithPlayer', null, true, array()),
			639 => array('GetWithinDistance', null, true, array('ObjectReferenceID', 'Float')),
			640 => array('GetActorValuePercent', null, true, array('Actor Value')),
			641 => array('IsUnique', null, true, array()),
			642 => array('GetLastBumpDirection', null, true, array()),
			643 => array('CameraShake', null, false, array('Float', 'Float')),
			644 => array('IsInFurnitureState', null, true, array('Furniture Anim Type')),
			645 => array('GetIsInjured', null, true, array()),
			646 => array('GetIsCrashLandRequest', null, true, array()),
			647 => array('GetIsHastyLandRequest', null, true, array()),
			648 => array('UpdateQuestInstanceGlobal', null, false, array('Quest', 'Global')),
			649 => array('SetAllowFlying', null, true, array('Integer')),
			650 => array('IsLinkedTo', null, true, array('TESObjectREFR', 'Keyword')),
			651 => array('GetKeywordDataForCurrentLocation', null, true, array('Keyword')),
			652 => array('GetInSharedCrimeFaction', null, true, array('TESObjectREFR')),
			653 => array('GetBribeAmount', null, true, array()),
			654 => array('GetBribeSuccess', null, true, array()),
			655 => array('GetIntimidateSuccess', null, true, array()),
			656 => array('GetArrestedState', null, true, array()),
			657 => array('GetArrestingActor', null, true, array()),
			658 => array('ClearArrestState', null, true, array()),
			659 => array('EPTemperingItemIsEnchanted', null, false, array()),
			660 => array('EPTemperingItemHasKeyword', null, false, array('Keyword')),
			661 => array('GetReceivedGiftValue', null, true, array()),
			662 => array('GetGiftGivenValue', null, true, array()),
			663 => array('ForceLocIntoAlias', null, false, array('Location', 'QuestAlias')),
			664 => array('GetReplacedItemType', null, true, array('Casting Source')),
			665 => array('SetHorseActor', null, true, array('TESObjectREFR')),
			666 => array('PlayReferenceEffect', 'pre', true, array('Reference Effect', 'Time', 'Aim-at target')),
			667 => array('StopReferenceEffect', 'sre', true, array('Reference Effect')),
			668 => array('PlayShaderParticleGeometry', 'pspg', false, array('Form', 'Float')),
			669 => array('StopShaderParticleGeometry', 'sspg', false, array('Form', 'Float')),
			670 => array('ApplyImageSpaceModifierCrossFade', 'imodcf', false, array('Imagespace Modifier ID', 'Float')),
			671 => array('RemoveImageSpaceModifierCrossFade', 'rimodcf', false, array('Float')),
			672 => array('IsAttacking', null, true, array()),
			673 => array('IsPowerAttacking', null, true, array()),
			674 => array('IsLastHostileActor', null, true, array()),
			675 => array('GetGraphVariableInt', null, true, array('String')),
			676 => array('GetCurrentShoutVariation', null, true, array()),
			677 => array('PlayImpactEffect', 'pie', true, array('String', 'String', 'Float', 'Float', 'Float', 'Float', 'Int', 'Int')),
			678 => array('ShouldAttackKill', null, true, array('Actor')),
			679 => array('SendStealAlarm', 'steal', true, array('Actor')),
			680 => array('GetActivationHeight', null, true, array()),
			681 => array('EPModSkillUsage_IsAdvanceSkill', null, false, array('Actor Value')),
			682 => array('WornHasKeyword', null, true, array('Keyword')),
			683 => array('GetPathingCurrentSpeed', null, true, array()),
			684 => array('GetPathingCurrentSpeedAngle', null, true, array('Axis')),
			685 => array('KnockAreaEffect', 'kae', true, array('Float', 'Float')),
			686 => array('InterruptCast', null, true, array()),
			687 => array('AddFormToFormList', null, false, array('Form', 'Form')),
			688 => array('RevertFormList', null, false, array('Form')),
			689 => array('AddFormToLeveledList', null, false, array('Form', 'Form', 'Int', 'Int')),
			690 => array('RevertLeveledList', null, false, array('Form')),
			691 => array('EPModSkillUsage_AdvanceObjectHasKeyword', null, false, array('Keyword')),
			692 => array('EPModSkillUsage_IsAdvanceAction', null, false, array('Skill Action')),
			693 => array('EPMagic_SpellHasKeyword', null, false, array('Keyword')),
			694 => array('GetNoBleedoutRecovery', null, true, array()),
			695 => array('SetNoBleedoutRecovery', null, true, array('Integer')),
			696 => array('EPMagic_SpellHasSkill', null, false, array('Actor Value')),
			697 => array('IsAttackType', null, false, array('Keyword')),
			698 => array('IsAllowedToFly', null, true, array()),
			699 => array('HasMagicEffectKeyword', null, true, array('Keyword')),
			700 => array('IsCommandedActor', null, true, array()),
			701 => array('IsStaggered', null, true, array()),
			702 => array('IsRecoiling', null, true, array()),
			703 => array('IsExitingInteractionQuick', null, true, array()),
			704 => array('IsPathing', null, true, array()),
			705 => array('GetShouldHelp', null, true, array('Actor')),
			706 => array('HasBoundWeaponEquipped', null, true, array('Casting Source')),
			707 => array('GetCombatTargetHasKeyword', 'gcthk', true, array('Keyword')),
			708 => array('UpdateLevel', null, false, array()),
			709 => array('GetCombatGroupMemberCount', 'gcgmc', true, array()),
			710 => array('IsIgnoringCombat', null, true, array()),
			711 => array('GetLightLevel', 'gll', true, array()),
			712 => array('SavePCFace', 'spf', false, array('String')),
			713 => array('SpellHasCastingPerk', null, false, array('Perk')),
			714 => array('IsBeingRidden', null, true, array()),
			715 => array('IsUndead', null, true, array()),
			716 => array('GetRealHoursPassed', null, false, array()),
			717 => array('UnequipAll', null, false, array()),
			718 => array('IsUnlockedDoor', null, true, array()),
			719 => array('IsHostileToActor', null, true, array('Actor')),
			720 => array('GetTargetHeight', null, true, array('TESObjectREFR')),
			721 => array('IsPoison', null, true, array()),
			722 => array('WornApparelHasKeywordCount', null, true, array('Keyword')),
			723 => array('GetItemHealthPercent', null, true, array()),
			724 => array('EffectWasDualCast', null, true, array()),
			725 => array('GetKnockStateEnum', null, true, array()),
			726 => array('DoesNotExist', null, false, array()),
	                );
		$luCTDA = array();
		foreach ($rawCTDA as $index => $idata) {
			$luCTDA[$index] = $idata[0];
		}
		$this->_parentlist->set_lookup('CTDA_function', $luCTDA);
		
		$this->_datadef['any'] = 
			array(// These entries appear in both OB and FO, without any change in meaning
			      'EDID' => array( 'keep' => 2, 'type' => 'string', 'desc' => 'Editor ID'),
			      'FULL' => array( 'keep' => 2, 'type' => 'str_index', 'val' => 'name', 'desc' => 'Full Name'),
			      'SCRI' => array( 'keep' => 0, 'type' => 'formid', 'desc' => 'ScriptID'),
			      'MODL' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path'),
			      'MOD2' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 2'),
			      'MOD3' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 3'),
			      'MOD4' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Model Path 4'),
			      'MODT' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data'),
			      'MO2T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 2'),
			      'MO3T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 3'),
			      'MO4T' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 4'),
			      'ICON' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Icon Path'),
			      'ICO2' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Icon Path 2'),
			
			      // FO additions

			      // FO replacement for ENAM from OB
			      // (except for CREA where EITM points to SPEL rectypes, not ENCH rectypes)
			      // use this as presumed datadef entry since ENAM has too many other uses
			      'EITM' => array( 'keep' => 1, 'val' => 'enchant_id', 'type' => 'formid', 'multi' => 'EITM'),
			      'DESC' => array( 'keep' => 1, 'type' => 'blob_index', 'desc' => 'Description'),
			      // number of KWDA (keyword) records
			      'KSIZ' => array( 'keep' => 0, 'type' => 'long', 'val' => 'nkeyword'),
			      'KWDA' => array( 'keep' => 1,
			                       'multi' => 'KWDA',
			                       'repeat_data' => 4,
			                       'data' => array( array( 'val' => 'keyword', 'type' => 'formid'))),
			      'ETYP' => array( 'keep' => 1, 'type' => 'formid'), //EQUP
			      
			      'MODS' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data S'),
			      'MO2S' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 2S'),
			      'MO3S' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 3S'),
			      'MO4S' => array( 'keep' => -1, 'type' => 'blob', 'desc' => 'Model Data 4S'),
			      'MICO' => array( 'keep' => 0, 'type' => 'string', 'desc' => 'Mini Icon Path'),
			      'YNAM' => array( 'keep' => -1, 'type' => 'formid', 'desc' => 'Pick Up Sound'),
			      // NB ZNAM also used for ECZY refs in CREA, NPC_
			      'ZNAM' => array( 'keep' => -1, 'type' => 'formid', 'desc' => 'Drop Sound'),
			
			      // should add OBND to 'any' but I don't know what it is
			      // fomm describes as unknown blob
			      // but in reality it's always a 12-byte subrecord, containing lots of zeros
			      // add meaning may be record-dependent
			      // (sometimes 2nd chunk is refids, but other times definitely not; ditto 3rd chunk)
			      'VMAD' => array( 'keep' => 0,
			      	               'multi' => 'VMAD',
					       'data' => array( array( 'val' => 'script_version', 'type' => 'vmad'))),
			      // these VMAD subfields are entirely artificial, but it's convenient to use the standard field structure
			      'VMAD-VSCR' => array( 'keep' => 0,
			                            'multi' => 'VMAD-VSCR',
						    'force' => 1, // need to make sure that force is only used if VMAD already exists
						    'data' => array( array( 'val' => 'sid', 'type' => 'ulong', 'extra' => 1))),
			      'VMAD-VSCR-VPRP' => array( 'keep' => 0,
			                                 'multi' => 'VMAD-VSCR-VPRP',
							 'force' => 1,
							 'data' => array( array( 'val' => 'propname', 'type' => 'string', 'extra' => 1),
							                  array( 'val' => 'propvalue', 'type' => 'string', 'extra' => 1))),
			     );
		// FO and OB
		$this->_datadef['ACHR'] =
			array('____' => array( 'keep' => 1,
				               'data' => array( array( 'type' => 'ulong', 'index' => 8, 'flag' => array(9 => 'dead')))),
			      'DATA' => array( 'keep' => 1,
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
			                       'data' => array( array( 'val' => 'enable_parent', 'type' => 'formid'), // parent object/trigger
								array( 'type' => 'ulong', 'flag' => array('opposite_parent', 'pop_in')))),
			      'XAPR' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'activate_parent', 'type' => 'formid'),
			                                        array( 'val' => 'activate_delay', 'type' => 'float'))),
			      'XSCL' => array( 'keep' => 0, 'val' => 'scale', 'type' => 'float'),
		
			      // speculative replacement for XHRS
			      'XHOR' => array( 'keep' => 0, 'val' => 'horse_id', 'type' => 'formid'),

			      'XLCN' => array( 'keep' => 0, 'val' => 'location_id', 'type' => 'formid'),
			      // Unknown additions from FO... others added, too, but even less idea on those
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      // Checks showed REFRs pointing to Markers
			      'XLKR' => array( 'keep' => 0, 'multi' => 'XLKR',
			      	     	       'data' => array(array('type' => 'formid', 'val' => 'XLKR1'), // REFR/ACHR/ACRE
					                       array('type' => 'formid', 'val' => 'XLKR2'))),
			      'XPRD' => array( 'keep' => 0, 'type' => 'float'),
			      'XAPD' => array( 'keep' => 0, 'type' => 'byte'),
			      'XLCM' => array( 'keep' => 0, 'type' => 'ulong', 'lookup' => 'ARCH_lvlmod'),
			     );
		
		$this->_datadef['ACTI'] =
			array('KNAM' => array( 'keep' => 0, 'type' => 'formid'),
			      // action that appears when you hover over item: 'Catch', 'Mine', 'Forge Numbers'
			      'RNAM' => array( 'keep' => 0, 'type' => 'str_index'),
			     );
			
		// ignoring ACTI
		// ADDN added by FO -- particle effect

		// FO and OB
		// Will SR still combine food with alchemy?
		$this->_datadef['ALCH'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'weight', 'type' => 'float'))),
			      // in FO flag changed to type 1=normal;3=food;5=medicine
			      // FO extra fields: withdrawal effect, addiction chance, consume sound
			      'ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'type' => 'ushort', 'flag' => array('!autocalc', 'food')),
			                                        array( 'type' => 'ushort', 'flag' => array(NULL, 'harmful')))),
			      //			                                        array( 'type' => 'ulong', 'val' => 'ENIT1'))),
			      //array( 'type' => 'ulong', 'flag' => array( '!autocalc', 'food')))),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'formid',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      // in FO effectb_id removed (length just 20 bytes); other values match up
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'float'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong'/*, 'lookup' => 'actorlist'*/))),
			      // second definition of FULL
			      // BUT not in FO3 (both FULL and SCIT disappeared)
			      'EFID-FULL' => array( 'keep' => 1,
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      // added by FO
			      // multiple CTDA records possible for a single EFID... have confirmed that CTDAs follow EFIDs
			      'EFID-CTDA' => array( 'keep' => -1,
			                            //'multi' => 'EFID-CTDA',
			                            'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                             array( 'val' => 'cond_value', 'type' => 'float'),
			                                             array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                             array( 'val' => NULL, 'type' => 'ushort'),
			                                             array( 'val' => 'cond_param1', 'type' => 'long'),
			                                             array( 'val' => 'cond_param2', 'type' => 'long'),
			                                             array( 'val' => NULL, 'type' => 'long'),
			                                             array( 'val' => 'function_ref', 'type' => 'long'))),
			     );
		
		// ALOC added by FNV
		// AMEF added by FNV
		
		// FO and OB
		$this->_datadef['AMMO'] =
			// FO appears to have done away with weight, turned damage into a byte? (total DATA-size=13)
			// (although FOMM says final byte is unknown; byte value 1-100)
			array('DATA' => array ('keep' => 1,
			                       'data' => array( array( 'val' => 'projectile', 'type' => 'formid'),
			                                        array( 'type' => 'long', 'flag' => array('resistnw', 'vanish')),
			                                        array( 'val' => 'damage', 'type' => 'float'),
			                                        array( 'val' => 'value_raw', 'type' => 'long'),
			                                        array( 'val' => 'value', 'type' => 'float', 'extra' => 1))),
			     );
		
		// skipping ANIO
		
		// in SR, but none of the items actually exist
		$this->_datadef['APPA'] =
			array('QUAL' => array( 'keep' => 1, 'val' => 'quality', 'type' => 'long'),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		// FO only
		// Parts of ARMO items?
		$this->_datadef['ARMA'] =
			// just randomly seeing what happens if BMDT data plugged in to BODT
			   array('BODT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('head', 'hair', 'body', 'hands', 'forearms', 'amulet', 'ring', 'feet', 'calves', 'shield', 'tail', 'long_hair', 'circlet', 'ears')),
						                array( 'type' => 'ushort', 'flag' => array('decap')),
			                                        array( 'type' => 'ulong', 'flag' => array (NULL, NULL, NULL, NULL, 'unplayable1')),
			                                        array( 'type'=> 'ulong', 'flag' => array('heavy', 'clothing')))),
				'BOD2' => array( 'keep' => 1,
                                               'data' => array( array( 'type' => 'ushort', 'flag' => array('head', 'hair', 'body', 'hands', 'forearms', 'amulet', 'ring', 'feet', 'calves', 'shield', 'tail', 'long_hair', 'circlet', 'ears')),
                                                                array( 'type' => 'ushort', 'flag' => array('decap')),
                                                                array( 'type'=> 'ulong', 'flag' => array('heavy', 'clothing')))),
				   'DNAM' => array( 'keep' => 0, 'type' => 'long', 'factor' => 0.01, 'desc' => 'Damage Resistance (DR)'),
				   'RNAM' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'primary_race'),
			      // Override standard MODL -> formid? (not str_index)
			      'MODL' => array('keep' => 0, 'multi' => 'MODL', 'type' => 'formid', 'val' => 'extra_race'),
			      '____' => array( 'keep' => 1,
                                               'data' => array( array( 'type' => 'ulong', 'index' => 8, 'flag' => array(2 => 'unplayable2')),
								array( 'type' => 'byte', 'val' => 'playable', 'extra' => 1))),
			     );
		// FO and OB
		$this->_datadef['ARMO'] =
			array( 'BODT' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ushort', 'flag' => array('head', 'hair', 'body', 'hands', 'forearms', 'amulet', 'ring', 'feet', 'calves', 'shield', 'tail', 'long_hair', 'circlet', 'ears')),
						                array( 'type' => 'ushort', 'flag' => array('decap')),
			                                        array( 'type' => 'ulong', 'flag' => array (NULL, NULL, NULL, NULL, 'unplayable1')),
			                                        array( 'type'=> 'ulong', 'flag' => array('heavy', 'clothing')))),
				'BOD2' => array( 'keep' => 1,
                                               'data' => array( array( 'type' => 'ushort', 'flag' => array('head', 'hair', 'body', 'hands', 'forearms', 'amulet', 'ring', 'feet', 'calves', 'shield', 'tail', 'long_hair', 'circlet', 'ears')),
                                                                array( 'type' => 'ushort', 'flag' => array('decap')),
                                                                array( 'type'=> 'ulong', 'flag' => array('heavy', 'clothing')))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'skill', 'type' => 'ubyte', 'lookup' => 'ARMO_skill', 'extra' => 1),
			                                        array( 'val' => 'type', 'type' => 'byte', 'lookup' => 'ARMO_type', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'DNAM' => array( 'keep' => 0, 'type' => 'long', 'val' => 'rating', 'factor' => 0.01, 'desc' => 'Damage Resistance (DR)'),
				   'RNAM' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'default_race'),
			      // Override standard MODL -> formid? (not str_index)
			      'MODL' => array('keep' => 0, 'multi' => 'MODL', 'type' => 'formid', 'val' => 'ARMA'),
			      'TNAM' => array('keep' => 0, 'val' => 'template_id', 'type' => 'formid'),
			      '____' => array( 'keep' => 0,
                                               'data' => array( array( 'type' => 'ulong', 'index' => 8, 'flag' => array(2 => 'unplayable2')),
								array( 'type' => 'byte', 'val' => 'playable', 'extra' => 1))),
			     );
		// ASPC added by FO: space? reverbs?
		// contains RDAT -> points to REGN rectype
		
		// relationships
		$this->_datadef['ASTP'] =
			array('DATA' => array( 'keep' => 1, 'type' => 'ulong', 'flag' => array('related')),
			      'FCHT' => array( 'keep' => 1, 'val' => 'f_child', 'type' => 'string'),
			      'FPRT' => array( 'keep' => 1, 'val' => 'f_parent', 'type' => 'string'),
			      'MCHT' => array( 'keep' => 1, 'val' => 'm_child', 'type' => 'string'),
			      'MPRT' => array( 'keep' => 1, 'val' => 'm_parent', 'type' => 'string'),
			     );
		// actor value info
		// FO addition (replacement for SKIL?)
		$this->_datadef['AVIF'] =
			array('ANAM' => array( 'keep' => 0,
			                       'type' => 'str_index',
			                       'desc' => 'Alternate Name'),
				   'PNAM' => array( 'keep' => 0, 'multi' => 'PNAM', 'type' => 'formid', 'val' => 'perk')
			     );
		
		// FO and OB; no changes (except FO adds a separate NOTE rectype)
		// NB Tomes don't have any special flags ID'ing them as Tomes -- just have a SCRI that does the spell part
		$this->_datadef['BOOK'] =
			array('CNAM' => array( 'keep' => 1, 'type' => 'str_index', 'val' => 'alt_name'),
			      'DATA' => array( 'keep' => 1,
			                       // DATA0: flags: 1=skillbook, FF00/65280=note, FF04/65284=spelltome
			                       'data' => array( array( 'type' => 'long', 'val' => 'type_raw'), // may be flags: 0, 1, FF00, FF04
			                                        // I think this is a multipurpose value, actorlist and spell list
			                                        array( 'type' => 'long', 'val' => 'skillspell'),
			                                        array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'type', 'type' => 'ubyte', 'lookup' => 'BOOK_type', 'extra' => 1),
			                                        array( 'val' => 'skill', 'type' => 'ubyte', 'lookup' => 'actorlist', 'extra' => 1),
			                                        array( 'val' => 'spell_id', 'type' => 'formid', 'extra' => 1),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		
		// FO addition: BPDT (body part data?)
		
		// FO addition: CAMS (cameras)
		// FNV: addition: CCRD (cards), CDCK (deck of cards)
		
		// FO and OB
		$this->_datadef['CELL'] =
			// Note DATA only has 1 byte -- rest of this is extra data
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('interior', 'haswater', NULL, NULL, NULL, 'public', NULL, NULL)),
			                                        array( 'val' => 'exterior', 'type' => 'boolean', 'extra' => 1),
			                                        array( 'val' => 'worldtype', 'lookup' => 'Worldtype', 'extra' => 1),
			                                        array( 'val' => 'worldspace', 'type' => 'ubyte', 'lookup' => 'Worldspace', 'extra' => 1),
			                                        array( 'val' => 'parentcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'world_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extloc_x', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extloc_y', 'type' => 'float', 'extra' => 1),
			                                        array( 'val' => 'extcell_id', 'type' => 'edid', 'extra' => 1),
			                                        array( 'val' => 'extdoor_id', 'type' => 'edid', 'extra' => 1))),
			      'XCCM' => array( 'keep' => 0, 'type' => 'formid'),
			      'XCLC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'cell_x', 'type' => 'long'),
			                                        array( 'val' => 'cell_y', 'type' => 'long'))),
			      // can be up to 52 bytes long? recpeating formids? (doesn't actually repeat at field level)
			      'XCLR' => array( 'keep' => 0,
						              'repeat_data' => 4,
						              'multi' => 'XCLR',
                                'data' => array( array('val' => 'XLCR', 'type' => 'formid'))),
			      'XCLW' => array( 'keep' => 1,
			                       'val' => 'water_height',
			                       'type' => 'float',
			                       'default' => 0),
			      'XCWT' => array( 'keep' => 0, 'type' => 'formid'), // water
			      'XOWN' => array( 'keep' => 1,
			                       'val' => 'owner_id',
			                       'type' => 'formid'),
			      // FO additions
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XCIM' => array( 'keep' => 0, 'type' => 'formid'), // image?
			      'XCMO' => array( 'keep' => 0, 'type' => 'formid'), // music
			      'XCAS' => array( 'keep' => 0, 'type' => 'formid'),
			      'XLCN' => array( 'keep' => 1, 'val' => 'location_id', 'type' => 'formid'),
			     );
		
		$this->_datadef['CLAS'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'long', 'val' => NULL), // flags???
			                                        array( 'type' => 'ubyte', 'val' => 'trainskl', 'lookup' => 'skilllist'),
			                                        array( 'type' => 'ubyte', 'val' => 'trainlvl'),
			                                        array( 'vallist' => 'skilllist', 'valsuffix' => '_wt', 'type' => 'ubyte'),
			                                        array( 'type' => 'float', 'val' => 'bleedout'),
			                                        array( 'type' => 'long', 'val' => 'voice'),
			                                        array( 'type' => 'byte', 'val' => 'health_wt'), //(does not included fixed 5 health/level)
			                                        array( 'type' => 'byte', 'val' => 'magicka_wt'),
			                                        array( 'type' => 'byte', 'val' => 'fatigue_wt'),
			                                        array( 'type' => 'byte', 'val' => NULL)))
			     );
		// skipping CLMT too
		// FNV additions: CHAL, CHIP
		
		$this->_datadef['COBJ'] =
			array('BNAM' => array('keep' => 0, 'type' => 'formid', 'val' => 'made_at'),
			      'CNAM' => array('keep' => 1, 'type' => 'formid', 'val' => 'created'),
			      // COCT is just the count of CNTO fields
			      //			      'COCT' => array( 'keep' => 0, 'type' => 'long'),
			      'NAM1' => array( 'keep' => 0, 'type' => 'short', 'val' => 'create_count'),
			      'CNTO' => array( 'keep' => 1,
			                       'multi' => 'CNTO',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
			                                        array( 'val' => 'item_count', 'type' => 'long'))),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			     );
		// FO and OB
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
			      'CNTO-COED' => array( 'keep' => 0,
			                            'multi' => 'CNTO',
			                            'data' => array( array( 'index' => 8, 'val' => 'item_hlth', 'type' => 'float', 'desc' => 'Item Condition'))),
			     );
		
		// FO addition: CPTH (more cams?)		
		
		// skipping CSTY (combat styles)
		// FNV addition: CSNO (casino)
		// FO addition: DEBR (debris?)
		// FNV addition: DEHY (dehydration)
		
		// FO and OB
		$this->_datadef['DIAL'] =
			array('QNAM' => array( 'keep' => 1, 'val' => 'quest', 'type' => 'formid'),
			      'BNAM' => array( 'keep' => 1, 'val' => 'branch', 'type' => 'formid'),
			      'SNAM' => array( 'keep' => 1, 'type' => 'string'),
			      'TIFC' => array( 'keep' => 1, 'type' => 'long'),
			      // now 4bytes
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ubyte'))),
			
			      // FO additions
			      'PNAM' => array( 'keep' => 0, 'val' => 'priority', 'type' => 'float', 'desc' => 'Topic Priority'),
			     );
		
		$this->_datadef['DLBR'] =
			array('DNAM' => array( 'keep' => 1, 'type' => 'long'),
			      'QNAM' => array( 'keep' => 1, 'type' => 'formid'),
			      'SNAM' => array( 'keep' => 1, 'type' => 'formid'),
			      'TNAM' => array( 'keep' => 1, 'type' => 'long'),
			     );
		$this->_datadef['DLVW'] =
			array('DNAM' => array( 'keep' => 1, 'type' => 'byte'),
			      'QNAM' => array( 'keep' => 1, 'val' => 'quest_id', 'type' => 'formid'),
			      'BNAM' => array( 'keep' => 1, 'multi' => 'BNAM', 'val' => 'branch', 'type' => 'formid'),
			      'TNAM' => array( 'keep' => 1, 'multi' => 'TNAM', 'type' => 'formid'),
			     );
		// FO addition: DOBJ -- default object manager.  Only one record
		
		// temporary to test structure of DNAM
		// content3 seems to be unable to handle DOBJ data (triggers out-of-memory errors)
		/*$this->_datadef['DOBJ'] =
			array('DNAM' => array( 'keep' => 1,
			                       'multi' => 'DNAM',
			                       'repeat_data' => 8,
			                       'data' => array( array( 'val' => 'test_str', 'len' => 4, 'type' => 'string'),
			                                        //array( 'val' => 'test_val', 'type' => 'long'),
			                                        array( 'val' => 'test_id', 'type' => 'formid'))));*/
		// FO and OB, but no info in fomm.xml
		// deleted sounds: ANAM, SNAM; deleted unknown OB entries BNAM (not in FO)
		$this->_datadef['DOOR'] =
			array('FNAM' => array( 'keep' => 1,
			                       // up to 32 now
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array(NULL, 'autodoor', 'hidden', 'minimal_use')))),
			     );		
		// FO addition: ECZN (encounter zone?)
		$this->_datadef['ECZN'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'owner_id', 'type' => 'formid'),
					                        array( 'val' => 'location_id', 'type' => 'formid'),
							        array( 'val' => 'owner_rank', 'type' => 'byte'),
								array( 'val' => 'min_level', 'type' => 'byte'),
								array( 'type' => 'ubyte', 'flag' => array('never_resets', 'match_PC_below_min', 'disable_combat_boundary')),
								array( 'val' => 'max_level', 'type' => 'byte'))));
		
		// skipping EFSH (effect shaders)
		
		// Enchantment
		// FO and OB
		$this->_datadef['ENCH'] =
			array('ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'enchantvalue_raw', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc', NULL, 'extend')),
			                                        array( 'type' => 'ulong', 'val' => 'casting_type', 'lookup' => 'casting_type'),
			                                        array( 'val' => 'enchantcost_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'delivery_type', 'type' => 'ulong', 'lookup' => 'delivery_type'),
			                                        array( 'val' => 'enchant_type', 'type' => 'ulong'),
			                                        array( 'val' => 'charge_time', 'type' => 'float'),
			                                        array( 'val' => 'base_ench', 'type' => 'formid'),
			                                        array( 'val' => 'slots', 'type' => 'formid'),
			                                      )),
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'formid',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'float'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'))),
//second definition of FULL -- need to accommodate in coding
			      // not used in FO
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			      // added by FO
			      // multiple CTDA records possible for a single EFID... have confirmed that CTDAs follow EFIDs
			      'EFID-CTDA' => array( 'keep' => -1,
			                            //'multi' => 'EFID-CTDA',
			                            'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                             array( 'val' => 'cond_value', 'type' => 'float'),
			                                             array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                             array( 'val' => NULL, 'type' => 'ushort'),
			                                             array( 'val' => 'cond_param1', 'type' => 'long'),
			                                             array( 'val' => 'cond_param2', 'type' => 'long'),
			                                             array( 'val' => NULL, 'type' => 'long'),
			                                             array( 'val' => 'function_ref', 'type' => 'long'))),
			      );
		
		// FO addition: EXPL (explosion)
		// skipping EYES
		
		// FO and OB; no fomm data
		// want to set up a way to view all NPCs in a given faction
		$this->_datadef['FACT'] =
			array(// in OB I have this as formid+short, but not sure
			      // switch to FO-based format, although I don't know meaning of final val
			      'XNAM' => array('keep' => 1,
			                      'multi' => 'XNAM',
			                      'data' => array( array( 'val' => 'opp_faction_id', 'type' => 'formid'),
			                                       array( 'val' => 'opp_modifier', 'type' => 'long'),
			                                       array( 'val' => 'combat', 'type' => 'ubyte', 'lookup' => 'FACT_combat'))),
			      // FO also has flags at bits 8 and 9; SR has even more
			      'DATA' => array( 'keep' => 1,
			                       'type' => 'ulong',
			                       'flag' => array('hidden', 'special_combat', NULL, NULL, NULL, NULL, 'track_crime', 'ignore_murder', 'ignore_assault', 'ignorer_stealing', 'ignore_trespass', '!report_members', 'def_crime_gold', 'ignore_pickpocket', 'vendor', 'can_be_owner', 'ignore_werewolf')),
			      'JAIL' => array( 'keep' => 0, 'val' => 'jail', 'type' => 'formid'),
			      'JOUT' => array( 'keep' => 0, 'val' => 'jail_outfit', 'type' => 'formid'),
			      'STOL' => array( 'keep' => 0, 'type' => 'formid'),
			      'PLCN' => array( 'keep' => 0, 'type' => 'formid'),
			      'PLVD' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'PLVD_flag', 'type' => 'long'),
			                                        array( 'val' => 'PLVD_id', 'type' => 'formid'))),
			      'VENC' => array( 'keep' => 0, 'val' => 'merccont_id', 'type' => 'formid'),
			      'VEND' => array( 'keep' => 0, 'val' => 'merc_list', 'type' => 'formid'),
			      'RNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank',
			                       'type' => 'ushort'),
			      'MNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank_male',
			                       'type' => 'str_index'),
			      'FNAM' => array( 'keep' => 0,
			                       'multi' => 'RNAM',
			                       'val' => 'rank_female',
			                       'type' => 'str_index'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			     );
		// OB only
		$this->_datadef['FLOR'] =
			array('PFIG' => array( 'keep' => 1,
			                       'val' => 'ingr_id',
			                       'type' => 'formid'),
			      'PFPC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'spring', 'type' => 'ubyte'),
			                                        array( 'val' => 'summer', 'type' => 'ubyte'),
			                                        array( 'val' => 'fall', 'type' => 'ubyte'),
			                                        array( 'val' => 'winter', 'type' => 'ubyte'))));
		
		// FO only
		$this->_datadef['FLST'] = 
			array('LNAM' => array('keep' => 1,
			                      'val' => 'item_id',
			                      'type' => 'formid',
			                      'multi' => 'LNAM'),
			     );
		
		
		$this->_datadef['FURN'] =
			array('KNAM' => array('keep' => 1,
					      'val' => 'interaction',
					      'type' => 'formid'),
			      'XMRK' => array('keep' => 1,
					      'val' => 'marker',
					      'type' => 'string'),
			     );

		// Global variable
		// According to FOMM, FNAM=115 means FLTV is int; FNAM=102 is float -> but from data anal, all can be extracted as floats
		// FO and OB
		$this->_datadef['GLOB'] =
			array('FNAM' => array( 'keep' => 1,
			                       'type' => 'string',
			                       'len' => 1,
			                       'desc' => 'Value type'),
			      'FLTV' => array( 'keep' => 1,
			                       'type' => 'float',
			                       'desc' => 'Value'));
		// Game setting
		// FO and OB
		$this->_datadef['GMST'] =
			array('DATA' => array( 'keep' => 1,
			                       'type' => 'gmst4',
			                       'desc' => 'Setting value'));
		
		// skipping GRAS, HAIR, IDLE (FO and OB)
		// skipping FO additions: HDPT, IDLM, IMAD, IMGS
		// FNV addition: HUNG (hunger), IMOD (item mods)
		
		// FO and OB
		$this->_datadef['INFO'] =
			// DATA in INFO is only 4 bytes long, appears to be flags
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'type', 'type' => 'ubyte', 'lookup' => 'DIAL_type'),
			                                        array( 'type' => 'ushort', 'flag' => array('goodbye', 'random', 'say_once', NULL, 'refusal', 'random_end', 'rumors')),
			                                        array( 'val' => 'group_id', 'type' => 'formid', 'extra' => 1))),
			      'DNAM' => array( 'keep' => 1,
			                       'type' => 'formid'),
			      'QNAM' => array( 'keep' => 1,
			                       'type' => 'formid'),
			      // in OB, had TRDT/NAM1/NAM2 as independent non-repeating variables
			      // (although that may have been wrong, given missing dialogue)
			      // choosing to use FO-based structure
			      // in FO: single int
			      'ANAM' => array( 'keep' => 0, 'type' => 'formid', 'desc' => 'Next speaker'),
				   'RNAM' => array( 'keep' => 0, 'type' => 'str_index', 'val' => 'response'),
			      'TCLT' => array( 'keep' => 0, 'type' => 'formid', 'desc' => 'Linked To Topic', 'multi' => 'TCLT'),
			      'TWAT' => array( 'keep' => 0, 'type' => 'formid'),
			      'TRDT' => array( 'keep' => 0,
			                       'multi' => 'TRDT',
			                       'subset' => 1,
			                       'desc' => 'Topic Response Data',
			                       'data' => array( array( 'val' => 'emotion_type', 'type' => 'ulong'/*, 'lookup' => 'INFO_emotion'*/),
			                                        array( 'val' => 'emotion_value', 'type' => 'ulong'))),
			      // NAM1/2/3 all repeat --
			      'TRDT-NAM1' => array( 'keep' => 0, 'val' => 'dialogue', 'type' => 'blob_index', 'desc' => 'Dialogue Text'),
			      // fomm says NAM2 is a byte, but anal says blob; ditto NAM3
			      'TRDT-NAM2' => array( 'keep' => 0, 'val' => 'notes', 'type' => 'blob'),
			      // OB and FO values that I'm skipping: CTDA, SCDA, SCHR, SCR0, SCTX
			      // OB-only values: CTDT
			      // FO additions: ANAM, DNAM, KNAM, LNAM, NAM3, NAME, NEXT, RNAM, SNAM, SNDD, TCLF
			      // SNAM -> idle record
			      // NEXT -> separator
			      'TRDT-NAM3' => array( 'keep' => 0, 'type' => 'blob'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'),
								array( 'val' => 'cpar1_formid', 'index' => 12, 'type' => 'formid'))),
			      'CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			     );
		// FO and OB (although only 1 FO record -> which is just 'do not create' placeholder)
		$this->_datadef['INGR'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'ENIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'ENIT0', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array( '!autocalc', 'food')))),
			      // See notes under ENCH about EFID/EFIT changes
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'type' => 'formid',
			                       'val' => 'effect_id',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'float'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_range', 'type' => 'ulong', 'lookup' => 'effect_range'),
			                                             array( 'val' => 'eff_act', 'type' => 'ulong'/*, 'lookup' => 'actorlist'*/))),
//second definition of FULL -- need to accommodate in coding
			      // but removed in FO
			      'EFID-FULL' => array( 'keep' => 1, 
			                            'val' => 'script_name',
			                            'type' => 'string'),
			     );
		
		// skipping FO addition: IPCT, IPDS
		
		// FO and OB -- but no fomm data
		$this->_datadef['KEYM'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		
		$this->_datadef['KYWD'] =
			array('CNAM' => array( 'keep' => 1,
			                       'type' => 'long',
			                       'val' => 'value'));
		// skipping LAND (FO and OB)
		// skipping FO addition LGTM
		
		$this->_datadef['LCTN'] =
// Commented out values seem to contribute to out-of-memory problems on content3
			array('FNAM' => array( 'keep' => 0, 'val' => 'faction', 'type' => 'formid'),
			      // up to 528 bytes?
//			      'LCID' => array( 'keep' => 0,
//			                       'data' => array( array( 'val' => 'LCID0', 'type' => 'formid'),
//			                                        array( 'val' => 'LCID1', 'type' => 'formid'),
//			                                      )),
//			      'LCEC' => array( 'keep' => 0, 'type' => 'formid'), // repeats (0-2 occurs)
			      // up to 264 bytes?
//			      'LCEP' => array( 'keep' => 0, 'type' => 'formid'),
//			      'LCPR' => array( 'keep' => 0,
//						              'multi' => 'LCPR',
//						              'repeat_data' => 12,
//						              'data' => array( array( 'val' => 'LCPR_ACHR', 'type' => 'formid'),
//												             array( 'val' => 'LCPR_CELL', 'type' => 'formid'),
//												             array( 'val' => 'LCPR_ex', 'type' => 'ulong'))),
//			      'LCSR' => array( 'keep' => 0,
//			                       'multi' => 'LCSR',
//			                       'repeat_data' => 16, //9904 bytes max
//			                       // third value seems to always be long=60 (corresponding formid is Skyrim WRLD)
//			                       // fourth value not a formid, probably not flags
//			                       'data' => array( array( 'val' => 'reftype', 'type' => 'formid'),
//			                                        array( 'val' => 'ref_id', 'type' => 'formid'))),
			      'ACSR' => array( 'keep' => 0,
			                       'multi' => 'ACSR',
			                       'repeat_data' => 16,
			                       'data' => array( array( 'val' => 'reftype', 'type' => 'formid'),
			                                        array( 'val' => 'ref_id', 'type' => 'formid'))),
//			      'LCUN' => array( 'keep' => 0,
//						              'multi' => 'LCUN',
//						              'repeat_data' => 12,
//						              'data' => array( array( 'val' => 'LCUN_NPC', 'type' => 'formid'),
//												             array( 'val' => 'LCUN_ACHR', 'type' => 'formid'),
//												             array( 'val' => 'LCUN_LCTN', 'type' => 'formid'))),
			      'LCEP' => array( 'keep' => -1, 'type' => 'formid'),
			      'MNAM' => array( 'keep' => 0, 'val' => 'marker_id', 'type' => 'formid'),
			      'NAM0' => array( 'keep' => 0, 'type' => 'formid'),
			      'NAM1' => array( 'keep' => 0, 'type' => 'formid'),
			      'PNAM' => array( 'keep' => 0, 'type' => 'formid'), // links to hold LCTN type?
			      'RNAM' => array( 'keep' => 0, 'type' => 'float'),
			     );
		// FO and OB, but no fomm data
		$this->_datadef['LIGH'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'time', 'type' => 'long', 'default' => -1),
			                                        array( 'val' => 'radius', 'type' => 'ulong'),
			                                        array( 'val' => 'color', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('dynamic', 'carried', 'negative', 'flicker', NULL, 'default_off', 'flicker slow', 'pulse', 'pulse slow', 'spot light', 'spot shadow')),
			                                        array( 'val' => 'falloff', 'type' => 'float', 'default' => 1),
			                                        array( 'val' => 'fov', 'type' => 'float', 'default' => 90),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'index' => 40),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      // skipping FNAM (OB+FO -> float); SNAM (FO -> refSOUN)
			     );
		// FO and OB, but no fomm data
		$this->_datadef['LSCR'] =
			array(// LNAM(OB+FO) 12 bytes, first4=refCELL/refWRLD
			      // FNV addition
			     );
		// FNV additions: LSCT (load screen type) 
		// skipping LTEX (FO and OB)
		
		// FO and OB -- see notes for LVLC
		$this->_datadef['LVLI'] =
			array('LVLF' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array('alllvl', 'each', 'useall', 'special')),
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
			      // typically, LVLD chancenone set to 100 -> presumably a script is setting
			      // LVLG later in game to make items start appearing only after some goal completed
			      // several LVLI lists can share same global variable
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			      // deleted in FO
			      'DATA' => array( 'keep' => -1)
			     );
		// FO only -- see notes for LVLC
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
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		// OB only -- but changed comparably to LVLC
		$this->_datadef['LVSP'] =
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
			      'LVLG' => array( 'keep' => 0, 'type' => 'formid'),
			     );
		// Game Messages
		// FO only
		$this->_datadef['MESG'] =
			array(// INAM, DNAM, TNAM also exist -- each int with unknown meaning
			      // Each ITXT entry is a possible response to the message
			      // (no ITXTs if message doesn't want a response)
			      'ITXT' => array( 'keep' => 0,
			                       'type' => 'str_index',
			                       'multi' => 'ITXT',
			                       'desc' => 'Button 1 Text'),
			     );
		
		// Magic Effect
		// FO and OB
		$this->_datadef['MGEF'] =
			array(// flag values unconfirmed for FO3 (but other values look comparable)
			      // av_id appears to be identifying this MGEF as the effect at index xx
			      // DATA is 152 bytes long -- I'm skipping most of the data
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('hostile', 'recover', 'detrimental', NULL, 'nohit', NULL, NULL,  NULL, 'dispel', 'nodur', 'nomag', 'noarea', 'fxpersist', 'no_recast', 'gory', 'hidden', NULL, NULL, NULL, NULL, NULL, 'altermag', 'alterdur')),
			                                        array( 'val' => 'basecost', 'type' => 'float'),
			                                        // looks like formid, but pointing to all types of diff rectypes
			                                        array( 'val' => 'secondary', 'type' => 'formid'),
			                                        // looks good
			                                        array( 'val' => 'school', 'type' => 'ulong', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'resist', 'type' => 'long'),
												array( 'val' => 'taper_weight', 'type' => 'float', 'index' => 28),
			                                        array( 'val' => 'spelllevel', 'index' => 40, 'type' => 'long'),
			                                        array( 'val' => 'casting_time', 'index' => 48, 'type' => 'float'),
			                                        array( 'val' => 'taper_curve', 'type' => 'float'),
			                                        array( 'val' => 'taper_duration', 'type' => 'float'),
			                                        array( 'val' => 'MGEF5', 'type' => 'float'),
			                                        array( 'val' => 'MGEF6', 'index' => 104, 'type' => 'float'),
			                                        array( 'val' => 'mgef_perk', 'index' => 136, 'type' => 'formid'),
			                                      )),
			      // replacement for DESC -> str_index confirmed
			      'DNAM' => array('keep' => 0, 'val' => 'DESC', 'type' => 'str_index'),
			      'MDOB' => array('keep' => 0, 'type' => 'formid'),
			     );
		
		// skipping FO addition MICN
		
		// FO and OB; no apparent changes
		$this->_datadef['MISC'] =
			array('____' =>  array('keep' => 1,
                                               'data' => array( array( 'type' => 'ulong', 'index' => 8, 'flag' => array(2 => '!playable')))),
			      'DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))));
		
		// skipping FO additions: MSTT, MUSC, NAVI, NAVM
		// FNV addition: MSET
	
		// FO and OB but no fomm data
		// deleted (non-kept) OB entries: ENAM, FGGA, FGGS, FGTS, FNAM, HNAM, HCLR, LNAM, NIFZ, ZNAM
		// want to set up a way to view all RELA records mentioning this NPC
		$this->_datadef['NPC_'] =
			array('ACBS' => array( 'keep' => 1,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array('female', 'essential', 'chargen', 'respawn', 'autocalc', 'is_unique', 'not_affect_stealth', 'offset', 'audio', NULL, NULL, 'protected', NULL, NULL, 'summonable', NULL, 'no_bleed', NULL, NULL, 'opp_g_anims', 'simple', NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, 'is_ghost', NULL, 'invulnerable')),
			                                        array( 'val' => 'magicka_raw', 'type' => 'short'),
			                                        array( 'val' => 'fatigue_raw', 'type' => 'short'),
			                                        array( 'val' => 'level_raw', 'type' => 'short'),
			                                        array( 'val' => 'calcmin', 'type' => 'ushort'),
			                                        array( 'val' => 'calcmax', 'type' => 'ushort'),
								array( 'val' => 'speed', 'type' => 'short'),
								array( 'val' => 'disposition_raw', 'type' => 'ushort'),
								array( 'type' => 'ushort', 'flag' => array('use_traits', 'use_stats', 'use_factions', 'use_spelllist', 'use_ai_data', 'use_ai_package', NULL, 'use_base_data', 'use_inventory', 'use_scripts', 'use_def_pack', 'use_attack', 'use_keywords')),
								array( 'val' => 'health_raw', 'type' => 'short'),
								array( 'val' => 'bleedout', 'type' => 'ushort'))),
			      // FO has 4 bytes at faction_rank, not just 1, but it's read as a byte (255->-1)
			      'SNAM' => array( 'keep' => 0,
			                       'multi' => 'SNAM',
			                       'data' => array( array( 'val' => 'faction_id', 'type' => 'formid'),
			                                        array( 'val' => 'faction_rank', 'type' => 'byte'))),
			      'SHRT' => array( 'keep' => 1, 'val' => 'short_name', 'type' => 'str_index'),
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
			                       'data' => array( array( 'val' => 'aggression', 'type' => 'ubyte', 'lookup' => 'NPC_aggression'),
			                                        array( 'val' => 'confidence', 'type' => 'ubyte', 'lookup' => 'NPC_confidence'),
			                                        array( 'val' => 'energy', 'type' => 'ubyte'),
			                                        array( 'val' => 'morality', 'type' => 'ubyte', 'lookup' => 'NPC_morality'),
			                                        array( 'val' => 'mood', 'type' => 'ubyte'),
			                                        array( 'val' => 'assistance', 'type' => 'ubyte', 'lookup' => 'NPC_assistance'),
			                                      )),
			      'PKID' => array( 'keep' => 0,
			                       'val' => 'package_id',
			                       'multi' => 'PKID',
			                       'type' => 'formid'),
			      'PNAM' => array( 'keep' => 0,
			                       'multi' => 'PNAM',
			                       'type' => 'formid'),
			      'CNAM' => array( 'keep' => 1,
			                       'val' => 'class_id',
			                       'type' => 'formid'),
			      // skills look good (there are 18 values before 0s start)... but there's a lot more after that
			      // set of mod values for skills?
			      'DNAM' => array( 'keep' => 1,
			                       'data' => array( array( 'vallist' => 'skilllist', 'valsuffix' => '_raw', 'type' => 'ubyte'),
			                                        )),
			      // FO additions
			      // VTCK (refVTYP), EAMT(2bytes), DNAM(?), PNAM(refHDPT), ENAM(refEYES), NAM4/5/6/7
			      // template?
			      'TPLT' => array( 'keep' => 0, 'val' => 'template_id', 'type' => 'formid'),
			      'ANAM' => array( 'keep' => 0, 'type' => 'formid'),
			      'WNAM' => array( 'keep' => 0, 'val' => 'creature_armor', 'type' => 'formid'),
			      'ATKR' => array( 'keep' => 0, 'val' => 'attack_race', 'type' => 'formid'),
			      'ATKD' => array('multi' => 'ATKD',
				              'subset' => 1,
				              'keep' => 0,
					      'data' => array( array( 'val' => 'damage_mult', 'type' => 'float'),
						               array( 'val' => 'attack_pct', 'type' => 'float'),
						               array( 'val' => 'spell_type', 'type' => 'formid'),
						               array( 'type' => 'ulong', 'flag' => array('ignore_weapon', 'bash_attack', 'power_attack', 'left_attack', 'rotating_attack')),
						               array( 'val' => 'attack_angle', 'type' => 'float'),
						               array( 'val' => 'strike_angle', 'type' => 'float'),
						               array( 'val' => 'stagger', 'type' => 'float'),
						               array( 'val' => 'attack_type', 'type' => 'formid'),
						               array( 'val' => 'knockdown', 'type' => 'float'),
						               array( 'val' => 'recovery_time', 'type' => 'float'),
						               array( 'val' => 'fatigue_mult', 'type' => 'float'))),
			      'ATKD-ATKE' => array('multi' => 'ATKD',
					  	   'keep' => 0,
						   'type' => 'string',
						    'val' => 'attack_event'),
			      'ZNAM' => array( 'keep' => 0, 'val' => 'combat_style', 'type' => 'formid'),
			      'DOFT' => array( 'keep' => 0, 'val' => 'default_outfit', 'type' => 'formid'),
			      'SOFT' => array( 'keep' => 0, 'val' => 'sleeping_outfit', 'type' => 'formid'),
			      'DPLT' => array( 'keep' => 0, 'val' => 'default_package', 'type' => 'formid'),
			      'CRIF' => array( 'keep' => 0, 'val' => 'crime_faction', 'type' => 'formid'),
			      'PRKR' => array( 'keep' => 0,
			                       'multi' => 'PRKR',
			                       'data' => array( array('val' => 'perk_id', 'type' => 'formid'),
			                                        array('val' => 'PRKR1', 'type' => 'long'))),
				   'VTCK' => array( 'keep' => 0, 'val' => 'voice_type', 'type' => 'formid'),
			     );
		$this->_datadef['OTFT'] = 
			array('INAM' => array('keep' => 1,
			                      'multi' => 'INAM',
			                      'repeat_data' => 4,
			                      'data' => array( array( 'val' => 'item_id', 'type' => 'formid'))),
			     );
		// FO and OB, but no fomm data
		$this->_datadef['PACK'] =
			array('PKDT' => array( 'keep' => 0,
			                       'data' => array( array( 'type' => 'ulong', 'flag' => array(2=> 'must_complete', 6=>'unlock_start', 7=>'unlock_end', 10=>'once_per_day', 13=>'preferred_speed', 17=>'sneak', 18=>'swim', 20=>'ignore_combat')),
			                                        array( 'type' => 'ubyte', 'flag' => array('is_template')),
			                                        array( 'type' => 'ubyte', 'val' => 'interrupt_override'),
			                                        array( 'type' => 'ubyte', 'val' => 'speed', 'lookup' => 'PACK_speed'),
			                                        array( 'type' => 'ubyte', 'val' => NULL),
								array( 'type' => 'ulong', 'flag' => array(9=>'world_interaction')))),
			      'PSDT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'month', 'type' => 'byte'),
			                                        array( 'val' => 'day', 'type' => 'ubyte', 'lookup' => 'PACK_day'),
			                                        array( 'val' => 'date', 'type' => 'byte'),
			                                        array( 'val' => 'hour', 'type' => 'byte'),
			                                        array( 'val' => 'minute', 'type' => 'byte'),
								array( 'val' => NULL, 'type' => 'byte'),
								array( 'val' => NULL, 'type' => 'short'),
								array( 'val' => 'duration', 'type' => 'long', 'factor' => 0.0166666666),
								)),
			      'QNAM' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'owner_quest'),
/*			      'IDLC' => array( 'keep' => 0, 'type' => 'byte', 'val' => 'idle_count'),
			      'IDLF' => array( 'keep' => 0, 'type' => 'byte', 'val' => 'idle_flag'),
			      'IDLT' => array( 'keep' => 0, 'type' => 'float', 'val' => 'idle_timer'),
			      'IDLA' => array( 'keep' => 1,
			                       'multi' => 'IDLA',
			                       'repeat_data' => 4,
			                       'data' => array( array( 'val' => 'idle', 'type' => 'formid'))),*/
			      'PKCU' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => NULL, 'type' => 'ulong'),
			                                        array( 'val' => 'template_id', 'type' => 'formid'),
								array( 'val' => NULL, 'type' => 'ulong'))),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'CTDA-CIS1' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			      'CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			      'XNAM' => array( 'keep' => 0,
					       'type' => 'byte'),
			      'ANAM' => array( 'keep' => 1,
			                       'multi' => 'ANAM',
					       'val' => 'activity_type',
					       'type' => 'string'),
			      // These CNAM values are also written to by all of PDTO, PLDT, and PTDA
			      'CNAM' => array( 'keep' => 1,
			                       'multi' => 'ANAM',
					       'data' => array( array( 'val' => 'activity_name_ref', 'type' => 'byte', 'extra' => 1),
					                        array( 'val' => 'activity_value', 'type' => 'PACK_CNAM'),
					                        array( 'val' => 'activity_formid', 'type' => 'formid', 'extra' => 1))),
			      'PDTO' => array( 'keep' => 1,
			                       'multi' =>  'ANAM',
			                       'data' => array( array( 'val' => 'topic_subtype', 'type' => 'packid_PDTO'))), // reads 8 bytes
			      'PLDT' => array( 'keep' => 1,
			                       'multi' =>  'ANAM',
			                       'data' => array( array( 'val' => 'PLDT_type', 'type' => 'packid_PLDT', 'lookup' => 'PLDT_type'), // reads 8 bytes
			                                        array( 'val' => 'radius', 'type' => 'ulong'))),
			      'PTDA' => array( 'keep' => 1,
			                       'multi' => 'ANAM',
					       // There is also a count value at bytes 8-11, but it's zero in all cases so far
			                       'data' => array( array( 'val' => 'PTDA_type', 'type' => 'packid_PTDA', 'lookup' => 'PTDA_type'))), // reads 8 bytes
			      'CITC' => array( 'keep' => 1,
			                       'multi' =>  'CITC',
					       'subset' => 1,
			                       'data' => array( array( 'val' => 'activity_type_ref', 'type' => 'string', 'extra' => 1),
					                        array( 'val' => 'CITC', 'type' => 'PACK_CITC'))),
			      'CITC-PNAM' => array( 'keep' => 0, 'multi' => 'CITC', 'type' => 'string', 'val' => 'procedure_type'),
			      'CITC-FNAM' => array( 'keep' => 0, 'multi' => 'CITC', 'type' => 'ulong', 'flag' => array('completes_package')),
			      'CITC-PKC2' => array( 'keep' => 0, 'type' => 'byte', 'multi' => 'CITC-PKC2'),
// POBA -- always
// always 3 INAMs followed by (sometimes SCHR after any/all INAM)
//  PDTO, POEA
//  PDTO, POCA
//  PDTO
			      'CITC-CTDA' => array( 'keep' => 0,
			                       'multi' => 'CITC-CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'UNAM' => array( 'keep' => 0,
			                       'multi' => 'UNAM',
					       'subset' => 1,
					       'val' => 'activity_id',
					       'type' => 'PACK_UNAM'),
			      'UNAM-BNAM' => array( 'keep' => 1,
			                            'multi' => 'UNAM',
						    'val' => 'activity_name',
				   	            'type' => 'string'),
			      'UNAM-PNAM' => array( 'keep' => 1,
			                            'multi' => 'UNAM',
						    'val' => 'PNAM_UNAM',
					            'type' => 'long'), // looks like may be variable-format... string/formid ?
			     );
		
		// Skill Perks
		// FO addition
		$this->_datadef['PERK'] =
			array(// confirmed that initial DATA subrec is 5 bytes, numranks correct
			      'DATA' => array( 'keep' => 1,
			                       // could first byte be placeholder for skill?
			                       // Skyrim version will also need to have prereq perk somehow
			                       'data' => array( array( 'val' => NULL, 'type' => 'byte'), // always 0  
			                                        array( 'val' => 'minlevel', 'type' => 'byte'),
			                                        array( 'val' => 'numranks', 'type' => 'byte'),
			                                        array( 'val' => 'selectable', 'type' => 'boolean'),
			                                        array( 'val' => 'temporary', 'type' => 'boolean'),
			                                        array( 'val' => 'skill', 'type' => 'byte', 'lookup' => 'actorlist', 'extra' => 1))),
			      // some CTDA records are before PRKE loops, others are in PRKE loops (and multiple)
			      // main CTDA record provides:
			      // if cond_function=0x43530115: the skill requirement (cond_value) and skill (cond_param1)
			      // if cond_function=0x435301C0: prereq perk requirement (cond_param1)
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'PRKE' => array( 'keep' => 1,
			                       'multi' => 'PRKE',
			                       'subset' => 1,
			                       // 0=Linked Quest Stage; 1=Linked SPEL; 2=Complex
			                       // 3 bytes of data, not 2?
			                       'data' => array( array( 'val' => 'perk_type', 'type' => 'byte'),
			                                        array( 'val' => 'rank_index', 'type' => 'byte'))),
			      //PRKE-DATA format depends upon perk_type
			      //need to have this entry here to ensure that DATA records aren't misread
			      //looks like technically PKRE section extends until a PKRF record is found
			      // -- probably do same way as CLAS/DATA in OB with len1/data1, len2/data2, len3/data3
			      // -> csread needs to be able to handle len3/data3
			      'PRKE-DATA' => array( 'keep' => 0,
			                            'len1' => 3,
			                            // 'Complex' (type=2) Effect Type ([X,3,3] for below affects combat stats depending on X: 0=normal damage  2=critical damage  6==damage to player limbs  8==VATS accuracy  36==critical chance AGAINST player"
			                            // type1 looks like an av_id -> actor value that is altered by perk
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
			      // on/off flag?
			      // either have PRKC *or* EPFT+EPFD
			      'PRKE-PRKC' => array('keep' => 0, 'type' => 'byte'),
			      'PRKE-EPFT' => array('keep' => 0, 'type' => 'byte'),
			      // can be 4-8 bytes
			      'PRKE-EPFD' => array('keep' => 0,
			                           // I'm betting actual mag is perk_mag*perk_factor
			                           'data' => array( array( 'val' => 'perk_mag', 'type' => 'float'),
			                                            array( 'val' => 'perk_factor', 'type' => 'float'))),
			      // EPFT, EPFD, PRKC in PRKE loop
			      // PRKF appears to be end-of-set marker
			     );
		
		// skipping PGRD (OB only), PGRE/PROJ/PWAT (FO only)

		// placed grenades
		$this->_datadef['PGRE'] =
			array('DATA' => array( 'keep' => 1,
				'data' => array( array( 'val' => 'x', 'type' => 'float'),
                                                                array( 'val' => 'y', 'type' => 'float'),
                                                                array( 'val' => 'z', 'type' => 'float'),
                                                                array( 'val' => 'rx', 'type' => 'float'),
                                                                array( 'val' => 'ry', 'type' => 'float'),
                                                                array( 'val' => 'rz', 'type' => 'float'))),
                              'XSCL' => array( 'keep' => 1,
                                               'val' => 'scale',
                                               'type' => 'float'),
                              'NAME' => array( 'keep' => 1,
                                               'data' => array( array( 'val' => 'base_id', 'type' => 'formid'),
                                                                array( 'val' => 'cell_id', 'type' => 'formid', 'extra' => 1))),
			      'XOWN' => array( 'keep' => 0,
					       'val' => 'owner',
					       'type' => 'formid'),
                              'XESP' => array( 'keep' => 0,
                                               'data' => array( array( 'val' => 'enable_parent', 'type' => 'formid'),
                                                                array( 'type' => 'ulong', 'flag' => array('opposite_parent', 'pop_in')))),
								);

		// placed hazards
		$this->_datadef['PHZD'] =
			array('DATA' => array( 'keep' => 1,
				'data' => array( array( 'val' => 'x', 'type' => 'float'),
                                                                array( 'val' => 'y', 'type' => 'float'),
                                                                array( 'val' => 'z', 'type' => 'float'),
                                                                array( 'val' => 'rx', 'type' => 'float'),
                                                                array( 'val' => 'ry', 'type' => 'float'),
                                                                array( 'val' => 'rz', 'type' => 'float'))),
                              'XSCL' => array( 'keep' => 1,
                                               'val' => 'scale',
                                               'type' => 'float'),
                              'NAME' => array( 'keep' => 1,
                                               'data' => array( array( 'val' => 'base_id', 'type' => 'formid'),
                                                                array( 'val' => 'cell_id', 'type' => 'formid', 'extra' => 1))),
                              'XESP' => array( 'keep' => 0,
                                               'data' => array( array( 'val' => 'enable_parent', 'type' => 'formid'),
                                                                array( 'type' => 'ulong', 'flag' => array('opposite_parent', 'pop_in')))),
								);

		// FO and OB
		// even in OB, my handling of QUST is crude
		$this->_datadef['QUST'] =
			array('FLTR' => array( 'keep' => 0, 'type' => 'string', 'val' => 'filter'),
			      'ANAM' => array( 'keep' => 1, 'type' => 'long', 'val' => 'alias_count'),
			      'DNAM' => array( 'keep' => 0,
			                       'data' => array( array( 'type' => 'ubyte', 'flag' => array('start_game', NULL, NULL, 'repeat_stages')),
					                        array( 'type' => 'ubyte', 'flag' => array('run_once')),
					                        array( 'val' => 'priority', 'type' => 'ubyte'),
					                        array( 'val' => NULL, 'type' => 'ubyte'),
					                        array( 'val' => NULL, 'type' => 'long'), // always 0
								array( 'val' => 'quest_type', 'type' => 'long', 'lookup' => 'QUST_type'),
								array( 'val' => 'fragment_sid', 'type' => 'ulong', 'extra' => 1))),
			      'QTGL' => array( 'keep' => 0, 'multi' => 'QTGL', 'type' => 'formid', 'val' => 'display_globals'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			      'INDX' => array( 'keep' => 1,
			                       'multi' => 'INDX',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'INDX', 'type' => 'ushort'),
			                                        array( 'type' => 'byte', 'flag' => array('start', 'shut_down')),
			                                        array( 'type' => 'byte', 'val' => 'INDX0'),
								array( 'type' => 'ushort', 'val' => 'fragment_num', 'extra' => 1),
								array( 'type' => 'ulong', 'val' => 'fragment_loc', 'extra' => 1),
								array( 'type' => 'ulong', 'val' => 'fragment_len', 'extra' => 1))),
			      'INDX-QSDT' => array( 'keep' => 0,
			                       'multi' => 'INDX-QSDT',
			                       'type' => 'ubyte',
			                       'subset' => 1,
			                       'flag' => array('complete', 'fail')),
			      'INDX-QSDT-CNAM' => array( 'keep' => 1,
			                                 'multi' => 'INDX-QSDT',
			                                 'type' => 'blob_index'),
			      'INDX-QSDT-NAM0' => array( 'keep' => 0, 'multi' => 'INDX-QSDT', 'type' => 'formid'),
			      'INDX-QSDT-QNAM' => array( 'keep' => 0, 'multi' => 'INDX-QSDT', 'type' => 'formid'),
			      'INDX-QSDT-SCHR' => array( 'keep' => -1,
			                                 'multi' => 'INDX-QSDT'),
			      'INDX-QSDT-SCTX' => array( 'keep' => 0,
			                                 'multi' => 'INDX-QSDT',
			                                 'type' => 'blob_index'),
			      'INDX-QSDT-CTDA' => array( 'keep' => 0,
			                       'multi' => 'INDX-QSDT-CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'INDX-QSDT-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'INDX-QSDT-CTDA'),
			      'QOBJ' => array( 'keep' => 1, 'multi' => 'QOBJ', 'type' => 'ushort', 'subset' => 1),
			      'QOBJ-NNAM' => array( 'keep' => 0, 'multi' => 'QOBJ', 'type' => 'blob_index'),
			      'QOBJ-FNAM' => array( 'keep' => 0, 'multi' => 'QOBJ', 'type' => 'long', 'flag' => array('combine_as_or')),
			      'QOBJ-QSTA' => array( 'keep' => 0, 'multi' => 'QOBJ-QSTA', 'subset' => 1,
			                            'data' => array( array( 'val' => 'QSTA0', 'type' => 'ulong'),
						                     array( 'val' => 'QSTA1', 'type' => 'ulong'))),
			      'QOBJ-QSTA-CTDA' => array( 'keep' => 0,
			                       'multi' => 'QOBJ-QSTA-CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'QOBJ-QSTA-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'QOBJ-QSTA-CTDA'),
			      // ALST: Fill Type = Reference -> ALFL probably non-existent
			      'ALST' => array( 'keep' => 0, 'multi' => 'ALST', 'type' => 'long', 'subset' => 1),
			      'ALST-VSCR' => array( 'keep' => 0, 'multi' => 'ALST-VSCR', 'force' => 1, 'val' => NULL),
			      'ALST-VSCR-VPRP' => array( 'keep' => 0, 'multi' => 'ALST-VSCR-VPRP', 'force' => 1, 'val' => NULL),
			      'ALST-ALID' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'string', 'subset' => 1),
			      'ALST-ALID-ALFR' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'specific_reference'),
			      'ALST-ALID-ALUA' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'unique_actor'),
			      'ALST-ALID-ALRT' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'location_ref_type'), // associated with current location?
			      'ALST-ALID-ALCO' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'created_object'),
			      'ALST-ALID-ALEQ' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'external_quest'),
			      'ALST-ALID-ALFE' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'string', 'val' => 'event_type'),
			      'ALST-ALID-FNAM' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long', 'flag' => array('reserves_item', 'optional', 'quest_object', 'reusable', NULL, NULL, 'essential', NULL, NULL, NULL, 'protected')),

			      'ALST-ALID-ALFA' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long'), // part of ALRT? decodes to an alias
			      'ALST-ALID-ALCA' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long'), // part of ALCO; decodes to formid + flag
			      'ALST-ALID-ALCL' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long'), // part of ALCO; probably lookup 1-4
			      'ALST-ALID-ALFD' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long'), // part of ALFE; may be separate bytes
			      'ALST-ALID-ALEA' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'long', 'val' => 'external_aliasid'), // part of ALEQ

			      'ALST-ALID-ALDN' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'display_name'),
			      'ALST-ALID-ALFC' => array( 'keep' => 0, 'multi' => 'ALST-ALID-ALFC', 'type' => 'formid', 'val' => 'faction_id'),
			      'ALST-ALID-ALPC' => array( 'keep' => 0, 'multi' => 'ALST-ALID-ALPC', 'type' => 'formid', 'val' => 'package_id'),
			      'ALST-ALID-ALSP' => array( 'keep' => 0, 'multi' => 'ALST-ALID-ALSP', 'type' => 'formid', 'val' => 'spell_id'),
			      'ALST-ALID-ECOR' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'combat_override'), // package_id
			      'ALST-ALID-KNAM' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid'),
			      'ALST-ALID-VTCK' => array( 'keep' => 0, 'multi' => 'ALST-ALID', 'type' => 'formid', 'val' => 'voice_type'), // "Additional Valid Voice Types for Export"
			      'ALST-ALID-KWDA' => array( 'keep' => 0,
			                                 'multi' => 'ALST-ALID-KWDA',
				                         'repeat_data' => 4,
			                                 'data' => array( array( 'val' => 'keyword', 'type' => 'formid'))),
			      'ALST-ALID-CNTO' => array( 'keep' => 0, 'multi' => 'ALST-ALID-CNTO',
			                                 'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
				                                 array( 'val' => 'item_count', 'type' => 'long'))),
			      'ALST-ALID-CTDA' => array( 'keep' => 0,
			                                 'multi' => 'ALST-ALID-CTDA',
			                                 'subset' => 1,
			                                 'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
				                                 array( 'val' => 'cond_value', 'type' => 'float'),
				                                 array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
				                                 array( 'val' => NULL, 'type' => 'ushort'),
				                                 array( 'val' => 'cond_param1', 'type' => 'long'),
				                                 array( 'val' => 'cond_param2', 'type' => 'long'),
				                                 array( 'val' => NULL, 'type' => 'long'),
				                                 array( 'val' => 'function_ref', 'type' => 'long'))),
			      'ALST-ALID-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'ALST-ALID-CTDA'),
			      // ALLS: Fill Type = Location -> most options probably don't apply
			      'ALLS' => array( 'keep' => 0, 'multi' => 'ALLS', 'type' => 'long', 'subset' => 1),
			      'ALLS-VSCR' => array( 'keep' => 0, 'multi' => 'ALLS-VSCR', 'force' => 1, 'val' => NULL),
			      'ALLS-VSCR-VPRP' => array( 'keep' => 0, 'multi' => 'ALLS-VSCR-VPRP', 'force' => 1, 'val' => NULL),
			      'ALLS-ALID' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'string', 'subset' => 1),
			      'ALLS-ALID-ALFL' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'formid', 'val' => 'specific_location'),
			      'ALLS-ALID-ALFE' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'string', 'val' => 'event_type'),
			      'ALLS-ALID-ALEQ' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'formid', 'val' => 'external_quest'),

			      'ALLS-ALID-FNAM' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'long', 'flag' => array('reserves_item', 'optional', NULL, 'reusable', 16 => 'allow_cleared')),
			      'ALLS-ALID-ALFD' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'long'), // part of ALFE; may be separate bytes
			      'ALLS-ALID-ALEA' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'long', 'val' => 'external_aliasid'), // part of ALEQ
			      'ALLS-ALID-ALDN' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'formid', 'val' => 'display_name'),
			      'ALLS-ALID-ALFC' => array( 'keep' => 0, 'multi' => 'ALLS-ALID-ALFC', 'type' => 'formid', 'val' => 'faction_id'),
			      'ALLS-ALID-ALPC' => array( 'keep' => 0, 'multi' => 'ALLS-ALID-ALPC', 'type' => 'formid', 'val' => 'package_id'),
			      'ALLS-ALID-ALSP' => array( 'keep' => 0, 'multi' => 'ALLS-ALID-ALSP', 'type' => 'formid', 'val' => 'spell_id'),
			      'ALLS-ALID-KNAM' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'formid'),
			      'ALLS-ALID-VTCK' => array( 'keep' => 0, 'multi' => 'ALLS-ALID', 'type' => 'formid', 'val' => 'voice_type'), // "Additional Valid Voice Types for Export"
			      'ALLS-ALID-KWDA' => array( 'keep' => 0,
			                                 'multi' => 'ALLS-ALID-KWDA',
				                         'repeat_data' => 4,
			                                 'data' => array( array( 'val' => 'keyword', 'type' => 'formid'))),
			      'ALLS-ALID-CNTO' => array( 'keep' => 0, 'multi' => 'ALLS-ALID-CNTO',
			                                 'data' => array( array( 'val' => 'item_id', 'type' => 'formid'),
				                                 array( 'val' => 'item_count', 'type' => 'long'))),
			      'ALLS-ALID-CTDA' => array( 'keep' => 0,
			                                 'multi' => 'ALLS-ALID-CTDA',
			                                 'subset' => 1,
			                                 'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
				                                 array( 'val' => 'cond_value', 'type' => 'float'),
				                                 array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
				                                 array( 'val' => NULL, 'type' => 'ushort'),
				                                 array( 'val' => 'cond_param1', 'type' => 'long'),
				                                 array( 'val' => 'cond_param2', 'type' => 'long'),
				                                 array( 'val' => NULL, 'type' => 'long'),
				                                 array( 'val' => 'function_ref', 'type' => 'long'))),
			      'ALLS-ALID-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'ALLS-ALID-CTDA'),
			     ); // multi on INDX-CTDA
		$this->_datadef['RACE'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'skl1', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus1', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl2', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus2', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl3', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus3', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl4', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus4', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl5', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus5', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl6', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus6', 'type' => 'ubyte'),
			                                        array( 'val' => 'skl7', 'type' => 'byte', 'lookup' => 'actorlist'),
			                                        array( 'val' => 'skbonus7', 'type' => 'ubyte'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'height_m', 'type' => 'float'),
			                                        array( 'val' => 'height_f', 'type' => 'float'),
			                                        array( 'val' => 'weight_m', 'type' => 'float'),
			                                        array( 'val' => 'weight_f', 'type' => 'float'),
			                                        array( 'type' => 'ushort', 'flag' => array( 'playable', 'facegen_head', 'child', NULL, NULL, 'no_shadow','swims','flies','walks','immobile', NULL, 'no_water_combat')),
								array('type' => 'ushort', 'flag' => array(NULL, NULL, NULL,'regen_hp_in_combat',NULL,'pc_dialogue',NULL,'allow_pickpocket')),
								array('type' => 'float', 'val' => 'health_raw'),
								array('type' => 'float', 'val' => 'magicka_raw'),
								array('type' => 'float', 'val' => 'fatigue_raw'),
								array('type' => 'float', 'val' => 'encumbrance_raw'),
								array('type' => 'float', 'val' => 'mass_raw'),
								array('type' => 'float', 'val' => 'accel_rate'),
								array('type' => 'float', 'val' => 'decel_rate'),
								array('type' => 'long', 'val' => 'size', 'lookup' => 'RACE_size'),
								array('type' => 'long', 'val' => NULL),
								array('type' => 'long', 'val' => NULL),
								array('type' => 'float', 'val' => 'injured_health'),
								array('type' => 'long', 'val' => NULL),
								array('type' => 'float', 'val' => 'health_regen'),
								array('type' => 'float', 'val' => 'magicka_regen'),
								array('type' => 'float', 'val' => 'fatigue_regen'),
								array('type' => 'float', 'val' => 'unarmed_damage'),
								array('type' => 'float', 'val' => 'unarmed_reach'),
								array('type' => 'long', 'val' => NULL),
								array('type' => 'float', 'val' => 'aim_angle_tolerance'),
								array('type' => 'float', 'val' => 'flight_radius'),
								array('type' => 'float', 'val' => 'angular_accel'),
								array('type' => 'float', 'val' => 'angular_tolerance'),
								array('type' => 'long', 'val' => NULL))),
			      'SPLO' => array( 'keep' => 1,
			                       'type' => 'formid',
			                       'val' => 'spell_id',
			                       'multi' => 'SPLO'),
			      'MTNM' => array('keep' => 0, 'type' => 'string', 'multi' => 'MTNM', 'val' => 'move_type'),
			      // subrecs not read in OB: CNAM, DNAM, ENAM, FGGA, FGGS, FGTS, FNAM, HNAM, MNAM, INDX,
			      // NAM0, NAM1, PNAM, UNAM, VNAM
			      // added by FO: YNAM, NAM2, VTCK(refVTYP), ONAM
				   'VNAM' => array('keep' => 0, 'type' => 'long'),
				   'NAM8' => array('keep' => 0, 'type' => 'formid', 'val' => 'morph_race'),
				   'RNAM' => array('keep' => 0, 'type' => 'formid', 'val' => 'armor_race'),
				   'DNAM' => array('keep' => 0,
				   	           'data' => array( array( 'val' => 'decap_armor_m', 'type' => 'formid'),
						                    array( 'val' => 'decap_armor_f', 'type' => 'formid'))),
				   'VTCK' => array('keep' => 0,
				   	           'data' => array( array( 'val' => 'voice_type_m', 'type' => 'formid'),
						                    array( 'val' => 'voice_type_f', 'type' => 'formid'))),
				   'WNAM' => array('keep' => 0, 'type' => 'formid', 'val' => 'skin'),
				   'QNAM' => array('keep' => 0, 'type' => 'formid', 'val' => 'equip_slot', 'multi' => 'QNAM'),
				   'UNES' => array('keep' => 0, 'type' => 'formid', 'val' => 'unarmed_equip_slot'),
				   'ATKD' => array('multi' => 'ATKD',
				                   'subset' => 1,
				                   'keep' => 0,
						   'data' => array( array( 'val' => 'damage_mult', 'type' => 'float'),
							                 array( 'val' => 'attack_pct', 'type' => 'float'),
							                 array( 'val' => 'spell_type', 'type' => 'formid'),
							                 array( 'type' => 'ulong', 'flag' => array('ignore_weapon', 'bash_attack', 'power_attack', 'left_attack', 'rotating_attack')),
							                 array( 'val' => 'attack_angle', 'type' => 'float'),
							                 array( 'val' => 'strike_angle', 'type' => 'float'),
							                 array( 'val' => 'stagger', 'type' => 'float'),
							                 array( 'val' => 'attack_type', 'type' => 'formid'),
							                 array( 'val' => 'knockdown', 'type' => 'float'),
							                 array( 'val' => 'recovery_time', 'type' => 'float'),
							                 array( 'val' => 'fatigue_mult', 'type' => 'float'))),
				   'ATKD-ATKE' => array('multi' => 'ATKD',
					     	        'keep' => 0,
						        'type' => 'string',
						        'val' => 'attack_event'),
			     );
		
		// skipping FO addition RADS
		
		// FO and OB, but no fomm data
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
			                       'flag' => array('visible', 'travel', 'hidden')),
			      'XOWN' => array( 'keep' => 1,
			                       'val' => 'owner_id',
			                       'type' => 'formid'), // it's not getting ID'd as a formid though
			      'XRNK' => array( 'keep' => 1,
			                       'val' => 'owner_rank',
			                       'type' => 'ulong'),
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
								array( 'val' => 'no_alarm', 'type' => 'ulong'))),
			      /*			      'XTRG' => array( 'keep' => 0,
			                       'val' => 'parent_id',
			                       'type' => 'formid'),*/
			      'XLOC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'lock_level', 'type' => 'ubyte'),
			                                         array( 'val' => 'key_id', 'index' => 4, 'type' => 'formid'),
			                                         array( 'type' => 'ulong', 'flag' => array(NULL, NULL, 'leveled')))),
			      // copied from ACHR/ACRE, just blindly assuming it's the same for REFR
			      'XESP' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'enable_parent', 'type' => 'formid'), // parent object/trigger
                                                                array( 'type' => 'ulong', 'flag' => array('opposite_parent', 'pop_in')))),
			      'XCNT' => array( 'keep' => 1,
			                       'val' => 'count',
			                       'type' => 'ulong'),
			      'TNAM' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'marker',
			                       /*'lookup' => 'TNAM_type'*/),
			      'ONAM' => array( 'keep' => 0,
			                       'val' => 'default_open',
			                       'type' => 'boolean'),
			      // OB entries dropped: XACT, XLCM, XLOD, XPCI, XRTM, XSED, XTRG
			      
			      // Unknown additions from FO... others added, too, but even less idea on those
			      // XNDP(refNAVM), XLTW(refREFR-PWAT), XRDS(float), XEMI(refLIGH,refREGN), XPWR, XPRD, etc.
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'), // some type of location
			      'XLIB' => array( 'val' => 'levelitem_id', 'keep' => 0, 'type' => 'formid'), // leveled item base object
			      'XLRT' => array( 'keep' => 0, 'type' => 'formid'), // LCRT -- used by quests
			      'XLKR' => array( 'keep' => 0, 'type' => 'formid', 'multi' => 'XLKR'), // REFR/ACHR/ACRE
			      'XTNM' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'display_name'), // MESG
			      'XATR' => array( 'keep' => 0, 'type' => 'formid', 'val' => 'XATR'),
			      'XAPR' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'activate_parent', 'type' => 'formid'),
			                                        array( 'val' => 'activate_delay', 'type' => 'float'))),
			     );
		
		// FNV additions: REPU (want to add once meaning checked) ... plus entries referring to REPU
		// skipping REGN, ROAD (FO+OB), SBSP(OB), FO: RGDL, SCOL
		
		// relationships
		$this->_datadef['RELA'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'parent', 'type' => 'formid'),
			                                        array( 'val' => 'child', 'type' => 'formid'),
			                                        array( 'val' => 'rank', 'type' => 'ushort', 'lookup' => 'RELA_rank'),
								array( 'type' => 'ushort', 'flag' => array(15 => 'secret')),
			                                        array( 'val' => 'assoc', 'type' => 'formid'))),
			     );
			
		$this->_datadef['SCEN'] =
			array('QNAM' => array( 'val' => 'quest', 'keep' => 1, 'type' => 'formid', 'multi' => 'QNAM'),
			      'NAM0' => array( 'val' => 'phase', 'keep' => 1, 'type' => 'string', 'multi' => 'NAM0', 'subset' => 1),
			      'NAM0-CTDA' => array( 'keep' => 0,
			                       'multi' => 'NAM0-CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'NAM0-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'NAM0-CTDA'),
			      'ENAM' => array('val' => 'action', 'keep' => 0, 'type' => 'long', 'multi' => 'ENAM', 'subset' => 1),
			      'ENAM-DATA' => array( 'val' => 'dialogue', 'keep' => 1, 'type' => 'formid'),
);/*			      
'ALID' => array('keep' => 0, 'type' => 'long'), 'ANAM' => array('keep' => 0, 'type' => 'short'),
'DEMO' => array('keep' => 0, 'type' => 'long'),
'DEVA' => array('keep' => 0, 'type' => 'long'),
'DNAM' => array('keep' => 0, 'type' => 'long'),
'DMAX' => array('keep' => 0, 'type' => 'float'),
'DMIN' => array('keep' => 0, 'type' => 'float'),
'SNAM' => array('keep' => 0, 'type' => 'float'),
'FNAM' => array('keep' => 0, 'type' => 'long'),
'HTID' => array('keep' => 0, 'type' => 'long'),
'INAM' => array('keep' => 0, 'type' => 'long'),
'LNAM' => array('keep' => 0, 'type' => 'long'),
'NAM0' => array('keep' => 0, 'type' => 'string'),
'PNAM' => array('keep' => 0, 'type' => 'long'),
'WNAM' => array('keep' => 0, 'type' => 'long'),
			     );
*/

		$this->_datadef['SCRL'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'SPIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'SPIT0', 'type' => 'formid'),
			                                        array( 'val' => 'SPIT1', 'type' => 'formid'),
			                                        array( 'val' => 'SPIT2', 'type' => 'ulong'),
			                                        array( 'val' => 'SPIT3', 'type' => 'float'),
			                                        array( 'val' => 'SPIT4', 'type' => 'ulong'),
			                                        array( 'val' => 'SPIT5', 'type' => 'ulong'),
			                                        array( 'val' => 'SPIT6', 'type' => 'float'),
			                                        array( 'val' => 'SPIT7', 'type' => 'float'),
			                                        array( 'val' => 'perk', 'type' => 'formid'),
			                                        array( 'val' => 'spellschool', 'type' => 'tinyint', 'lookup' => 'actorlist', 'extra' => 1),
			                                        array( 'val' => 'spelllevel', 'type' => 'tinyint', 'lookup' => 'spelllevels', 'extra' => 1))),
			      'MDOB' => array( 'keep' => 0, 'type' => 'formid'), // STAT
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'val' => 'effect_id',
			                       'type' => 'formid',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'float'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             // is this flags?
			                                             // 0x01 = does effect have a duration
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'))),
			      // added by FO
			      // multiple CTDA records possible for a single EFID... have confirmed that CTDAs follow EFIDs
			      'EFID-CTDA' => array( 'keep' => 0,
			                            'multi' => 'EFID-CTDA',
			                            'subset' => 1,
			                            'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                             array( 'val' => 'cond_value', 'type' => 'float'),
			                                             array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                             array( 'val' => NULL, 'type' => 'ushort'),
			                                             array( 'val' => 'cond_param1', 'type' => 'long'),
			                                             array( 'val' => 'cond_param2', 'type' => 'long'),
			                                             array( 'val' => NULL, 'type' => 'long'),
			                                             array( 'val' => 'function_ref', 'type' => 'long'))),
			      'EFID-CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'EFID-CTDA'),
			     );
		
		$this->_datadef['SHOU'] =
			array('MDOB' => array( 'keep' => 0, 'type' => 'formid'), // STAT
			      'SNAM' => array( 'keep' => 1,
			                       'multi' => 'SNAM',
			                       'data' => array (array( 'val' => 'word', 'type' => 'formid'),
			                                        array( 'val' => 'spell', 'type' => 'formid'),
			                                        array( 'val' => 'recharge_time', 'type' => 'float'))),
			     );
		// OB only
		$this->_datadef['SLGM'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'))),
			      'SOUL' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'soul_contained',
			                       'lookup' => 'soullist'),
			      'SLCP' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'val' => 'soul_capacity',
			                       'lookup' => 'soullist'),
			      'NAM0' => array( 'keep' => 0, 'val' => 'filled_gem', 'type' => 'formid'),
			      );
		
		$this->_datadef['SMBN'] =
			array('DNAM' => array( 'keep' => 1, 'type' => 'ulong'),
			      'PNAM' => array( 'keep' => 1, 'val' => 'parent', 'type' => 'formid'),
			      'SNAM' => array( 'keep' => 1, 'val' => 'sibling', 'type' => 'formid'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			     );
		$this->_datadef['SMEN'] =
			array( 'PNAM' => array( 'keep' => 1, 'val' => 'parent', 'type' => 'formid'),
			      'SNAM' => array( 'keep' => 1, 'val' => 'sibling', 'type' => 'formid'),
			      'ENAM' => array( 'keep' => 1, 'val' => 'event_type', 'type' => 'string', 'lookup' => 'SMEN_event_type'),
			     );
		$this->_datadef['SMQN'] =
			array('DNAM' => array( 'keep' => 1,
			                       'data' => array( array('val' => 'DNAM0', 'type' => 'ushort'),
			                                        array('val' => 'DNAM1', 'type' => 'ushort'))),
			      'PNAM' => array( 'keep' => 1, 'val' => 'parent', 'type' => 'formid'),
			      'SNAM' => array( 'keep' => 1, 'val' => 'sibling', 'type' => 'formid'),
			      'NNAM' => array( 'keep' => 1,
			                       'multi' => 'NNAM',
			                       'data' => array( array( 'val' => 'quest_id', 'type' => 'formid')),
						'subset' => 1),
			      'NNAM-RNAM' => array( 'keep' => 1, 'val' => 'reset_time', 'type' => 'float'),
			      'XNAM' => array( 'keep' => 0, 'type' => 'long'),
			      'CTDA' => array( 'keep' => 0,
			                       'multi' => 'CTDA',
			                       'subset' => 1,
			                       'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                        array( 'val' => 'cond_value', 'type' => 'float'),
			                                        array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                        array( 'val' => NULL, 'type' => 'ushort'),
			                                        array( 'val' => 'cond_param1', 'type' => 'long'),
			                                        array( 'val' => 'cond_param2', 'type' => 'long'),
			                                        array( 'val' => NULL, 'type' => 'long'),
			                                        array( 'val' => 'function_ref', 'type' => 'long'))),
			      'CTDA-CIS2' => array( 'keep' => 0, 'type' => 'string', 'multi' => 'CTDA'),
			     );
		// skipping SOUN (FO+OB)
		
		$this->_datadef['SPEL'] =
			array('SPIT' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'spellcost_raw', 'type' => 'ulong'),
			                                        array( 'type' => 'ulong', 'flag' => array('!autocalc')),
			                                        array( 'val' => 'type', 'type' => 'ulong', 'lookup' => 'SPEL_type'),
			                                        array( 'val' => 'charge_time', 'type' => 'float'),
			                                        array( 'val' => 'casting_type', 'type' => 'ulong', 'lookup' => 'casting_type'),
			                                        array( 'val' => 'delivery_type', 'type' => 'ulong', 'lookup' => 'delivery_type'),
			                                        array( 'val' => 'cast_duration', 'type' => 'float'),
			                                        array( 'val' => 'spell_range', 'type' => 'float'),
			                                        array( 'val' => 'perk', 'type' => 'formid'),
			                                        array( 'val' => 'spellschool', 'type' => 'tinyint', 'lookup' => 'actorlist', 'extra' => 1),
			                                        array( 'val' => 'spelllevel', 'type' => 'tinyint', 'lookup' => 'spelllevels', 'extra' => 1))),
			      'CIS2' => array( 'keep' => 0, 'type' => 'string'),
			      'MDOB' => array( 'keep' => 0, 'type' => 'formid'), // STAT
			      'EFID' => array( 'keep' => 1,
			                       'multi' => 'EFID',
			                       'val' => 'effect_id',
			                       'type' => 'formid',
			                       'subset' => 1),
			      'EFID-EFIT' => array( 'keep' => 1,
			                            'data' => array( array( 'val' => 'eff_mag', 'type' => 'float'),
			                                             array( 'val' => 'eff_area', 'type' => 'ulong'),
			                                             array( 'val' => 'eff_dur', 'type' => 'ulong'))),
			      // added by FO
			      // multiple CTDA records possible for a single EFID... have confirmed that CTDAs follow EFIDs
			      'EFID-CTDA' => array( 'keep' => 0,
			                            'multi' => 'EFID-CTDA',
			                            'data' => array( array( 'val' => 'cond_op', 'type' => 'ulong'),
			                                             array( 'val' => 'cond_value', 'type' => 'float'),
			                                             array( 'val' => 'cond_function', 'type' => 'ushort', 'lookup' => 'CTDA_function'),
			                                             array( 'val' => NULL, 'type' => 'ushort'),
			                                             array( 'val' => 'cond_param1', 'type' => 'long'),
			                                             array( 'val' => 'cond_param2', 'type' => 'long'),
			                                             array( 'val' => NULL, 'type' => 'long'),
			                                             array( 'val' => 'function_ref', 'type' => 'long'))),
			     );
		
		// skipping FO+OB: STAT, TES4, TREE, WATR
		// skipping FO: TACT, TERM, TXST, VTYP
		// FNV: SLPD
		
		$this->_datadef['TREE'] =
			array('PFIG' => array( 'keep' => 1,
			                       'val' => 'ingr_id',
			                       'type' => 'formid'),
			      'PFPC' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'spring', 'type' => 'ubyte'),
			                                        array( 'val' => 'summer', 'type' => 'ubyte'),
			                                        array( 'val' => 'fall', 'type' => 'ubyte'),
			                                        array( 'val' => 'winter', 'type' => 'ubyte'))));
		// FO and OB
		// but way more complex in FO....
		$this->_datadef['WEAP'] =
			array('DATA' => array( 'keep' => 1,
			                       'data' => array( array( 'val' => 'value_raw', 'type' => 'ulong'),
			                                        array( 'val' => 'weight', 'type' => 'float'),
			                                        array( 'val' => 'damage', 'type' => 'short'),

			                                        array( 'val' => 'type', 'type' => 'byte', 'lookup' => 'WEAP_type', 'extra' => 1),
			                                        array( 'val' => 'value', 'type' => 'ulong', 'extra' => 1),
			                                        array( 'val' => 'levellist_id', 'type' => 'formid', 'extra' => 1))),
			      'DNAM' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => NULL/*'typeb'*/, 'type' => 'long'),
			                                        array( 'val' => 'speed', 'type' => 'float'),
			                                        array( 'val' => 'reach', 'type' => 'float'),
// no_drop for followers means doesn't appear in follower inventory
			                                        array( 'type' => 'ushort', 'flag' => array(3 => 'no_drop', 7 => 'unplayable')),
			                                        array( 'val' => NULL, 'type' => 'ushort'), // always 145: 128+16+1
			                                        array( 'val' => 'sight_fov', 'type' => 'float'),
			                                        array( 'val' => NULL, 'type' => 'long'), // always 0
// These four bytes are NOT a formid -- its just that there are only two possible values which happen to correspond with two REFR values
			                                        array( 'val' => 'vats_to_hit', 'type' => 'byte'),
			                                        array( 'val' => NULL, 'type' => 'byte'), // always -1
								array( 'val' => 'num_projectiles', 'type' => 'short'),
			                                        array( 'val' => 'min_range', 'type' => 'float'),
			                                        array( 'val' => 'max_range', 'type' => 'float'),
			                                        array( 'val' => NULL, 'type' => 'long'), // always 0
			                                        array( 'val' => NULL, 'type' => 'long'), // flags
			                                        array( 'val' => NULL, 'type' => 'float'), // always 1
			                                        array( 'val' => NULL, 'type' => 'float'), // values from 0.1-5, but not displayed in CK
			                                        array( 'val' => 'rumble_left', 'type' => 'float'),
			                                        array( 'val' => 'rumble_right', 'type' => 'float'),
			                                        array( 'val' => 'rumble_duration', 'type' => 'float'),
								array( 'val' => 'skill_lu', 'type' => 'long', 'index' => 76, 'lookup' => 'actorlist'),
								array( 'val' => NULL, 'type' => 'long', 'index' => 88), // always -1
								array( 'val' => 'stagger', 'type' => 'float', 'index' => 96))),
			      'EAMT' => array( 'keep' => 1, 'type' => 'short', 'val' => 'enchantcharge', 'desc' => 'Enchantment Charge Amount'),
			      'NNAM' => array( 'keep' => 1, 'type' => 'string', 'desc' => 'Projectile Node'),
			      'CNAM' => array( 'keep' => 0, 'val' => 'template_id', 'type' => 'formid'),
			      'VNAM' => array( 'keep' => 0, 'type' => 'ulong'),
			      'CRDT' => array( 'keep' => 0,
			                       'data' => array( array( 'val' => 'crit_damage', 'type' => 'long'),
			                                        array( 'val' => 'crit_mult', 'type' => 'float'),
			                                        array( 'val' => 'CRDT2', 'type' => 'long'))),
			     );
		// Words of Power
		$this->_datadef['WOOP'] =
			array('TNAM' => array('keep' => 0, 'val' => 'translate', 'type' => 'str_index'));
		// FO and OB but no fomm data
		$this->_datadef['WRLD'] =
			array('WNAM' => array( 'keep' => 1, 'val' => 'parent_id', 'type' => 'formid'),
			      'CNAM' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'NAM2' => array( 'keep' => 0,
			                       'type' => 'formid'),
			      'DATA' => array( 'keep' => 1,
			                       'type' => 'ubyte',
			                       'flag' => array( 'small', 'notravel')),
			      'ONAM' => array( 'keep' => 1,
					       'data' => array( array( 'val' => 'map_scale', 'type' => 'float'),
							        array( 'val' => 'offset_x', 'type' => 'float'),
                                                                array( 'val' =>
'offset_y', 'type' => 'float'),
                                                                array( 'val' =>
'offset_z', 'type' => 'float'))),
			      // FO additions
			      'XEZN' => array( 'keep' => 0, 'type' => 'formid'),
			      'XLCN' => array( 'keep' => 0, 'val' => 'location_id', 'type' => 'formid'),
			     );
		
		// skipping WTHR (OB+FO)
	}
	
	protected function _get_cells_separate() {
		return
			array(array('RiftenMercerFreyHouse' => 'RiftenRatway02'),
			      array('RiftenRaggedFlagon' => 'RiftenRatway01'),
			      array('RiftenRaggedFlagon' => 'RiftenRatway02'),
			      array('WinterholdCollegeMidden01' => 'WinterholdCollegeHallofCountenance'),
			      array('Kagrenzel01' => 'StonyCreekCave01'),
			      array('WindhelmBloodworks' => 'WindhelmPalaceoftheKings'),
			      array('WhiterunJail02' => 'WhiterunDragonsreachBasement'),
			      array('BronzeWaterCave' => 'Irkngthand04'),
			           );
	}
	
	protected function _get_cells_preferred() {
		return
			array(// don't use Hall of the Dead
			      'WhiterunHalloftheDeadCatacombs' => array('exit' => '0x00094bd9'),
			      'RiftenTempleofMara' => array('0x0004227b'),
			      'SolitudeHalloftheDeadCatacombs' => array('0x000471c1'),
			      'SnowVeilSanctum01' => array('exit' => '0x00055d0d'),
			      'Irkngthand04' => array('exit' => '0x0005423b'),
			      'DushnikhYalLonghouse' => array('exit' => '0x00019919'),
			      'RiftenRatway01' => array('exit' => '0x00048fa0'),
			      'FellglowKeep02' => array('exit' => '0x00071ae6'),
			      'Valthume01' => array('exit' => '0x0008ce77'),
			      'SightlessPit03' => array('exit' => '0x0002731d'),
			      'RiftenMistveilKeep' => array('exit' => '0x00042282'),
			      'MarkarthUnderStoneKeep' => array('exit' => '0x00098276'),
			     );
	}
}
