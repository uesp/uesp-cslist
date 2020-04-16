<?php

// used by the web-frontend to read data
 
include('/home/uesp/secrets/cslist.secrets');

global $gConnection;
$gConnection = new mysqli($uespCSListReadDBHost, $uespCSListReadUser, $uespCSListReadPW, "CSData_SRData");

?>
