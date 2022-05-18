<?php include 'header.php' ?>
<!---
<html>
<meta name="viewport" content="width=device-width, initial-scale=1">
<LINK href="styles/simple.css" rel="stylesheet" type="text/css">


<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">


<table width=400 >
<tr>
  <td width=10% bgcolor="lightgrey">
    <font size=+3><strong>DigiPi</strong> </font>
  </td>
  <td width=10% bgcolor="lightgrey" >
    <img align=right height=40px src="radio.png">
  </td>
</tr>
</table>
<br/>

-->

<!---
<h1><strong>DigiPi</strong></h1>
--->

<form action="index.php" method="post">

<?php   


$submit = "none";

if (isset($_POST["tnc"])) {
  $submit = $_POST["tnc"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start tnc');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop tnc');
      echo $output;
  }
}


if (isset($_POST["digipeater"])) {
  $submit = $_POST["digipeater"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start digipeater');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop digipeater');
      echo $output;
  }
}

if (isset($_POST["tnc300b"])) {
  $submit = $_POST["tnc300b"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start tnc300b');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop tnc300b');
      echo $output;
  }
}

if (isset($_POST["winlinkrms"])) {
  $submit = $_POST["winlinkrms"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start winlinkrms');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop winlinkrms');
      echo $output;
  }
}

if (isset($_POST["pat"])) {
  $submit = $_POST["pat"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start pat');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop pat');
      echo $output;
  }
}

if (isset($_POST["ardop"])) {
  $submit = $_POST["ardop"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start ardop');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop ardop');
      echo $output;
  }
}

if (isset($_POST["rigctld"])) {
  $submit = $_POST["rigctld"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start rigctld');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop rigctld');
      echo $output;
  }
}

if (isset($_POST["node"])) {
  $submit = $_POST["node"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start node');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop node');
      echo $output;
  }
}

if (isset($_POST["wsjtx"])) {
  $submit = $_POST["wsjtx"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start wsjtx');
      sleep(5);
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop wsjtx');
      echo $output;
  }
}

if (isset($_POST["fldigi"])) {
  $submit = $_POST["fldigi"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start fldigi');
      sleep(5);
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop fldigi');
      echo $output;
  }
}

if (isset($_POST["js8call"])) {
  $submit = $_POST["js8call"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start js8call');
      sleep(5);
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop js8call');
      echo $output;
  }
}

if (isset($_POST["sstv"])) {
  $submit = $_POST["sstv"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start sstv');
      sleep(5);
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop sstv');
      echo $output;
  }
}


?>


<table width="400">
 <col width="10px" />
 <col width="300px" />
 <col width="120" />

<?php

# give systemd a chance to settle down
sleep(2);


#-- tnc -------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active tnc');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>TNC & APRS igate</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="tnc" value="on"> ';
echo '<input type="submit" name="tnc" value="off">';
echo "</font>";
echo "</td></tr>";

#-- tnc300b ----------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active tnc300b');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>TNC & APRS igate (HF)</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="tnc300b" value="on"> ';
echo '<input type="submit" name="tnc300b" value="off">';
echo "</font>";
echo "</td></tr>";

#-- digipeater -------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active digipeater');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>APRS Digipeater</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="digipeater" value="on"> ';
echo '<input type="submit" name="digipeater" value="off">';
echo "</font>";
echo "</td></tr>";


#-- Linux NODE AX.25 ------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active node');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>Linux Node AX.25</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="node" value="on"> ';
echo '<input type="submit" name="node" value="off">';
echo "</font>";
echo "</td></tr>";


#-- Winlink Server -------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active winlinkrms');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>Winlink Email Server</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="winlinkrms" value="on"> ';
echo '<input type="submit" name="winlinkrms" value="off">';
echo "</font>";
echo "</td></tr>";


#-- Pat Email Client -----------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active pat');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>Pat Winlink Email Client</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="pat" value="on"> ';
echo '<input type="submit" name="pat" value="off">';
echo "</font>";
echo "</td></tr>";


#-- ARDOP ---------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active ardop');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>ARDOP Modem</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="ardop" value="on"> ';
echo '<input type="submit" name="ardop" value="off">';
echo "</font>";
echo "</td></tr>";


#-- RIGCTLD ---------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active rigctld');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>Rig Control Daemon</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="rigctld" value="on"> ';
echo '<input type="submit" name="rigctld" value="off">';
echo "</font>";
echo "</td></tr>";


#-- WSJTX FT8  -------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active wsjtx');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="lightgreen">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>WSJTX FT8</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="wsjtx" value="on"> ';
echo '<input type="submit" name="wsjtx" value="off">';
echo "</font>";
echo "</td></tr>";


#-- SSTV --------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active sstv');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="lightgreen">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>Slow Scan TV</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="sstv" value="on"> ';
echo '<input type="submit" name="sstv" value="off">';
echo "</font>";
echo "</td></tr>";


#-- FLDIGI --------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active fldigi');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="lightgreen">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>FLDigi</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="fldigi" value="on"> ';
echo '<input type="submit" name="fldigi" value="off">';
echo "</font>";
echo "</td></tr>";


#-- JS8CALL -------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active js8call');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="lightgreen">';
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
  }
echo "&nbsp;";
echo "</td><td>";
echo "<font size=+1>JS8Call</font></td>";
echo '<td align="right" nowrap>';
echo '<input type="submit" name="js8call" value="on"> ';
echo '<input type="submit" name="js8call" value="off">';
echo "</font>";
echo "</td></tr>";


#craiger systemd thinks a sigkill is a failure, so reset failed service status
#This will turn red/failed service into grey/stopped 
$output = shell_exec('sudo systemctl reset-failed fldigi 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed sstv 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed wsjtx 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed ardop 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed tnc300b 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed digipeater 2> /dev/null');
$output = shell_exec('sudo systemctl reset-failed tnc 2> /dev/null');
$output = shell_exec('sudo systemctl reset-failed node 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed winlinkrms 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed pat 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed js8call 2> /dev/null'); 

?>

</table>

<br/>
<br/>

<!--<table cellpadding="4" bgcolor="#dddddd" border="1" style="border-width:1px;border-color:black; border-collapse:collapse;"  > -->
<table>
<tr>
  <td width="100px">
    <script language="JavaScript">
    document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8080' + '" target="new" title="Pat Email Client"><strong>PatEmail</strong></a> ' );
    </script>
  </td>
  <td width="100px">
    <a href="axcall.php" target="new" title="Connect to radio/BBS"><strong>AXCall</strong></a>
  </td>
  <td width="100px">      
    <a href="/js8" target="js8" title="Display JS8Call screen"><strong>JS8Call</strong></a>
  </td>
</tr>
<tr>
  <td>
    <a href="/ft8" target="ft8" title="Dispaly FT8 screen"><strong>WSJTX FT8</strong>
  </td>
  <td>
    <a href="/tv" target="tv" title="Dispaly SSTV screen"><strong>SSTV</strong>
  </td>
  <td>
    <a href="/fld" target="fld" title="Display FLDigi screen"><strong>FLDigi</strong></a>
  </td>
</tr>
<tr>
  <td>
    <a href="/wifi.php" title="Setup Wifi connection"><strong>Wifi</strong></a>
  </td>
  <td>
    <a href=/shell.php target="shell" title="Command prompt"><strong>Shell</strong></a>
  </td>
  <td>
    <a href=/log.php title="Direwolf Log" target="log"><strong>PktLog</strong></a>
  </td>
</tr>
<tr>
  <td >
    <a href=/syslog.php title="System log file" target="syslog"><strong>SysLog</strong></a>
  </td>
  <td >
    <a href=/index.php><strong>Refresh</strong></a>
  </td>
  <td colspan="1">
    <a href=/help.php><strong>Help</strong></a>
  </td>
</tr>

<?php
  if (!file_exists("/var/cache/digipi/localized.txt")) {
    echo '<tr><td colspan=3><a href="/setup.php" title="REQUIRED!  Enter your callsign and other local information" " target="setup"><font color="green"><strong>Initialize</strong></font></a> </td></tr>';
  }
?>  

</table>




<br/><br/>
    <input type="submit" name="reboot" value="Reboot">
    &nbsp;
    <input type="submit" name="shutdown" value="Shutdown">   
    &nbsp;
    <input title="Write current application configurations (ft8, js8call, etc) to SD card" type="submit" name="save" value="Save Configs">  
    &nbsp; 
    <br/><br/>
    <small>1.6-2 KM6LYW ©2022</small>

<br/><br/>


<?php
if (isset($_POST["reboot"])) {
  $submit = $_POST["reboot"];
  if ( $submit == 'Reboot' ) {
      echo "<br/><br/><strong><font color=red>Restarting DigiPi...</font></strong><br/> ";
#      if (isset($_SERVER['SERVER_ADDR'])) {
#          $IP = $_SERVER['SERVER_ADDR'];
#      }
#      else {
#          $IP = "0.0.0.0";
#      }
      $output = shell_exec("sudo killall direwatch.py");
      $output = shell_exec("sudo /home/pi/digibanner.py -b DigiPi -s Rebooting..."); 
      $output = shell_exec("sudo /sbin/shutdown -r 0");
      echo $output;
  }
}

if (isset($_POST["shutdown"])) {
  $submit = $_POST["shutdown"];
  if ( $submit == 'Shutdown' ) {
      echo "<br/><br/><strong><font color=red>Shutting down DigiPi...</font></strong><br/> ";
      $output = shell_exec("sudo killall direwatch.py");
      $output = shell_exec("sudo /home/pi/digibanner.py -b Digipi -s Shutdown..."); 
      $output = shell_exec("sudo /sbin/shutdown -h 0");
      echo $output;
  }
}

if (isset($_POST["save"])) {
  $submit = $_POST["save"];
  if ( $submit == 'Save Configs' ) {
      echo "<br/><br/><strong><font color=red>Saving configuration...</font></strong><br/> ";
      $output = shell_exec("sudo -i -u pi /home/pi/saveconfigs.sh");
      #echo $output;
      echo "<br/><br/><strong><font color=red>Please reboot or shutdown gracefully.</font></strong><br/> ";
  }
}

?>

<br/>
<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="/dot"><font size="+3" color="#eeeeee">.</font></a>

</form>

</font>
</body>
</html>
