#!/usr/bin/php -q
<?php
require_once "csdata.inc";
$rectype = "WEAP";
$csread = new MWRead();
$wk = new MWTable($rectype);

//$csread->set_doprint();
$csread->get_data($rectype, CSRead::KEEPOPT_ALL);
$cs = $csread->read();

$styles = $cs->get_item_groups($rectype, 'styles');

foreach ($styles as $style) {
	
	$set = $cs->get_item_set($rectype, $style);
	print "$style\n";
	foreach ($set as $key => $item) {
		print "  $key ";
		if (!is_null($item))
			print $item->edid(). " ". $item->name()." ".$item->get('skill');
		print "\n";
		
	}
	
	// add check whether all items are same armor class... use cuirass for table section, not just set[0]
	$set = $cs->get_item_set($rectype, $style, 'base');
	$wk->set_columns($rectype, 'style_page');
	$wk->start_table();
	$wk->table_section("{$style} Weapons", $set[0]);
	foreach ($set as $subset)
		$wk->table_row($subset);
	$wk->end_table();
	print "\n";
	
	foreach ($set as $subset) {
		foreach ($subset as $item) {
			$matches = $cs->find_icon_matches($item);
			if (count($matches)) {
				print $item->name()." (".$item->edid().")\n";
				foreach ($matches as $mitem) {
					print "  ".$mitem->name()." (".$mitem->edid().")\n";
				}
			}
		}
	}
	print "\n\n\n";
}
?>
