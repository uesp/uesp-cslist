<?php

function set_max(&$output, $input) {
	if (is_array($input))
		$value = max($input);
	else
		$value = $input;
	if (!is_null($value) && (is_null($output) || $value>$output))
		$output = $value;
	return $output;
}

function set_min(&$output, $input) {
	if (is_array($input))
		$value = min($input);
	else
		$value = $input;
	if (!is_null($value) && (is_null($output) || $value<$output))
		$output = $value;
	return $output;
}

?>
