<?php
require_once "mwdata.inc";
require_once "mwsaveread.inc";

$cs = new MWData();
// this should fill in all the lookups from obread.inc.... but is it still needed?
$cs->get_data();

if ($argc>1 && substr($argv[$argc-1],-4)=='.ess') {
	$filename = $argv[$argc-1];
}
else {
	$filename = 'Nephelee0001.ess';
}
print "processing file $filename\n\n";

if (0) {
$fp = fopen("/home/katja/uesp/charstats/data/".$filename, "rw");
$attrs = array(57, 50, 40, 43, 55, 49, 40, 40);
$attrdelts = array(17, 10, 0, 3, 15, 9, 0, 0);
$skills = array(41, 22, 47, 36, 20, 44);
$skilldelts = array(6, 2, 8, 1, 0, 9, 0);

$testdata = $attrs;
$imax = 0;
while (!feof($fp)) {
	$byte = fread($fp, 1);
	if (feof($fp))
		break;
	$val = my_unpack('C', $byte);
	if ($val==$testdata[0]) {
		$loc = ftell($fp);
		$found = true;
		for ($i=1; $i<count($testdata); $i++) {
			$byte = fread($fp, 1);
			$val = my_unpack('C', $byte);
			if ($val!=$testdata[$i]) {
				$found = false;
				break;
			}
		}
		
		$imax = max($i, $imax);
		if ($found) {
			print "test data found at loc $loc\n";
		}
		fseek($fp, $loc);
	}
}
fclose($fp);
print "maximum matched-sequence length $imax\n";
exit;
}

$save = new MWSaveRead($filename, $cs);

$save->read();




/*
Newly created items (custom potions, etc) have a super-long number as their edid/name
They're also listed after the NPC, so number is encountered before its definition
 (perhaps scan file first for new items, then come back for NPC data?)

Player info in 'player' NPC_ record
and in 'PlayerSaveGame' NPCC record

inventories in two places seem to contain same items, but NPCC version has extra details (health, etc)
... actually, on closer look, NPCC version has some extra items too
*/