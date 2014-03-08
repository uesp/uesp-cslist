<?php
/* This was originally named wikitable.inc
   Moved to OutputFormat to be a more generalized output-formatting filter
   -> set up OutputFormat once at start of a script, then put all output through it
   -> needs to be able to handle all of the content for a webpage, not just the table
   -> needs to handle plain text, HTML, wiki, and csv outputs

   Original uses of these functions have *not* been updated

   Needs to add some general html sanitizing to all HTML output
   Want to do it early in formatting -- before any HTML tags are added by formatting
   But need to make sure it only gets done once to a given value
*/
include_once 'common.inc';
include_once 'csitem.inc';

class OutputFormat {
	protected $_fpout;
	
	// will actually need to get even fancier with NS: Morrowind/Tribunal/Bloodmoon; Oblivion/Shivering
	// skills, base items should use main namespace, other links use local namespace
	protected $_params =
		array('namespace' => 'Oblivion',
		      'tabletype' => NULL,
		      'details' => NULL,
		      'tablehead' => NULL,
		      'start_function' => NULL,
		      'end_function' => NULL,
		      'page_title' => NULL,
		      '_isheader' => FALSE,
		      '_firstout' => TRUE,
		      '_finish_done' => FALSE,
		     );
	protected $_rowvalues;
	protected $_rowstring;
	protected $_columns;
	
	// standard column definitions
	// format can be a single value or an array of formatting functions, which will be applied in order
	protected $_coldata =
		array(/*'icon' => array('summable' => 0, 'title' => '', 'format' => 'icon'),*/
		      'name' => array('summable' => 0, 'title' => 'Name', 'format' => 'le'),
		      'id' => array('summable' => 0, 'title' => 'ID', 'format' => 'id'),
		      'formid' => array('summable' => 0, 'title' => 'FormID', 'format' => array('formid', 'id')),
		      'edid' => array('summable' => 0, 'title' => 'edID', 'format' => 'id'),
		      'fileid' => array('title' => 'Source File', 'format' => 'fileid'),
		      'mod_status' => array('title' => 'Mod Status', 'format' => 'mod_status'),
		      'weight' => array('summable' => 1, 'title' => 'Weight', 'format_title' => 'title_icon'),
		      'health' => array('summable' => 1, 'title' => 'Health', 'format_title' => 'title_icon'),
		      'value' => array('summable' => 1, 'title' => 'Value', 'format_title' => 'title_icon'),
		      'rating' => array('summable' => 0, 'title' => 'Rating', 'format_title' => 'title_icon'),
		      'enchantpts' => array('summable' => 1, 'title' => 'Enchant'),
		      'choprng' => array('title' => 'Chop'),
		      'slashrng' => array('title' => 'Slash'),
		      'thrustrng' => array('title' => 'Thrust'),
		      'damage' => array('title' => 'Damage', 'format' => 'num_tenths', 'format_title' => 'title_icon'),
		      'ingr_id' => array('title' => 'Ingredient', 'format' => 'ordid_as_name'),
		     );
	
	function __construct($fpout=NULL) {
		//if (!isset($fpout))
		//	$this->_fpout = STDOUT;
		if (!isset($fpout))
			$this->_fpout = fopen('php://output', 'w');
		else
			$this->_fpout = $fpout;
	}
	
	function __destruct() {
		if (empty($this->_params['_finish_done']))
			$this->finish();
	}
	
	public static function getInstance($format=NULL, $fpout=NULL) {
		// needs to also accomodate game-specific subclasses, such as MWOutputWiki
		$format = ucfirst(strtolower($format));
		$class = 'Output'.$format;
		if (class_exists($class))
			return new $class($fpout);
		else
			return new OutputText($fpout);
	}
	
	function set_param($value, $param) {
		// Don't allow external code to alter internal params
		if (substr($param,0,1)!='_')
			$this->_params[$param] = $value;
	}
	
	function set_start_functions($start, $end=NULL) {
		$this->_params['start_function'] = $start;
		$this->_params['end_function'] = $end;
	}
	
	// Primarily here so html can add page header/footer
	function start_output() {
		if (empty($this->_params['_firstout']))
			return;
		if (!empty($this->_params['start_function'])) {
			$function = $this->_params['start_function'];
			$text = $function($this->_params['page_title']);
			fwrite($this->_fpout, $text);
		}
		$this->_params['_firstout'] = FALSE;
		$this->_params['_finish_done'] = FALSE;
	}
	
	function finish() {
		if (!empty($this->_params['end_function'])) {
			$function = $this->_params['end_function'];
			$text = $function();
			fwrite($this->_fpout, $text);
		}
		$this->_params['_finish_done'] = TRUE;
	}
	
	function text($text) {
		$this->start_output();
		fwrite($this->_fpout, $text);
		//fwrite($this->_fpout, $this->preformat($text));
	}
	
	function start_list() {
		$this->start_output();
	}
	function list_item($text, $format=TRUE) {
		if ($format)
			$text = $this->preformat($text);
		fwrite($this->_fpout, $text."\n");
	}
	function end_list() {
	}
	
	// some of contents were originally in __construct
	function init_table($tabletype=NULL, $details=NULL) {
		$this->_params['tabletype'] = $tabletype;
		$this->_params['details'] = $details;
		
		$this->_columns = array('name', 'id', 'weight', 'value', 'magic_desc');
		$this->_params['tablehead'] = "class=\"wikitable sortable\"";
		$this->_params['tableend'] = '';
		
		$this->set_default_columns();
	}
	
	//placeholder for now... being overwritten by derived class' function
	function set_default_columns($tabletype=NULL, $details=NULL) {
		if (!is_null($tabletype))
			$this->_params['tabletype'] = $tabletype;
		if (!is_null($details))
			$this->_params['details'] = $details;
	}
	
	// columns can either be a simple array of column names
	// or it can also contain extra information for _coldata
	function set_columns($columns) {
		$this->_columns = array();
		foreach ($columns as $col => $coldata) {
			if (is_integer($col)) {
				$col = $coldata;
				$coldata = NULL;
			}
			$this->_columns[] = $col;
			if (isset($coldata) && is_array($coldata)) {
				if (!isset($this->_coldata[$col])) {
					$this->_coldata[$col] = $coldata;
				}
				else {
					$this->_coldata[$col] = array_merge($this->_coldata[$col], $coldata);
				}
			}
		}
	}
	
	function get_columns($option=NULL) {
		$output = array();
		foreach ($this->_columns as $column) {
			if (is_null($option) || (!empty($this->_coldata[$column][$option])))
				array_push($output, $column);
		}
		return $output;
	}
	
	function set_cell_format($column, $format) {
		$this->_rowvalues[$column]['format'] = $format;
	}
	// would like to integrate skip and colspan into one call...
	// set direction to TRUE to simply skip column (regardless of what that does for HTML tables)
	// set direction to left/right/top/bottom to merge this cell with the cell in given direction
	// * currently only left/right options work
	function set_cell_skip($column, $direction=TRUE) {
		$this->_rowvalues[$column]['skip'] = $direction;
	}
	
	function start_table() {
		$this->start_output();
		$this->table_header();
	}
	
	function table_header() {
		$this->start_new_row(true, true);
		foreach ($this->_columns as $column) {
			$this->add_row_value($column, $this->format_title($column));
		}
		$this->end_row();
	}
	
	function start_new_row($isheader=false, $isfirst=false) {
		$this->_params['_isheader'] = $isheader;
		$this->_rowvalues = array();
		$this->_rowstring = "";
	}
	
	function add_row_value($column, $value, $addformat=false, $item=NULL, $index=NULL) {
		if (!$addformat)
			$this->_rowvalues[$column]['value'] = $value;
		else {
			$outstr = $this->format_value($column, $value, $item, $index);
			$this->_rowvalues[$column]['value'] = $outstr;
		}
	}
	
	function format_title($column) {
		$this->_params['_currcolumn'] = $column;
		if (isset($this->_coldata[$column]['title'])) {
			$title = $this->_coldata[$column]['title'];
		}
		else {
			$title = ucwords(str_replace('_', ' ', $column));
		}
		$title = $this->preformat($title);
		if (isset($this->_coldata[$column]['format_title'])) {
			if (is_array($this->_coldata[$column]['format_title'])) {
				foreach ($this->_coldata[$column]['format_title'] as $format) {
					$format = 'format_'.$format;
					if (method_exists($this, $format))
						$title = $this->$format($title);
				}
			}
			else {
				$format = 'format_'.$this->_coldata[$column]['format_title'];
				if (method_exists($this, $format))
					$title = $this->$format($title);
			}
		}
		return $title;
	}
	
	function format_value($column, $value, $item=NULL, $index=NULL) {
		$this->_params['_currcolumn'] = $column;
		$value = $this->preformat($value);
		if (isset($this->_coldata[$column]['format'])) {
			if (is_array($this->_coldata[$column]['format'])) {
				foreach ($this->_coldata[$column]['format'] as $format) {
					$format = 'format_'.$format;
					if (method_exists($this, $format))
						$value = $this->$format($value, $item, $index);
				}
			}
			else {
				$format = 'format_'.$this->_coldata[$column]['format'];
				if (method_exists($this, $format))
					$value = $this->$format($value, $item, $index);
			}
		}
		return $value;
	}
	
	function end_row() {
		$prevformat = false;
		
		$collast = NULL;
		$colskip = 0;
		$colspan = array();
		for ($ic=0; $ic<count($this->_columns); $ic++) {
			$column = $this->_columns[$ic];
			if (empty($this->_rowvalues[$column]['skip'])) {
				$colspan[$ic] = 1 + $colskip;
				$colskip = 0;
				$collast = $ic;
			}
			elseif ($this->_rowvalues[$column]['skip']=='left' && isset($collast)) {
				$colspan[$ic] = 0;
				$colspan[$collast]++;
			}
			elseif ($this->_rowvalues[$column]['skip']=='right') {
				$colspan[$ic] = 0;
				$colskip++;
			}
		}
		
		for ($ic=0; $ic<count($this->_columns); $ic++) {
			$column = $this->_columns[$ic];

			if (!isset($this->_rowvalues[$column]['value']))
				$this->_rowvalues[$column]['value'] = '';
			
			$this->_start_cell($ic, $column, $colspan[$ic]);
			if (empty($this->_rowvalues[$column]['skip']))
				$this->_add_cell_to_row($ic, $column);
			$this->_end_cell($ic, $column, $colspan[$ic]);
		}
		
		fwrite ($this->_fpout, $this->_rowstring);
		fwrite ($this->_fpout, "\n");
	}
	
	function _start_cell($ic, $column, $colspan=NULL) {
	}
	function _end_cell($ic, $column, $colspan=NULL) {
	}
	
	// primarily here for sake of csv output -- allow any final formatting tweaks to be done if necessary
	function _add_cell_to_row($ic, $column) {
		$this->_rowstring .= $this->_rowvalues[$column]['value'];
	}
	
	function table_section($title, $data=NULL) {
	}
	
	function total_row(&$data) {
		if (is_null($data))
			return;
		$this->start_new_row();
		if ($this->_columns[0]=='icon') {
			// Should get this to go through set_cell_skip/set_cell_format functions
			// ... and then have those functions generalize for different output formats
			// But for now, 'format' is just ignored by non-HTML-ish formats
//			$this->_rowvalues['icon']['skip'] = true;
//			$this->_rowvalues[$this->_columns[1]]['style'] = 'colspan=2';
			$this->_rowvalues['icon']['skip'] = 'left';
			$istart=2;
		}
		else {
			$istart=1;
		}
		$this->add_row_value($this->_columns[$istart-1], "Totals");
		for ($i=$istart; $i<count($this->_columns); $i++) {
			$column = $this->_columns[$i];
			if (!isset($data[$column]))
				$outstr = "";
			else
				$outstr = $data[$column];
			$outstr = $this->format_value($column, $outstr);
			$this->add_row_value($column, $outstr);
		}
		$this->end_row();
	}
	
	// $data is either a single CSItem
	// *OR* an array of multiple CSItems, all of which are being printed within a single row
	// (e.g., for left and right gloves in MW)
	function table_row(&$data, $index=NULL) {
		if (is_null($data))
			return;
		
		$this->start_new_row();
		$empty = TRUE;
		foreach ($this->_columns as $column) {
			$outitems = array();
			if (!is_array($data)) {
				$value = $data->get($column, $index);
				if (!is_array($value)) {
					$outitems[] = $this->format_value($column, $value, $data, $index);
				}
				else {
					foreach ($value as $val) {
						$outitems[] = $this->format_value($column, $val, $data, $index);
					}
				}
			}
			else if (count($data)>0) {
				$allsame = true;
				$val0 = $data[0]->get($column, $index);
				foreach ($data as $item) {
					if ($item->get($column, $index)!=$val0)
						$allsame = false;
				}
				
				for ($i=0; $i<count($data); $i++) {
					if ($i && $allsame)
						break;
					$item = $data[$i];
					$value = $item->get($column, $index);
					if ($value!==false && $value!=='') {
						$outitems[] = $this->format_value($column, $value, $item, $index);
					}
				}
			}
			$outstr = $this->combine_cell_items($outitems);
			if ($outstr!=='')
				$empty = FALSE;
			$this->add_row_value($column, $outstr);
		}
		if (!$empty)
			$this->end_row();
	}
	
	function combine_cell_items($outitems) {
		return implode('; ', $outitems);
	}

	function end_table($finish=TRUE) {
		$this->_params['tabletype'] = NULL;
		if (!empty($this->_params['tableend']))
			fwrite($this->_fpout, $this->_params['tableend']);
		if ($finish)
			$this->finish();
	}
	
	// placeholder for HTML to do htmlentities
	function preformat($text) {
		return $text;
	}
	
	function format_num_comma($value, $item=NULL, $index=NULL) {
		if (is_null($value) || $value==='')
			return $value;
		$tempstr = sprintf("%g",$value);
		$subs = preg_split('/\./', $tempstr);
		$outstr = number_format($subs[0]);
		if (count($subs)>1) {
			$outstr .= ".".$subs[1];
		}
		return $outstr;
	}
	
	function format_num_tenths($value, $item=NULL, $index=NULL) {
		// length check is to make function work on numbers like 99.55
		$outstr = sprintf("%.1f", $value);
		if (strlen($outstr)<=strlen($value))
			return $outstr;
		else
			return $value;
	}
	
	function format_null_dashes($value, $item=NULL, $index=NULL) {
		if (is_null($value) || !$value)
			return "—";
		else
			return $value;
	}
	
	function format_gender($value, $item=NULL, $index=NULL) {
		if (!$value)
			return 'Male';
		else
			return 'Female';
	}
	
	function format_ordid_as_name($value, $item=NULL, $index=NULL, $useid=TRUE) {
		global $gCSData;
		if (empty($value))
			return '';
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$titem = $list->get_t_item($value);
		if (empty($titem))
			return $value;
		$unique = $titem->get('mod_status_unique');
		$text = $titem->get('name');
		if (empty($text))
			$text = $titem->get('edid');
		$formid = $titem->get('formid');
		if (!empty($formid)) {
			$formid = $this->format_formid($formid, $item, $index);
			if (empty($text))
				$text = $formid;
			elseif ($useid)
				$text .= ' ('.$formid.')';
		}
		if (!empty($text)) {
			if (!$unique)
				$text .= ' *';
			return $text;
		}
		else
			return $value;
	}
	
	function format_ordid_or_type($value, $item=NULL, $index=NULL) {
		global $gCSData;
		if (empty($value))
			return '';
		if ($value<0) {
			return -1*$value;
		}
		return $this->format_ordid_as_name($value, $item, $index);
	}

		
	function format_sid($value, $item=NULL, $index=NULL) {
		global $gCSData;
		if (empty($value))
			return '';
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$query = 'SELECT filename, mod_status from scripts where sid='.$value;
		$script = $list->do_query($query, 'onerow');
		if (empty($script))
			return $value;
		$text = $script['filename'];
		if (!empty($text)) {
			if (!($script['mod_status']&0x1))
				$text .= ' *';
			return $text;
		}
		else
			return $value;
	}
	
	function format_pack_date($value, $item, $index) {
		if (empty($value))
			return 'Any';
		return $value;
	}
	
	function format_pack_timeval($value, $item, $index) {
		if ($value>=255 || $value<0)
			return 'Any';
		return $value;
	}

	function format_pack_activity_id($value, $item, $index) {
		if (!isset($value) || $value==='')
			return $value;
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$template = $item->get('template_id');
		if (isset($template))
			$src = $list->get_item($template);
		else
			$src = $item;

		$text = NULL;
		foreach ($src->indices('activity_id') as $index) {
			$aid = $src->get('activity_id', $index);
			if ($aid==$value) {
				$text = $src->get('activity_name', $index);
				break;
			}
		}

		if ($src!=$item)
			$src->release();
		if (!empty($text))
			return $text;
		else
			return $value;
	}
	
	// Deal with operators from CTDA records
	function format_cond_op($value, $item, $index) {
		$op = ($value&0x00f0)>>4;
		$text = '';
		if ($op==0)
			$text = '==';
		elseif ($op==2)
			$text = '!=';
		elseif ($op==4)
			$text = '>';
		elseif ($op==6)
			$text = '>=';
		elseif ($op==8)
			$text = '<';
		elseif ($op==10)
			$text = '<=';
		else
			return $value;
		
		if ($value&0x01)
			$text = 'OR '.$text;
		return $text;
	}
	// attempt to guess at whether this value is a reference or just a plain number
	// likely to mess up alot with '20'
	function format_cond_param($value, $item, $index) {
		if ($value<=0)
			return $value;
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		
		if (!empty($item)) {
			$ordid = $item->convert_formid($value);
		}
		else {
			// This formid won't be correct for higher filenums....
			$formid = '0x'.sprintf("%08lx",$value);
			$ordid = $list->get_ordid($formid);
		}
		
		$titem = $list->get_t_item($ordid);
		if (empty($titem))
			return $value;
		$rectype = $titem->get('rectype');
		if (empty($rectype))
			return $value;
		if ($value<500 && ($rectype=='STAT' || $rectype=='GLOB' || $rectype=='SKIL' || $rectype=='DIAL'))
			return $value;
		// low-numbered MISC records include items such as gold
		// 4=PrisonMarker (at least for skyrim)
		if (($value==4) || ($value<100 && $rectype=='MISC'))
			return $value;
		return $this->format_ordid_as_name($titem->get('ordid'), $item, $index);
	}
	
	function format_link_rectype($value, $item=NULL, $index=NULL) {
		return $value;
	}
	
	function format_spell_source($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return $value;
		$ordids = explode(' ', $value);
		$links = array();
		foreach ($ordids as $ordid) {
			$links[] = $this->format_ordid_as_name($ordid, $item, $index);
		}
		return implode(', ', $links);
	}
	
	function format_formid($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return $value;
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		return $list->get_mod_formid($value);			
	}
	
	function format_fileid($value, $item=NULL, $index=NULL) {
		if (!isset($value))
			return $value;
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$filename = $list->get_file_name($value);
		if (!empty($filename))
			return $filename;
		else
			return $value;
	}
	
	function format_mod_status($value, $item=NULL, $index=NULL) {
		if (!isset($value))
			return $value;
		$text = '';
		if ($value&0x01) {
			$text .= 'FormID has not been modified.';
		}
		elseif ($value&0x02) {
			$text .= 'This is the default version of this FormID, but it has previously been modified. Other versions are:';
		}
		elseif ($value&0x04) {
			$text .= 'This is a modified, mod-specific version of this FormID. Other versions are:';
		}
		else {
			$text .= 'This is an out-of-date version of this FormID. Other versions are:';
		}
		$others = array();
		if (isset($item) && !($value&0x01)) {
			$list = $this->get_cslist($item);
			if (!empty($list)) {
				$query = "SELECT ordid, edid, fileid, mod_status FROM AllItems where formid='".addslashes($item->get('formid'))."' and ordid!='".$item->get('ordid')."' order by mod_status&0x01 desc, mod_status desc, fileid";
				$res = $list->do_query($query);

				while ($row = $list->row_query($res)) {
					$others[] = $row;
				}
			}
		}
		if (!empty($others)) {
			$text .= $this->format_mod_status_others($others, $item, $list);
			if (!($value&0x02)) {
				$text .= "The values from the default version of this FormID are indicated below by &quot;Default&quot;, when those values differ from this version of the FormID.";
			}
		}
		return $text;
	}

	function format_mod_status_others($others, $item, $list) {
		$text = "\n";
		foreach ($others as $row) {
			$text .= "* ".$list->get_file_name($row['fileid']);
			if ($row['mod_status']&0x2) {
				$text .= " (Default version)";
			}
			$text .= "\n";
		}
		return $text;
	}
	
	function get_cslist($item=NULL) {
		global $gCSData;
		$list = NULL;
		if (!empty($item))
			$list = $item->parentlist();
		if (empty($list)) {
			if (isset($gCSData))
				$list = $gCSData;
			else
				return NULL;
		}
		return $list;
	}
}

class OutputText extends OutputFormat {
	function table_section($title, $data=NULL) {
		fwrite($this->_fpout, "\n-- $title --\n");
	}
	
	function init_table() {
		parent::init_table();
		$this->_params['tableend'] = "\n";
	}
	
	function _start_cell($ic, $column, $colspan=NULL) {
		if ($ic)
			$this->_rowstring .= "\t";
	}
	
	function combine_cell_items($outitems) {
		return implode('; ', $outitems);
	}
}

class OutputCsv extends OutputFormat {
	// don't print non-table text
	function text($text) {
	}
	
	// don't print table_section titles
	function table_section($title, $data=NULL) {
	}
	
	function init_table() {
		parent::init_table();
		$this->_params['tableend'] = "\n";
	}
	
	function _start_cell($ic, $column, $colspan=NULL) {
		if ($ic)
			$this->_rowstring .= ",";
	}
	
	function combine_cell_items($outitems) {
		return implode('; ', $outitems);
	}
	
	// Check whether cell contains any commas, and if so embed cell in quotation marks
	function _add_cell_to_row($ic, $column) {
		$value = $this->_rowvalues[$column]['value'];
		if (strpos($value, ',')!==FALSE) {
			// replace any existing quotation marks with double-quotation-marks
			$value = preg_replace('/"/', '""', $value);
			$value = '"'.$value.'"';
		}
		$this->_rowstring .= $value;
	}
}

class OutputHtml extends OutputFormat {
	function start_list() {
		$this->start_output();
		fwrite($this->_fpout, "<ul>\n");
	}
	function list_item($text, $format=TRUE) {
		if ($format)
			$text = $this->preformat($text);
		fwrite($this->_fpout, "<li>".$text."</li>\n");
	}
	function end_list() {
		fwrite($this->_fpout, "</ul>\n");
	}
	
	function init_table() {
		parent::init_table();
		$this->_params['tableend'] = "</table>\n";
	}
	
	function start_table() {
		$this->start_output();
		// need to handle clear:both param better
		fwrite($this->_fpout, "<table ".$this->_params['tablehead']." style='clear:both'>\n");
		// calls table_header()
		parent::start_table();
	}
	
	function start_new_row($isheader=false, $isfirst=false) {
		parent::start_new_row($isheader, $isfirst);
		$this->_rowstring .= "<tr>";
		$this->_params['_prevstyle'] = FALSE;
	}
	
	function table_section($title, $data=NULL) {
		fwrite($this->_fpout, "<tr>\n");
		fwrite($this->_fpout, "<td colspan=".count($this->_columns).">");
		fwrite($this->_fpout, $title);
		fwrite($this->_fpout, "</td></tr>\n");
	}
	
	function _start_cell($ic, $column, $colspan=NULL) {
		if (isset($colspan)&&$colspan===0)
			return;
		if ($this->_params['_isheader']) {
			$this->_params['_cell_div'] = 'th';
		}
		else {
			if (isset($this->_coldata[$column]['asheader']))
				$this->_params['_cell_div'] = 'th';
			else
				$this->_params['_cell_div'] = 'td';
		}
		
		$this->_rowstring .= '<'.$this->_params['_cell_div'];
		if (isset($this->_rowvalues[$column]['style'])) {
			$this->_rowstring .= ' '.$this->_rowvalues[$column]['style'];
		}
		if (isset($colspan)&&$colspan>1) {
			$this->_rowstring .= ' colspan="'.$colspan.'"';
		}
		$this->_rowstring .= '>';
	}
	function _end_cell($ic=NULL, $column=NULL, $colspan=NULL) {
		if (isset($colspan)&&$colspan===0)
			return;
		$this->_rowstring .= '</'.$this->_params['_cell_div'].">\n";
	}
	function combine_cell_items($outitems) {
		return implode('<br>', $outitems);
	}
	
	function end_row() {
		parent::end_row();
		fwrite($this->_fpout, "</tr>\n");
	}
	
	function preformat($text) {
		return htmlentities($text, ENT_COMPAT, 'UTF-8');
	}
	
	function format_pre_tags($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return $value;
		return '<pre>'.$value.'</pre>';
	}
	
	function get_item_url($value, $item=NULL, $urltype=NULL) {
		$list = $this->get_cslist($item);
		if (empty($list))
			return NULL;
		if (!is_object($value)) {
			$titem = $list->get_t_item($value);
		}
		else {
			$titem = $value;
		}
		if (empty($titem))
			return NULL;
		$rectype = $titem->get('rectype');
		if (empty($rectype))
			return NULL;
		
		$url = $_SERVER['PHP_SELF'];
		$url .= '?game='.$_GET['game'];
		if (($formid = $titem->get('formid')) && (!isset($urltype)||$urltype=='formid')) {
			$url .= '&formid='.$this->format_formid($formid);
		}
		elseif (($edid = $titem->get('edid')) && (!isset($urltype)||$urltype!='ordid')) {
			$url .= '&edid='.urlencode($edid);
			if ($_GET['game'] == 'mw')
			{ $url .= '&rectype='.$rectype;
			}
		}
		elseif ($ordid = $titem->get('ordid')) {
			$url .= '&ordid='.$ordid;
		}
		else {
			return NULL;
		}
		return $url;
	}
	
	function get_sid_url($value, $item=NULL, $urltype=NULL) {
		$list = $this->get_cslist($item);
		if (empty($list))
			return NULL;
		$query = 'SELECT sid, filename, mod_status from scripts where sid='.$value;
		$script = $list->do_query($query, 'onerow');
		if (empty($script))
			return NULL;
		
		$url = $_SERVER['PHP_SELF'];
		$url .= '?game='.$_GET['game'];
		if ($script['mod_status']&0x2) {
			$url .= '&script='.$script['filename'];
		}
		else {
			$url .= '&sid='.$script['sid'];
		}
		return $url;
	}
	
	function format_mod_status_others($others, $item, $list) {
		$text = "\n<ul>";
		foreach ($others as $row) {
			$text .= '<li><a href="'.$this->get_item_url($row['ordid'],$item,($row['mod_status']&0x2?NULL:'ordid')).'">
'.$list->get_file_name($row['fileid']).'</a>';
			if ($row['mod_status']&0x2) {
				$text .= " (Default version)";
			}
			$text .= "</li>\n";
		}
		$text .= "</ul>";
		return $text;
	}
	
	function format_link_main($value, $item=NULL, $index=NULL) {
		if (empty($value) || empty($item))
			return $value;
		$url = $this->get_item_url($item, $item);
		if (empty($url))
			return $value;
		return '<a href="'.$url.'">'.$value.'</a>';
	}
	
	function format_link_info($value, $item=NULL, $index=NULL) {
		if (empty($value) || empty($item) || !ctype_digit($value))
			return $value;
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$query = 'SELECT ordid FROM INFO_Record WHERE INAM="'.$value.'"';
		$group = $item->get('group_id');
		if (!empty($group))
			$query .= ' AND group_id="'.$group.'"';
		// Can't use 'onerow' because there may be multiple matches .... just want to grab first
		$res = $list->do_query($query);
		if (empty($res))
			return $value;
		$info_row = $list->row_query($res);
		if (empty($info_row) || empty($info_row['ordid']))
			return $value;
		$titem = $list->get_t_item($info_row['ordid']);
		$url = $this->get_item_url($titem, $item);
		if (empty($url))
			return $value;
		return '<a href="'.$url.'">'.$value.'</a>';
	}
	
	function format_link_item($value, $item=NULL, $index=NULL) {
		$url = $this->get_item_url($value, $item);
		if (empty($url))
			return $value;
		return '<a href="'.$url.'">'.htmlentities($value, ENT_COMPAT, 'UTF-8').'</a>';
	}

	function format_link_rectype($value, $item=NULL, $index=NULL) {
		$url = $_SERVER['PHP_SELF'];
		$url .= '?game='.$_GET['game'];
		$url .= '&rec='.$value;
		return '<a href="'.$url.'">'.htmlentities($value, ENT_COMPAT, 'UTF-8').'</a>';
	}
	
	function format_ordid_as_name($value, $item=NULL, $index=NULL, $useid=TRUE) {
		if (empty($value))
			return '';
		$origval = $value;
		$value = parent::format_ordid_as_name($value, $item, $index, $useid);
		$url = $this->get_item_url($origval, $item);
		if (empty($url))
			return $value;
		return '<a href="'.$url.'">'.htmlentities($value, ENT_COMPAT, 'UTF-8').'</a>';
	}
	
	function format_sid($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return '';
		$origval = $value;
		$value = parent::format_sid($value, $item, $index);
		$url = $this->get_sid_url($origval, $item);
		if (empty($url))
			return $value;
		return '<a href="'.$url.'">'.htmlentities($value, ENT_COMPAT, 'UTF-8').'</a>';
	}
	
	function format_unchecked($value, $item=NULL, $index=NULL) {
		if (empty($value) && $value!=='0')
			return $value;
		return '<span class="unchecked">'.$value.'</span>';
	}
}

class OutputWiki extends OutputFormat {
	// don't print non-table text
	function text($text) {
	}
	
	function list_item($text, $format=TRUE) {
		if ($format)
			$text = $this->preformat($text);
		fwrite($this->_fpout, "* ".$text."\n");
	}
	
	function init_table() {
		parent::init_table();
		$this->_params['tableend'] = "|}\n";
	}
	
	function start_table() {
		$this->start_output();
		fwrite($this->_fpout, "{| ".$this->_params['tablehead']."\n");
		// calls table_header()
		parent::start_table();
	}
	
	function start_new_row($isheader=false, $isfirst=false) {
		parent::start_new_row($isheader, $isfirst);
		if (!$isfirst)
			$this->_rowstring .= "|-";
		$this->_params['_prevstyle'] = FALSE;
	}
	
	function table_section($title, $data=NULL) {
		fwrite($this->_fpout, "|-\n");
		fwrite($this->_fpout, "!colspan=".count($this->_columns)."|");
		fwrite($this->_fpout, "{{LE|".$title."}}");
		fwrite($this->_fpout, "\n");
	}
	
	function _start_cell($ic, $column) {
		if (isset($colspan)&&$colspan===0)
			return;
		if ($this->_params['_isheader']) {
			$div = "!";
			$same = 'head_sameline';
		}
		else {
			if (isset($this->_coldata[$column]['asheader']))
				$div = "!";
			else
				$div = "|";
			$same = 'sameline';
		}
		
		$adddiv = FALSE;
		if ($ic && empty($this->_params['_prevstyle']) && !empty($this->_coldata[$column][$same])) {
			if (!strlen($this->_rowvalues[$this->_columns[$ic-1]]['value']))
				$this->_rowstring .= " ";
			$adddiv = TRUE;
		}
		elseif (!empty($this->_rowstring))
			$this->_rowstring .= "\n";
		$this->_rowstring .= $div;
			
		if (isset($this->_rowvalues[$column]['style']) || (isset($colspan)&&$colspan>1)) {
			if (isset($this->_rowvalues[$column]['style']))
				$this->_rowstring .= $this->_rowvalues[$column]['style'];
			if (isset($colspan)&&$colspan>1)
				$this->_rowstring .= ' colspan='.$colspan;
			$this->_rowstring .= "|";
			$this->_params['_prevstyle'] = TRUE;
		}
		else {
			if ($adddiv)
				$this->_rowstring .= $div;
			$this->_params['_prevstyle'] = FALSE;
		}
	}
	
	function combine_cell_items($outitems) {
		return implode('<br>', $outitems);
	}
	
	// in wiki context, return the presumed wiki page name
	function get_item_url($value, $item=NULL) {
		$list = $this->get_cslist($item);
		if (empty($list))
			return NULL;
		if (!is_object($value)) {
			$titem = $list->get_t_item($value);
		}
		else {
			$titem = $value;
		}
		if (empty($titem))
			return NULL;
		$rectype = $titem->get('rectype');
		if (empty($rectype))
			return NULL;
		
		$url = $titem->file_ns();
		$url .= ':';
		
		$name = $titem->get('name');
		if (empty($name))
			return NULL;
		$url .= $name;
		return $url;
	}

	// This doesn't really have any meaning, and may be better to just return NULL....
	function get_sid_url($value, $item=NULL) {
		$list = $this->get_cslist($item);
		if (empty($list))
			return NULL;
		$query = 'SELECT sid, filename, mod_status from scripts where sid='.$value;
		$script = $list->do_query($query, 'onerow');
		if (empty($script))
			return NULL;
		
		$url = $titem->file_ns();
		$url .= ':';
		$url .= $script['filename'];
		return $url;
	}
	
	function format_link_main($value, $item=NULL, $index=NULL) {
		return $this->format_le($value, $item, $index);
	}
	
	function format_le($value, $item=NULL, $index=NULL) {
		if (!$value)
			return $value;
		$outstr = "{{LE|".$value;
		if (preg_match('/Left\s+|\s+L\s+/', $value)) {
			$value = preg_replace('/Left\s+/', '', $value);
			$value = preg_replace('/\s+L\s+/', ' ', $value);
			$outstr .= "|".$value;
		}
		$outstr .= "}}";
		return $outstr;
	}
	
	function format_formid($value, $item=NULL, $index=NULL) {
		$value = parent::format_formid($value, $item, $index);
		if (substr($value,0,2)=='0x'){
			$value = substr($value,2);
			if (substr($value,0,2)!='00')
				$value = 'xx'.substr($value,2);
			return $value;
		}
		else
			return $value;
	}
	
	function format_id($value, $item=NULL, $index=NULL) {
		if (!$value)
			return $value;
		// this is formid-specific processing, but for some reason format_formid isn't being triggered properly
		if (strlen($value)==10 && substr($value,0,2)=='0x') {
			$value = substr($value,2);
			if (substr($value,0,2)!='00')
				$value = 'xx'.substr($value,2);
		}
		$outstr = "{{ID|".$value."}}";
		return $outstr;
	}

	function format_link_name($value, $item=NULL, $index=NULL) {
		return "[[".$item->namespace().":".$value."|".$value."]]";
	}
	
	function format_link_section($value, $item=NULL, $index=NULL) {
		if (preg_match('/(.*),\s*(.*)/', $value, $matches)) {
			return "[[".$this->_params['namespace'].":".$matches[1]."#".$matches[2]."|".$value."]]";
		}
		else {
			return "[[".$this->_params['namespace'].":".$value."|".$value."]]";
		}
	}
	
	function format_mod_link($value, $item=NULL, $index=NULL) {
		if ($item->file_ns()!=$this->_params['namespace']) {
			$value .= "<sup>[[".$item->namespace().":".$item->file_ns()."|".$item->modabbrev()."]]</sup>";
		}
		elseif ($item->modname()) {
			$value .= "<sup>[[".$item->namespace().":".$item->modname()."|".$item->modabbrev()."]]</sup>";
		}
		return $value;
	}
	
	function format_title_icon($value) {
		$this->_rowvalues[$this->_params['_currcolumn']]['style'] = 'width=22';
		return '[[Image:OB'.$value.'Icon_small.png|'.$value.']]';
	}
	
	function format_magic_desc($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return $value;
		// Alchemy effects don't have initial 'type:' text
		//if (!preg_match('/([^:]*):\s*(.*)/', $value, $matches))
		//	return $value;
		if (!preg_match('/([^:]*):\s*(.*)/', $value, $matches))
			$matches = array(NULL, '', $value);
			
		$list = $this->get_cslist($item);
		if (!empty($list))
			$ns = $list->get_namespace();
		else
			$ns = NULL;
		$type = $matches[1];
		$effs = preg_split('/\s*;\s*/', $matches[2]);
		$output = '';
		if ($ns=='Morrowind')
			$output .= $type;
		foreach ($effs as $eff) {
			if (!empty($output))
				$output .= "<br>\n";
			if (preg_match('/([^,]*),\s*(.*)/', $eff, $matches)) {
				$ename = $matches[1];
				$edetails = $matches[2];
			}
			else {
				$ename = $eff;
				$edetails = '';
			}
			// need to convert skill/attribute names to Skill/Attribute; other ename conversions
			$ebase = $ename;
			$ewords = explode(' ', $ename);
			if (count($ewords)>1 && in_array($ewords[0], array('Damage', 'Drain', 'Absorb', 'Restore', 'Fortify'))) {
				if (in_array($ewords[1], array('Health', 'Magicka', 'Fatigue', 'Stamina'))) {
				}
				elseif (in_array($ewords[1], array('Agility', 'Endurance', 'Intelligence', 'Luck', 'Personality', 'Speed', 'Strength', 'Willpower'))) {
					$ebase = $ewords[0].' Attribute';
				}
				else {
					$ebase = $ewords[0].' Skill';
				}
			}
			if ($ns=='Morrowind') {
				$output .= '{{MW_EffectLink|';
				if ($ebase!=$ename)
					$output .= $ebase.'|'.$ename.'}}';
				else
					$output .= $ename.'}}';
			}
			else {
				$output .= '[['.$ns.':'.$ebase.'|'.$ename.']]';
				
				if (isset($item)) {
					$rec = $item->get('rectype');
					if ($rec=='WEAP' || $rec=='AMMO') {
						$edetails = preg_replace('/\s*on Touch$/', '', $edetails);
					}
					elseif ($rec=='CLOT' || $rec=='ARMO' || $rec=='ALCH') {
						$edetails = preg_replace('/\s*on Self$/', '', $edetails);
					}
				}
			}
			if ($edetails!='')
				$output.= " ".$edetails;
		}
		// extra line break at start of entry to fix wikitable strangness with <br> in cells
		if (count($effs))
			$output = "\n".$output;
		return $output;
	}
	
	function format_pre_tags($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return $value;
		return '<pre>'.$value.'</pre>';
	}
	
	function format_ordid_as_name($value, $item=NULL, $index=NULL, $useid=TRUE) {
		if (empty($value))
			return '';
		$origval = $value;
		$value = parent::format_ordid_as_name($value, $item, $index, FALSE);
		$url = $this->get_item_url($origval, $item);
		if (empty($url))
			return $value;
		return '[['.$url.'|'.$value.']]';
	}
	
	function format_sid($value, $item=NULL, $index=NULL) {
		if (empty($value))
			return '';
		$origval = $value;
		$value = parent::format_sid($value, $item, $index);
		$url = $this->get_sid_url($origval, $item);
		if (empty($url))
			return $value;
		return '[['.$url.'|'.$value.']]';
	}
	
	function format_ingr_eff($value, $item, $index) {
		$list = $this->get_cslist($item);
		if (empty($list))
			return $value;
		$titem = $list->get_t_item($value);
		if (empty($titem))
			return $value;
		$text = $titem->get('name');
		$value = '[['.$titem->namespace().':'.$text.'|'.$text.']]';
		
		$type = NULL;
		$ks = $titem->indices('keyword');
		foreach ($ks as $k) {
			$kywd = $titem->get('keyword', $k);
			$keyid = $list->get_t_item($kywd)->get('edid');
			if ($keyid=='MagicAlchBeneficial') {
				$type = 'good';
				break;
			}
			elseif ($keyid=='MagicAlchHarmful') {
				$type = 'bad';
				break;
			}
		}
		if (!empty($ks) && !isset($type)) {
			if ($text=='Resist Poison')
				$type = 'good';
			elseif ($text=='Frenzy')
				$type = 'bad';
			elseif ($text=='Fear')
				$type = 'bad';
		}
		
		if (isset($type)) {
			if ($type=='good') {
				$value = 'class="EffectPos"|'.$value;
			}
			else {
				$value = 'class="EffectNeg"|'.$value;
			}
		}
		
		return $value;
	}
	
}

class MWOutputWiki extends OutputWiki {
	function __construct($tabletype, $fpout=STDOUT) {
		parent::__construct($tabletype, $fpout);
		$this->_params['namespace'] = "Morrowind";
	}
	
	function set_default_columns($tabletype=NULL, $details=NULL) {
		if (!is_null($tabletype))
			$this->_params['tabletype'] = $tabletype;
		if (!is_null($details))
			$this->_params['details'] = $details;
		
		if ($this->_params['tabletype'] == 'ARMO') {
			$this->_columns = array('icon', 'name', 'edid', 'weight', 'health', 'value', 'armor', 'enchantpts');
			if ($this->_params['details']=='style_page') {
				$this->_params['tablehead'] = "class=\"wikitable vmid\" style=\"text-align:center;\"";
				$this->_coldata['name']['head_sameline'] = true;
				$this->_coldata['edid']['head_sameline'] = true;
				$this->_coldata['health']['sameline'] = true;
				$this->_coldata['value']['sameline'] = true;
				$this->_coldata['icon']['asheader'] = true;
				$this->_coldata['name']['asheader'] = true;
				$this->_coldata['armor']['asheader'] = true;
				$this->_coldata['weight']['format'] = 'num_tenths';
				$this->_coldata['health']['format'] = 'num_comma';
				$this->_coldata['value']['format'] = 'num_comma';
			}
		}
		elseif ($this->_params['tabletype'] == 'WEAP') {
# find a way to highlight which of chop/slash/thrust is preferred for that weapon?
			$this->_columns = array('icon', 'name', 'edid', 'weight', 'health', 'value', 'chop', 'slash', 'thrust', 'speed', 'reach', 'enchantpts');
			if ($this->_params['details']=='style_page') {
				array_splice($this->_columns, 3, 0, 'weaptypename');
				$this->_params['tablehead'] = "class=\"wikitable vmid\" style=\"text-align:center;\"";
				$this->_coldata['name']['head_sameline'] = true;
				$this->_coldata['edid']['head_sameline'] = true;
				$this->_coldata['slash']['head_sameline'] = true;
				$this->_coldata['thrust']['head_sameline'] = true;
				$this->_coldata['reach']['head_sameline'] = true;
				$this->_coldata['enchantpts']['head_sameline'] = true;
				$this->_coldata['health']['sameline'] = true;
				$this->_coldata['value']['sameline'] = true;
				$this->_coldata['slash']['sameline'] = true;
				$this->_coldata['thrust']['sameline'] = true;
				$this->_coldata['reach']['sameline'] = true;
				$this->_coldata['enchantpts']['sameline'] = true;
				$this->_coldata['value']['sameline'] = true;
				$this->_coldata['icon']['asheader'] = true;
				$this->_coldata['name']['asheader'] = true;
				$this->_coldata['chop']['asheader'] = true;
				//				$this->_coldata['slash']['asheader'] = true;
				//$this->_coldata['thrust']['asheader'] = true;
				$this->_coldata['weaptypename']['format'] = 'link_section';
				$this->_coldata['weight']['format'] = 'num_tenths';
				$this->_coldata['health']['format'] = array('num_comma', 'null_dashes');
				$this->_coldata['value']['format'] = 'num_comma';
				$this->_coldata['chop']['format'] = 'null_dashes';
				$this->_coldata['slash']['format'] = 'null_dashes';
				$this->_coldata['thrust']['format'] = 'null_dashes';
				$this->_coldata['speed']['format'] = 'null_dashes';
				$this->_coldata['reach']['format'] = 'null_dashes';
				$this->_coldata['enchantpts']['format'] = 'null_dashes';
			}
		}
		elseif ($this->_params['tabletype'] == 'special items') {
			$this->_columns = array('base item', 'special item', 'magic_desc', 'special type');
			$this->_coldata['base item'] = array('title' => 'Base Item');
			$this->_coldata['special item'] = array('title' => 'Special Item', 'format' => array('link_name', 'mod_link'));
			$this->_coldata['special type'] = array('title' => 'Item Type');
		}
	}
	
	function format_icon($value, $item, $index) {
		if (!$value)
			return $value;
		$name = preg_replace('/(Left|Right)\s+/', '', $item->name());
		$name = preg_replace('/\s+[LR]\s+/', ' ', $name);
		$outstr = "[[Image:MW_Icon_A_".$name.".png|".$name."]]";
		return $outstr;
	}
	
	function table_section($title, $data=NULL) {
		fwrite($this->_fpout, "|-\n");
		fwrite($this->_fpout, "!colspan=".count($this->_columns)."|");
		fwrite($this->_fpout, "{{LE|".$title."}}");
		if (!is_null($data) && $this->_params['tabletype'] == 'ARMO' && $this->_params['details']=='style_page') {
			fwrite($this->_fpout, " ([[".$this->_params['namespace'].":Base Armor#".$data->get('skill')."|".$data->get('skill').")");
		}
		fwrite($this->_fpout, "\n");
	}
}

?>
