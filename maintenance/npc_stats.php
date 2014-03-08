<?php
require_once "obdata.inc";
$cs = new OBData();
$cs->get_data();

$cs->set_pc_level($level);
$npc = $cs->get_item($npc_name);
?>
<table>
<tr>
</tr>
</table>

