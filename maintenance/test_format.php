#!/usr/bin/php -q
<?php
/**
 * Collection of functions that can be used to scan a new esm file with unknown format
 * Tested on Fallout3, but really set up in preparation for Skyrim

 * Thoughts:
 * 1) Just try reading file with scanrecs TRUE
 *  * Does file conform to standard record format?
 *  * See whether minor tweaks are enough to get things going (# of bytes in size? Variants on XXXX?)
 *  * Worst case, start scanning for strings and look for patterns
 * 2) Look for familiar rectypes and subrecord types
 *  * Are the subrecord lengths similar to OB values?
 *  * How far can I get by plugging in OB defns?  And then try subtle tweaks to OB defns
 * 3) I probably want to be ready to export the results of my experiments to wiki pages
 *  * Originally just a list of record types, tables of subrecords for each record
 *  * Plus perhaps stats on frequency, size, etc.

 *  * Once I start decoding defns -- should I immediately write results to DB?
 *   (would make it easier to do comparisons/analyses ... but harder to see effects of tweaking
 *    binary -> data conversion)

 * * Want to be able to re-read one set of items without destroying rest of DB
 *   So I can analyse one chunk of items at a time
 * * In which case, plan would be to do first scan to fill AllItems table
 *   Then re-do one rectype at a time

 * Issues with early analyses:
 * 1) how to exclude test data (test NPCs, test cells, etc)
 *    in general, assume that I'll generate a list of everything to start with, and someone else will manually
*    filter the list
 * 2) how to exclude conversations and other non-quests from quest list -> whether or not qust_indx records exist
 * 3) can't distinguish between generic NPCs (Bandit Marauder) and individual NPCs
 * 4) getting location names....
 * where do marker names come from?
*   in OB, REFRs to markers have names (AngaMapMarker has name 'Anga')
*   ditto in FO 
*   base item is MapMarker (*not* XMarker) -> ordid=13491
*  'SELECT * from AllItems left join REFR_Record using (ordid) where base_id=13491 and name is not null'
*  For SR, probably want to check all STATs where edid like '%Marker' -> see which entries have names
* 
* WRLDs:
*  SELECT * from AllItems where rectype='WRLD'
*
* CELLs:
*  SELECT * from AllItems where rectype='WRLD' and name is not null
*  (*MUST* be limited somehow, otherwise list takes minutes to print)
*  But presumably most of these names are interiors -- and any of interest are duplicated in marker list?

 *    when do I want cell names; when do I want marker names (can I read marker info?)
 *    interior vs exterior
 *    attaching cells to a single entrance
 *    dungeons vs houses

 * Need to test what happens if datadef is longer than actual data

 * Want to setup some functions to compare subrec lens with expected lengths from input format
 * Note that first scan *MUST* get REFR/ACHR/ACRE NAME
 * Need to track parent groups for each rectype
 * Do I even want to track entire group structure?

 * To run with format-testing functions:
 * Need to first have done initial read into DB
 * $cs->set_param(TRUE, 'scanrecs')
 * $cs->set_param(TRUE, 'testrecs')
 * (Make sure redo_db false, too)
 * .. and probably want to make sure that test_sub_format is skipping CELL/REFR/INFO on first pass
 * Need to set up FOList and pass it to FORead

 * Be ready to auto-gen Mod_File_Format table?
 * Or let others do it -- potentially from info provided at webpage?

 */
require_once "foread.inc";
//$file = 'Fallout3.esm';
//$file = 'Fallout3.esm.mod';
$file = 'FalloutNV.esm';
$cs = new FORead();

//require_once "csread.inc";
//$file = 'Oblivion.esm.mod';
//$cs = new CSRead();

$cs->set_param(FALSE, 'skip_compressed');

$cs->set_param(TRUE, 'doprint');
//$cs->set_param(FALSE, 'doprint');
$cs->set_param(TRUE, 'scanrecs');
//$cs->set_param(TRUE, 'test_compress');

$cs->open_file($file);
$start = $cs->extract_single(12);
print "First 12 characters: ".$cs->display_raw_data($start)."\n";
$cs->close_file();

/*
$cs->file_seek(185401092);
//$cs->file_seek(8842942);
$header = $cs->read_test_record();

$finalsize = $cs->extract_single('ulong');
$compressed_data = $cs->extract_single($header['size']-4);
$encode = base64_encode($compressed_data);
print "full size = ".$finalsize."\n";
print "compressed size = ".strlen($compressed_data)."\n";
print "start = ".substr($encode,0,20)."\n";
print "end = ".substr($encode,-20)."\n";

require_once ('zlib/ZlibDecompress.php');
$zlib = new ZlibDecompress;
//$chunk = gzuncompress($compressed_data);
$chunk = $zlib->inflate(substr($compressed_data,2));
// tests that do not work:
// gzinflate
// taking off first 4 bytes
// adding recommended string to start of data
if ($chunk!==FALSE) {
	print "test uncompress successful, size =".strlen($chunk)."\n";
	$encode = base64_encode($chunk);
	print "start = ".substr($encode,0,20)."\n";
	print "end = ".substr($encode,-20)."\n";
}

$recompress = gzcompress($chunk);
print "recompressed length = ".strlen($recompress)."\n";
$encode = base64_encode($recompress);
print "start = ".substr($encode,0,20)."\n";
print "end = ".substr($encode,-20)."\n";

exit;
*/

//Error extracting compressed data at loc 185401092
//finalsize = 4385 origsize=4088
//base64_encoded=
//start = eJwlVodbU9cbvhCy9yZs
//end = bWDJNCZih/wfbnBOzA==
//ref = H4sIAAAAAAA=

/* If file format is OK  */
$cs->read_file($file);
$cs->print_reclist();

/* Scan to find string locations
$cs->open_file($file);
$cs->read_test_record();
$cs->close_file();

$cs->open_file($file);
$cs->extract_single(12);
for ($i=0; $i<30; $i++) {
	$cs->find_string(4);
}
$cs->close_file();
*/

/* Skip first record, and test second record
$cs->open_file($file);
$cs->file_seek(54);
while ($cs->read_next()) {
}
*/
/*
$cs->open_file($file);
$cs->file_seek(6028445);
//$cs->file_seek(6030220);
$cs->read_test_record();
for ($i=0; $i<30; $i++) {
	$cs->find_string(4);
}
*/
