<?php
	$start_time = microtime(TRUE);

	$operating_system = PHP_OS_FAMILY;

	if ($operating_system === 'Windows') {
		// Win CPU
		$wmi = new COM('WinMgmts:\\\\.');
		$cpus = $wmi->InstancesOf('Win32_Processor');
		$cpuload = 0;
		$cpu_count = 0;
		foreach ($cpus as $key => $cpu) {
			$cpuload += $cpu->LoadPercentage;
			$cpu_count++;
		}
		// WIN MEM
		$res = $wmi->ExecQuery('SELECT FreePhysicalMemory,FreeVirtualMemory,TotalSwapSpaceSize,TotalVirtualMemorySize,TotalVisibleMemorySize FROM Win32_OperatingSystem');
		$mem = $res->ItemIndex(0);
		$memtotal = round($mem->TotalVisibleMemorySize / 1000000,2);
		$memavailable = round($mem->FreePhysicalMemory / 1000000,2);
		$memused = round($memtotal-$memavailable,2);
		// WIN CONNECTIONS
		$connections = shell_exec('netstat -nt | findstr :' . $_SERVER['SERVER_PORT'] . ' | findstr ESTABLISHED | find /C /V ""');
		$totalconnections = shell_exec('netstat -nt | findstr :' . $_SERVER['SERVER_PORT'] . ' | find /C /V ""');
	} else {
		// Linux CPU
		$load = sys_getloadavg();
		$cpuload = round($load[0], 2);
#		$cpuload = $load[0];
		$cpu_count = shell_exec('nproc');
		// Linux MEM
		$free = shell_exec('free');
		$free = (string)trim($free);
		$free_arr = explode("\n", $free);
		$mem = explode(" ", $free_arr[1]);
		$mem = array_filter($mem, function($value) { return ($value !== null && $value !== false && $value !== ''); }); // removes nulls from array
		$mem = array_merge($mem); // puts arrays back to [0],[1],[2] after 
		$memtotal = round($mem[1] / 1000000,3);
		$memused = round($mem[2] / 1000000,3);
		$memfree = round($mem[3] / 1000000,3);
		$memshared = round($mem[4] / 1000000,2);
		$memcached = round($mem[5] / 1000000,2);
		$memavailable = round($mem[6] / 1000000,2);
		// Linux Connections
		$connections = `netstat -ntu | grep -E ':80 |443 ' | grep ESTABLISHED | grep -v LISTEN | awk '{print $5}' | cut -d: -f1 | sort | uniq -c | sort -rn | grep -v 127.0.0.1 | wc -l`; 
		$totalconnections = `netstat -ntu | grep -E ':80 |443 ' | grep -v LISTEN | awk '{print $5}' | cut -d: -f1 | sort | uniq -c | sort -rn | grep -v 127.0.0.1 | wc -l`; 
		$model = shell_exec('cat /proc/cpuinfo | grep Model | cut -d: -f 2');
		$cputemp = shell_exec('sudo vcgencmd measure_temp | cut -d= -f 2');
		$wifi = shell_exec('iwconfig wlan0 | grep Link');
		$time = shell_exec('date');
	}

	$memusage = round(($memused/$memtotal)*100);		

	$phpload = round(memory_get_usage() / 1000000,2);

	$diskfree = round(disk_free_space(".") / 1000000000,3);
	$disktotal = round(disk_total_space(".") / 1000000000,3);
	$diskused = round($disktotal - $diskfree,3);
	$diskusage = round($diskused/$disktotal*100);

?>


<?php include 'header.php' ?>
<meta http-equiv="refresh" content="3">


		<p><br><nobr><strong><?php echo $model; ?></strong></nobr></p>
<hr width="400px" align="left">


<p>Radio / GPS </p>
<ul>
<?php
echo "<pre style=\"font-family: variable;\">";
if (file_exists('/dev/serial/by-id')) {
   $handle = opendir('/dev/serial/by-id');
   while (false !== ($entry = readdir($handle))) {
       if ($entry != "." && $entry != ".." ) {
         $link = readlink("/dev/serial/by-id/" . $entry);
         $link = substr($link, 6);
         echo($link . ":  ");
         echo $entry . "<br>";
       }
   }
   closedir($handle);
} 
else {
   echo "--- no serial devices ---";
}

echo "</pre>";
?>
</ul>

<hr width="400px" align="left">
<p>Audio Interface</p>
<ul>
<?php
echo "<pre style=\"font-family: variable;\">";
$output = shell_exec("cat /proc/asound/cards | tr -s '  ' ");
echo $output . "</pre>";
?>
</ul>
<hr width="400px" align="left">
</p>
		<p>Memory Usage: <?php echo $memusage; ?>%</p>
		<p>CPU Load Average: <?php echo $cpuload; ?></p>
		<p>Disk Usage: <?php echo $diskusage; ?>%</p>
		<p>Network Connections: <?php echo $totalconnections; ?></p>
		<p>WiFi: <?php echo "<nobr>" . $wifi . "</nobr>"; ?></p>
		<p>CPU Count: <?php echo $cpu_count; ?></p>
		<p>CPU Temp: <?php echo $cputemp; ?></p>
		<p>Mem Total: <?php echo $memtotal; ?> GB</p>
		<p>Mem Used: <?php echo $memused; ?> GB</p>
		<p>Mem Available: <?php echo $memavailable; ?> GB</p>
		<p>SD Free: <?php echo $diskfree; ?> GB</p>
		<p>SD Used: <?php echo $diskused; ?> GB</p>
		<p>SD Total: <?php echo $disktotal; ?> GB</p>
		<p>Host Name: <?php echo $_SERVER['SERVER_NAME']; ?></p>
		<p>Host Address: <?php echo $_SERVER['SERVER_ADDR']; ?></p>
		<p>System Clock: <?php echo "<nobr>" . $time . "</nobr>"; ?></p>
<br>
</body>
</html>
