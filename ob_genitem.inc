<?php
require_once('csitem.inc');

/* Oblivion-specific version of GenItem
   Since CSItem is written in an Oblivion-centric manner, relatively few code changes are needed here
*/
class GenItem extends CSItem {
}

/* Functions common to all functions that deal with magic effects */

class GenItem_GenMagic extends GenItem {
	public function mgef_specific_name($index) {
		if (is_null($ename = $this->get('effect', $index)))
			return NULL;
		
		if (!is_null($actorname = $this->get('eff_act', $index)))
			$ename = preg_replace('/\s+(Skill|Attribute)/', ' '.$actorname, $ename);
		
		return $ename;
	}
	
	protected function magic_desc_line_extra($index, $lineout) {
		return $lineout;
	}
}
