#!/usr/bin/php -q
<?php
require_once "obdata.inc";
$cs = new OBData();
$cs->get_data();

foreach (array("DLCBattlehornDragonswordPower3", "NDLpGreavesShieldOther", "NDBlessingOfTalos", "BSRitualBlessedWord", "VampireHuntersSight") as $edid) {
	$spell = $cs->get_item($edid);
	print "spell=".$spell->get('name')." spellcost=".$spell->get('spellcost')." autocost=".$spell->get('auto_spellcost')." spelllevel=".$spell->get('spelllevel')." autolevel=".$spell->get('auto_spelllevel')." autocalc=".$spell->get('autocalc')."\n";
}

$spell = $cs->newitem('SPEL');
$spell->set_custom_spell();
$spell->add_custom_effect('Fire Damage', 5, 5, 10);
$spell->add_custom_effect('Fire Damage', 70, 1, 20);

print "custom spell\n";
foreach ($spell->indices('effect_id') as $index) {
	print $spell->magic_desc_line($index)."  cost=".$spell->effect_cost($index)."\n";
}
print "Total cost=".$spell->get('spellcost')." spelllevel=".$spell->get('spelllevel')."\n";
?>
