<?php
//PHP Error handling functions
require_once('error.php.inc');



//Set the internal character encoding to UTF-8
mb_http_output('UTF-8');
mb_regex_encoding('UTF-8');
mb_internal_encoding('UTF-8');



//Set the include path to make it easier to use include() and require()
set_include_path(get_include_path() . PATH_SEPARATOR . getcwd());



//we DONT need compression from PHP itself
//web server / reverse proxy handles this task for us
ini_set('zlib.output_compression', 'Off');
ini_set('zlib.output_compression_level', '0');



//DEFINE PHP_VERSION_ID IF NOT ALREADY DEFINED
if (!defined('PHP_VERSION_ID')) {
	$version = explode('.', PHP_VERSION);
	define('PHP_VERSION_ID', ($version[0] * 10000 + $version[1] * 100 + $version[2]));
}



//Getvar Library (GET/POST variables)
require_once('_getvar/getvar.php.inc');
$get = new getvar;



//Main Library
require_once('common.php.inc');



//Altaform Base Code
require_once('altaform.php.inc');



//Altaform Object Types
require_once('types.php.inc');



//URL Parser
require_once('url.php.inc');



//Set the content type for this document to HTML with UTF-8 encoding
if (!headers_sent()) {
	if (strpos($get->server('HTTP_USER_AGENT'), 'MSIE') !== false) {
		header('X-UA-Compatible: IE=edge,chrome=1');
	}
	header('Content-Type: text/html; charset=utf-8');
	header('Content-Language: en_US');
}



//Host Information for Config
$host	= $get->server('HTTP_HOST');
$pos	= strpos($host, ':');
if ($pos) $host = substr($host, 0, $pos);



//Main Configuration File
if (is_file("_config/$host/config.php.inc")) {
	require_once("_config/$host/config.php.inc");
} else if (is_file("_config/virtual/config.php.inc")) {
	require_once("_config/virtual/config.php.inc");
} else {
	error500('Unknown Domain: ' . $get->server('HTTP_HOST'));
}



//Initialize afurl
$afurl->_all();



//Upgrade to HTTPS connection
if ($get->server('HTTP_UPGRADE_INSECURE_REQUESTS')) {
	if (!$afurl->https  &&  !empty($site['secure'])) {
		header('Vary: upgrade-insecure-requests');
		header('Cache-Control: no-cache');
		$afurl->redirect('https'.substr($afurl->all, 4), 307);
	}
	if ($afurl->https) {
		header('Content-Security-Policy: upgrade-insecure-requests');
	}
}



//Set the time zone and locale
date_default_timezone_set(
	empty($site['timezone']) ? 'UTC' : $site['timezone']
);

setlocale(LC_CTYPE,
	empty($site['ctype']) ? 'en_US.UTF-8' : $site['ctype']
);



//HTTP Options / origins
if (!empty($site['origins'])) {
	if (in_array($get->server('HTTP_ORIGIN'), $site['origins'])) {
		header('Access-Control-Allow-Origin: '.$get->server('HTTP_ORIGIN'));
		header('Access-Control-Allow-Methods: POST, GET, OPTIONS');
		header('Access-Control-Allow-Headers: Content-Type, Cache-Control, X-Requested-With');
		header('Access-Control-Allow-Credentials: true');
	}
	if (strtolower($get->server('REQUEST_METHOD')) === 'options') {
		return;
	}
}



//PHP Universal Database Library (PUDL)
if (!empty($site['pudl'])  &&  is_array($site['pudl'])) {
	if (!isset($site['pudl']['type'])  &&  isset($site['pudl']['server'])) {
		$site['pudl']['type'] = is_array($site['pudl']['server']) ? 'Galera' : 'MySqli';
	}
	if (!empty($site['pudl']['type'])) {
		$pudltype = 'pudl'.$site['pudl']['type'];
		require_once('_pudl/'.$pudltype.'.php');
		$db = $pudltype::instance($site['pudl']);
		if (!empty($site['pudl']['connected'])) call_user_func($site['pudl']['connected']);
	}
}



//Initialize all the things!
$dbsession = false;
if (isset($db)) {
	$db->debugger('_pudl_debug');
	$get->pudl = $db;

	//PUDL Session Handler
	require_once('_pudl/pudlSession.php');
	$dbsession = new pudlSession($db, 'pudl_session',
		isset($site['session_name'])	? $site['session_name']		: false,
		isset($site['session_domain'])	? $site['session_domain']	: false
	);
}



//Initialize Altaform class
$af = altaform::create( $dbsession );
$db->time($af);
$af->login();



//Parse the URL and load the page!
while ($afurl->reparse) {
	$afurl->reparse = false;
	$afpath = $afurl->process();
	if (is_string($afpath)) require($afpath);
	chdir($afurl->directory);
}
