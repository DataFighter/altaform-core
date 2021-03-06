<?php


define('AF_YEAR',  31536000);
define('AF_MONTH',  2592000);
define('AF_WEEK',    604800);
define('AF_DAY',      86400);
define('AF_HOUR',      3600);
define('AF_MINUTE',      60);
define('AF_SECOND',       1);



class aftime {

	// ARRAY OF TIME PERIOD CHUNKS
	public static $chunks = [
		[AF_YEAR,	'year'],
		[AF_MONTH,	'month'],
		[AF_WEEK,	'week'],
		[AF_DAY,	'day'],
		[AF_HOUR,	'hour'],
		[AF_MINUTE,	'minute'],
	];



	public static function get($data) {
		if (is_int($data))		return $data;
		if (ctype_digit($data))	return (int) $data;
		return strtotime($data);
	}



	public static function from($offset, $precision=0) {
		global $af;
		if (!$precision) return $af->time() - $offset;
		return (floor($af->time()/$precision)*$precision) - $offset;
	}



	public static function since($timestamp, $maxoffset=86400) {
		global $af;
		if ($timestamp == 0) return "Never";

		if ($af->time() > $timestamp) {
			$tense = ' ago';
			$since = $af->time() - $timestamp;
			if ($since == 1) return "1 second$tense";
			if ($since < 60) return "$since seconds$tense";
		} else {
			$tense = '';
			$since = $timestamp - $af->time();
			if ($since == 0) return 'Now';
			if ($since == 1) return '1 second';
			if ($since < 60) return "$since seconds";
		}

		if ($maxoffset > 0  &&  $since > $maxoffset) return date('F jS, Y \a\t g:i A', $timestamp);

		// $j saves performing the count function each time around the loop
		for ($i = 0, $j = count(self::$chunks); $i < $j; $i++) {
			$seconds = self::$chunks[$i][0];
			$name = self::$chunks[$i][1];
			// finding the biggest chunk (if the chunk fits, break)
			if (($count = floor($since / $seconds)) > 1) break;
		}

		$print = ($count == 1) ? '1 '.$name : "$count {$name}s";

		for ($x = $i+1;	$x < $j;	$x++) {
			// now getting the second item
			$seconds2 = self::$chunks[$x][0];
			$name2 = self::$chunks[$x][1];

			// add second item if it's count greater than 0
			if (($count2 = floor(($since - ($seconds * $count)) / $seconds2)) != 0) {
				$print .= ($count2 == 1) ? ', 1 '.$name2 : ", $count2 {$name2}s";
				break;
			}
		}

		return $print . $tense;
	}



	public static function nearestZone($lat, $lon) {
		global $af, $site;

		$path  = 'https://maps.googleapis.com/maps/api/timezone/json';
		$path .= "?location=$lat,$lon";
		$path .= '&timestamp=' . $af->time();
		$path .= '&sensor=false';
		$path .= '&key=' . $site['g_server_key'];

		$data = @file_get_contents($path);
		$json = @json_decode($data, true);

		if (!empty($json)) return $json;
		return false;
	}



	static function daterange($start, $end, $month='F', $year=', Y') {
		$start	= (int) $start;
		$end	= (int) $end;
		$text	= date($month.' jS', $start);

		if ((int)($start / 86400)  ==  (int)($end / 86400)) {
		} else if (date('n', $start) === date('n', $end)) {
			$text .= '-' . date('jS', $end);
		} else if (date('Y', $start) !== date('Y', $end)) {
			$text .= date($year, $start);
			$text .= ' - ' . date($month.' jS', $end);
		} else {
			$text .= ' - ' . date($month.' jS', $end);
		}

		return $text . date($year, $end);
	}
}
