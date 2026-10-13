<html>
<meta name="viewport" content="width=device-width, initial-scale=1">
<LINK href="styles/simple.css" rel="stylesheet" type="text/css">

<title>DigiPi Initialization</title>

<body style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<table width="600" >
<tr>
  <td  width=10% style="padding-bottom: 10px; background-image: linear-gradient(to top, white 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;"  >
    <a style="color:#000"  href="/"><font size=+3><strong>DigiPi</strong></font></a>
  </td>

  <td width=10% style="padding-bottom: 10px; background-image: linear-gradient(to top, white 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;" >
    <a href="https://digipi.org" target="craiger">
        <img align="right" height="40px" src="/km6lyw.png">
    </a>
  </td>
</tr>
</table>



<h3>DigiPi Initialization</h3>
<div  style="width:600px;">
<p>
<strong>Please try to fill out this form completely the first time</strong>, as
you cannot currently come back and make changes here later.  If you need to
make subsequent edits, see /home/pi/localize.env for items you can change
after-the fact.  You can always reflash your SD card and start over if not
sure. 

<form action="setup.php" method="post">
<table width=600>
  <tr><td>Callsign</td><td> <input type="text" name="call" size="8" value="KX6XXX"></td><td>  Base callsign, no sid/suffix</td></tr>
  <tr><td>Winlink Password</td><td> <input type="password" name="wlpass" size="8" value="XXXXXX"></td><td> <a href="https://www.winlink.org/user" target="winlink"> Create Account</a></td></tr>
<!--
  <tr><td>APRS Password</td><td> <input type="text" name="aprspass" size="8" value="12345" ></td><td>  <a href="https://apps.magicbug.co.uk/passcode/" target="aprspass"> Generate</a></td></tr>
-->
  <tr><td>APRS Password</td><td> <input type="text" name="aprspass" size="8" value="12345" ></td><td>  <a href="https://n5dux.com/ham/aprs-passcode/" target="aprspass"> Generate</a></td></tr>
  <tr><td>Grid Square</td><td> <input type="text" name="grid" size="8" value="CN99mv"></td><td>  <a href="https://www.levinecentral.com/ham/grid_square.php" target="grid"> Find</a></td></tr>
  <tr><td>Lattitude</td><td> <input type="text" name="lat" size="8" value="40.9999"></td><td>  <a href="https://www.latlong.net" target="gps">Locate</a> (12.3456)</td></tr>
  <tr><td>Longitude</td><td> <input type="text" name="lon" size="8" value="-120.9999"></td><td>  <a href="https://www.latlong.net" target="gps">Locate</a> (-123.4567) </td></tr>
  <tr><td>GPS Device</td><td> <input type="text" name="gps" size="8" value="ttyACM1"></td><td> ttyACM1, ttyUSB0</td></tr>
  <tr><td>AX.25 Node Pass</td><td> <input type="text" name="nodepass" size="8" value="abc123"></td><td>  any alpha-numeric string </td></tr>
  <tr><td>Default Mode</td><td> <select name="defaultservice">
       <option value="standby">Standby</option>
       <option value="tnc">APRS TNC</option>
       <option value="tnc300b">APRS TNC HF</option>
       <option value="tracker">APRS Tracker</option>
       <option value="digipeater">APRS Digipeater</option>
       <option value="webchat">APRS WebChat</option>
       <option value="node">AX.25 Node</option>
       <option value="winlinkrms">Winlink Server</option>
       <option value="wsjtx">WSJTX</option>
       <option value="js8call">JS8Call</option>
       <option value="sstv">SSTV</option>
       <option value="fldigi">FLDigi</option>
       </select>
       </td><td>  Service to start at boot </td></tr>
  <tr><td>Screen Type</td><td> <select name="displaytype" >
       <option value=st7789>ST7789 240x240</option>
       <option value=ili9486>ILI9486 320x480</option>
       <option value=ili9341>ILI9341 240x320</option>
     </select>
     </td>
     <td>Optional gpio display</td>
     </tr>
<!--  
  <tr><td>Enable FLRig</td><td> <input type="checkbox" name="flrig" value=""></td><td>Use FLRig for CAT control</tr>
-->
  <tr><td>Large Display</td><td> <input type="checkbox" name="bigvnc" " value=""></td><td> Use with PC or large tablet </td></tr>

<script>      
  function checkIfUSB() {
      if (document.getElementById('riginterfaceid').value == 'usbradio') {
        document.getElementById('usbsettings').style.display = '';
      } else {
        document.getElementById('usbsettings').style.display = 'none';
  }
}
</script>

<tr><td>
    Radio Interface
   </td>
   <td colspan=2>
    <select onchange="checkIfUSB()" class="select form-control" id="riginterfaceid" name="riginterface">
      <option id="aioc" value="aioc">AIOC</option>
      <option id="usbradio" value="usbradio">USB-Connected Radio</option>
      <option id="aioc" value="aioc">CM108</option>
      <option id="aioc" value="aioc">TOADs DI</option>
      <option id="aioc" value="aioc">AllScan URI</option>
      <option id="fepi" value="fepi">Fe-Pi Hat, GPIO12</option>
      <option id="digirig" value="digirig">DigiRig Mobile</option>
      <option id="drapizero" value="drapizero">DRA Pi Zero Hat</option>
      <option id="digipihat" value="digipihat">DigiPi Hat</option>
      <option id="aiz" value="aiz">Audio Injector, GPIO12</option>
      <option id="usbaudio" value="usbaudio">USB Audio, GPIO12</option>
    </select>
   </td>
</tr>

<tr>
  <td colspan=3>
    <div id="usbsettings" name="usbsettings" style="display: none"> 
      <table > 
        <tr><td bgcolor="#eeeeee" nowrap>Rig number</td><td> <input type="text" name="rignumber" size="8" value="3085"></td><td nowrap><font size="-1">Rig Number (<a href="riglist.txt" target="new"><strong>see rig list</strong></a>)</font></td></tr>
        <tr><td bgcolor="#eeeeee" nowrap>Device file</td><td> <input type="text" name="devicefile" size="8" value="ttyACM0"></td><td nowrap><font size="-1">y991,ic7300=<strong>ttyUSB0</strong>&nbsp;&nbsp;  ic705=<strong>ttyACM0</strong></font></td></tr>
        <tr><td bgcolor="#eeeeee" nowrap>Baud rate</td><td> <input type="text" name="baudrate" size="8" value="115200"></td><td nowrap><font size="-1">y991=<strong>38400</strong>, ic7300=<strong>19200</strong>, ic705=<strong>115200</strong></font></td></tr>
     </table>  
   </div> 
  </td>
</tr>
</table>


<?php
if (file_exists('/dev/serial/by-id')) {
   echo "<br/><strong>Detected Radio/GPS devices</strong><br/>";
   $handle = opendir('/dev/serial/by-id');
   while (false !== ($entry = readdir($handle))) {
       if ($entry != "." && $entry != ".." ) {
         $link = readlink("/dev/serial/by-id/" . $entry);
         $link = substr($link, 6); 
         echo("<strong>" . $link . "</strong>" . ":  ");
         echo $entry . "<br>";
       }
   }
   closedir($handle);
}
else {
   echo "<br/><strong>No USB/serial Radio/GPS devices detected.</br>";
}

if (file_exists('/proc/asound/cards')) {
   echo "<br>";
   echo "<strong>Detected audio interface</strong>: <br>";
   echo "<code style=\"font-family: variable;\">";
   $output = shell_exec("cat /proc/asound/cards");
   echo $output . "</code>";
}
else {
   echo "<br><strong>ALSA sound subsystem unavailable.</strong><br>";
}
?>

<br/>
<br/>
<br/>

<?php
if (!isset($_POST["reboot"])) {
  if (!file_exists("/var/cache/digipi/localized.txt")) {
    if (!isset($_POST["submit"])) {
      echo '<input type="submit" name="submit" value="Initialize" >';
    }
  }
  else {
    echo '<div style="width:600px;">';
    echo '<font size=+1 color=red>This Digipi was already initialized.</font><br/>  <br/>
    To make changes, please edit the config files manually. Please read /home/pi/localize.env as a guide.  <br/><br/>
    Remove /var/cache/digipi/localized.txt if you know what you\'re doing and would like to see 
    the Initialize button here again.';
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

$output = shell_exec("sudo /usr/local/bin/remount > /dev/null");

if (isset($_POST["reboot"])) {
   $submit = $_POST["reboot"];
   if ( $submit == 'Restart' ) {
       echo "</br>Restarting DigiPi.<br/> ";
       $IP = $_SERVER['SERVER_ADDR'];
       $output = shell_exec("sudo /sbin/shutdown -r 0");
       echo $output;
   }
}


if (isset($_POST["submit"])) {
   if (strlen($_POST["call"]) > 0) {
      $call = addslashes($_POST["call"]);
      $output = shell_exec("sudo sed -i 's/NEWCALL=.*/NEWCALL=$call/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["wlpass"]) > 0) {
      $wlpass = addslashes($_POST["wlpass"]);
      $output = shell_exec("sudo sed -i 's/NEWWLPASS=.*/NEWWLPASS=$wlpass/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["aprspass"]) > 0) {
      $aprspass = addslashes($_POST["aprspass"]);
      $output = shell_exec("sudo sed -i 's/NEWAPRSPASS=.*/NEWAPRSPASS=$aprspass/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["grid"]) > 0) {
      $grid = addslashes($_POST["grid"]);
      $output = shell_exec("sudo sed -i 's/NEWGRID=.*/NEWGRID=$grid/gi'  /home/pi/localize.env");
   }
   if (strlen($_POST["lat"]) > 0) {
      $lat = addslashes($_POST["lat"]);
      $output = shell_exec("sudo sed -i 's/NEWLAT=.*/NEWLAT=$lat/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["lon"]) > 0) {
      $lon = addslashes($_POST["lon"]);
      $output = shell_exec("sudo sed -i 's/NEWLON=.*/NEWLON=$lon/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["gps"]) > 0) {
      $gps = addslashes($_POST["gps"]);
      $output = shell_exec("sudo sed -i 's/NEWGPS=.*/NEWGPS=$gps/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["nodepass"]) > 0) {
      $nodepass = addslashes($_POST["nodepass"]);
      $output = shell_exec("sudo sed -i 's/NEWNODEPASS=.*/NEWNODEPASS=$nodepass/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["displaytype"]) > 0) {
      $displaytype = addslashes($_POST["displaytype"]);
      $output = shell_exec("sudo sed -i 's/NEWDISPLAYTYPE=.*/NEWDISPLAYTYPE=$displaytype/gi' /home/pi/localize.env ");
   }
   if (strlen($_POST["defaultservice"]) > 0) {
      $defaultservice = addslashes($_POST["defaultservice"]);
      if ($defaultservice != "standby") {  # default is /home/pi/online.sh aka standby
         $output = shell_exec("sudo sed -i 's/^ExecStart=.*/ExecStart=systemctl start $defaultservice/gi' /etc/systemd/system/digipi-boot.service");
      }
   }

   $rignumber  = "3085";
   $devicefile = "ttyACM0";
   $baudrate   = "115200";
   $i2caudio   = "fepi";
   $riginterface = $_POST["riginterface"];
   switch ($riginterface) {
      case "aioc":
         $rignumber = "CM108";
         $devicefile = "hidraw0";
         $baudrate  = "115200";
         $i2caudio   = "fepi";
         break;
      case "fepi":
         $rignumber = "GPIO";
         $devicefile = "ttyACM0";
         $baudrate  = "115200";
         $i2caudio   = "fepi";
         break;
      case "digirig":
         $rignumber = "RTS";
         $devicefile = "ttyUSB0";
         $baudrate  = "115200";
         break;
      case "drapizero":
         $rignumber = "GPIO";
         $devicefile = "ttyACM0";
         $baudrate  = "115200";
         $i2caudio   = "drapizero";
         break;
      case "digipihat":
         $rignumber = "DTR";
         $devicefile = "ttyUSB0";
         $baudrate  = "115200";
         break;
      case "aiz":
         $rignumber = "GPIO";
         $devicefile = "ttyACM0";
         $baudrate  = "115200";
         $i2caudio   = "aiz";
         break;
      case "usbaudio":
         $rignumber = "GPIO";
         $devicefile = "ttyACM0";
         $baudrate  = "115200";
         break;
      case "usbradio":
         if (strlen($_POST["rignumber"]) > 0) {
            $rignumber = addslashes($_POST["rignumber"]);
         }
         if (strlen($_POST["devicefile"]) > 0) {
            $devicefile = addslashes($_POST["devicefile"]);
         }
         if (strlen($_POST["baudrate"]) > 0) {
            $baudrate = addslashes($_POST["baudrate"]);
         }
         break;
       default:
         echo "ERROR:  Unrecognized riginterface:  $riginterface";
   }
   $output = shell_exec("sudo sed -i 's/NEWRIGNUMBER=.*/NEWRIGNUMBER=$rignumber/gi' /home/pi/localize.env ");
   $output = shell_exec("sudo sed -i 's/NEWDEVICEFILE=.*/NEWDEVICEFILE=$devicefile/gi' /home/pi/localize.env ");
   $output = shell_exec("sudo sed -i 's/NEWBAUDRATE=.*/NEWBAUDRATE=$baudrate/gi' /home/pi/localize.env ");
   $output = shell_exec("sudo sed -i 's/NEWI2CAUDIO=.*/NEWI2CAUDIO=$i2caudio/gi' /home/pi/localize.env ");
   

   if (isset($_POST["flrig"])) {
      $output = shell_exec("sudo sed -i 's/NEWFLRIG=.*/NEWFLRIG=1/gi' /home/pi/localize.env ");
   }
   if (isset($_POST["bigvnc"])) {
      $output = shell_exec("sudo sed -i 's/NEWBIGVNC=.*/NEWBIGVNC=1/gi' /home/pi/localize.env ");
   }

   echo 'Changes applied. </br></br>';
   echo'<form action="setup.php" method="post">';
   echo '<input type="submit" name="reboot" value="Restart"> for changes to take effect.';
   echo'</form>';
   echo "<br/>";
   echo "<br/>";
   echo "<br/>";

# DEBUG show variables
#   $output = shell_exec('head -23 /home/pi/localize.env | tail -17 ');
#   echo "<pre> $output </pre>";

   $output = shell_exec('sudo /home/pi/localize.sh');

}

?>

<br>

<table width="600" style="padding-bottom: 3px;" >
<tr>
  <td  width=10% style="padding: 3px; background-image: linear-gradient(to bottom, #fafafa 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;"  >
    <small><br>2.0-1 KM6LYW ©2025</small>
  </td>
</tr>
</table>








</font>
</div>
</body>
</html>
