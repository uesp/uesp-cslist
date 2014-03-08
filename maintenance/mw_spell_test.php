#!/usr/bin/php -q
<?php
require_once "csread.inc";
$csread = new MWRead();

//$csread->set_doprint();
$csread->set_filestoread('all');

$csread->request_data('CLOT');
$csread->request_data('ENCH');
$csread->request_data('SPEL');
$csread->request_data('MGEF');
$cs = $csread->read();

//$spells = $cs->list_items_by_type('SPEL');
$spells = array($cs->get_ordid('warywrd ability'), $cs->get_ordid('akaviri danger-sense'), $cs->get_ordid('fay ability'), $cs->get_ordid('elfborn ability'), $cs->get_ordid('wombburn'), $cs->get_ordid('magicka mult bonus_5'), $cs->get_ordid('magicka mult bonus_15'));
foreach ($spells as $spell) {
	$item = $cs->get_item($spell);
	if (!is_object($item))
		continue;
	print "'".$item->get('name')."' (".$item->get('edid').")\n";
	print $item->get('magic_desc')."\n";
}

$item = $cs->get_item('mantle of woe');
print "'".$item->get('name')."' (".$item->get('edid').")\n";
print $item->get('magic_desc')."\n";
?>