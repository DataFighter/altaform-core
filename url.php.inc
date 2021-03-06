<?php

require_once('_closure/closure.php');


class afurl {
	use closures;


	public function __construct() {
		global $get;

		$this->directory	= getcwd();
		$this->uri			= urldecode($get->server('REQUEST_URI', ''));
		$this->domain		= $get->server('HTTP_HOST', '');
		$this->referer		= $get->server('HTTP_REFERER', '');
		$this->https		= strtolower($get->server('HTTPS'));
		$this->https		= $this->https !== 'off'  &&  !empty($this->https);
		$this->protocol		= $this->https ? 'https' : 'http';
		$this->host			= $this->protocol . '://' . $this->domain;
		$this->af_host		= $this->host;
		$this->parts		= (array)parse_url($this->uri);
		$this->query		= str_replace(' ', '+', $this->uri . (empty($this->parts['query']) ? '?' : '&'));

		if (substr($this->uri, 0, 2) === '//') $this->redirect('/');

		if (empty($this->parts['path'])) error404();

		if ($this->parts['path'][0] !== '/') {
			$this->parts['path']  = '/' . $this->parts['path'];
		}

		if ($this->parts['path'] === '/') {
			$this->url		= '/';
			$this->part		= [];
			return;		//EARLY OUT FOR HOMEPAGE, NO FOLDERS TO PROCESS!
		}

		assert500(
			$this->parts['path'][1] !== '_',
			'HACKING ATTEMPT'
		);

		if (substr($this->parts['path'], -1, 1) === '/') {
			assert405(
				$get->server('REQUEST_METHOD') !== 'POST',
				'Attempting to redirect POST data. URL should not have trailing /'
			);
			$this->redirect(
				substr($this->parts['path'], 0, -1) .
				(empty($this->parts['query']) ? '' : ('?'.$this->parts['query']))
			);
		}

		$this->part		= explode('/', $this->parts['path']);
		$this->part[]	= '';
		foreach ($this->part as $val) {
			if (strlen($val)) $this->url .= '/' . urlencode($val);
		}
	}



	public function _all() {
		$base		= $this->host . $this->base;
		$this->full	= $base . $this->url;
		$this->all	= $this->full;
		if (!empty($this->parts['query'])  &&  $this->parts['query']!=='jq=1') {
			$this->all .= '?' . $this->parts['query'];
		}

		if (empty($this->cdn))		$this->cdn		= $base . '/cdn';
		if (empty($this->static))	$this->static	= $base . '/static';
		if (empty($this->upload))	$this->upload	= $base . '/upload';
	}



	public function __invoke($path, $base=false) {
		if (!is_array($path)) {
			return ($base ? ($this->base.'/') : '') . $this->clean($path);
		}

		$url = $base ? ($this->base.'/') : '';
		foreach ($path as $item) $url .= $this->clean($item) . '/';
		return rtrim($url, '/');
	}



	public function user($user, $base=false) {
		$user = empty($user['user_url']) ? $user['user_id'] : $user['user_url'];

		if (is_bool($base)) return $this($user, $base);

		$args = func_get_args();
		$args[0] = $user;

		return $this($args);
	}



	public function clean($fragment) {
		return ($fragment instanceof afurlsafe)
			? (string) $fragment
			: strtolower(urlencode($fragment));
	}



	public function vid($new_id=false) {
		if ($new_id !== false) $this->id = afstring::int($new_id);
		return $this->id;
	}



	public function reparse($prepend=[], $append=[], $replace=false) {
		if (empty($prepend)  &&  empty($append)) return;

		if (!is_array($prepend))	$prepend	= [$prepend];
		if (!is_array($append))		$append		= [$append];
		if ($replace)				$this->part	= [];

		$prepend	= array_reverse($prepend);

		$prepend[]	= '';
		$append[]	= '';

		array_shift($this->part);
		array_pop($this->part);

		foreach ($prepend as $item) {
			if ($item === false) continue;
			array_unshift($this->part, $item);
		}

		foreach ($append as $item) {
			if ($item === false) continue;
			$this->part[] = $item;
		}

		$this->reparse = true;
	}



	public function replace($prepend=[], $append=[]) {
		$this->reparse($prepend, $append, true);
	}



	public function process() {
		global $af, $site;

		//Recursion limit
		static $recurse = 0;
		if ($recurse++ == 20) error500('INTERNAL REDIRECT RECURSION LIMIT REACHED');

		//Reset virtual pathing, in case this is a reprocess
		$this->virtual = [];

		//	load our home page!!
		if (count($this->part) < 3) {
			return $this->reparse($this->homepage);
		}


		$count = count($this->part) - 1;

		if ($count === 2  &&  !empty($site['g_auth'][$this->part[1]])) {
			return print($site['g_auth'][$this->part[1]] . ' ' . $this->part[1]);
		}


		for ($i=1; $i<$count; $i++) {

			if (is_dir($this->part[$i])) {
				$this->chdir( $this->part[$i] );
				if ($count-$i == 1) {
					if (is_file('index.php'))	return 'index.php';
					if (is_file('index.hh'))	return 'index.hh';
					if (is_file('index.tpl'))	return $af->renderPage('index.tpl');
					error404();
				}
				continue;
			}


			if ($count-$i == 1) {
				$file = $this->part[$i];
				if (is_file("$file.php"))		return "$file.php";
				if (is_file("$file.hh"))		return "$file.hh";
				if (is_file("$file.tpl"))		return $af->renderPage("$file.tpl");
				if (is_file('virtual.php'))		return $this->virtualize($i, 'virtual.php');
				if (is_file('virtual.hh'))		return $this->virtualize($i, 'virtual.hh');
				if (!is_dir('virtual'))			error404();
				$this->chdir( $this->virtualize($i, 'virtual') );
				if ($count-$i == 1) {
					if (is_file('index.php'))	return 'index.php';
					if (is_file('index.hh'))	return 'index.hh';
					if (is_file('index.tpl'))	return $af->renderPage('index.tpl');
					error404();
				}
				continue;
			}


			if (is_dir('virtual')) {
				$this->chdir( $this->virtualize($i, 'virtual') );
				continue;
			}


			if (is_file('virtual.php'))			return $this->virtualize($i, 'virtual.php');
			if (is_file('virtual.hh'))			return $this->virtualize($i, 'virtual.hh');

			error404();
		}
	}



	private function virtualize($start, $return=false) {
		if (!empty($this->virtual)) return $return;

		$count = count($this->part)-1;
		if ($start >= $count) error500();

		for ($x=$start; $x<$count; $x++) {
			$this->virtual[] = $this->part[$x];
		}

		$this->vid($this->virtual[0]);

		return $return;
	}



	public static function cdn($hash, $key='hash') {
		global $afurl;
		$hash = self::cdnHash($hash, $key);
		if (empty($hash)) return false;
		$path = self::cdnPath($hash);
		return empty($path) ? false : $afurl->cdn . '/' . $path;
	}



	public static function cdnAll(&$list, $key='img', $hashkey='hash') {
		if (!is_array($list)) return false;
		if (empty($key)) $key = 'img';
		foreach ($list as &$val) {
			$val[$key] = self::cdn($val, $hashkey);
		} unset($val);
		return true;
	}



	public static function cdnPath($hash) {
		if (empty($hash)) return false;
		if (strlen($hash) === 16) $hash = bin2hex($hash);
		$hash = strtolower($hash);
		return	substr($hash, 0, 3) . '/' .
				substr($hash, 3, 3) . '/' .
				$hash;
	}



	public static function cdnFile($hash, $ext=false) {
		$hash = self::cdnHash($hash);
		if (empty($hash)) return false;
		if ($ext) return 'cdn/' . self::cdnPath($hash) . '.' . $ext;
		return 'cdn/' . self::cdnPath($hash);
	}



	public static function cdnHash($hash, $key='hash') {
		if (empty($hash))					return false;
		if (!is_array($hash))				return $hash;
		if (!empty($hash[$key]))			return $hash[$key];
		if (!empty($hash['thumb_hash']))	return $hash['thumb_hash'];
		if (!empty($hash['file_hash']))		return $hash['file_hash'];
		return false;
	}



	public static function post($url, $post=[], $options=[], $session=false) {
		$ch = curl_init();

		$agent = ini_get('user_agent');
		if (!empty($agent)  &&  is_string($agent)) {
			curl_setopt($ch, CURLOPT_USERAGENT, $agent);
		}

		if (!empty($session)) {
			curl_setopt($ch, CURLOPT_COOKIE, $session);
		}

		curl_setopt_array($ch, $options+[
			CURLOPT_URL				=> $url,
			CURLOPT_AUTOREFERER		=> true,
			CURLOPT_BINARYTRANSFER	=> true,
			CURLOPT_RETURNTRANSFER	=> true,
			CURLOPT_CONNECTTIMEOUT	=> 20,
			CURLOPT_TIMEOUT			=> 20,
			CURLOPT_POST			=> true,
			CURLOPT_POSTFIELDS		=> $post,
			CURLOPT_HTTPHEADER		=> ['Expect:'],
		]);

		$contents			= curl_exec($ch);
		$data				= curl_getinfo($ch);
		$data['error']		= curl_error($ch);
		$data['errno']		= curl_errno($ch);
		$data['content']	= $contents;

		curl_close($ch);

		return $data;
	}



	//301 Moved Permanently
	//302 Found (temporary)
	//307 Temporary Redirect (keeps POST data)
	//308 Permanent Redirect (keeps POST data)
	public static function redirect($url, $type=301, $die=true) {
		global $afurl;
		if (is_array($url)) $url = $afurl($url, true);
		header("Location: $url", true, $type);
		echo '<html><head><meta http-equiv="refresh" content="0;URL=\'';
		echo htmlspecialchars($url) . '\'" /></head><body>';
		echo "<script>top.location.href='";
		echo addslashes($url) . "';</script></body></html>";
		if ($die) exit;
	}



	//Move into a folder, and test security if needed
	private function chdir($path) {
		chdir($path);

		if (is_file('_altaform.php.inc')) {
			extract($GLOBALS, EXTR_REFS | EXTR_SKIP);
			require_once('_altaform.php.inc');
		}
	}



	public function __debugInfo() {
		$dump = [];
		foreach ($this as $key => $item) {
			if (!is_object($item)) $dump[$key] = $item;
		}
		return $dump;
	}



	//SET IN CONSTRUCTOR
	public $directory;
	public $uri;
	public $domain;
	public $referer;
	public $https;
	public $protocol;
	public $host;
	public $af_host;
	public $part;
	public $parts;
	public $query;
	public $url;

	//SET DYNAMICALLY OR BY CONFIG
	public $base		= '';
	public $cdn			= '';
	public $static		= '';
	public $upload		= '';
	public $virtual		= [];
	public $id			= 0;
	public $jq			= false;
	public $reparse		= true;
	public $homepage	= 'homepage';
}


$afurl = new afurl;



class afurlsafe {
	public function __construct($string) { $this->string = $string; }
	public function __toString() { return $this->string; }
	private $string;
}
