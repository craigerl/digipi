<?php include 'header.php' ?>


<?php   

$submit = "none";

if (isset($_POST["tnc"])) {
  $submit = $_POST["tnc"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start tnc');
  }
  else {
      $output = shell_exec('sudo systemctl stop tnc');
  }
}

if (isset($_POST["digipeater"])) {
  $submit = $_POST["digipeater"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start digipeater');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop digipeater');
  }
}

if (isset($_POST["tracker"])) {
  $submit = $_POST["tracker"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start tracker');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop tracker');
  }
}

if (isset($_POST["webchat"])) {
  $submit = $_POST["webchat"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start webchat');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop webchat');
  }
}

if (isset($_POST["tnc300b"])) {
  $submit = $_POST["tnc300b"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start tnc300b');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop tnc300b');
  }
}

if (isset($_POST["winlinkrms"])) {
  $submit = $_POST["winlinkrms"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start winlinkrms');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop winlinkrms');
  }
}

if (isset($_POST["pat"])) {
  $submit = $_POST["pat"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start pat');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop pat');
  }
}

if (isset($_POST["ardop"])) {
  $submit = $_POST["ardop"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start ardop');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop ardop');
  }
}

if (isset($_POST["rigctld"])) {
  $submit = $_POST["rigctld"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start rigctld');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop rigctld');
  }
}

if (isset($_POST["node"])) {
  $submit = $_POST["node"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start node');
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop node');
  }
}

if (isset($_POST["wsjtx"])) {
  $submit = $_POST["wsjtx"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start wsjtx');
      sleep(3);
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop wsjtx');
  }
}

if (isset($_POST["fldigi"])) {
  $submit = $_POST["fldigi"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start fldigi');
      sleep(3);
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop fldigi');
  }
}

if (isset($_POST["js8call"])) {
  $submit = $_POST["js8call"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start js8call');
      sleep(3);
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop js8call');
  }
}

if (isset($_POST["sstv"])) {
  $submit = $_POST["sstv"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo systemctl start sstv');
      sleep(3);
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo systemctl stop sstv');
      echo $output;
  }
}


echo '<table width="400px">';


# give systemd a chance to settle down
sleep(1);

#-- tnc -------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active tnc');
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>APRS TNC/igate</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="tnc" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='tnc' value='on'  $checked />";
echo '	<span class="switch-label" ></span> ';
echo '	<span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';

#-- tnc300b ----------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active tnc300b');
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>APRS HF TNC/igate</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="tnc300b" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='tnc300b' value='on'  $checked />";
echo '	<span class="switch-label" ></span> ';
echo '	<span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- digipeater -------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active digipeater');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>APRS Digipeater</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="digipeater" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='digipeater' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- tracker -------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active tracker');
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>APRS GPS Tracker</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="tracker" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='tracker' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- webchat ----------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active webchat');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>APRS WebChat</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="webchat" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='webchat' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- Linux NODE AX.25 ------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active node');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>AX.25 Node Network</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="node" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='node' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- Winlink Server -------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active winlinkrms');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>Winlink Email Server</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="winlinkrms" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='winlinkrms' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- Pat Email Client -----------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active pat');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>Pat Winlink Email Client</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="pat" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='pat' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- ARDOP ---------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active ardop');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>ARDOP Modem</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="ardop" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='ardop' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- RIGCTLD ---------------------------------------------

#echo "<tr>";
#$output = shell_exec('systemctl is-active rigctld');
#$output = chop($output);
#  if ($output == "active")
#  {
#     echo '<td bgcolor="lightgreen">';
#     $checked = "checked";
#  }
#  elseif ($output == "failed")
#  {
#     echo '<td bgcolor="red">';
#     $checked = "";
#  }
#  else
#  {
#     echo '<td bgcolor="lightgrey">';
#     $checked = "";
#  }
#echo '</td>';
#echo '<td>';
#echo '<font size=+1>Rig Control Daemon</font></td>';
#echo '<td nowrap>';
#echo '<form action="index.php" method="post">';
#echo '<label class="switch switch-light">';
#echo '  <input type="hidden" name="rigctld" value="off">';
#echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='rigctld' value='on'  $checked />";
#echo '  <span class="switch-label" ></span> ';
#echo '  <span class="switch-handle"></span> ';
#echo '</label>';
#echo '</form>';
#echo '</font>';
#echo '</td></tr>';


#-- WSJTX FT8  -------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active wsjtx');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>WSJTX FT8</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="wsjtx" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='wsjtx' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- SSTV --------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active sstv');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>Slow Scan TV</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="sstv" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='sstv' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- FLDIGI --------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active fldigi');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>FLDigi</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="fldigi" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='fldigi' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#-- JS8CALL -------------------------------------------------

echo "<tr>";
$output = shell_exec('systemctl is-active js8call');
#$output = str_replace("failed", "inactive", $output);
$output = chop($output);
  if ($output == "active")
  {
     echo '<td bgcolor="lightgreen">';
     $checked = "checked";
  }
  elseif ($output == "failed")
  {
     echo '<td bgcolor="red">';
     $checked = "";
  }
  else
  {
     echo '<td bgcolor="lightgrey">';
     $checked = "";
  }
echo '</td>';
echo '<td>';
echo '<font size=+1>JS8Call</font></td>';
echo '<td nowrap>';
echo '<form action="index.php" method="post">';
echo '<label class="switch switch-light">';
echo '  <input type="hidden" name="js8call" value="off">';
echo "  <input onChange='this.form.submit()' class='switch-input' type='checkbox' name='js8call' value='on'  $checked />";
echo '  <span class="switch-label" ></span> ';
echo '  <span class="switch-handle"></span> ';
echo '</label>';
echo '</form>';
echo '</font>';
echo '</td></tr>';


#craiger systemd thinks a sigkill is a failure, so reset failed service status
#This will turn red/failed service into grey/stopped 
$output = shell_exec('sudo systemctl reset-failed fldigi 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed sstv 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed wsjtx 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed ardop 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed tnc300b 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed tracker 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed digipeater 2> /dev/null');
$output = shell_exec('sudo systemctl reset-failed tnc 2> /dev/null');
$output = shell_exec('sudo systemctl reset-failed node 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed winlinkrms 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed pat 2> /dev/null'); 
$output = shell_exec('sudo systemctl reset-failed js8call 2> /dev/null'); 
?>
</table>


<p>
&nbsp;
</p>

<table width=400>
<tr>
  <td width="100px">
    <script language="JavaScript">
    document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8080' + '" target="pat" title="Pat Email Client"><strong>PatEmail</strong></a> ' );
    </script>
  </td>
  <td width="100px">
    <a href="axcall.php" target="axcall" title="Connect to radio/BBS"><strong>AXCall</strong></a>
  </td>
  <td width="100px">      
    <a href="/js8" target="js8" title="Display JS8Call screen"><strong>JS8Call</strong></a>
  </td>
</tr>
<tr>
  <td>
    <a href="/ft8" target="ft8" title="Dispaly FT8 screen"><strong>WSJTX</strong>
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
    <a href=/direwatch.php title="Display LCD screen" target="screen"><strong>Screen</strong></a>
  </td>
  <td colspan="1">
     <a href="/webchat.php" target="webchat" title="APRS Messaging"><strong>Webchat</strong></a>
  </td>
</tr>
<tr>
   <td>
     <a href="/alsa.php" target="audio" title="Audio Settings"><strong>Audio</strong></a>
   </td>
   <td>
     <a href="/sysinfo.php" target="sysinfo" title="System Information"><strong>SysInfo</strong></a>
   </td>
   <td>
     <a href="/gps.php" target="gps" title="GPS Status"><strong>GPS</strong></a>
   </td>
</tr>
<tr>
   <td colspan="3">
     <a href="/" title="Refresh current status"><strong>Refresh</strong></a>
   </td>
</tr>

<?php
  if (!file_exists("/var/cache/digipi/localized.txt")) {
    echo '<tr><td colspan=3><a href="/setup.php" title="REQUIRED!  Enter your callsign and other local information" " target="setup"><font color="green"><strong>Initialize</strong></font></a> </td></tr>';
  }
?>  

</table>



<p>

<form action="index.php" method="post">
    <input type="submit" name="reboot" value="Restart">
    &nbsp;
    <input type="submit" name="shutdown" value="Shutdown">   
    &nbsp;
    <input title="Write current application configurations/logs to SD card" type="submit" name="save" value="Save Configuration">  
    &nbsp; 
    <br/><br/>

<table width="400" style="padding-bottom: 3px;" >
<tr>
  <td  width=10% style="padding: 3px; background-image: linear-gradient(to bottom, #fafafa 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;"  >
    <small><br>2.0-1 KM6LYW ©2025</small>
  </td>
</tr>
</table>

</form>
</p>


<p>
<?php
$submit = "none";
if (isset($_POST["reboot"])) {
  $submit = $_POST["reboot"];
  if ( $submit == 'Restart' ) {
      echo "<br/><br/><strong><font color=red>Restarting DigiPi...</font></strong><br/> ";
      $output = shell_exec("sudo killall direwatch.py");
      putenv(shell_exec("grep ^NEWDISPLAYTYPE= /home/pi/localize.env | tail -1"));
      $output = shell_exec("sudo /home/pi/digibanner.py -b DigiPi -s Restarting... -d \$NEWDISPLAYTYPE");
      $output = shell_exec("sudo /sbin/shutdown -r 0");
  }
}
if (isset($_POST["shutdown"])) {
  $submit = $_POST["shutdown"];
  if ( $submit == 'Shutdown' ) {
      echo "<br/><br/><strong><font color=red>Shutting down DigiPi...</font></strong><br/> ";
      $output = shell_exec("sudo killall direwatch.py");
      putenv(shell_exec("grep ^NEWDISPLAYTYPE= /home/pi/localize.env | tail -1"));
      $output = shell_exec("sudo /home/pi/digibanner.py -b DigiPi -s Shutdown... -d \$NEWDISPLAYTYPE");
      $output = shell_exec("sudo /sbin/shutdown -h 0");
  }
}
if (isset($_POST["save"])) {
  $submit = $_POST["save"];
  if ( $submit == 'Save Configuration' ) {
      echo "<br/><br/><strong><font color=red>Saving configuration...</font></strong><br/> ";
      $output = shell_exec("sudo -i -u pi /home/pi/saveconfigs.sh");
      echo "<br/><br/><strong><font color=red>Please restart or shutdown gracefully.</font></strong><br/> ";
  }
}
?>
</p>

<br/>
<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="/dot"><font size="+3" color="#eeeeee">.</font></a>



</p>
</body>
</html>
