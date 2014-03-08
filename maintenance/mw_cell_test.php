#!/usr/bin/php -q
<?php
require_once "mwdata.inc";
$cs = new MWData();
$cs->get_data();

$armor = $cs->get_ordid('iron_cuirass');
$armoritem = $cs->get_item($armor);
print "type_lu=".$armoritem->get('type_lu')." type=".$armoritem->get('type')."\n";
print "INDX_lu=".$armoritem->get('INDX_lu', 0)." INDX=".$armoritem->get('INDX', 0)."\n";
print "skill=".$armoritem->get('skill')."\n";
//$cs->find_locs($armor);

//$cs->_setup_cells();
exit;
$cs->find_ordid_uses(141753);
//$edid = "Bitter Coast Region [-8,-2]";
//$ws = MWItem_CELL::worldspace_from_edid($edid);
//$lu = $cs->get_lookup_id("Worldspace", $ws);

//print "$edid $ws $lu\n";
?>