<?php
/**
 * Create redirect list in format expected by NepheleBot

 * Note that perl equivalent to this script functioned mostly by reading an existing wiki page, getting
 * Linkable Entry's on the page, and then creating corresponding redirects

 * This script works instead by getting complete list of items from DB and generating list
 * Which means extra work needs to be done to work out target of redirect
 * Also means I'm not necessarily excluding test items

* Check whether any other info has been getting added onto redirect pages (esp {{#save}} requests)
*/

//require_once "mwdata.inc";
require_once "obdata.inc";
//require_once "srdata.inc";
$type = 'Clot';

$cs = CSData::getInstance();
$cs->init_db();

// For each type, either default_target/default_cat need to be set in initial setup
// *or* else base_target/cat need to be set per-item
$rectype = NULL;
$default_target = NULL;
$default_cat = NULL;
$default_disambig = NULL;

$query_tables = array('AllItems');
// global 'where' conditions
// never want items with empty names
$query_where = array(" name IS NOT NULL AND name <> '' ");

if ($type=='Spells') {
	$rectype = 'SPEL';
	$query_where[] = ' type_lu = 0 ';
	// Non-Skyrim defaults -- in Skyrim, change to be school-specific
	$default_target = 'Spells';
	$default_cat = 'Magic-Spells';
	$default_disambig = 'spell';
}
elseif ($type=='Ammo') {
	$rectype = 'AMMO';
	$default_target = 'Ammunition';
	$default_cat = 'Ammunition';
	$default_disambig = 'item';
}
elseif ($type=='Weap') {
	// Really want to skip Staves .. or have special treatment for them
	$rectype = 'WEAP';
	$default_target = 'Weapons';
	$default_cat = 'Weapons';
	$default_disambig = 'item';
}
elseif ($type=='Armor') {
	$rectype = 'ARMO';
	$default_target = 'Armor';
	$default_cat = 'Armor';
	$default_disambig = 'item';
}
elseif ($type=='Clot') {
	$rectype = 'CLOT';
	$default_target = 'Clothing';
	$default_cat = 'Clothing';
	$default_disambig = 'item';
}

if (count($query_tables)==1 && isset($rectype)) {
	$query_tables[] = $rectype.'_Record';
	$query_where[] = " rectype = '$rectype' ";
}

$query = 'SELECT AllItems.ordid FROM ';
foreach ($query_tables as $i => $table) {
	if ($i) {
		$query .= ' INNER JOIN '.$table;
		// this check is probably obsolete -- replaced by rectype variable -- but doesn't hurt to leave it here
		if (strlen($table)==4) {
			$query .= '_Record';
			$query_where[] = " rectype = '$table' ";
		}
		$query .= ' USING (ordid) ';
	}
	else {
		$query .= ' '.$table.' ';
	}
}

$query .= ' WHERE '.implode(' AND ', $query_where);
// order by fileid to make sure fileid=0 entries are processed first (don't want Shivering: links for a Oblivion item)
$query .= ' ORDER BY fileid';

$res = $cs->do_query($query);

// keep track of done items in order to avoid creating duplicate items
// (e.g., for leveled items that all have same name but different ordid);
$done = array();

// Extra complications to handle:
// Don't want mod-added items to point to standard items page; they should redirect to mod-specific items page
// Should identify cases where disambig-type text needs to be added to page name

while ($row=$cs->row_query($res)) {
	$item = $cs->get_item($row['ordid']);
	$name = $item->get('name');
	
	// Should I auto-skip these entries or not??
	// But if I am skipping, want to do it before $done creation (in case test is only in edid)
	if (preg_match('/\bTest/', $name) || preg_match('/Test/i', $item->get('edid'))) {
		fwrite(STDERR, "Skipping possible test entry, name = $name (edid=".$item->get('edid').", ordid=".$row['ordid'].")\n");
		continue;
	}
	
	$name = preg_replace('/ /', '_', $name);
	if (!empty($done[$name]))
		continue;
	$done[$name] = TRUE;
	$ns = $item->namespace();
	
	$base_target = NULL;
	$cat = NULL;
	// name_ex is placeholder for adding disambig info to page name
	$name_ex = '';
	
	// identify possible disambig cases
	$disambig = FALSE;
	if (isset($rectype)) {
		$cquery = "SELECT ordid FROM AllItems where name='".addslashes($item->get('name'))."' AND rectype<>'$rectype'";
		$cres = $cs->do_query($cquery);
		if (!empty($cres)) {
			$crow = $cs->row_query($cres);
			if (!empty($crow))
				$disambig = TRUE;
		}
	}
	
	if ($type=='Spells') {
		if ($ns=='Skyrim') {
			$base_target = $ns.':'.$item->get('spellschool').'_Spells';
			$cat = $ns.'-Magic-Spells-'.$item->get('spellschool');
		}
	}
	elseif ($type=='Weap') {
		$enchant = $item->get('magic_desc');
		if (empty($enchant)) {
			$base_target = $ns.':Base_Weapons';
		}
		else {
			$base_target = $ns.':Generic_Magic_Weapons';
		}
		$skill = $item->get('skill');
		$itemtype = $item->get('type');
		if ($skill=='Marksman' || $skill=='Archery') {
			$base_target = $ns.':Bows';
			$cat = $ns.'-Weapons-Bows';
		}
		else {
		// Assume for Skyrim that weapon type also identifies skill (e.g. greatsword = two-handed)
			$cat = $ns.'-Weapons-'.preg_replace('/ /', '_', $itemtype).'s';
		}
	}
	elseif ($type=='Armor') {
		$enchant = $item->get('magic_desc');
		if (empty($enchant)) {
			$base_target = $ns.':Base_Armor';
		}
		else {
			$base_target = $ns.':Generic_Magic_Apparel';
		}
		// get rid of ' Armor' at end of skill
		$skill = substr($item->get('skill'),0,-6);
		$itemtype = $item->get('type');
		$itemtype = preg_replace('/ /', '_', $itemtype);
		// handle Other...
		if ($itemtype=='Cuirass') {
			$itemtype .= 'es';
		}
		elseif (substr($itemtype,-1)!='s' && $itemtype!='Other') {
			$itemtype .= 's';
		}
		$cat = $ns.'-Armor-'.$itemtype.'-'.$skill;
	}
	elseif ($type=='Clot') {
		$enchant = $item->get('magic_desc');
		if (empty($enchant)) {
			$base_target = $ns.':Base_Clothing';
		}
		else {
			$base_target = $ns.':Generic_Magic_Apparel';
		}
		$skill = $item->get('skill');
		$itemtype = $item->get('type');
		if ($type=='Ring' || $type=='Amulet') {
			$cat = $ns.'-Jewelry-'.$type.'s';
		}
		else {
			$itemtype = preg_replace('/ /', '_', $itemtype).'s';
			$itemtype = preg_replace('/sss/', 'ses', $itemtype);
			$cat = $ns.'-Clothing-'.$itemtype;
		}
	}
	
	if ($disambig && empty($name_ex) && isset($default_disambig)) {
		$name_ex = ' ('.$default_disambig.')';
	}
	if (!isset($base_target)) {
		if (!isset($default_target)) {
			print "Neither base_target nor default_target set\n";
			exit;
		}
		$base_target = $ns.':'.$default_target;
	}
	if (!isset($cat)) {
		if (!isset($default_cat)) {
			print "Neither cat nor default_cat set\n";
			exit;
		}
		$cat = $ns.'-'.$default_cat;
	}
	
	print $ns.':'.$name.$name_ex."\t";
	print $base_target.'#'.$name."\t";
	print $cat;
	
	print "\n";
}