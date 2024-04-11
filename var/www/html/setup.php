<!-- <?php include 'header.php' ?>-->
<html>
<meta name="viewport" content="width=device-width, initial-scale=1">
<LINK href="styles/simple.css" rel="stylesheet" type="text/css">


<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">


<table width=600>
<tr>
  <td align="left" width=10% bgcolor="lightgrey">
    <a style="color:#000"  href="/"><font size=+3><strong>DigiPi</strong> </font></a>
  </td>
  <td width=10% bgcolor="lightgrey" >
    <a href="http://craiger.org/digipi" target="craiger">
      <img align=right height=40px src="radio.png">
    </a>
  </td>
</tr>
</table>
<br/>


<h3>DigiPi Initialization</h3>
<div  style="width:600px;">
<p>
<strong>Please try to fill out this form completely the first time</strong>, as you cannot currently come
back and make changes here later.  If you need to make subsequent edits, see
/home/pi/localize.sh for a list of files you can tweak manually.  You can
always reflash your SD card and start over if not sure. 
<br/><br/>
</p>


<p>

<form action="setup.php" method="post">
<table width=600>
  <tr><td>Callsign</td><td> <input type="text" name="call" size="8" value="KX6XXX"></td><td>  Base callsign, no sid/suffix</td></tr>
  <tr><td>Winlink Password</td><td> <input type="text" name="wlpass" size="8" value="XXXXXX"></td><td> <a href="https://www.winlink.org/user" target="winlink"> Create Account</a></td></tr>
  <tr><td>APRS Password</td><td> <input type="text" name="aprspass" size="8" value="12345" ></td><td>  <a href="https://apps.magicbug.co.uk/passcode/" target="aprspass"> Generate</a></td></tr>
  <tr><td>Grid Square</td><td> <input type="text" name="grid" size="8" value="CN99mv"></td><td>  <a href="https://www.levinecentral.com/ham/grid_square.php" target="grid"> Find</a></td></tr>
  <tr><td>Lattitude</td><td>  <input type="text" name="lat" size="8" value="40.9999"></td><td>  <a href="https://www.latlong.net" target="gps">Locate</a> (12.3456)</td></tr>
  <tr><td>Longitude</td><td> <input type="text" name="lon" size="8" value="-120.9999"></td><td>  <a href="https://www.latlong.net" target="gps">Locate</a> (-123.4567) </td></tr>
  <tr><td>AX.25 Node Pass</td><td> <input type="text" name="nodepass" size="8" value="abc123"></td><td>  any alpha-numeric string </td></tr>
  <tr><td>Enable FLRig</td><td> <input type="checkbox" name="flrig" value=""></td><td>Use FLRig for CAT control</tr>
  <tr><td>Large Display</td><td> <input type="checkbox" name="bigvnc" " value=""></td><td> Use with PC or large tablet </td></tr>
</table>
<br>
<table width=600>
<tr><td colspan=3>USB-connected GPS Devices</td></tr>
  <tr><td bgcolor="#eeeeee">&nbsp;&nbsp;Device file</td><td> <input type="text" name="gpsdevicefile" size="8" value="ttyACM0"></td><td>USB GPS dongle=<strong>ttyACM0,</strong>  ic705=<strong>ttyACM1</strong></td></tr>
</table>
<br>
<table width=600>
<tr><td colspan=3>USB-connected radios, aprs/winlink/ax25/packet settings</td></tr>
  <tr><td bgcolor="#eeeeee">&nbsp;&nbsp;Rig number</td><td> <input type="text" name="rignumber" size="8" value="3085"></td><td><a href="riglist.txt" target="new"><strong>Rig Number</strong></a>, DigiRig=<strong>RTS</strong>, DigiPiHat=<strong>DTR</strong></td></tr>
  <tr><td bgcolor="#eeeeee">&nbsp;&nbsp;Device file</td><td> <input type="text" name="devicefile" size="8" value="ttyACM0"></td><td> ys991,ic7300=<strong>ttyUSB0</strong>&nbsp;&nbsp;  ic705=<strong>ttyACM0</strong></td></tr>
  <tr><td bgcolor="#eeeeee">&nbsp;&nbsp;Baud rate</td><td> <input type="text" name="baudrate" size="8" value="115200"></td><td> ys991=<strong>38400</strong>, ic7300=<strong>19200</strong>, ic705=<strong>115200</strong></td></tr>
</table>
<br/>
<br/>
<?php
if (!isset($_POST["reboot"])) {
  if (!file_exists("/var/cache/digipi/localized.txt")) {
    if (!isset($_POST["submit"])) {
      echo '<input type="submit" name="submit" value="Initialize">';
    }
  }
  else {
    echo '<div style="width:600px;">';
    echo '<font size=+1 color=red>This Digipi was already initialized.</font><br/>  <br/>
    To make changes, please edit the config files manually. Please read /home/pi/localize.sh as a guide.  <br/><br/>
    Remove /var/cache/digipi/localized.txt if you know what you\'re doing and would like to see 
    the submit button here again.';
    echo '</div>';
  }
}
?>
  <br/>
</p>
</form>

<?php   

$submit = "none";
$reboot = "none";

$output = shell_exec("sudo /usr/local/bin/remount");

if (isset($_POST["reboot"])) {
   $submit = $_POST["reboot"];
   if ( $submit == 'Reboot' ) {
       echo "</br>Restarting device.<br/> ";
       $IP = $_SERVER['SERVER_ADDR'];
       echo "DigiPi will be available at <a href=http://$IP/>http://$IP/</a> in approximately one minute.<br>";
       $output = shell_exec("sudo /sbin/shutdown -r 0");
       echo $output;
   }
}


if (isset($_POST["submit"])) {
   if (strlen($_POST["call"]) > 0) {
      $call = addslashes($_POST["call"]);
      $output = shell_exec("sudo sed -i 's/NEWCALL=.*/NEWCALL=$call/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["wlpass"]) > 0) {
      $wlpass = addslashes($_POST["wlpass"]);
      $output = shell_exec("sudo sed -i 's/NEWWLPASS=.*/NEWWLPASS=$wlpass/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["aprspass"]) > 0) {
      $aprspass = addslashes($_POST["aprspass"]);
      $output = shell_exec("sudo sed -i 's/NEWAPRSPASS=.*/NEWAPRSPASS=$aprspass/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["grid"]) > 0) {
      $grid = addslashes($_POST["grid"]);
      $output = shell_exec("sudo sed -i 's/NEWGRID=.*/NEWGRID=$grid/gi'  /home/pi/localize.sh");
   }
   if (strlen($_POST["lat"]) > 0) {
      $lat = addslashes($_POST["lat"]);
      $output = shell_exec("sudo sed -i 's/NEWLAT=.*/NEWLAT=$lat/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["lon"]) > 0) {
      $lon = addslashes($_POST["lon"]);
      $output = shell_exec("sudo sed -i 's/NEWLON=.*/NEWLON=$lon/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["nodepass"]) > 0) {
      $nodepass = addslashes($_POST["nodepass"]);
      $output = shell_exec("sudo sed -i 's/NEWNODEPASS=.*/NEWNODEPASS=$nodepass/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["rignumber"]) > 0) {
      $rignumber = addslashes($_POST["rignumber"]);
      $output = shell_exec("sudo sed -i 's/NEWRIGNUMBER=.*/NEWRIGNUMBER=$rignumber/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["devicefile"]) > 0) {
      $devicefile = addslashes($_POST["devicefile"]);
      $output = shell_exec("sudo sed -i 's/NEWDEVICEFILE=.*/NEWDEVICEFILE=$devicefile/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["baudrate"]) > 0) {
      $baudrate = addslashes($_POST["baudrate"]);
      $output = shell_exec("sudo sed -i 's/NEWBAUDRATE=.*/NEWBAUDRATE=$baudrate/gi' /home/pi/localize.sh ");
   }
   if (isset($_POST["flrig"])) {
      $output = shell_exec("sudo sed -i 's/NEWFLRIG=.*/NEWFLRIG=1/gi' /home/pi/localize.sh ");
   }
   if (isset($_POST["bigvnc"])) {
      $output = shell_exec("sudo sed -i 's/NEWBIGVNC=.*/NEWBIGVNC=1/gi' /home/pi/localize.sh ");
   }
   if (strlen($_POST["gpsdevicefile"]) > 0) {
      $devicefile = addslashes($_POST["devicefile"]);
      $output = shell_exec("sudo sed -i 's/NEWGPSDEVICE=.*/NEWGPSDEVICE=$devicefile/gi' /home/pi/localize.sh ");
   }

   echo 'Changes applied. </br></br>';
   echo'<form action="setup.php" method="post">';
   echo '<input type="submit" name="reboot" value="Reboot"> for changes to take effect.';
   echo'</form>';
   echo "<br/>";
   echo "<br/>";
   echo "<br/>";
   $output = shell_exec('head -17 /home/pi/localize.sh | tail -13 ');

   $output = shell_exec('sudo /home/pi/localize.sh');

}

?>

<br>


</font>
</div>
</body>
</html>
