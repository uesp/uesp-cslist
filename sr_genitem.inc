<?php
require_once('csitem.inc');

/* Skyrim-specific version of GenItem
*/
class GenItem extends CSItem {
	// Get total value of an item (base value + enchantment value)
	function derive_value() {
		// This is a short-circuit for sake of items where value read from data file is the correct value
		$value = $this->_getexact('value');
		if (!empty($value))
			return $value;
		
		$base_value = $this->get('value_raw');
		$enchants = $this->get('enchant_id');
		if (empty($enchants))
			return $base_value;
		
		if (!is_array($enchants))
			$enchants = array($enchants);

		$enchant_charge = $this->get('enchantcharge');
		$enchant_value = 0;
		foreach ($enchants as $eid) {
			$item = $this->_parentlist->get_item($eid);
			$enchant_value += $item->get('enchantvalue');
			$item->release();
		}
		// +0.5 appears necessary in Skyrim -- but only for apparel, not weapons
		if ($this->_rectype === 'WEAP')
			 $enchant_value = floor(8*$enchant_value);
		else
			 $enchant_value = floor($enchant_value+0.5);
		return $base_value + floor(0.12*$enchant_charge) + $enchant_value;
	}

	public function find_keyword($find_keyword, $exact=FALSE) {
		$nk = $this->get_index('keyword');
		if ($nk===FALSE) {
			if ($exact)
				return NULL;
			else
				return array();
		}
		$find_len = strlen($find_keyword);
		$matches = array();
		foreach ($this->indices('keyword') as $k) {
			$kywd = $this->get('keyword', $k);
			$keyid = $this->_parentlist->get_edid($kywd);
			if ($exact) {
				if ($keyid==$find_keyword)
					return $keyid;
			}
			else {
				if (substr($keyid,0,$find_len)==$find_keyword) {
					$matches[] = $keyid;
				}
				elseif (substr($keyid,0,3)=='DLC' && substr($keyid,4,$find_len)==$find_keyword) {
					$matches[] = $keyid;
				}
			}
		}
		if ($exact)
			return FALSE;
		else
			return $matches;
	}
	
	protected function _llist_text($row, $basetext) {
		if (substr($basetext,-1,1)!=' ')
			$basetext .= ' ';
		$text = '';
		if ($row['countmax']>1) {
			if ($row['countmin']<$row['countmax'])
				$text .= $row['countmin'].'-';
			$text .= $row['countmax'].' sample';
			if ($row['countmax']>1)
				$text .= 's';
			$text .= ' ';
		}
		if ($row['pav']>=1) {
			$text .= $basetext;
			$text = preg_replace('/sample/', 'guaranteed sample', $text);
		}
		elseif ($row['pav_lvl']>=1) {
			$text .= $basetext.'(only at level>='.$row['lmin'].') ';
		}
		elseif ($row['lmin']>1) {
			$text = round($row['pav_lvl']*100).'% chance at level>='.$row['lmin'].' of '.$text.'being '.$basetext;
		}
		else {
			$text = round($row['pav']*100).'% chance of '.$text.'being '.$basetext;
		}
		return ucfirst($text);
	}
	
	protected function _get_merctype($ordid) {
		$edid = $this->_parentlist->get_edid($ordid);
		$merctype = strtolower(preg_replace('/VendorItems/', '', $edid));
		if ($merctype=='misc')
			$merctype = 'general goods';
		
		if ($merctype=='db')
			return '[[Babette]]';
		elseif ($merctype=='innkeeperorgnar')
			return 'the [[Sleeping Giant Inn]]';
		return 'all '.$merctype.' merchants';
	}
	protected function _get_mercname($ordid) {
		$rectype = $this->_parentlist->get_rectype($ordid);
		$edid = $this->_parentlist->get_edid($ordid);
		
		$mercnames = array('Nelacar',
		                   'Wuunferth' => 'Wuunferth the Unliving',
		                   'Dravynea' => NULL,//'Dravynea the Stoneweaver', // Dravynea is broken
		                   'Farengar' => 'Farengar Secret-Fire',
		                   'Sybille' => 'Sybille Stentor',
		                   'Wylandriah',
		                   'Madena',
		                   'Enthir',
		                   'Falion',
		                   'MarkarthWizard' => 'Calcelmo',
		                   'Riverwood' => 'Riverwood Trader',
		                   'Colette' => 'Colette Marence',
		                   'Drevis' => 'Drevis Neloren',
		                   'Faralda',
		                   'Phinis' => 'Phinis Gestor',
		                   'Tolfdir',
		                   'Spouse' => NULL,
		                   'Heljarchen' => NULL,
		                  );
		$name = $query = NULL;
		foreach ($mercnames as $key => $value) {
			if (is_int($key))
				$key = $value;
			if (strpos($edid, $key)!==FALSE) {
				if (!isset($value))
					return NULL;
				return
					'[['.$value.']]';
				break;
			}
		}
		
		if ($rectype=='FACT') {
			$query = 'SELECT cell_id FROM FACT_Record inner join REFR_Record on (FACT_Record.merccont_id=REFR_Record.ordid) where FACT_Record.ordid='.$ordid;
		}
		if (!empty($query)) {
			$res = $this->do_query($query);
			if ($res->num_rows==1) {
				$row = $this->row_query($res);
				$cell = $this->_parentlist->get_item($row['cell_id']);
				if (!$cell->get('exterior') && $cell->get('name'))
					$name = $cell->get('name');
				$cell->release();
			}
		}
		
		if (!empty($name)) {
			return '[['.$name.']]';
		}
		
		return $edid;
	}
	
	// need to take into account count on all levlist uses
	public function get_sources() {
		$drops = $mercs = array();
		$droplists = $merclists = array();
		$other = array();
		
		for ($irep=0; $irep<2; $irep++) {
			if (!$irep)
				$query = 'SELECT AllItems.ordid, AllItems.name, AllItems.edid, 1 as pav, 1 as pav_lvl, 1 as lmin, 1 as countmin, 1 as countmax FROM AllItems INNER Join NPC__Record using (ordid) where deathitem_id='.$this->ordid();
			else
				$query = 'SELECT AllItems.ordid, AllItems.name, AllItems.edid, list_ordid, pav, pav_lvl, lmin, countmin, countmax FROM AllItems INNER Join NPC__Record using (ordid) inner join LevelList on (NPC__Record.deathitem_id=LevelList.list_ordid) where LevelList.item_ordid='.$this->ordid().' order by pav desc, pav_lvl desc';
			
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				if (isset($row['list_ordid']))
					$droplists[] = $row['list_ordid'];
				if ($row['pav']<0.2 && count($drops)>5)
					break;
				// anything this small should just be coming from general loot list, so if I'm just worried about whether  it's available *somewhere*, general loot check should find it
				if ($row['pav']<0.06)
					break; 
				$src = '';
				$src = $this->_llist_text($row, 'dropped by ');
				if ($row['name'])
					$src .= '[['.$row['name'].']]s';
				elseif ($row['edid'])
					$src .= $row['edid'];
				$drops[] = "* $src\n";
			}
		}
		
		$query = 'SELECT FACT_Record.ordid, merc_list, list_ordid, pav, pav_lvl, lmin, countmin, countmax FROM FACT_Record inner join REFR_Record on (FACT_Record.merccont_id=REFR_Record.ordid) inner join LevelList on (REFR_Record.base_id=LevelList.list_ordid) and item_ordid='.$this->get('ordid');
		$res = $this->do_query($query);
		$mercdata = array();
		while ($row=$this->row_query($res)) {
			$merclists[] = $row['list_ordid'];
			$mercdata[$row['merc_list']][$row['ordid']] = $row;
		}
		foreach ($mercdata as $merc_list => $mdata) {
			if (empty($merc_list)) {
				foreach ($mdata as $ordid => $row) {
					$mname = $this->_get_mercname($ordid);
					if ($mname)
						$mercs[] = '* '.$this->_llist_text($row, 'sold by ').$mname."\n";
				}
				continue;
			}
			$query = 'SELECT count(*) as num from FACT_Record where merc_list='.$merc_list;
			$qrow = $this->do_query($query, 'onerow');
				
			$n100 = array();
			$pmax = $pmin = NULL;
			$cmin = NULL;
			foreach ($mdata as $ordid => $row) {
				if ($row['pav']>=1) {
					$n100[] = $ordid;
					if (!isset($cmin) || $row['countmin']<$cmin)
						$cmin = $row['countmin'];
				}
				// Herluin, WiseWomenhas unusual numbers of each ingredient type
				elseif (preg_match('/Heljarchen|Herluin|WiseWoman|Atub/', $this->_parentlist->get_edid($ordid))) {
					continue;
				}
				elseif (!isset($pmax)) {
					$pmax = $row;
					$pmin = $row;
				}
				elseif ($row['pav']<$pmin['pav']) {
					$pmin = $row;
				}
				elseif ($row['pav']>$pmax['pav']) {
					$pmax = $row;
				}
			}
				
			if (!empty($n100)) {
				if (count($n100)==$qrow['num']) {
					$txt = '* ';
					if ($cmin>1)
						$txt .= 'At least '.$cmin.' samples always';
					else
						$txt .= '1 sample always';
					
					$txt .=" sold by ".$this->_get_merctype($merc_list)."\n";
					$mercs[] = $txt;
				}
				else {
					foreach ($n100 as $ordid) {
						$mname = $this->_get_mercname($ordid);
						if ($mname)
							$mercs[] = '* '.$this->_llist_text($mdata[$ordid], 'sold by ').$mname."\n";
					}
				}
			}
			if (count($n100)<count($mdata)) {
				if ($qrow['num']==count($mdata)) {
					$txt = $this->_llist_text($pmax, 'sold by ');
					$merctype = $this->_get_merctype($merc_list);
					if (!empty($n100))
						$merctype = preg_replace('/all /', 'all other ', $merctype);
					$txt .= $merctype;
					if ($pmin['pav']<$pmax['pav'])
						$txt .= ' (min chance = '.floor($pmin['pav']*100).'%)';
					$mercs[] = '* '.$txt."\n";
				}
				elseif (count($mdata)>3) {
					$txt = $this->_llist_text($pmax, 'sold by ');
					$merctype = $this->_get_merctype($merc_list);
					if (empty($n100))
						$merctype = preg_replace('/all /', 'most ', $merctype);
					else
						$merctype = preg_replace('/all /', 'most other ', $merctype);
					$txt .= $merctype;
					if ($pmin['pav']<$pmax['pav'])
						$txt .= ' (min chance = '.floor($pmin['pav']*100).'%)';
					foreach ($mdata as $ordid => $row) {
						if (!in_array($ordid, $n100)) {
							$mname = $this->_get_mercname($ordid);
							if ($mname)
								$txt .= '; '.$mname;
						}
					}
					$mercs[] = '* '.$txt."\n";
				}
				else {
					foreach ($mdata as $ordid => $row) {
						if (in_array($ordid, $n100))
							continue;
						$mname = $this->_get_mercname($ordid);
						if ($mname)
							$mercs[] = '* '.$this->_llist_text($row, 'sold by ').$mname."\n";
					}
				}
			}
		}
		
		// harvest from flowers and trees
		// includes various non-flowers such as hanging rabbits
		$harvest = array();
		$flornames = array();
		$florids = array();
		foreach (array('FLOR', 'TREE') as $src) {
			// Based on Chickens' Eggs, harvest percentages are meaningless
			//			$query = "SELECT ordid, name from AllItems inner join {$src}_Record using (ordid) WHERE ingr_id=".$this->ordid()." and spring>0";
			$query = "SELECT ordid, name from AllItems inner join {$src}_Record using (ordid) WHERE ingr_id=".$this->ordid();
			$res = $this->do_query($query);
			while ($row=$this->row_query($res)) {
				$flornames[$row['name']] = TRUE;
				$florids[] = $row['ordid'];
			}
		}
		if (!empty($flornames)) {
			$txt = 'Can be harvested from '.implode(' and ',array_keys($flornames)).'.';
			$item = $this->_parentlist->get_item($florids[0]);
			$locs = $item->formatted_location_list_multi($florids,15);
			if (!empty($locs))
				$txt .= ' '.preg_replace('/guaranteed samples/', 'guaranteed sources', $locs);
			$harvest[] = $txt;
			$item->release();
		}
		// then there are the ACTIs... I've found where the records are connected -- in the VMAD record -- but
		// getting it from there automatically isn't going to happen any time soon
		// Plus in most cases, I want to search for the spawning ACTIs, not the catch-able ACTIs
		$actis = array('CritterPondFish01' => array('CritterPondFish01Ingredient' => 1),
		               'CritterPondFish02' => array('CritterPondFish02Ingredient' => 1),
		               'CritterPondFish03' => array('CritterPondFish03Ingredient' => 1),
		               'CritterPondFish04' => array('CritterPondFish04Ingredient' => 1),
		               'CritterPondFish05' => array('CritterPondFish05Ingredient' => 1),
		               'CritterBee' => array('CritterBeeIngredient' => 1),
		               'CritterDragonFly02' => array('DragonflyBlue' => 1),
		               'CritterDragonfly01' => array('DragonflyOrange' => 1),
		               'CritterFirefly' => array('FireflyThorax' => 1),
		               'CritterMothLuna' => array('MothWingLuna' => 2),
		               'CritterMothBlue' => array('MothWingBlue' => 2),
		               'CritterMothMonarch' => array('MothWingMonarch' => 2),
		               'TreeFloraNirnroot01' => array('Nirnroot' => 1),
		               'TreeFloraNirnrootRed01' => array('NirnrootRed' => 1),
		               'FCAmbWaterfallSalmon02' => array('FoodSalmon' => 1),
		               'FCAmbWaterfallSalmon01' => array('FoodSalmon' => 1),
		               'CritterSalmon02' => array('FoodSalmon' => 1),
		               'CritterSalmon01' => array('FoodSalmon' => 1),
		               // CONT rather than ACTI... but has a spawner... and needs to be done for Beaks, too
		               'fcHawkGO' => array('HawkFeathers' => 3, 'HawkBeak' => 1),
		               'BeeHive' => array('CritterBeeIngredient' => 1, 'BeeHiveHusk' => 1, 'BeeHoneyComb' => 1),
		               'BeeHiveVacant' => array('CritterBeeIngredient' => 1, 'BeeHiveHusk' => 1),
		               'TreasSpiderEggSac01' => array('SpiderEgg' => 1),
		               'TreasSpiderWebSac' => array('SpiderEgg' => 1),
		               'TreasSpiderWebSacSkeever' => array('SkeeverTail' => 1),
		               'wispCorpseContainer' => array('glowDust' => 1),
		               'defaultGhostCorpse' => array('Ectoplasm' => 1),
		              );
			 // Dragonfly/Fish (salmon): hours 6-18; max 10
		$spawners = array('DragonFlySpawner' => array('critterDragonflyTypes'),
		                  'FishSpawner' => array('FishSchoolTypes'),
		                  // hours 20-23; max 10
		                  'FireflySpawner' => array('critterFireflylist'),
		                  // hours 6-18; max 10... has luna, blue, and monarch
		                  'MothSpawner' => array('TestRegularMothOnly'),
		                  'critterSpawnPond_Deep' => array('critterFishTypes', 'critterDragonflyTypes'),
		                  'critterSpawnPond_Deep_FishOnly' => array('critterFishSalmonTypes'),
		                  'critterSpawnDragonflies24x7_Many' => array('critterDragonflyTypes'),
		                  'critterSpawnDragonfliesFireflies24x7' => array('critterFireflylist', 'critterDragonflyTypes'),
		                  'critterSpawnInsects_Bees' => array('critterInsectsBee'),
		                  'critterSpawnDragonflies24x7' => array('critterDragonflyTypes'),
		                  'critterSpawnPond_Small' => array('critterFishPondTypes', 'critterDragonflyTypes'),
		                  'critterSpawnPond_Shallow' => array('critterFishPondTypes', 'critterDragonflyTypes'),
		                  'critterSpawnFirefly24x7' => array('critterFireflylist'),
		                  'critterSpawnInsects_ManyNocturnalOnly' => array('critterInsectsNocturnal'),
		                  'critterSpawnInsects_SingleNocturnalOnly' => array('critterInsectsNocturnal'),
		                  'critterSpawnInsects_Many' => array('critterInsectsDiurnal', 'critterInsectsNocturnal'),
		                  'critterSpawnInsects_Single' => array('critterInsectsDiurnal', 'critterInsectsNocturnal'),
		                  'critterSpawnInsects_Few' => array('critterInsectsDiurnal', 'critterInsectsNocturnal'),
		                  'FXbirdOPreyNoNestACT' => array('fcHawkGO'),
		                  'FXbirdOPreyACT' => array('fcHawkGO'),
		                 );
		$count = 1;
		$curractis = array();
		$actinames = array();
		foreach ($actis as $acti => $adata) {
			foreach ($adata as $edid => $c) {
				if ($edid==$this->get('edid')) {
					$curractis[] = $this->_parentlist->get_ordid($acti);
					$count = $c;
					$actinames[$this->_parentlist->get_name($acti)] = 1;
					break;
				}
			}
		}
		
		if (!empty($curractis)) {
			$txt = 'Can be harvested from '.implode(' and ', array_keys($actinames));
			if ($count>1)
				$txt .= ", obtaining $count samples each time";
			$txt .= ".\n";
			$harvest[] = $txt;
			$item = $this->_parentlist->get_item($curractis[0]);
			$locs = $item->formatted_location_list_multi($curractis,15);
			if (!empty($locs)) {
				$harvest[] = '* '.preg_replace('/guaranteed samples/', 'guaranteed sources', $locs);
			}
			$item->release();
			
			// then get locations of all spawners
			$currspawns = array();
			
			foreach ($spawners as $spawner => $lists) {
				$spordid = $this->_parentlist->get_ordid($spawner);
				foreach ($lists as $list) {
					$listordid = $this->_parentlist->get_ordid($list);
					if (in_array($listordid, $curractis)) {
						$currspawns[] = $spordid;
						break;
					}
					else {
						$squery = 'SELECT count(*) as num FROM LevelList where list_ordid='.$listordid.' and item_ordid IN ('.implode(', ',$curractis).')';
						$srow = $this->do_query($squery, 'onerow');
						if (!empty($srow['num'])) {
							$currspawns[] = $spordid;
							break;
						}
					}
				}
			}
			
			if (!empty($currspawns)) {
				$item = $this->_parentlist->get_item($currspawns[0]);
				$locs = $item->formatted_location_list_multi($currspawns, 15);
				if (!empty($locs))
					$harvest[] = preg_replace('/guaranteed samples/', 'spawning sites', $locs);
				$item->release();
			}
		}
			
		$query = 'SELECT edid from AllItems inner join FLST_LNAM_Record using (ordid) where item_id='.$this->ordid().' AND edid not like \'%AtrFrg%\'';
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			$other[] = "* In FLST ".$row['edid']."\n";
		}
		
		if ($this->get('edid')!='Gold001') {
		$query = 'SELECT AllItems.edid, LevelList.* from AllItems inner join LevelList on (AllItems.ordid=LevelList.list_ordid) where item_ordid='.$this->ordid();
		$query .= ' AND rectype!=\'FLST\' and edid not like \'QA%\' and edid not like \'DEMO%\' and edid not like \'E3%\' and edid not like \'%LockList\' and edid not like \'%ResidentList\' and edid not like \'%MerchantHeljarchen%\'';
		if (count($merclists) || count($droplists)) {
			$query .= ' and list_ordid not in ('.implode(', ', array_merge($merclists, $droplists)).')';
		}
		$query .= ' order by pav desc';
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			if (count($other)>50 || (count($other)>10 && $row['pav']<30) || (count($other)>5 && $row['pav']<0.06))
				break;
			$other[] = "* ".$this->_llist_text($row, 'given by ').'list '.$this->_parentlist->get_edid($row['list_ordid'])."\n";
		}
		}
		
		return implode('', $drops).implode('', $mercs).implode('',$harvest).implode('',$other);
	}
	
	public function get_usage() {
		$used = array();
		$created = array();
		$other = array();
		
		$query = 'SELECT ordid, item_count from COBJ_CNTO_Record WHERE item_id='.$this->ordid();
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			$item = $this->_parentlist->get_item($row['ordid']);
			$made_at = $this->_parentlist->get_edid($item->get('made_at'));
			$text = '* '.$row['item_count'].' '.$this->get('name').($row['item_count']>1?'s are':' is');
			
			$indices = $item->indices('item_id');
			if (count($indices)>1) {
				$text .= ' one of '.count($indices).' items';
			}
			
			$text .= ' needed ';
			if ($made_at=='CraftingCookpot')
				$text .= 'to [[Cooking|cook]] ';
			elseif ($made_at=='CraftingSmelter')
				$text .= 'at [[Smelter]]s to create ';
			elseif ($made_at=='CraftingSmithingArmorTable')
				$text .= 'at [[Workbenche]]es to improve ';
			elseif ($made_at=='CraftingSmithingForge')
				$text .= 'at [[Forge]]s to create ';
			elseif ($made_at=='CraftingSmithingSharpeningWheel')
				$text .= 'at [[Grindstone]]s to improve ';
			elseif ($made_at=='CraftingSmithingSkyforge')
				$text .= 'at the [[Skyforge]] to create ';
			elseif ($made_at=='CraftingTanningRack')
				$text .= 'at [[Tanning Rack]]s to create ';
			
			$count = $item->get('create_count');
			$text .= ($count==1?'a':$count).' [['.$this->_parentlist->get_name($item->get('created')).']]'.($count>1?'s':'');
			
			$indices = $item->indices('cond_function');
			foreach ($indices as $index) {
				if ($item->get('cond_function', $index)!='HasPerk')
					continue;
				$param = $this->convert_formid($item->get('cond_param1', $index));
				$text .= ' (requires perk [['.$this->_parentlist->get_name($param).']])';
			}
			
			$used[] = $text."\n";
			$item->release();
		}
		
		$query = 'SELECT ordid, edid from AllItems inner join LevelList on (AllItems.ordid=LevelList.list_ordid) WHERE item_ordid='.$this->ordid().' and rectype=\'FLST\' and edid like \'AtrFrg%\'';
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			$type = NULL;
			if (preg_match('/RecipeList/', $row['edid'])) {
				// all recipe entries should show up again under invididual recipe
				continue;
			}
			if (preg_match('/ResultList/', $row['edid'])) {
				$type = 'create';
				$result = $row['ordid'];
				$squery = 'SELECT subid from FLST_LNAM_Record where ordid='.$result.' and item_id='.$this->ordid();
				$srow = $this->do_query($squery, 'onerow');
				$subid = $srow['subid'];
				$recipelist = $this->_parentlist->get_ordid(preg_replace('/Result/', 'Recipe', $row['edid']));
				$squery = 'SELECT item_id from FLST_LNAM_Record where ordid='.$recipelist.' and subid='.$subid;
				$srow = $this->do_query($squery, 'onerow');
				$recipe = $srow['item_id'];
			}
			else {
				$type = 'use';
				$recipe = $row['ordid'];
				$squery = 'SELECT ordid, edid, subid from AllItems inner join FLST_LNAM_Record using (ordid) WHERE item_id='.$row['ordid'];
				$srow = $this->do_query($squery, 'onerow');
				$recipelist = $srow['edid'];
				$subid = $srow['subid'];
				$result = $this->_parentlist->get_ordid(preg_replace('/Recipe/', 'Result', $srow['edid']));
				$squery = 'SELECT item_id FROM FLST_LNAM_Record where ordid='.$result.' AND subid='.$subid;
				$srow = $this->do_query($squery, 'onerow');
				$result = $srow['item_id'];
			}
			
			$text = '* ';
			$item = $this->_parentlist->get_item($recipe);
			$indices = $item->indices('item_id');
			if (preg_match('/SigilStone/', $this->_parentlist->get_edid($recipelist)))
				$sigiltext = ' (requires the [[Sigil Stone]])';
			else
				$sigiltext = '';
			if ($type=='create') {
				$text .= 'Can be created at the [[Atronach Forge]] from: ';
				$srcs = array();
				foreach ($indices as $index) {
					$id = $item->get('item_id', $index);
					$name = $this->_parentlist->get_name($id);
					if (empty($name))
						$name = $this->_parentlist->get_edid($id);
					$srcs[] = '1 [['.$name.']]';
				}
				$text .= implode(' + ',$srcs);
				$text .= $sigiltext;
				$created[] = $text."\n";
			}
			else {
				$singlesrc = FALSE;
				foreach ($indices as $index) {
					if ($item->get('item_id',$index)==$this->ordid())
						$singlesrc = TRUE;
				}
				if ($singlesrc) {
					$text .= 'Is one of ';
				}
				else {
					$multisrc = NULL;
					foreach ($indices as $index) {
						$squery = 'SELECT list_ordid from LevelList where list_ordid='.$item->get('item_id',$index).' and item_ordid='.$this->ordid();
						$srow = $this->do_query($squery, 'onerow');
						if (!empty($srow)) {
							$multisrc = $srow['list_ordid'];
							break;
						}
					}
					if (empty($multisrc)) {
						fwrite(STDERR, "Error finding Atronach Forge parent item\n");
					}
					$text .= $this->_parentlist->get_edid($multisrc).' is one of ';
				}
				$text .= count($indices).' items needed at the [[Atronach Forge]] to create a ';
				$resultname = $this->_parentlist->get_name($result);
				if (empty($resultname))
					$resultname = $this->_parentlist->get_edid($result);
				if ($resultname=='LvlDremoraMelee')
					$resultname = 'Unbound Dremora';
				$text .= '[['.$resultname.']]';
				$text .= $sigiltext;
				$used[] = $text."\n";
			}
			$item->release();	
		}
		
		$query = 'SELECT ordid from AllItems inner join COBJ_Record using (ordid) WHERE created='.$this->ordid().' order by edid';
		$res = $this->do_query($query);
		while ($row=$this->row_query($res)) {
			$item = $this->_parentlist->get_item($row['ordid']);
			$made_at = $this->_parentlist->get_edid($item->get('made_at'));
			
			$text = '* ';
			if ($item->get('created_count')>1)
				$text .= $item->get('created_count').' '.$this->get('name').'s can be ';
			else
				$text .= 'Can be ';
			
			if ($made_at=='CraftingCookpot')
				$text .= 'created by [[Cooking|cooking]] from: ';
			elseif ($made_at=='CraftingSmelter')
				$text .= 'created at [[Smelter]]s from: ';
			elseif ($made_at=='CraftingSmithingArmorTable')
				$text .= 'improved at [[Workbenche]]es using: ';
			elseif ($made_at=='CraftingSmithingForge')
				$text .= 'created at [[Forge]]s from: ';
			elseif ($made_at=='CraftingSmithingSharpeningWheel')
				$text .= 'improved at [[Grindstone]]s using: ';
			elseif ($made_at=='CraftingSmithingSkyforge')
				$text .= 'created at the [[Skyforge]] from: ';
			elseif ($made_at=='CraftingTanningRack')
				$text .= 'created at [[Tanning Rack]]s from: ';
			
			$indices = $item->indices('item_id');
			$srcs = array();
			foreach ($indices as $index) {
				$srcs[] = $item->get('item_count', $index).' [['.$this->_parentlist->get_name($item->get('item_id', $index)).']]'.($item->get('item_count', $index)>1?'s':'');
			}
			$text .= implode(' + ',$srcs);
			
			$indices = $item->indices('cond_function');
			foreach ($indices as $index) {
				if ($item->get('cond_function', $index)!='HasPerk')
					continue;
				$param = $this->convert_formid($item->get('cond_param1', $index));
				$text .= ' (requires perk [['.$this->_parentlist->get_name($param).']])';
			}
			
			$created[] = $text."\n";
			$item->release();
		}
		$squery = "SELECT * from QUST_ALID_Record where created_object=".$this->ordid();
		$sres = $this->do_query($squery);
		while ($srow=$this->row_query($sres)) {
			$index = $srow['subid'];
			$qid = $srow['ordid'];
			$quest = $this->_parentlist->get_item($qid);
			$qname = $quest->get_questname();
			$used[] = '* Used in quest [['.$qname.']]'."\n";
			$quest->release();
		}
		if (in_array($this->get('edid'), array('deathBell', 'Nightshade', 'Nirnroot'))) {
			$used[] = "* 20 samples are needed for the quest [[Few and Far Between]]\n";
		}
		if (in_array($this->get('edid'), array('JazBay'))) {
			$used[] = "* 20 samples are needed for the quest [[Smooth Jazbay]]\n";
		}
		if (in_array($this->get('edid'), array('NirnrootRed'))) {
			$used[] = "* 30 samples are needed for the quest [[A Return To Your Roots]]\n";
		}
		if (in_array($this->get('edid'), array('FireSalts'))) {
			$used[] = "* 10 samples are needed for the quest [[Stoking the Flames]]\n";
		}
		if (in_array($this->get('edid'), array('IceWraithTeeth'))) {
			$used[] = "* 5 samples are needed for the quest [[Ice Cold]]\n";
		}
		if (in_array($this->get('edid'), array('DaedraHeart', 'TrollFat'))) {
			$used[] = "* 1 sample is needed for the quest [[The Cursed Tribe]]\n";
		}
		if (in_array($this->get('edid'), array('deathBell', 'vampireDust'))) {
			$used[] = "* 1 sample is needed for the quest [[The Only Cure]]\n";
		}
		if (in_array($this->get('edid'), array('BriarHeart', 'MS12MammothTuskPowder'))) {
			$used[] = "* 1 sample is needed for the quest [[Repairing the Phial]]\n";
		}
		if (in_array($this->get('edid'), array('OreGold'))) {
			$used[] = "* 1 is needed for the quest [[Ringmaker]]\n";
		}
		if (in_array($this->get('edid'), array('GemSapphireFlawless'))) {
			$used[] = "* 2 are needed for the quest [[Ringmaker]]\n";
		}
		if (in_array($this->get('edid'), array('FoodPotato', 'Wheat', 'Nirnroot', 'FoodCabbage', 'FoodLeek', 'FoodGourd'))) {
			$keyword = $this->_parentlist->get_ordid('Farmer'.$this->get('name'));
			$squery = 'SELECT name from AllItems INNER JOIN NPC__KWDA_Record using (ordid) where keyword='.$keyword;
			$sres = $this->do_query($squery);
			$npcs = array();
			while ($srow=$this->row_query($sres)) {
				$npcs[] = '[['.$srow['name'].']]';
			}
			sort($npcs);
			$npcs[count($npcs)-1] = 'or '.$npcs[count($npcs)-1];
			$used[] = "* Can be sold to ".implode(', ',$npcs)." for the quest [[Gather Wheat]]\n";
		}
		
		return implode('', $created).implode('', $used);
	}
}

/* Functions common to all functions that deal with magic effects */

class GenItem_GenMagic extends GenItem {
	public function mgef_specific_name($index) {
		if (is_null($ename = $this->get('effect', $index)))
			return NULL;
		return $ename;
	}
	protected function magic_desc_line_extra($index, $lineout) {
		return $lineout;
	}
	
	public function effect_cost($index) {
		if (!array_key_exists('effect_id', $this->_data) || !is_array($this->_data['effect_id']) || !array_key_exists($index, $this->_data['effect_id']))
			return NULL;
		if (!is_object($effect = $this->_parentlist->get_item($this->get('effect_id', $index))))
			return NULL;
		$mag = $this->get('eff_mag', $index);
		$dur = $this->get('eff_dur', $index);
		$cost = $effect->get('basecost');
		if ($mag > 1)
			$cost *= pow($mag, 1.1);
		if ($dur && $this->get('casting_type')!='Concentration')
			$cost *= pow($dur/10, 1.1);
		$effect->release();
		return $cost;
	}
	
	// Value of apparel
	public function effect_value($index) {
		if (!array_key_exists('effect_id', $this->_data) || !is_array($this->_data['effect_id']) || !array_key_exists($index, $this->_data['effect_id']))
			return NULL;
		$type = $this->get('casting_type');
		$mag = $this->get('eff_mag', $index);
		$dur = $this->get('eff_dur', $index);
		$area = $this->get('eff_area', $index);
		if (!is_object($effect = $this->_parentlist->get_item($this->get('effect_id', $index))))
			return NULL;
		$cost = $effect->get('basecost');
		if ($mag > 1)
			$cost *= pow($mag, 1.1);
		// type=0 is Apparel (constant effect) -- no more adjustments done
		if ($type!='Constant Effect') {
		 // weapons
			 
			// only include duration for on-strike weapons (cast and forget), not for flexible duration (concentration)
			if ($type!='Concentration') {
 				if ($dur)
					$cost *= pow($dur/10, 1.1);
			}
		}
		$effect->release();
		// rounding needs to be done at very end -- in case multiple effects are being added together
		return $cost;
	}
	
	// $name_only option is used by effect_desc
	function magic_summ($name_only=FALSE) {
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
			foreach ($eitem->indices('effect_id') as $ie) {
				$line = $eitem->get('magic_summ_line', $ie);
				if (empty($line))
					continue;
				if (!empty($output)) {
					if (substr($output,-1)=='.')
						$output = substr($output,0,-1);
					$output .= '; ';
				}
				$output .= $line;
			// magic_summ only shows first effect... well, not true for potions/poisons...
				/*if (!empty($output))
					break 2;*/
			}
		}
		$this->_release_items($enchitems);
		return $output;
	}
	
	public function magic_summ_line($index) {
		$mgef = $this->get_as_item('effect_id', $index);
		$lineout = $mgef->get('DESC');
		if (empty($lineout))
			return $lineout;
		
		$mag = $this->get('eff_mag', $index);
		$dur = $this->get('eff_dur', $index);
		if ($mag && $lineout) {
			$lineout = preg_replace('/<mag>/', $mag, $lineout);
		}
		if ($dur && $lineout) {
			$lineout = preg_replace('/<dur>/', $dur, $lineout);
		}
		$lineout = preg_replace('/<(.*?)>/', '$1', $lineout);
		
		return $lineout;
	}
}
