<?php
/* Start of code for a webpage that provides lists of items by rectype */

// To fix:
// Display of MW/CELL, MW/LAND, MW/TES3
// Record counts
// Why don't MW DIAL records contain anything? -> should they link to their INFO records?
// Display of MW/INFO -> need to show ordid, or something useful
// for individual INFO's need NNAM/PNAM to be links to relevant INAM record
// -- done, except extra issue is that NNAM/PNAM aren't unique
// not handling x_id/x relation yet -- affecting MW/INGR individual items

// no INFOs in FNV?

// For Skyrim
// add in a way of reporting on validated vs non-validated columns
// get record format to highlight subrecs that have changed in last week?

// after setting up gCSData check whether History has active entry
// if so, global flag?
// print text? (Notice: XXXX database records are currently being updated)
// if $_GET['rec'] -> only do notice if that rectype is being affected
// if no History table, or if no History entries -> notice that no data available?

// why didn't setup_cells work on MW?

// need to fix record stats better -> subrec total numbers have to be scaled down

// want to create a more advanced search page:
// allow search to be done on:
// name, edid, script name, book text, dialogues, script content, (quest aliases?.... save more options until I come across a need for them?)

global $gCSData;
global $gOutput;
global $gHistory;
global $reclist;
global $gamenames;
$gamenames = array('ob' => 'Oblivion',
                   'mw' => 'Morrowind',
                   /*'fo' => 'Fallout',*/
                   'sr' => 'Skyrim',);
global $libdir;
$libdir = './';

if (file_exists($libdir.'/.update')) {
	$_GET['game'] = 'ob';
	$_GET['format'] = 'html';
	print start_page('Game Data Offline');
	print 'This tool is temporarily offline for a code update. Please check back shortly.';
	print end_page();
	return;
}

require_once($libdir.'outputformat.inc');

$params = process_get();
// Testing!!
//$_GET['rec'] = 'WEAP';
//$_GET['edid'] = 'FIDG';
//$_GET['formid'] = '0x000c89aa';
//$_GET['game'] = 'ob';

$gCSData = setup_cs();
if (0 && empty($gHistory['DB'])) {
	print start_page($gamenames[$_GET['game']].' Game Data Offline');
	print 'The '.$_GET['game'].' database is currently unavailable. Please check back shortly.';
	print end_page();
	return;
}

$gOutput = OutputFormat::getInstance($_GET['format']);
$gOutput->set_start_functions('start_page', 'end_page');

if (!empty($_GET['history'])) {
	display_history();
}
elseif (!empty($_GET['more'])) {
	display_more_search($_GET['search']);
}
elseif (!empty($_GET['search'])) {
	display_search($_GET['search']);
}
elseif (isset($_GET['formid']) || isset($_GET['ordid']) || isset($_GET['edid'])) {
	display_cell($_GET['formid'], $_GET['edid'], $_GET['ordid']);
}
elseif (isset($_GET['script']) || isset($_GET['sid'])) {
	display_script($_GET['script'], $_GET['sid']);
}
elseif (isset($_GET['stats'])) {
	display_rectype_stats($_GET['rec']);
}
elseif (isset($_GET['rec'])) {
	display_rectype($_GET['rec']);
}
else {
	display_main();
}

function process_get() {
	global $gamenames;
	
	$format = NULL;
	if (isset($_GET['format'])) {
		$format = strtolower($_GET['format']);
		$format = preg_replace('/[^a-z_]/', '', $format);
	}
	if ($format=='wiki' || $format=='csv')
		$_GET['format'] = $format;
	else
		$_GET['format'] = 'html';
	
	$game = NULL;
	if (isset($_GET['game']))
		$game = strtolower(substr($_GET['game'],0,2));
	if (!isset($game) || !isset($gamenames[$game]))
		$game = 'ob';
	$_GET['game'] = $game;
	
	$rec = NULL;
	if (isset($_GET['rec'])) {
		$rec = strtoupper(substr($_GET['rec'],0,4));
		$rec = preg_replace('/[^A-Z0-9_]/', '', $rec);
		if (!empty($rec) && strlen($rec)<4)
			$rec = str_pad($rec, 4, '_');
	}
	if (empty($rec))
		$_GET['rec'] = NULL;
	else
		$_GET['rec'] = $rec;
	
	if (!empty($_GET['rec']) && isset($_GET['stats']) && !empty($_GET['stats']))
		$_GET['stats'] = TRUE;
	else
		$_GET['stats'] = NULL;
	
	$formid = NULL;
	if (isset($_GET['formid'])) {
		$formid = strtolower(substr($_GET['formid'],0,10));
		if (substr($formid,0,2)=='0x')
			$formid = substr($formid,2);
		elseif (substr($formid,0,1)=='x')
			$formid = substr($formid,1);
		// first two digits of formid may be letters indicating mod
		// can't convert those mod letters into internal id until cs is loaded
		if (strlen($formid)==8) {
			$modid = substr($formid,0,2);
			$modid = preg_replace('/[^a-z0-9]/', '0', $modid);
			$formid = substr($formid,2);
			$formid = preg_replace('/[^a-f0-9]/', '0', $formid);
			$formid = $modid.$formid;
		}
		else {
			$formid = preg_replace('/[^a-f0-9]/', '0', $formid);
		}
		if (!empty($formid))
			$formid = '0x'.str_pad($formid, 8, '0', STR_PAD_LEFT);
	}
	if (empty($formid))
		$_GET['formid'] = NULL;
	else
		$_GET['formid'] = $formid;
	
	$ordid = NULL;
	if (isset($_GET['ordid'])) {
		$ordid = (int) $_GET['ordid'];
	}
	if (empty($ordid))
		$_GET['ordid'] = NULL;
	else
		$_GET['ordid'] = $ordid;
	
	$edid = NULL;
	// NOTE: addslashes is being done by csdata, so don't do it here!!
	if (empty($_GET['edid']))
		$_GET['edid'] = NULL;
	
	if (isset($_GET['sid']))
		$_GET['sid'] = (int) $_GET['sid'];
	if (empty($_GET['sid']))
		$_GET['sid'] = NULL;
	if (isset($_GET['script']))
		$_GET['script'] = preg_replace('/[^A-Za-z0-9_]/', '', $_GET['script']);
	if (empty($_GET['script']))
		$_GET['script'] = NULL;

	$offset = 0;
	if (!empty($_GET['offset']))
		$offset = (int) $_GET['offset'];
	$_GET['offset'] = $offset;
	
	// process all filter inputs
	foreach ($_GET as $key => $value) {
		if (substr($key,0,7)!='filter_')
			continue;
		if ($value<0)
			unset($_GET[$key]);
		else
			$_GET[$key] = (int) $value;
	}	
}

function setup_cs() {
	global $gCSData, $reclist, $libdir;
	
	$game = $_GET['game'];
	require_once $libdir.$game.'data.inc';
	$gamedata = strtoupper($game).'Data';
	$gCSData = new $gamedata();
	$gCSData->init_db();
	
	$query = "SELECT rectype, nsub FROM Reclist where subrec='' order by rectype";
	$res = $gCSData->do_query($query);
	while ($row = $gCSData->row_query($res)) {
		$reclist[$row['rectype']] = $row['nsub'];
	}
	if ($_GET['rec']!='REFR' && isset($_GET['rec']) && !isset($reclist[$_GET['rec']])) {
		$_GET['rec'] = NULL;
		$_GET['stats'] = NULL;
	}
		
	get_history();
	return $gCSData;
}

function get_history($redo_rec=FALSE) {
	global $gCSData, $gHistory;
	
	if (!empty($gHistory) && !$redo_rec)
		return;
	
	if (!$gCSData->hastable('History'))
		return;
	
	if (empty($gHistory)) {
		$query = 'SELECT active, time, text FROM History WHERE History.rectype IS NULL ORDER BY History.setid DESC';
		$res = $gCSData->do_query($query);
		if (empty($res))
			return;
		$row = $gCSData->row_query($res);
		if (empty($row))
			return;
		$gHistory['DB'] = array('active' => $row['active'], 'time' => $row['time'], 'text' => $row['text']);
	}
	
	if (!empty($_GET['rec'])) {
		$query = 'SELECT active, time, hid FROM History WHERE History.rectype=\''.$_GET['rec'].'\' ORDER BY History.setid DESC';
		$res = $gCSData->do_query($query);
		if (empty($res))
			return;
		$row = $gCSData->row_query($res);
		if (empty($row))
			return;
		$gHistory[$_GET['rec']] = array('active' => $row['active'], 'time' => $row['time'], 'hid' => $row['hid']);
	}
}

function display_main() {
	global $gCSData, $gOutput, $gamenames;
	
	$gOutput->set_param($gamenames[$_GET['game']].' Game Data', 'page_title');
	
	$query = "SELECT * FROM Reclist where subrec='' order by rectype";
	
	$gOutput->text('<h3>About CSList</h3>'."\n");
	$gOutput->text('<p>CSList displays the raw game data for Morrowind, Oblivion, and Skyrim.  It was created to be a tool for UESP editors and patrollers to use as part of improving and maintaining <a href="http://www.uesp.net/wiki/Main_Page">UESPWiki</a>.  It is <b>not</b> intended to be a user-friendly way to learn about the Elder Scrolls games.</p>'."\n");
	$gOutput->text('<p style="color:red; font-size: larger">If you do not understand what this information means, or how to use this webpage, then <b>do not use CSList.</b>  Go to <a href="http://www.uesp.net/wiki/Main_Page">UESPWiki</a> for user-friendly game information.</p>'."\n");
	$gOutput->text('<p>The only available documentation for CSList is at <a href="http://www.uesp.net/wiki/UESPWiki:CSList">UESPWiki:CSList</a>.  General information about the game data format, such as the meaning of the record names, can be found at <a href="http://www.uesp.net/wiki/Morrowind_Mod:Mod_File_Format">Morrowind Mod</a>, <a href="http://www.uesp.net/wiki/Oblivion_Mod:Mod_File_Format">Oblivion Mod</a>, and <a href="http://www.uesp.net/wiki/Skyrim_Mod:Mod_File_Format">Skyrim Mod</a>.</p>'."\n");
	
	$gOutput->text('<h3>Record Summary</h3>'."\n");
	$res = $gCSData->do_query($query);
	$gOutput->init_table();
	$gOutput->set_columns(array('rectype' => array('title' => 'Record', 'format' => 'link_rectype'),
	                            'nsub' => array('title' => '# of Records'),
	                            'totsize' => array('title' => 'Total Size'),
	                            'avsize' => array('title' => 'Av. Size', 'format' => 'num_tenths'),
	                            'minsize' => array('title' => 'Min. Size'),
	                            'maxsize' => array('title' => 'Max. Size')));
	$gOutput->start_table();
	while ($row=$gCSData->row_query($res)) {
		$gOutput->start_new_row();
		$row['totsize'] = floor($row['nsub']*$row['avsize']+0.5);
		foreach (array('rectype', 'nsub', 'totsize', 'avsize', 'minsize', 'maxsize') as $valname) {
			$gOutput->add_row_value($valname, $row[$valname], TRUE);
		}
		$gOutput->end_row();
	}
	$gOutput->end_table();
}

function display_rectype_stats($rectype) {
	global $gCSData, $gOutput, $gHistory, $gamenames, $reclist;
	
	$gOutput->set_param($gamenames[$_GET['game']]." $rectype Record Statistics", 'page_title');
	
	$squery = $_SERVER['QUERY_STRING'];
	$squery = preg_replace('/&stats=[^&]+/', '', $squery);
	$gOutput->text('<div style="float:right;width=30%">Return to <a href="'.$_SERVER['PHP_SELF'].'?'.$squery.'">record data</a>'."</div>\n");
	$query = "SELECT * from Reclist where rectype='$rectype' order by subrec";
	$res = $gCSData->do_query($query);
	
	$gOutput->init_table();
	$gOutput->set_columns(array('subrec' => array('title' => 'Subrecord'),
	                            'nsub' => array('title' => 'Total Number'),
	                            'minnum' => array('title' => 'Min # / Record'),
	                            'maxnum' => array('title' => 'Max # / Record'),
	                            'minsize' => array('title' => 'Min. Size'),
	                            'maxsize' => array('title' => 'Max. Size'),
	                            'avsize' => array('title' => 'Av. Size', 'format' => 'num_tenths')));
	$gOutput->start_table();
	while ($row=$gCSData->row_query($res)) {
		$gOutput->start_new_row();
		foreach (array('subrec', 'nsub', 'minnum', 'maxnum', 'minsize', 'maxsize', 'avsize') as $valname) {
			$gOutput->add_row_value($valname, $row[$valname], TRUE);
		}
		$gOutput->end_row();
	}
	$gOutput->end_table(FALSE);
	
	if (!isset($gHistory[$_GET['rec']]['hid'])) {
		$gOutput->finish();
		return;
	}
	
	$query = 'SELECT text FROM History WHERE hid='.$gHistory[$_GET['rec']]['hid'];
	$row = $gCSData->do_query($query, 'onerow');
	if (empty($row) || empty($row['text'])) {
		$gOutput->finish();
		return;
	}
	$format = unserialize($row['text']);
	ksort($format);
		
	// copied from csread.inc
	$datatype = array(
	                              'raw' => array('len' => 1), // unprocessed raw data (may be a string)
	                              'short' => array( 'code' => "s", 'len' => 2, 'sql' => 'smallint'),
	                              'ushort' => array( 'code' => "S", 'len' => 2, 'sql' => 'smallint UNSIGNED' ),
	                              'long' => array( 'code' => "l", 'len' => 4, 'sql' => 'int'),
	                              'ulong' => array( 'code' => "L", 'len' => 4, 'sql' => 'int UNSIGNED' ),
	                              'byte' => array( 'code' => "c", 'len' => 1, 'sql' => 'tinyint'),
	                              'ubyte' => array( 'code' => "C", 'len' => 1, 'sql' => 'tinyint UNSIGNED' ),
	                              'float' => array( 'sub' => "process_float", 'len' => 4, 'sql' => 'float' ),
	                              'double' => array( 'code' => 'd', 'len' => 8 ),
	                              'string' => array( 'sub' => "process_string", 'sql' => 'varchar(25)' ),
		// bytes takes arbitrary number of bytes and creates a hex'd string
	                              'bytes' => array( 'sub' => "process_bytes" ),
	                              'blob' => array( 'sub' => "process_string", 'sql' => 'blob' ),
	                              'edid' => array('sub' => "process_edid", 'sql' => 'int' ),
	                              'infoid' => array('sub' => "process_infoid", 'sql' => 'varchar(255)' ),
	                              'dialid' => array('sub' => "process_dialid", 'sql' => 'varchar(255)' ),
	                              'formid' => array( 'sub' => "process_formid", 'len' => 4, 'sql' => 'int'),
	                              'raw_formid' => array( 'sub' => "process_raw_formid", 'len' => 4),
	                              'mgefid' => array( 'sub' => "process_mgefid", 'len' => 2, 'sql' => 'int' ),
	                              'mgefidl' => array( 'sub' => "process_mgefid", 'len' => 4, 'sql' => 'int' ),
	                              'mgefstr' => array( 'sub' => "process_mgefstr", 'len' => 4, 'sql' => 'int'),
	                              'boolean' => array( 'sub' => "process_boolean", 'len' => 1, 'sql' => 'boolean' ),
	                              'boolean2' => array( 'sub' => "process_boolean", 'len' => 2, 'sql' => 'boolean' ),
	                              'boolean4' => array( 'sub' => "process_boolean", 'len' => 4, 'sql' => 'boolean' ),
	                              'rev_boolean' => array( 'sub' => "process_rev_boolean", 'len' => 1, 'sql' => 'boolean' ),
	                              'rev_boolean2' => array( 'sub' => "process_rev_boolean", 'len' => 2, 'sql' => 'boolean' ),
	                              'rev_boolean4' => array( 'sub' => "process_rev_boolean", 'len' => 4, 'sql' => 'boolean' ),
	                              'gmst' => array( 'sub' => 'process_gmst', 'sql' => 'varchar(255)'),
	                              'glob' => array( 'sub' => "process_glob", 'len' => 4, 'sql' => 'float'),
	                              'lvld' => array( 'sub' => "process_lvld", 'len' => 1, 'sql' => 'tinyint UNSIGNED' ),
	                              'systemtime' => array( 'sub' => "process_systemtime", 'len' => 16 ),
		// special processing for PLDT/PTDT subrecord contents in PACK records (can be formids OR numbers)
		// save as ordid or *negative* of number
	                              'packid' => array( 'sub' => 'process_packid', 'len' => 4, 'sql' => 'int'),
	                              'packid_PLDT' => array( 'sub' => 'process_packid_PLDT', 'len' => 8, 'sql' => 'int'),
	                              'packid_PTDT' => array( 'sub' => 'process_packid_PTDT', 'len' => 8, 'sql' => 'int'),
	                              'str_index' => array('sub' => 'process_str_index', 'len' => 4, 'sql' => 'varchar(255)'),
	                              'blob_index' => array('sub' => 'process_str_index', 'len' => 4, 'sql' => 'blob'),
	                              'keyword' => array('sub' => 'process_keyword', 'sql' => 'int'),
	                 );
	
	// Info that isn't being represented:
	// * data2/data3/etc cases
	// * whether a subrec is repeatable ('multi', 'subset')
	// * sqltable
	// * description
	// * (ignoring keep, but not relevant here)
	// But, because the stats are post-processing, some things that are captured:
	// * dependable data/array format
	// * expanding vallist loops of variables (esp in NPC_ records)
	$gOutput->text('<h3>Record Format</h3>'."\n");
	$gOutput->text('<p>This is a simplified summary of the record format that was used by this program to extract data from the original game file.  It is not necessarily the correct format, nor is it intended to provide complete information about the format.  Subrecords that do not appear in this list are not currently being read.  Any formats that appear in <span class="unchecked">red italics</span> are complete guesses and have not yet been confirmed in any way.</p>'."\n");
	
	$gOutput->init_table();
	$gOutput->set_columns(array('subrec' => array('title' => 'Subrecord'),
	                            'byte' => array('title' => 'Byte Start'),
	                            'len' => array('title' => 'Length'),
	                            'format' => array('title' => 'Format'),
	                            'varname' => array('title' => 'Variable Name'),
	                            'extra' => array('title' => 'Extra'),
	                           ));
	$gOutput->start_table();
	foreach ($format as $subrec => $subdata) {
		/*		print '<pre>';
		var_dump($subdata);
		print '</pre>';*/
		$data = 'data';
		if (!isset($subdata[$data])) {
			// if multiple datas, pull out the longest one
			// (doesn't really represent cases where datas provide completely different variables)
			if (isset($subdata['data3']))
				$data = 'data3';
			elseif (isset($subdata['data2']))
				$data = 'data2';
			elseif (isset($subdata['data1']))
				$data = 'data1';
			else
				continue;
		}
		$byte = 0;
		foreach ($subdata[$data] as $i => $ddata) {
			if (!empty($ddata['extra']))
				continue;
			if (isset($ddata['index']))
				$byte = $ddata['index'];
			$len = NULL;
			$dtype = NULL;
			if (isset($datatype[$ddata['type']])) {
				$dtype = $datatype[$ddata['type']];
				if (isset($dtype['len']))
					$len = $dtype['len'];
			}
			if (isset($ddata['len']))
				$len = $ddata['len'];
			
			$gOutput->start_new_row();
			if (!$i)
				$gOutput->add_row_value('subrec', $subrec);
			$gOutput->add_row_value('byte', $byte);
			if (isset($len)) {
				$gOutput->add_row_value('len', $len);
				$byte += $len;
			}
			else
				$gOutput->add_row_value('len', '*');
			if (isset($ddata['val'])) {
				$val = $ddata['val'];
				$checked = $gCSData->is_datachecked($_GET['rec'], $ddata['val']);
				if (!$checked)
					$val = '<span class="unchecked">'.$val.'</span>';
				$gOutput->add_row_value('varname', $val);
			}
			elseif (!empty($ddata['flag']))
				$gOutput->add_row_value('varname', '(flags)');
			$gOutput->add_row_value('format', $ddata['type']);
			
			if (!empty($ddata['flag'])) {
				// add to extra column
				$text = 'Flag values:<ul>';
				foreach ($ddata['flag'] as $f => $val) {
					if (!isset($val))
						continue;
					$checked = $gCSData->is_datachecked($_GET['rec'], $val);
					$hex = sprintf('0x%x', (1 << $f));
					if ($checked) {
						$text .= "<li>$hex=$val</li>\n";
					}
					else {
						$text .= '<li class="unchecked">'."$hex=$val".'</li>'."\n";
					}
				}
				$text .= '</ul>';
				$gOutput->add_row_value('extra', $text);
			}
			elseif (!empty($ddata['lookup'])) {
				// add to extra column
				$lu = $gCSData->get_lookup_list($ddata['lookup']);
				$text = 'Lookup values:<ul>';
				if (count($lu)<20) {
					foreach ($lu as $f => $val) {
						$text .= "<li>$f=$val</li>\n";
					}
					$text .= '</ul>';
					$gOutput->add_row_value('extra', $text);
				}
			}
			elseif (!empty($ddata['factor'])) {
				$gOutput->add_row_value('extra', 'Multiplied by '.$ddata['factor']);
			}
			$gOutput->end_row();
		}
	}
	
	$gOutput->end_table();
}

function display_rectype($rectype) {
	global $gCSData, $gOutput, $gamenames, $reclist;
	$query_limit = 100;
	
	$itemset = CSItem::newitem($rectype, NULL, 3, $gCSData);
	$itemset->start_itemset(array('limit' => $query_limit, 'offset' => $_GET['offset']));
	//	$itemset->start_itemset(array('limit' => 30, 'offset' => $_GET['offset']));
	
	$gOutput->set_param($gamenames[$_GET['game']]." $rectype Records", 'page_title');
	
	$text = '<div style="float:right;width=30%">'."\n";
	$text .= '<b>Options:</b><ul>'."\n";
	$text .= '<li>See <a href="'.$_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'].'&stats=1">record statistics</a></li>'."\n";
	$text .= '<li>Get this page in <a href="'.$_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'].'&format=wiki">wiki format</a></li>'."\n";
	$text .= '<li>Get this page in <a href="'.$_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'].'&format=csv">csv format</a></li>'."\n";
	$text .= "</ul></div>\n";
	$gOutput->text($text);
	
	$query = $itemset->get_itemset_query(array('getcount' => TRUE));
	$row = $gCSData->do_query($query, 'onerow');
	$ntotal = $row['count'];
	if (empty($_GET['offset']))
		$rec0 = 0;
	else
		$rec0 = $_GET['offset'];
	$recf = min($rec0+$query_limit-1, $ntotal-1);
	
	$gOutput->text("Displaying records $rec0-$recf of $ntotal\n");
	$gOutput->text($itemset->display_filters());
	
	$offsets = '';
	if ($ntotal>$query_limit) {
		$url = $_SERVER['PHP_SELF'].'?'.$_SERVER['QUERY_STRING'];
		$url = preg_replace('/&offset=\d*/', '', $url);
		$offsets = '<div style="margin-top: 3px">Go to record:';
		$n = floor($rec0/$query_limit);
		$m = floor($ntotal/$query_limit);
		$dispvals = array(0, 1, 2, $n-2, $n-1, $n, $n+1, $n+2, $m-2, $m-1, $m);
		$dispvals = array_unique($dispvals);
		sort($dispvals);
		foreach ($dispvals as $i => $off) {
			if ($off<0)
				continue;
			if ($off*$query_limit>$ntotal)
				continue;
			if ($off*$query_limit==$rec0) {
				$link = '<b>'.($off*$query_limit).'</b>';
			}
			else {
				$link = '<a href="'.$url.'&offset='.($off*$query_limit).'">'.($off*$query_limit).'</a>';
			}
			if ($off>0 && ($dispvals[$i-1]<$off-1)) {
				$offsets .= '&nbsp;...';
			}
			$offsets .= '&nbsp;';
			$offsets .= $link;
		}
		$offsets .= '</div>';
	}
	
	if (!empty($offsets))
		$gOutput->text($offsets);
	$gOutput->init_table();
	$gOutput->set_columns($itemset->get_display_columns('list_'.$_GET['format']));
	$gOutput->start_table();
	// need to skip duplicate entries
	// (where .esp modifies an entry from original .esm)
	// wikioutput should perhaps merge entries that are identical-except-for-formid
	$last_id = NULL;
	while ($item=$itemset->next_itemset()) {
		if (isset($last_id) && $item->get('id')==$last_id)
			continue;
		$gOutput->table_row($item);
		$last_id = $item->get('id');
		$item->release();
	}
	$gOutput->end_table(FALSE);
	if (!empty($offsets))
		$gOutput->text($offsets);
	$gOutput->finish();
}

function display_cell($formid, $edid, $ordid) {
	global $gCSData, $gOutput;
	
	$item = NULL;
	foreach (array('formid', 'edid', 'ordid') as $id) {
		if (empty($$id))
			continue;
		$item = $gCSData->get_item($$id, NULL, $_GET['rectype'], array('strictmatch' => TRUE));
		if (!empty($item))
			break;
	}
	
	if (empty($item)) {
		display_main();
		return;
	}
	$_GET['rec'] = $item->get('rectype');
	get_history(TRUE);
	$item->display_item_data();
}

function display_script($script, $sid) {
	global $gCSData, $gOutput;
	$row = NULL;
	if ($gCSData->hastable('scripts')) {
		$query = 'SELECT * from scripts WHERE ';
		if (!empty($sid))
			$query .= 'sid='.$sid;
		else
			$query .= 'filename=\''.addslashes($script).'\'';
		$row = $gCSData->do_query($query, 'onerow');
	}
	if (empty($row)) {
		display_main();
		return;
	}

	$gOutput->set_param($row['filename'].' Script', 'page_title');
	$text = '<pre>'.$row['contents'].'</pre>';
	$gOutput->text($text);
	$gOutput->finish();
}

function display_more_search($search) {
	global $reclist, $gCSData, $gOutput;

	$gOutput->set_param('Advanced Search', 'page_title');
	
	$url = $_SERVER['PHP_SELF'];
	$text = '';
	$text .= '<form action"'.$url.'" method="get">'."\n";
	$text .= '<input type="hidden" name="game" value="'.$_GET['game'].'" />'."\n";
//	if (!empty($_GET['rec']))
//		$text .= '<input type="hidden" name="rec" value="'.$_GET['rec'].'" />'."\n";
	$text .= '<input type="hidden" name="do_more" value="1" />'."\n";
	$text .= '<input type="text" size="50" maxlength="100" name="search" value="'.htmlentities($_GET['search']).'"/>'."\n";
	$text .= '<table class="wikitable"><tr><td>'."\n";
	if (!empty($reclist)) {
		$text .= 'Limit search to records of type:<br/>'."\n";
		$text .= '<select name="rec">'."\n";
		$text .= "<option value=\"\">-- Search All --</option>\n";
		foreach ($reclist as $rectype => $nsub) {
			$text .= "<option value=\"$rectype\"";
			if ($rectype==$_GET['rec'])
				$text .= ' selected="1"';
			$text .= ">$rectype</option>\n";
		}
		$text .= "</select>\n";
		$text .= "</td><td>\n";
	}
	$text .= 'Search for text in:<br/>'."\n";
	$text .= '<input type="checkbox" name="do_name" checked="1"/>Record name<br/>'."\n";
	$text .= '<input type="checkbox" name="do_edid" checked="1"/>EditorID<br/>'."\n";
	if ($gCSData->hastable('scripts'))
		$text .= '<input type="checkbox" name="do_script" checked="1"/>Script name<br/>'."\n";
	$text .= '<input type="checkbox" name="do_book" />Book text<br/>'."\n";
	$text .= '<input type="checkbox" name="do_dialogue" />Dialogue text<br/>'."\n";
	$text .= '<input type="checkbox" name="do_contents" />Script text<br/>'."\n";
	$text .= '</td></tr></table>'."\n";
	$text .= '<input type="submit" value="Search" />'."\n";
	$text .= '</form>';
	$gOutput->text($text);
	$gOutput->finish();
}

// Note that $search has *not* been sanitized
function display_search($search) {
	global $gCSData;
	
	$error = NULL;
	$list = array();
	if (preg_match('/^0x[0-9a-z]{0,2}[0-9a-f]{1,6}$/i', $search) && empty($_GET['do_more'])) {
		// preg_match sanitizes for formid
		$formid = $search;
		$item = $gCSData->get_item($formid);
		// should add option here to search to recognize 0x?? or 0xxx as wildcards
		if (!empty($item)) {
			$list[0] = array('formid' => $formid);
		}
		else {
			$error = 'Unable to find an entry matching FormID '.$formid;
		}
	}
	else {
		// search names and edids -- search for exact match first, then use like
		for ($iexact=(empty($_GET['do_more'])?1:0); $iexact>=0; $iexact--) {
			$list = array();
			$reclist = array();
			foreach (array('name', 'edid') as $chkname) {
				if (!empty($_GET['do_more']) && empty($_GET['do_'.$chkname]))
					continue;
				$query = 'SELECT ordid, name, edid, formid, rectype FROM AllItems WHERE ';
				if ($iexact) {
					$query .= "$chkname='".addslashes($search)."' ";
				}
				elseif ($chkname=='name') {
					$query .= " $chkname LIKE '%".addcslashes($search, "'%_\\")."%' ";
				}
				else {
					$query .= " CONVERT($chkname USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
				}
				if (!empty($_GET['do_more']) && !empty($_GET['rec']))
					$query .= " AND rectype='".$_GET['rec']."'";
				$query .= ' AND mod_status&0x2>1';
				$query .= ' ORDER BY ';
				$query .= ' length(name), length(edid), fileid, formid, edid, name';
				$query .= ' LIMIT 50';

				$res = $gCSData->do_query($query);
				if (empty($res))
					continue;
//				$nrows = $res->num_rows;
//var_dump($nrows);
//				if (!$nrows)
//					continue;
				
				while ($row=$gCSData->row_query($res)) {
					$row['formid'] = $gCSData->get_mod_formid($row['formid']);
					$list[$row['ordid']] = $row;
					if (isset($_GET['rec']) && $row['rectype']==$_GET['rec']) {
						$reclist[$row['ordid']] = $row;
					}
				}
			}
			if (count($list)>=1)
				break;
		}
	}
	// Only search scripts if original list is empty -- don't want to have scripts and edids getting intermingled
	if ($gCSData->hastable('scripts') && ((empty($list) && empty($_GET['do_more'])) || !empty($_GET['do_script']))) {
		$query = "SELECT sid, filename FROM scripts WHERE filename LIKE '%".addcslashes($search, "'%_\\")."%' AND mod_status&0x2>1 ";
		$query .= " ORDER BY length(filename), filename";
		$query .= ' LIMIT 50';
		$res = $gCSData->do_query($query);
		if (!empty($res)) {
			while ($row=$gCSData->row_query($res)) {
				$list[] = array('script' => $row['filename']);
			}
		}
	}

	if (count($list)<50 && !empty($_GET['do_book'])) {
		if ($_GET['game']=='mw') {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join BOOK_Record using (ordid) WHERE CONVERT(TEXT USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
		}
		else {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join BOOK_Record using (ordid) WHERE CONVERT(var_DESC USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
		}
		$query .= ' AND mod_status&0x2>1';
		$query .= ' ORDER BY ';
		$query .= ' fileid, formid, edid, name';
		$query .= ' LIMIT 50';
		$res = $gCSData->do_query($query);
		if (!empty($res) && $res->num_rows) {
			while ($row=$gCSData->row_query($res)) {
				$row['formid'] = $gCSData->get_mod_formid($row['formid']);
				$list[$row['ordid']] = $row;
			}
		}
	}
	if (count($list)<50 && !empty($_GET['do_dialogue'])) {
		if ($_GET['game']=='sr') {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join INFO_TRDT_Record using (ordid) WHERE CONVERT(dialogue USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
		}
		elseif ($_GET['game']=='mw') {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join INFO_Record using (ordid) WHERE CONVERT(infotext USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
		}
		// Oblivion
		else {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join INFO_Record using (ordid) WHERE CONVERT(NAM1 USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
		}
		$query .= ' AND mod_status&0x2>1';
		$query .= ' ORDER BY ';
		$query .= ' fileid, formid, edid, name';
		$query .= ' LIMIT 500';
		
		$res = $gCSData->do_query($query);
		if (!empty($res) && $res->num_rows) {
			while ($row=$gCSData->row_query($res)) {
				$row['formid'] = $gCSData->get_mod_formid($row['formid']);
				$list[$row['ordid']] = $row;
			}
		}
	}
	if (count($list)<50 && !empty($_GET['do_contents'])) {
		if ($_GET['game']=='sr' && $gCSData->hastable('scripts')) {
			$query = "SELECT sid, filename FROM scripts WHERE CONVERT(contents USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
			$query .= ' AND mod_status&0x2>1';
			$query .= ' ORDER BY ';
			$query .= ' filename';
		}
		else {
			$query = "SELECT ordid, name, edid, formid, rectype FROM AllItems inner join SCPT_Record using (ordid) WHERE CONVERT(SCTX USING utf8) COLLATE utf8_unicode_ci LIKE '%".addcslashes($search, "'%_\\")."%' ";
			$query .= ' AND mod_status&0x2>1';
			$query .= ' ORDER BY ';
			$query .= ' fileid, formid, edid, name';
		}
		$query .= ' LIMIT 50';
		
		$res = $gCSData->do_query($query);
		if (!empty($res) && $res->num_rows) {
			while ($row=$gCSData->row_query($res)) {
				if (isset($row['filename']))
					$list[] = array('script' => $row['filename']);
				else {
					$row['formid'] = $gCSData->get_mod_formid($row['formid']);
					$list[$row['ordid']] = $row;
				}
			}
		}
	}
	
	if (count($list)==1) {
		$ordid = array_pop(array_keys($list));
		foreach (array('formid', 'edid', 'ordid', 'script') as $idname) {
			if (isset($list[$ordid][$idname])) {
				header("Location: ".$_SERVER['PHP_SELF'].'?game='.$_GET['game'].'&'.$idname.'='.$list[$ordid][$idname]);
				return;
			}
		}
	}
	print start_page('Search Results');
	if (!empty($list)) {
		if (!empty($reclist) && count($reclist)<count($list)) {
			$list = $reclist;
			$recmatch = TRUE;
		}
		else {
			$recmatch = FALSE;
		}
		if (count($list)>=500) {
			print '<p>More than 500 matches found; only first 500 matches are being shown.';
		}
		else {
			print '<p>'.count($list).' matches found.';
		}
		if ($recmatch) {
			$url = $_SERVER['PHP_SELF'].'?game='.$_GET['game'].'&search='.urlencode($_GET['search']);
			print ' Only '.$_GET['rec'].' record types were searched.  To search all records, <a href="'.$url.'">click here</a>.';
		}
		elseif (count($list)>=50 && !isset($list[0]['script'])) {
			print ' To limit the search to a single record type, first select that record type from the left-hand menu.';
		}
		print '</p>'."\n";
		print '<ul>'."\n";
		foreach ($list as $row) {
			print '<li>';
			$text = array();
			$idstr = '';
			foreach (array('name', 'edid', 'formid', 'script', 'ordid') as $idname) {
				if (empty($row[$idname]))
					continue;
				if (empty($idstr) || $idname!='ordid') {
					$idstr = $idname.'='.$row[$idname];
				}
				if (!empty($text) && $idname=='ordid')
					continue;
				$text[] = htmlentities($row[$idname], ENT_COMPAT, 'UTF-8');
			}
			if ($_GET['game'] === 'mw' && isset($row['rectype']))
				$idstr .= '&rectype=' . $row['rectype'];
			foreach ($text as $i => $str) {
				if ($i)
					print " / ";
				print '<a href="'.$_SERVER['PHP_SELF'].'?game='.$_GET['game'].'&'.$idstr.'">'.$str.'</a>';
			}
			if (isset($row['rectype']))
				print ' ('.$row['rectype'].')';
			print '</li>';
		}
		print '</ul>'."\n";
	}
	else {
		print "Unable to find results matching: <i>".htmlentities($search, ENT_COMPAT, 'UTF-8').'</i>';
	}
	print end_page();
}

function display_history() {
	global $gCSData;
	$title = 'History of Database Updates';
	if (!empty($_GET['rec']))
		$title .= ' for '.$_GET['rec'].' Records';
	print start_page($title);
	
	$query = 'SELECT History.* FROM History ';
	if (!empty($_GET['rec'])) {
		$query .= " INNER JOIN History AS H2 on (History.setid=H2.setid AND H2.rectype='".$_GET['rec']."') ";
	}
	$query .= ' WHERE History.rectype IS NULL ORDER BY History.setid DESC LIMIT 50';
	$res = $gCSData->do_query($query);
	print "<ul>\n";
	while ($row=$gCSData->row_query($res)) {
		if (empty($_GET['rec'])) {
			$squery = 'SELECT rectype FROM History WHERE setid='.$row['setid'].' AND rectype is NOT NULL ORDER BY rectype LIMIT 11';
			$sres = $gCSData->do_query($squery);
			$nsub = $sres->num_rows;
		}
		print "<li>\n";
		print $row['time'];
		if (!empty($row['text']))
			print ': '.$row['text'];
		print "\n";
		if (empty($_GET['rec'])) {
			print "<ul><li>\n";
			if ($nsub>10) {
				print 'More than 10 record types updated';
			}
			else {
				print "$nsub record type".(($nsub==1)?'':'s').' updated';
				if ($nsub>0 && $nsub<=10) {
					print ': ';
					for ($s=0; $srow=$gCSData->row_query($sres); $s++) {
						if ($s)
							print ', ';
						print $srow['rectype'];
					}
				}
			}
			print "</li></ul>\n";
		}
		print "</li>\n";
	}
	print "</ul>\n";
	
	print end_page();
}

// want a way to add title
function start_page($title=NULL) {
	global $reclist, $gamenames;
	$gametitle = $gamenames[$_GET['game']].' Game Data';
	
	if ($_GET['format']!= 'html') {
		if ($_GET['format']=='wiki')
			header("Content-type: text/plain; charset=utf-8");
		else
			header("Content-type: text/plain; charset=utf-8");

		$text = '';
		return $text;
	}
	
	header("Content-type: text/html; charset=utf-8");
	
	$text = '';
	$text .= "<html>\n";
	$text .= "<head>\n";
	$text .= "<title>".(empty($title)?$gametitle:$title)."</title>\n";
	$text .= '<link rel="stylesheet" type="text/css" href="cslist.css?1" media="all">'."\n";
	$text .= '<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />'."\n";
	$text .= '<META NAME="ROBOTS" CONTENT="NOINDEX, NOFOLLOW">'."\n";
	$text .= "</head>\n";
	$text .= "<body>\n";
	$text .= '<div id="globalWrapper">'."\n";
	
	$url = $_SERVER['PHP_SELF'];
	$text .= '<div class="section">'."\n";
	$text .= "<h1><a href=\"$url?game=".$_GET['game']."\">$gametitle</a></h1>\n";
	
	$text .= '<table width="100%" class="noborder"><tr><td width="150 px" style="border-right: solid thin #aaaaaa;">'."\n";
	$text .= '<b>Select game:</b><br/>'."\n";
	$text .= '<form action="'.$url.'" method="get"><select name="game" onchange="this.form.submit()">'."\n";
	foreach ($gamenames as $abbrev => $game) {
		$text .= '<option value="'.$abbrev.'"';
		if ($_GET['game']==$abbrev)
			$text .= ' selected';
		$text .= '>'.$game.'</option>'."\n";
	}
	$text .= "</select></form>\n";
	
	$text .= '<br/><b>Search:</b>'."\n";
	$text .= '<form action="'.$url.'" method="get">'."\n";
	$text .= '<input type="hidden" name="game" value="'.$_GET['game'].'" />'."\n";
	if (!empty($_GET['rec']))
		$text .= '<input type="hidden" name="rec" value="'.$_GET['rec'].'" />'."\n";
	$text .= '<input type="text" size="15" maxlength="100" name="search" />'."\n";
	$text .= '<input type="submit" value="Search" />'."\n";
	$text .= '<input type="submit" name="more" value="More..." />'."\n";
	$text .= "</form>\n";
	
	if (!empty($reclist)) {
		$text .= '<br/><b>Select a record type:</b>'."\n";
		$text .= "<ul>\n";
		foreach ($reclist as $rectype => $nsub) {
			$text .= '<li><a href="'.$url.'?game='.$_GET['game'].'&rec='.$rectype.'">'."$rectype ($nsub)</a></li>\n";
		}
		$text .= "</ul>\n";
	}
	
	$text .= "</td>\n";
	$text .= '<td>'."\n";
	if (!empty($title))
		$text .= "<h2>$title</h2>\n";
	return $text;
}

function end_page() {
	global $gHistory;
	if ($_GET['format']!= 'html') {
		return '';
	}
	
	$text = '';
	if (!empty($gHistory)) {
		$text .= '<div width="100%" style="text-align:center">'."\n";
		$text .= '<hr>';
		$link = $_SERVER['PHP_SELF'].'?history=1&game='.$_GET['game'];
		if (!empty($gHistory['DB']['active'])) {
			$text .= '<i style="color:DarkRed">Database is <a href="'.$link.'">currently</a> being updated</i>'."\n";
		}
		else {
			$text .= '<i>Database last updated <a href="'.$link.'">';
			$text .= display_time($gHistory['DB']['time']);
			$text .= '</a></i>'."\n";
		}
		if (isset($_GET['rec']) && isset($gHistory[$_GET['rec']])) {
			$text .= '<br/>';
			$link .= '&rec='.$_GET['rec'];
			if (!empty($gHistory[$_GET['rec']]['active'])) {
				$text .= '<i style="color:DarkRed">'.$_GET['rec'].' records are <a href="'.$link.'">currently</a> being updated</i>'."\n";
			}
			else {
				$text .= '<i>'.$_GET['rec'].' records last updated <a href="'.$link.'">';
				$text .= display_time($gHistory[$_GET['rec']]['time']);
				$text .= '</a></i>'."\n";
			}
		}
		$text .= "</div>\n";
	}
	
	$text .= '</td></tr></table>'."\n";
	$text .= "</div></div></body></html>\n";
	return $text;
}

function display_time($time) {
	$ts = strtotime($time);
	if (empty($time) || empty($ts))
		return 'never';
	$delt = (time()-$ts)/3600;
	if ($delt<=12)
		return 'today';
	elseif ($delt<=36)
		return 'yesterday';
	elseif ($delt<=7*24)
		return 'this week';
	elseif ($delt<=14*24)
		return 'last week';
	else
		return strftime('%m/%d/%Y', $ts);
}
