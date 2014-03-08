<?php
require_once('csitem.inc');

/* Fallout-specific version of GenItem
*/
class GenItem extends CSItem {
}

/* Functions common to all functions that deal with magic effects */

class GenItem_GenMagic extends GenItem {
	public function mgef_specific_name($index) {
		if (is_null($ename = $this->get('effect', $index)))
			return NULL;
		return $ename;
	}
	protected function magic_desc_line_extra($index, $lineout) {
		return $lineout;
	}
}