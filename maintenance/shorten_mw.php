#!/usr/bin/php -q
<?php
# deleting:
#  all LAND, PGRD records
#  all CELL-FRMR's based on STAT objects (except for markers '/Marker/')
#  all CELL-FRMR's based on SOUN, LTEX objects
# unlike MW, there are no compressed records; other types of deleted records don't exist in MW

# Morrowind.esm is already openable in emacs, so from that point of view size reduction is just to make it easier for emacs to open it
# secondary purpose is to reduce the processing time when manipulating these files

# deletions selected to focus on the data that is taking up the most space
# also want to be sure that global raw searches (e.g., for a formid)
#   always find the data, even if it's in a record whose purpose is
#   currently unknown... Therefore not just deleting all unknown
#   records
require_once "csdata.inc";
$csread = new MWRead();

$csread->get_data('STAT');
$csread->get_data('SOUN');
$csread->get_data('LTEX');

$cs = $csread->read();

$idraw = array_merge($cs->get_item_ids('STAT'), $cs->get_item_ids('SOUN'), $cs->get_item_ids('LTEX'));
$ids = array();
for ($i=0; $i<count($idraw); $i++) {
	if (!preg_match('/Marker/', $idraw[$i])) {
		$ids[$idraw[$i]] = 1;
	}
}

$file = "../input/Morrowind.esm";
$fpin = fopen ($file, "r");
$fpout = fopen("$file.mod", "w");

while (!feof($fpin)) {
	read_rec();
}
fclose($fpin);
fclose($fpout);

function read_rec() {
	global $fpin, $fpout, $ids;
	
	$outstr = "";
	
	$rectype = fread($fpin, 4);
	$outstr .= $rectype;
	if (feof($fpin)) {
		fwrite($fpout, $outstr);
		return;
	}
	//	print $rectype."\n";
	$data = fread($fpin, 4);
	$outstr .= $data;
	$vals = unpack("L", $data);
	$size = $vals[1];
# unknown header, usually 0
	$data = fread($fpin, 4);
	$outstr .= $data;
	
	$data = fread($fpin, 4);
	$outstr .= $data;
	
	$loc = ftell($fpin);
	$nextrec = $loc + $size;
	
	if (preg_match('/LAND|PGRD/', $rectype)) {
		fseek($fpin, $nextrec);
		return;
	}
	if (!preg_match('/CELL/', $rectype)) {
		$outstr .= fread($fpin, $size);
		fwrite($fpout, $outstr);
		return;
	}	
	
	$infrmr = 0;
	$keepfrmr = 1;
	while (ftell($fpin)<$nextrec) {
		$recordstr = "";
		
		$type = fread($fpin, 4);
		$recordstr .= $type;
		
		$data = fread($fpin, 4);
		$recordstr .= $data;
		$vals = unpack("L", $data);
		$recsize = $vals[1];
		
		$data = fread($fpin, $recsize);
		$recordstr .= $data;
		
		if ($type=='FRMR') {
			$infrmr = 1;
			$keepfrmr = -1;
			$frmrstr = $recordstr;
		}
		elseif (!$infrmr) {
			$outstr .= $recordstr;
		}
		elseif ($type=='NAME') {
			$name = NULL;
			for ($l=0; $l<strlen($data); $l++) {
				if (substr($data, $l, 1) == "\0") {
					$name = substr($data, 0, $l);
					break;
				}
			}
			if (is_null($name))
				$name = $data;
			if (array_key_exists($name, $ids)) {
				$keepfrmr = 0;
				$frmrstr = "";
			}
			else {
				$keepfrmr = 1;
				$outstr .= $frmrstr;
				$outstr .= $recordstr;
				$frmrstr = "";
			}
		}
		elseif ($keepfrmr<0) {
			$frmrstr .= $recordstr;
		}
		elseif ($keepfrmr>0) {
			$outstr .= $recordstr;
		}
	}
	
	$newsize = strlen($outstr)-16;
	//print "$size $newsize\n";
	$outstr = substr($outstr,0,4).pack('L', $newsize).substr($outstr,8);
	fwrite($fpout, $outstr);
	
	return;
}

