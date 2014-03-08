#!/usr/bin/php -q
<?php
require_once "obdata.inc";
$cs = new OBData();
$cs->set_datamode(CSData::DATAMODE_READ);
$csread = $cs->get_data();
$csread->set_doprint();
	//$csread->set_filestoread('all');
$csread->request_data('RACE');
$csread->read();

$race = $cs->get_item('WoodElf');
print "Bosmer\n";
for ($i=1; $i<=7; $i++) {
	$skill = $race->get('skl'.$i);
	$bonus = $race->get('skbonus'.$i);
	print "$skill $bonus\n";
}

$race = $cs->get_item('Breton');
print "Breton\n";
for ($i=1; $i<=7; $i++) {
	$skill = $race->get('skl'.$i);
	$bonus = $race->get('skbonus'.$i);
	print "$skill $bonus\n";
}
?>
