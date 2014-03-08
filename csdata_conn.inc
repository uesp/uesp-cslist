<?php

// used by the web-frontend to read data
 
include('/home/uesp/secrets/cslist.secrets');

global $gConnection;
$gConnection = mysql_connect($uespCSListReadDBHost, $uespCSListReadUser, $uespCSListReadPW, TRUE);

?>
