<?php include 'header.php' ?>

<!----
<html>
<meta name="viewport" content="width=device-width, initial-scale=1">
<LINK href="styles/simple.css" rel="stylesheet" type="text/css">


<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<h1><strong>DigiPi</strong></h1>

---->

<h3>Wifi Setup</h3>


<form action="wifi.php" method="post">
  <p>
  SSID: <input type="text" name="ssid"><br/> 
  </p>
  <p>
  PASS: <input type="password" name="password"><br/>
  </p>
  <p>
  <input type="submit" name="wifi" value="Submit"> &nbsp; &nbsp; &nbsp; &nbsp;
  <input type="submit" name="reboot" value="Restart">
  </p>
</form>


<p>
<?php   

$submit = "none";

if (isset($_POST["wifi"])) {
  $submit = $_POST["wifi"];
  if ( $submit == 'Submit' ) {
      $ssid = $_POST["ssid"];
      $password = $_POST["password"];
      $output = shell_exec("sudo /etc/wpa_supplicant/setup_wifi.sh '$ssid' '$password'");
      echo "Wifi settings updated. Please restart for changes to take effect.";
  }
}

if (isset($_POST["reboot"])) {
  $submit = $_POST["reboot"];
  if ( $submit == 'Restart' ) {
      echo "<font color='red'>Restarting DigiPi</font>.<br/> <br/>";
      echo "DigiPi will likely be available at one of the following locations in a few moments<ul>";
      echo "<a href=http://digipi/>http://digipi/</a> <br>";
      echo "<a href=http://digipi.local/>http://digipi.local/</a> <br>";
      $IP = $_SERVER['SERVER_ADDR'];
      echo "<a href=http://$IP/>http://$IP/</a> <br>";
      echo "</ul>";
      echo "Please Check your router/firewall DHCP assignments if the above links do not respond.<br>";
      echo "If the 'DigiPi' wifi hotspot re-appears, DigiPi had trouble connecting to your wifi router.<br><br>";
      sleep(2);
      putenv(shell_exec("grep ^NEWDISPLAYTYPE= /home/pi/localize.env | tail -1"));
      $output = shell_exec("sudo /home/pi/digibanner.py -b DigiPi -s Restarting... -d \$NEWDISPLAYTYPE");
      $output = shell_exec("sudo /sbin/shutdown -r 0");
  }
}


?>

</p>

</font>
</body>
</html>
