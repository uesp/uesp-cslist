<?php

// used by manually invoked csread to read/write data
// and create/drop tables

include('/home/uesp/secrets/cslist.secrets');

global $gConnection;
$gConnection = mysql_connect($uespCSListWriteDBHost, $uespCSListWriteUser, $uespCSListWritePW, TRUE);

?>
