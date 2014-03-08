The ZlibDecompress.php file contains the class ZlibDecompress, which requires the HuffmanTable.php file/class to work. 
To use it, simply call the inflate() method:

$zlib = new ZlibDecompress;
$inflated = $zlib->inflate(substr($compressed_stream,2));

It can be used in conjunction with gzuncompress() function, calling it only when the latter fails to decompress the stream:

ini_set("memory_limit","-1");
$inflated = @gzuncompress($stream);
if (!$inflated) {
  require_once("ZlibDecompress.php");
  $zlib = new ZlibDecompress;
  $inflated = $zlib->inflate(substr($stream, 2));    
}

Note that when using the inflate() method you must cut the first two bytes (i.e. the zlib stream header) from the compressed pdf stream.
