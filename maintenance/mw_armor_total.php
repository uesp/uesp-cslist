#!/usr/bin/php -q
<?php
require_once "csread.inc";
require_once "wikitable.inc";
$rectype = "ARMO";
$csread = new MWRead();
$wk = new MWTable($rectype);

//$csread->set_doprint();
$csread->set_filestoread('all');
$csread->request_data($rectype, CSRead::KEEPOPT_ALL | CSRead::KEEPOPT_ENCH);
$cs = $csread->read();

$styles = $cs->get_item_groups($rectype, 'styles');
$ex_styles = $cs->get_item_groups($rectype, 'styles', 'parentonly');

$stats = $wk->get_columns('summable');
$sumdata = array();

foreach ($styles as $style) {
	$sumdata[$style] = array();
	foreach ($stats as $stat)
		$sumdata[$style][$stat] = 0;
	
	$set = $cs->get_item_set($rectype, $style);
	print "$style\n";
	foreach ($set as $key => $item) {
		print "  $key ";
		if (!is_null($item))
			print $item->edid(). " ". $item->name()." ".$item->get('skill')." ".$item->get_index('ENAM');
		print "\n";
		
	}
	
	foreach ($set as $key => $item) {
		if (is_null($item))
			continue;
		
		foreach ($stats as $stat)
			$sumdata[$style][$stat] += $item->get($stat);
	}
	print "Stats:\n";
	foreach ($stats as $stat) {
		print "   {$stat}: ".$sumdata[$style][$stat]."\n";
	}	
}
	
foreach (array_merge($styles, $ex_styles) as $style) {
	if (!is_null($cs->item_group_parent($rectype, 'styles', $style)))
		continue;
	
	$set = $cs->get_item_set($rectype, $style, 'base');
	$cuirass = array();
	foreach ($set as $itemid => $data) {
		if ($data['item']->get('armortype') == 'Cuirass') {
			$cuirass[$style] = $data['item'];
			break;
		}
	}
	
	$substyles = $cs->get_item_groups($rectype, 'styles', 'sub', $style);
	foreach ($substyles as $sub) {
		$subset = $cs->get_item_set($rectype, $sub, 'base');
# merge manually to ensure that items are in correct order
		foreach ($subset as $itemid => $data) {
			if (array_key_exists($itemid, $set) && $data['group']==$sub)
				unset ($set[$itemid]);
			if (!array_key_exists($itemid, $set))
				$set[$itemid] = $data;
			if ($data['item']->get('armortype') == 'Cuirass')
				$cuirass[$sub] = $data['item'];
		}
	}
	
	$done = array();
	
	$wk->set_columns($rectype, 'style_page');
	$wk->start_table();
	
	foreach (array_merge(array($style), $substyles, array('Other Items')) as $group) {
		$first = true;
		foreach ($set as $itemid => $data) {
			if (array_key_exists($itemid, $done) && $done[$itemid])
				continue;
			if ($data['group'] != $group)
				continue;
			$item = $data['item'];
			if ($item->get_isenchanted())
				continue;
			
			if ($first) {
				if (array_key_exists($group, $cuirass))
					$wk->table_section("{$group} Armor", $cuirass[$group]);
				else
					$wk->table_section("{$group} Armor");
				if (array_key_exists($group, $sumdata)) {
					$wk->total_row($sumdata[$group]);
				}
				$first = false;
			}
			
			$done[$itemid] = true;
			if (array_key_exists('pair', $data)) {
				$pairs = array_merge(array($item), $data['pair']);
				$wk->table_row($pairs);
				foreach ($data['pair'] as $pitem)
					$done[$pitem->id()] = true;
			}
			else 
				$wk->table_row($item);
		}
	}
	
	// add check whether all items are same armor class... use cuirass for table section, not just set[0]
	$wk->end_table();
	print "\n\n\n";
	
	$wk->set_columns('special items');
	$wk->start_table();
	
	$done = array();
	foreach ($set as $itemid => $data) {
		$item = $data['item'];
		if (array_key_exists($itemid, $done) && $done[$itemid])
			continue;
		$matches = $cs->find_icon_matches($item);
		$first = true;
		if (count($matches)) {
			foreach ($matches as $mitem) {
				if (array_key_exists($mitem->id(), $done) && $done[$mitem->id()])
					continue;
				$wk->start_new_row();
				if ($first) {
					if (count($matches)>1)
						$wk->set_cell_format('base item', 'rowspan='.count($matches));
					$wk->add_row_value('base item', "'''".$item->name()."'''<br>{{ID|(".$item->edid().")}}");
					$done[$itemid] = true;
					$first = false;
				}
				else {
					$wk->set_cell_skip('base item');
				}
				$wk->add_row_value('special item', $mitem->name(), true, $mitem);
				$wk->add_row_value('enchantment', $mitem->get('enchantment_iconized'), true, $mitem);
				$str = "";
				if (preg_match('/uniq/i', $mitem->edid())) {
					$str = "Unique";
				}
				$str .= " (".$mitem->edid().")";
				$wk->add_row_value('special type', $str);
				$done[$mitem->id()] = true;
				$wk->end_row();
			}
		}
	}
	$exlist = array();
	foreach ($set as $itemid => $data) {
		if (array_key_exists($itemid, $done) && $done[$itemid])
			continue;
		$exlist[$itemid] = $data;
	}
	if (count($exlist)) {
		$first = true;
		foreach ($exlist as $itemid => $data) {
			$item = $data['item'];
			$wk->start_new_row();
			if ($first) {
				if (count($exlist)>1)
					$wk->set_cell_format('base item', 'rowspan='.count($exlist));
				$wk->add_row_value('base item', "'''Other Related Items'''");
				$first = false;
			}
			else {
				$wk->set_cell_skip('base item');
			}
			$wk->add_row_value('special item', $item->name(), true, $item);
			$wk->add_row_value('enchantment', $item->get('enchantment'), true, $item);
			$wk->add_row_value('special type', $item->edid());
			$wk->end_row();
		}
	}
	$wk->end_table();
	print "\n\n\n";
}

?>
