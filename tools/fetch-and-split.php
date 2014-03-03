<?

$base = "http://planet.osm.org/planet/experimental/";

echo "starting at ".date('d.m.Y H:i:s')."\n";
echo "looking for newest history file on planet.osm.org\n";
$html = file_get_contents("$base?C=M;O=D");
if(!preg_match("/history-([^\\.]+)\\.osm\\.pbf/", $html, $m))
{
	echo "uum.. defective html\n";
	exit(1);
}

$remote = $m[0];
$date = $m[1];

$cwd = realpath(".");
@mkdir("full-history-extracts", 0775, true);
@mkdir("full-history", 0775, true);

$local = file_get_contents("full-history-extracts/latest-stamp");

echo "this is ".__FILE__." at ".date('Y-m-d H:i:s')."\n";
echo "newest file on planet.osm.org is $remote\n";
echo "latest local extracts are made of $local\n";

if($date == $local)
{
	echo "no new dump available on planet.osm.org\n";
	echo "ending\n";
	exit(0);
}

mail("peter@mazdermind.de", "starting history split",
	"newest file on planet.osm.org is $remote\n".
	"latest local extracts are made of $local\n"
);

chdir("full-history");

// TODO: remove old dumps

if(in_array('--skip-download', $argv))
{
	echo "skipping download of new dump\n";
}
else
{
	echo "fetching new dump from planet.osm.org\n";
	system("wget -nc $base/$remote.md5 $base/$remote");
}

if(in_array('--skip-md5sum', $argv))
{
	echo "skipping md5sum check\n";
}
else
{
	echo "checking md5sum\n";
	system("md5sum -c $remote.md5", $ret);
	if(0 != $ret)
	{
		echo "md5sum mismatch\n";
		echo "ending\n";
		exit(2);
	}
}

$oshfile = str_replace('.osm', '.osh', $remote);
$oshpath = getcwd().'/'.$oshfile;
@symlink($remote, $oshfile);


chdir($cwd);

// TODO: remove old extracts

echo "creating destination folders\n";
@mkdir("full-history-extracts/$date", 0775, true);

foreach(glob('*.conf') as $conf)
{
	foreach(file($conf) as $confline)
	{
		if(preg_match('/[^\s]+/', $confline, $m))
		{
			if(trim($m[0]) == '' || $m[0][0] == '#')
				continue;

			$dir = dirname($m[0]);
			if($dir == '')
				continue;

			$crdir = "full-history-extracts/$date/$dir";
			if(file_exists($crdir))
				continue;

			echo "$date/$dir\n";
			@mkdir($crdir, 0775, true);
		}
	}

	copy($conf, "full-history-extracts/$date/$conf");
	echo "splitting according to $conf\n";

	chdir("full-history-extracts/$date");
	system("/usr/bin/osm-history-splitter $oshpath $conf 1>&2");
	chdir($cwd);
}

echo "updating latest-stamp-file and -symlink\n";
file_put_contents("full-history-extracts/latest-stamp", $date);
unlink("full-history-extracts/latest");
symlink($date, "full-history-extracts/latest");

echo "finished at ".date('d.m.Y H:i:s')."\n";
