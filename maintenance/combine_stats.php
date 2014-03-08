<?php

/* Also:
  number of different player chars
  max time played for each char
  number of unique completed quests?... perhaps just for my sake, to figure out which quests
   (including DLC quests) to complete (or find save game containing completion)
*/

$nread = 0;
$dp = opendir('.');
$testdata = array();
$errors = array();
while ($file=readdir($dp)) {
	if (!preg_match('/^anal.*\.out$/', $file))
		continue;
	
	$fp = fopen($file, 'r');
	$instats = false;
	while (($line=fgets($fp))!==false) {
		if ($line=="-----\n") {
			fgets($fp);
			$errline=fgets($fp);
			// skip textline copy if this file includes textlines
			if (substr($errline,0,1)==' ')
				$errline=fgets($fp);
			$errline = trim($errline);
			$errline = preg_replace('/\s*\(.*/', '', $errline);
			@ $errors[$errline]['count']++;
			@ $errors[$errline][$file]++;
		}
		if (substr($line,0,10)!='----------')
			continue;
		if (preg_match('/^-+Serialized Stats-+\s*$/', $line)) {
			$instats = true;
			break;
		}
	}
	$newdata = array();
	if ($instats) {
		while (($line=fgets($fp))!=false) {
			$spacepos = strpos($line, ' ');
			$valname = substr($line,0,$spacepos);
			$valser = trim(substr($line,$spacepos));
			$newdata[$valname] = unserialize($valser);
		}
	}
	fclose($fp);
	if (empty($newdata))
		continue;
	$nread++;
	merge_data($testdata, $newdata);
}

print "Number of anal.out files read: $nread\n\n";

print "Processed records: ".$testdata['nproc']."\nProcessed errors: ".$testdata['nprocerr']."\n";
print "Zero-size records; ".$testdata['nzero']."\n";

foreach (array('good', 'bad') as $qual) {
	print ucfirst($qual)." combos:\n";
	if (!isset($testdata['combos'][$qual]))
		continue;
	ksort($testdata['combos'][$qual], SORT_NUMERIC);
	foreach ($testdata['combos'][$qual] as $rectype => $cdata) {
		print "  $rectype (".count($cdata)." combos):\n";
		ksort($cdata, SORT_NUMERIC);
		foreach ($cdata as $combo => $num) {
			print "       \"$combo\" ($num)\n";
		}
	}
}
print "nfs by fs:\n";
ksort($testdata['nfs']);
$nfsrecs = array();
foreach ($testdata['nfs'] as $f => $rectypes) {
	$first = true;
	ksort($rectypes);
	foreach ($rectypes as $type => $fdata) {
		if ($first) {
			$first = false;
			printf("  %4d  ", $f);
		}
		else {
			print "        ";
		}
		print $type;
		printf("  %6d  %6d  %6d    %5.1f\n", $fdata['tot'], $fdata['good'], $fdata['bad'], ($fdata['bad']/$fdata['tot'])*100);
		$nfsrecs[$type] = true;
	}
}
ksort($nfsrecs);
print "nfs by rectype:\n";
foreach ($nfsrecs as $type => $x) {
	$first = true;
	foreach ($testdata['nfs'] as $f => $rectypes) {
		if (!isset($rectypes[$type]))
			continue;
		if ($first) {
			$first = false;
			print "  $type  ";
		}
		else {
			print "        ";
		}
		$fdata = $rectypes[$type];
		printf("  %4d  %6d  %6d  %6d    %5.1f\n", $f, $fdata['tot'], $fdata['good'], $fdata['bad'], ($fdata['bad']/$fdata['tot'])*100);
	}
}

print "propstats:\n";
ksort($testdata['propstats']);
foreach ($testdata['propstats'] as $prop => $pdata) {
	print "  \"$prop\":  ".$pdata['total']." total\n";
	ksort($pdata);
	if (isset($pdata['Inv']))
		print "        Inv   ".$pdata['Inv']."\n";
	foreach ($pdata as $rectype => $num) {
		if (strlen($rectype)!=4 || strtoupper($rectype)!=$rectype)
			continue;
		print "        $rectype ".$pdata[$rectype]."\n";
	}
	if (isset($pdata['follow1'])) {
		print "        follow:";
		for ($i=1; ;$i++) {
			if (!isset($pdata['follow'.$i]))
				break;
			print " $i=".$pdata['follow'.$i];
		}
		print "\n";
	}
}
if (isset($testdata['propstats']['0x4b']))
var_dump($testdata['propstats']['0x4b']);

print "textsizes:\n";
ksort($testdata['textsizes']);
foreach ($testdata['textsizes'] as $section => $sdata) {
	print "  $section: ".$sdata['num']." total\n";
	ksort($sdata);
	print "        size range: ".$sdata['min']."-".$sdata['max']."\n";
	ksort($sdata['sizes']);
	print "             vals: ".implode(array_keys($sdata['sizes'])," ")."\n";
	arsort($sdata['sizes']);
	if (count($sdata['sizes'])>1) {
		print "             common: ";
		$i=0;
		foreach ($sdata['sizes'] as $sz => $num) {
			if ($i>6)
				break;
			$i++;
			print "$sz($num) ";
		}
		print "\n";
	}
	foreach ($sdata as $rectype => $num) {
		if (strlen($rectype)!=4 || strtoupper($rectype)!=$rectype)
			continue;
		print "        $rectype ".$sdata[$rectype]."\n";
	}
}

print "textstats:\n";
foreach ($testdata['textstats'] as $section => $sdata) {
	print "  $section: \n";
	ksort($sdata['vals']);
	print "       datalen=";
	foreach($sdata['vals'] as $val => $num) {
		print " $val($num)";
	}
	print "\n";
	ksort($sdata['nfound']);
	print "       nfound=";
	$nfmax = NULL;
	foreach($sdata['nfound'] as $val => $num) {
		print " $val($num)";
		$nfmax = $val;
	}
	print "\n";
	foreach (array('forward', 'backward') as $dir) {
		arsort($sdata[$dir]);
		print "       $dir\n";
		foreach ($sdata[$dir] as $val => $num) {
			if (strpos($val,'x')!==false)
				continue;
			print "            $val($num)";
			for ($j=1; $j<=$nfmax; $j++) {
				if (!isset($sdata[$dir][$val.' x'.$j]))
					continue;
				print "     x$j(".$sdata[$dir][$val.' x'.$j].")";
			}
			print "\n";
		}
	}
}

ksort($testdata);
$donevals = array('nproc', 'nprocerr', 'nzero', 'combos', 'nfs', 'propstats', 'textsizes', 'textstats');
foreach ($testdata as $valname => $vdata) {
	if (in_array($valname, $donevals))
		continue;
	
	print "\n$valname:\n";
	var_dump($vdata);
}

print "----------- Errors -----------";

ksort($errors);
foreach ($errors as $errline => $edata) {
	print "\n$errline\n";
	print "  total = ".$edata['count']."\n";
	ksort($edata);
	foreach ($edata as $file => $count) {
		if ($file=='count')
			continue;
		print "  $file = $count\n";
	}
}

function merge_data(&$testdata, &$newdata, $topname=NULL) {
	if (!is_array($newdata)) {
		if (!isset($testdata))
			$testdata = $newdata;
		elseif (isset($topname) && $topname=='min')
			$testdata = min($testdata, $newdata);
		elseif (isset($topname) && $topname=='max')
			$testdata = max($testdata, $newdata);
		else
			$testdata += $newdata;
	}
	else {
		foreach ($newdata as $valname => $vdata) {
			if (!isset($testdata[$valname])) {
				$testdata[$valname] = $newdata[$valname];
			}
			else
				merge_data($testdata[$valname], $newdata[$valname], $valname);
		}
	}
}