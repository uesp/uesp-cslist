#!/usr/bin/php -q
<?php
/**
 * Collection of functions that can be used to scan a new esm file with unknown format
 * Tested on Fallout3, but really set up in preparation for Skyrim

 * Thoughts:
 * 1) Just trying reading file with scanrecs TRUE
 *  * Does file conform to standard record format?
 *  * See whether minor tweaks are enough to get things going (# of bytes in size? Variants on XXXX?)
 *  * Worst case, start scanning for strings and look for patterns
 * 2) Look for familiar rectypes and subrecord types
 *  * Are the subrecord lengths similar to OB values?
 *  * How far can I get by plugging in OB defns?  And then try subtle tweaks to OB defns
 * 3) I probably want to be ready to export the results of my experiments to wiki pages
 *  * Originally just a list of record types, tables of subrecords for each record
 *  * Plus perhaps stats on frequency, size, etc.
 */
//require_once "foread.inc";
//$file = 'Fallout3.esm';
//$cs = new FORead();

require_once "csread.inc";
$file = 'Oblivion.prepatch.esm.mod';
$cs = new CSRead();
$cs->set_param(TRUE, 'fix_compressed');

//require_once "mwread.inc";
//$file = 'Tribunal.esm';
//$cs = new MWRead();
//$cs->set_param(TRUE, 'fix_compressed');

$cs->convert_file($file);