<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi APRS Map</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<?php
exec("sudo su -c \"/home/pi/direwolf-dashboard.sh > /dev/null 2> /dev/null &\" pi " );

$count = 0;
$hit = 0;
while ( $count < 10  ) {
   $output = shell_exec("netstat -nvat | grep LIST | grep :8088 | wc -l");
   if ( $output > 0 ) {
      echo "<script>";
      echo "window.location.href =  window.location.protocol + '//' + window.location.hostname + ':8088'";
      echo "</script>";
      $hit = 1;
      break;
   }
   sleep(1);
   $count++;
}
if ( ! $hit ) {
   echo "<h3>Unable to connect to Map service.</h3>";
   echo "<h3>See <a href=/syslog.php>SysLog</a> for details.<h4>";
   echo <<<JS
   <script>
   document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8088' + '">' + window.location.protocol + '//' + window.location.hostname + ':8088' + '</a>' );
   </script>
   JS;
}
?>

<p>
<br/>


<br>
</font>
</body>
</html>

