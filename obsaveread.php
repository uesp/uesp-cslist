<?php
require_once "obdata.inc";
require_once "obsaveread.inc";

/*$str = pack('f', -4.);
print $str."\n";
for ($i=0; $i<strlen($str); $i++) {
	$s = substr($str,$i,1);
	list($x, $n) = unpack('C', $s);
	printf("%d     %3d   0x%02x\n", $i, $n, $n);
}*/

$cs = new OBData();
// this should fill in all the lookups from obread.inc.... but is it still needed?
$cs->get_data();

if ($argc>1 && substr($argv[$argc-1],-4)=='.ess') {
	$filename = $argv[$argc-1];
}
else {
	$filename = 'text2609.ess';
}
//$save = new OBSaveRead('Save 2605 - Nenya - Arch-Mage\'s Tower Council Chambers, Level 6, Playing Time 41.28.20.ess', $cs);
//$save = new OBSaveRead('xboxsave1313.ess', $cs);
//$save = new OBSaveRead('xboxsave0451.ess', $cs);
//$save = new OBSaveRead('textsave2606.ess', $cs);
//$save = new OBSaveRead('xboxsave2427.ess', $cs);
print "processing file $filename\n\n";

$save = new OBSaveRead($filename, $cs);

$save->read();

/*

what is update.esp?  can I get it from xbox?  .... and then, what is script at formid 0x010047af?
need text version of 2617 or 2650 (flag 16 on ACHR); 2912 or 2950 (misc. flag 3 with data)
can I also get a text version of a file where Empty Flag size>2?
check values of enchanted items in 2451

Need to redo knownflags (use flagnames instead) to do a better job of picking up new flags
Are there flags other than 20 than have fallen through the cracks?
*/

// what to do with data after reading it?  how to integrate with default data?
// be able to read xbox saves that still have CON record -- not possible; instead just detect and return error
// do still need to check SI treatment -- make it so that for 0x00 objects that were redefined by
//  SI, the SI version is loaded if the save file uses SI; or else, use the vanilla version
// check version numbers -- I should be able to test both v125 and v126 save files
//  (do both v125 and v126 records appear in same save file?)

/*

Original info on Save_File_Format pages probably took info primarily from text files
BUT section sizes in text files frequently wrong
-- many lines in text files have 0 listed for size of all subrecords -- whenever item supposedly "NOT LOADED"
-- lines with multiple 6/7/etc records have lock(9), owner(7) -- but sizes don't total to match actual total
-- or when Moved and Havok Moved both present, 2 listed as size of Havok Moved -- but is that just 6/7/etc section header?

Conclusions from analysis:
 Q: Do Created REFRs show up in createdList? A: No

Had Havok Move Flag only in non-loaded records
Significance of Enabled/Disabled flag.... 
need to check whether enabled/disabled changed on base objects
.... not possible, because enabled/disabled never changed on base objects (unless flag!=30 for those objects)

Loaded objects are ones in current cell, ones in Tamriel's base cell (0x00023777 -> the cell where items are
  put that are explicitly supposed to stay loaded)

Is there extra data in ACRE records?  How much extra in ACHR?
Can any overall flags be used to infer meaning of properties? e.g., cell changed
What new overall flags are there?

why are there sections of iref where formids are all 0?  And worse yet, why are those irefs then being used?

Game Modifiers(22) and Magic Modifiers(20) UESP doc suspect
- impossible for size of Magic Modifiers to be 0, but value regularly seen (min value would be 2)
- if that is the format, it's not located at position 41 with other data of that format
  (cases with 22+20 set, but nfinal=0)
- do stats on records?  scan for possible sections?  check whether offsets consistent?
- stats just on sizes
*/