<?php

// used by manually invoked csread to read/write data
// and create/drop tables

include('/home/uesp/secrets/cslist.secrets');

global $gConnection;
$gConnection = new mysqli($uespCSListWriteDBHost, $uespCSListWriteUser, $uespCSListWritePW, "CSData_SRData");


