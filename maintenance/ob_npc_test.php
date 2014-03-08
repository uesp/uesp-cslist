#!/usr/bin/php -q
<?php
require_once "obdata.inc";
$cs = new OBData();
$cs->get_data();

$cs->set_pc_level(46);
#$npc = $cs->get_item('BeggarICMarketSimplicia');
#$npc = $cs->get_item('ICMarketGuardPostDay01');
$npc = $cs->get_item('SchlerusSestius');
print $npc->get('marksman')."\n";
if (0) {
foreach (array('aggression', 'confidence', 'energy', 'responsibility', 'strength', 'intelligence', 'willpower', 'personality') as $attrib) {
	print "$attrib ".$npc->get($attrib)."\n";
}
$totals = $npc->persuasion_reactions();
foreach ($totals as $reaction => $value) {
	print "$reaction $value\n";
}
}
?>
