#!/usr/bin/php -q
<?php
require_once "mwdata.inc";

$cs = new MWData();
if (0) {
	$cs->set_datamode(CSData::DATAMODE_READ);
	$csread = $cs->get_data();
	//$csread->set_doprint();
	//$csread->set_filestoread('all');
	$csread->request_data('ARMO');
	$csread->request_data('NPC_');
	$csread->request_data('LEVI');
	$csread->read();
}
else {
	$cs->get_data();
}

$armor = $cs->get_ordid('imperial helmet armor');
print "$armor\n";
$armor_item = $cs->get_item($armor);
print "weight=".$armor_item->get('weight')."\n";

$llist = $cs->get_item('l_n_armor_helmet');
print "l_n_armor_helmet id=".$llist->ordid()."\n";
print 'imperial helmet armor/l_n_armor_helmet = '.$llist->hasitem($armor)."\n";
$li = $llist->contains_list();
$plvl = $li->get_plvl($armor);
//var_dump($plvl);

$llist = $cs->get_item('l_n_armor');
print "l_n_armor id=".$llist->ordid()."\n";
print 'imperial helmet armor/l_n_armor = '.$llist->hasitem($armor)."\n";
print 'l_n_armor_helmet/l_n_armor = '.$llist->hasitem($cs->get_ordid('l_n_armor_helmet'))."\n";

$li = $llist->contains_list();
$plvl = $li->get_plvl($cs->get_ordid('l_n_armor_helmet'));
$plvl = $li->get_plvl($armor);
//var_dump($plvl);

$npc = $cs->get_item('Ekkhi');
print 'Ekkhi: '.$npc->ordid().' '.$npc->hasitem($armor)."\n";
$li = $npc->contains_list();
$plvl = $li->get_plvl($armor);
var_dump($plvl);

$indices = $npc->indices('item_id');
foreach ($indices as $i) {
	print $npc->get('item_count',$i).' '.$npc->get('item_id',$i).' '.$cs->get_edid($npc->get('item_id',$i))."\n";
}
?>