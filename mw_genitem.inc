<?php
require_once('csitem.inc');

/* Morrowind-specific version of GenItem
   Override id functions to use edid as primary id instead of formid
*/
class GenItem extends CSItem {
	/* 
	   override default id-related functions
	*/
	public function formid() {
		return NULL;
	}
	
	public function id() {
		return $this->edid();
	}
	
	public function set_edid($value) {
		$this->_edid = $value;
	}
	
	public function set_id($id) {
		if (!is_null($id) && $id!='')
			$this->_edid = $id;
	}
}

class GenItem_GenMagic extends GenItem {
	// used by several records (ENCH, SPEL, etc) so put this function here
	public function eff_mag($index) {
		$magmin = $this->get('eff_magmin', $index);
		$magmax = $this->get('eff_magmax', $index);
		if (!$magmin && !$magmax) {
			$mag = '';
		}
		elseif ($magmin==$magmax) {
			$mag = $magmin;
		}
		else {
			$mag = $magmin."-".$magmax;
		}
		return $mag;
	}
	
	// special treatment for Fortify Maximum Magicka
	protected function magic_desc_line_extra($index, $lineout) {
		if (preg_match('/(Fortify Maximum Magicka,)\s*(\d*)(\s*pt[^\s]*)/', $lineout, $matches)) {
			$pts = $matches[2];
			$lineout = $matches[1].' '.sprintf('%.1f', $pts/10).'x INT ('.$matches[2].$matches[3].')';
		}
		return $lineout;
	}
	
	public function mgef_specific_name($index) {
		if (is_null($ename = $this->get('effect', $index)))
			return NULL;
		
		if (!is_null($skillname = $this->get('eff_skill', $index)))
			$ename = preg_replace('/\s+Skill/', ' '.$skillname, $ename);
		elseif (!is_null($attribname = $this->get('eff_attrib', $index)))
			$ename = preg_replace('/\s+Attribute/', ' '.$attribname, $ename);
		
		return $ename;
	}
}
