<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Bluetooth Setup</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<?php
exec("/home/pi/bluetooth.sh > /dev/null 2> /dev/null &");
echo "Connecting...";
sleep(3);
$count = 0;
$hit = 0;
while ( $count < 8  ) {
   $output = shell_exec("netstat -nvat | grep LIST | grep :7684 | wc -l");
   if ( $output > 0 ) {
      echo "<script>";
      echo "window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7684'";
      echo "</script>";
      $hit = 1;
      break;
   }
   sleep(1);
   $count++;
}
if ( ! $hit ) {
   echo "<h3>Unable to connect to Bluetooth service.</h3>";
}
?>


<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7684' + '">' + window.location.protocol + '//' + window.location.hostname + ':7684' + '</a>' );
</script>


<br>
</font>
</body>
</html>

