#!/usr/bin/php -q
<?php
//print memory_get_usage()."\n";
//print memory_get_usage(true)."\n";
//print memory_get_peak_usage()."\n";
//print memory_get_peak_usage(true)."\n";
# program to read entire set of CS files and save to mysql database
# presumably this will only be run occasionally -- for example, when the relevance of a new set of data is
#  discovered, or when a mistake in the original data is found
# nevertheless, it does need to be possible to rerun it easily

# try to make it possible to still use CS data directly via csread if desired??
#  (in particular, it would help with debugging when data needs to be revised)

# in which case... item records need to be able to take input from either source and then be used interchangeably
# csdata needs to manage interface with a toplevel mode variable controlling which type of access
#  if in DB mode, then get_item calls trigger mysql reads and creation of appropriate item
#  only read sub-arrays (e.g., NPCO lists) as necessary? ... or just do it all, since get_item should only
#  be used on items for which I really want stats (doesn't require reading all ARMO records, for example.  And
#  secondary information, such as CELL lists will probably never get fully read in)
#  actually, read in amount of information depending upon situation.  When creating location lists, want to have
#  cells (to create cell names -- more efficient to have lists of integer IDs that get translated into names as needed)
#  but probably don't want any FRMR records (or only FRMR records that are of interest)

# create a set of DB tables using datadef information
# name tables after records (ARMO, WEAP, CELL, etc)
# for any multi records use compound name (CELL-FRMR, NPC_-NPCO)
# scan table names to determine DB structure
#   how to record lookup entries?  any other entries with extra required processing?
# ? use COMMENT option for overall table and for columns to provide information such as corresponding lookup table, format
# main Item table with common information (ordid, edid, file, rectype, etc.)

# once entire CS file has been read, need to rescan edids and change all edids to record numbers

# full expansion of leveled lists?
# - easiest way to do lookups, because recursive scans within mysql would require excessive overhead
#   (far easier to just do single SELECT WHERE <llistentry exists>)
# - will take some CPU and storage to fully implement, but the CPU cost should be one-time, instead of
#   repeatedly.  And storage isn't an issue any more.
# - if fully expanded, still want to record heritage of information

# basic CSData functions should work in either DB or CSRead mode
#  but more specialized functions will be DB only (scans for item locations, or whether item is in any leveled lists)
#  probably anything requiring CELL records and most tasks requiring leveled lists

//require_once "mwdata.inc";
//require_once "obdata.inc";
require_once "srdata.inc";

// At simplest level, read can be triggered with two commands:
//$cs = CSData::getInstance();
//$cs->get_data(CSData::DATAMODE_WRITE);

// But adding a call to set_csread allows more flexibility in setting read options

$cs = CSData::getInstance();
$csread = $cs->set_csread();
$csread->set_param(FALSE, 'redo_db');
$csread->set_param(TRUE, 'keepall');
//$csread->set_param(FALSE, 'doprint');
$csread->set_param(2, 'doprint');
$csread->set_param(FALSE, 'calc_levlists');
//$csread->set_param(TRUE, 'scanrecs');
//$csread->set_param(TRUE, 'testrecs');
$csread->set_param(FALSE, 'setup_cells');
//$csread->set_param(TRUE, 'redo_cells');

//print memory_get_peak_usage(true)."\n";
//$cs->get_data(CSData::DATAMODE_WRITE);
// note that redoing WRLD/CELL/REFR has to basically be done all together
//$cs->get_data(CSData::DATAMODE_WRITE, array('WRLD', 'CELL', 'REFR', 'ACHR', 'ACRE'));

//$cs->get_data(CSData::DATAMODE_WRITE, array('NPC_', 'LVLI', 'RACE', 'CLAS', 'WEAP', 'QUST'));
$cs->get_data(CSData::DATAMODE_WRITE);

//$cs->set_datamode(CSData::DATAMODE_WRITE);
//$cs->init_db();
//$csread->setup_cells();
//$csread->create_mod_status();

?>
