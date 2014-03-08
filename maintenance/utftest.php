<?php
// note that before running this on content3, need to do:
// GRANT ALTER ON `CSData_OBData`.* TO CSData_user@xx
require_once "mwdata.inc";
$doconvert = FALSE;

// For converting existing data
// Can't just use mysql alter table and have mysql do the conversion, because I'm not starting with true latin-1
// So mysql would mangle/lose various punctuation characters
// So probably need to change column to binary; manually convert any entries; then change column to utf-8
// (also note that OB AllItems.name currently contains mix of utf-8 and windows-1252, because of re-reading
//  BOOK data)

$cs = CSData::getInstance();
$cs->set_connection('csread_conn.inc');
$cs->init_db();

$query = "SHOW TABLES";
$res = $cs->do_query($query);

$tchk = $tutf = $tiso = 0;
$toconv = array();
while ($row=$cs->row_query($res, MYSQL_NUM)) {
	$table = $row[0];
	
	$tquery = "SHOW FULL COLUMNS FROM {$table}";
	$tres = $cs->do_query($tquery);
	while ($trow=$cs->row_query($tres)) {
		$field = $trow['Field'];
		$type = $trow['Type'];
		if (!preg_match('/char|binary|blob/', $type))
			continue;
		if ($doconvert && isset($trow['Collation']) && $trow['Collation']=='utf8_unicode_ci')
			continue;
		// Skip IAD* rectypes
		if ($table=='Reclist' && $field=='subrec')
			continue;
		print "Checking $table . $field\n";
		if ($doconvert && preg_match('/char/', $type)) {
			$mtype = preg_replace('/char/', 'binary', $type);
			$aquery = "ALTER TABLE $table MODIFY COLUMN $field $mtype";
			if (!empty($trow['Comment'])) {
				$aquery .= " COMMENT '".$trow['Comment']."'";
			}
			$cs->do_query($aquery);
		}
				
		$cquery = "select * from $table where $field regexp '[^0-9a-zA-Z[:space:][:punct:]]'";
		$cres = $cs->do_query($cquery);
		$nchk = 0;
		$niso = 0;
		$nutf = 0;
		
		while ($crow=$cs->row_query($cres)) {
			$nchk++;
			$value = $crow[$field];
			$chk = is_utf($value);
			if (!$chk) {
				/*				print "============\n";
				print "$table $field";
				if (isset($crow['ordid']))
					print " ".$crow['ordid'];
				print "\n===========\n";
				print "$value\n";
				print "============\n";*/
				$niso++;
				$conv = iconv('Windows-1252', 'UTF-8', $value);
				$chk = is_utf($conv);
				if (!$chk)
					print "error converting $value to $conv\n";
				//				print "$conv\n";
				
				if ($doconvert && isset($crow['ordid'])) {
					$uquery = "UPDATE $table SET $field='".addslashes($conv)."' WHERE ordid=".$crow['ordid'];
					$cs->do_query($uquery);
				}
				elseif ($doconvert) {
					print "Unable to do conversion for $table . $field\n";
				}
			}
			else {
				//				print "$value\n";
				$nutf++;
			}
		}
			//		print "Checked $nchk, iso $niso, utf $nutf\n";
		$tchk += $nchk;
		$tiso += $niso;
		$tutf += $nutf;
		
		if ($niso) {
			$toconv[] = $table.'.'.$field;
		}
		elseif ($nchk) {
			print "utf in $table $field\n";
		}
		if ($doconvert && preg_match('/char/', $type)) {
			$aquery = "ALTER TABLE $table MODIFY COLUMN $field $type collate utf8_unicode_ci";
			if (!empty($trow['Comment'])) {
				$aquery .= " COMMENT '".$trow['Comment']."'";
			}
			$cs->do_query($aquery);
		}
	}
}

print "\nTotal checked $tchk, iso $tiso, utf $tutf\n";
var_dump($toconv);

function is_utf($text) {
	$nutf = 0;
	$nnot = 0;
	$not = array();
	for ($i=0; $i<strlen($text); $i++) {
		$ord = ord(substr($text,$i,1));
		$nex = 0;
		if ($ord<=127)
			continue;
		elseif ($ord<=191 || $ord>=248) {
			$nnot++;
			$not[] = $ord;
			continue;
		}
		elseif ($ord<=223)
			$nex = 1;
		elseif ($ord<=239)
			$nex = 2;
		else
			$nex = 3;
		
		for ($n=0; $n<$nex; $n++, $i++) {
			if ($nex>=strlen($text)) {
				$not[] = $ord;
				$nnot++;
				break 2;
			}
			$ordx = ord(substr($text,$i,1));
			if ($ordx<=127) {
				$not[] = $ord;
				$nnot++;
				break;
			}
		}
		if ($n>=$nex) {
			$nutf++;
		}
	}
	//	var_dump($not);
	if (!$nnot)
		return TRUE;
	else
		return FALSE;
}
