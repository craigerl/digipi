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
  <input type="submit" name="reboot" value="Reboot">
  </p>
</form>



<?php   


$submit = "none";

if (isset($_POST["wifi"])) {
  $submit = $_POST["wifi"];
  if ( $submit == 'Submit' ) {
      $ssid = $_POST["ssid"];
      $password = $_POST["password"];
      $output = shell_exec("sudo /etc/wpa_supplicant/setup_wifi.sh '$ssid' '$password'");
      echo $output;
  }
}

if (isset($_POST["reboot"])) {
  $submit = $_POST["reboot"];
  if ( $submit == 'Reboot' ) {
      echo "Restarting device.<br/> ";
      echo "Try <a href=http://digipi/>http://digipi/</a> in a couple of minutes.<br>";
      echo "Check your router/firewall DHCP assignments if that doesn't work.";
      $output = shell_exec("sudo /home/pi/digibanner.py -b DigiPi -s Rebooting...");
      $output = shell_exec("sudo /sbin/shutdown -r 0");
      echo $output;
  }
}


?>

<br>

<!----
<a href=/index.php><strong>Refresh</strong></a>
&nbsp;&nbsp;
<script language="JavaScript">
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8080' + '" target="new"><strong>Pat Client</strong></a> ' );
</script>
&nbsp;&nbsp;
<a href=/help.html><strong>Help</strong></a>
-->

</font>
</body>
</html>
