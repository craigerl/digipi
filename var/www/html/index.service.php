<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<table width=100% bgcolor=#eeeeee>
<tr>
<td width=10%>
  <font size=+3><strong>DigiPi&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</strong> </font>
</td>
<td width=10% >
  <img height=50px src="radio.jpg">
</td>
<td>&nbsp;</td>
</tr>
</table>

<form action="index.php" method="post">
<table>
<tr>
  <td>
    <font size=+2>TNC 1200baud/VHF</font>
  </td>
  <td>
    <input type="submit" name="tnc" value="on">
    <input type="submit" name="tnc" value="off">
  </td>
</tr>
<tr>
  <td>
    <font size=+2>APRS Digipeater</font>
  </td>
  <td>
    <input type="submit" name="aprs" value="on">
    <input type="submit" name="aprs" value="off">
  </td>
</tr>
<tr>
  <td>
    <font size=+2>TNC 300baud/HF</font>
  </td>
  <td>
    <input type="submit" name="tnc300b" value="on">
    <input type="submit" name="tnc300b" value="off"> 
  </td>
</tr> 
<tr>
  <td>
    <font size=+2>VHF Winlink Gateway </font>
  </td>
  <td>
    <input type="submit" name="winlinkrms" value="on">
    <input type="submit" name="winlinkrms" value="off"> 
  </td>
</tr> 
<tr>
  <td>
    <font size=+2>Pat Winlink Client </font>
  </td>
  <td>
    <input type="submit" name="pat" value="on">
    <input type="submit" name="pat" value="off"> 
  </td>
</tr> 
<tr>
  <td>
    <font size=+2>ARDOP Modem </font>
  </td>
  <td>
    <input type="submit" name="ardop" value="on">
    <input type="submit" name="ardop" value="off"> 
  </td>
</tr> 
<tr>
  <td>
    <font size=+2>Rig Control Daemon &nbsp; </font>
  </td>
  <td>
    <input type="submit" name="rigctld" value="on">
    <input type="submit" name="rigctld" value="off"> 
  </td>
</tr> 

<!-------
<tr>
  <td>
    <font size=+2>AX.25 Networking </font>
  </td>
  <td>
    <input type="submit" name="ax25" value="on">
    <input type="submit" name="ax25" value="off"> 
  </td>
</tr> 
--------->
</table>
</form>



<?php   


$submit = "none";

if (isset($_POST["tnc"])) {
  $submit = $_POST["tnc"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service digipeater stop');
      $output = shell_exec('sudo service winlinkrms stop'); 
      $output = shell_exec('sudo service tnc start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service tnc stop');
      echo $output;
  }
}


if (isset($_POST["aprs"])) {
  $submit = $_POST["aprs"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service tnc stop');       
      $output = shell_exec('sudo service winlinkrms stop');  
      $output = shell_exec('sudo service digipeater start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service digipeater stop');
      echo $output;
  }
}

if (isset($_POST["tnc300b"])) {
  $submit = $_POST["tnc300b"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service tnc stop');       
      $output = shell_exec('sudo service winlinkrms stop');  
      $output = shell_exec('sudo service tnc300b start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service tnc300b stop');
      echo $output;
  }
}

if (isset($_POST["winlinkrms"])) {
  $submit = $_POST["winlinkrms"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service digipeater stop');
      $output = shell_exec('sudo service tnc stop');
      $output = shell_exec('sudo service winlinkrms start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service winlinkrms stop');
      echo $output;
  }
}

if (isset($_POST["pat"])) {
  $submit = $_POST["pat"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service pat start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service pat stop');
      echo $output;
  }
}

if (isset($_POST["ardop"])) {
  $submit = $_POST["ardop"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service winlinkrms stop');
      $output = shell_exec('sudo service ardop start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service ardop stop');
      echo $output;
  }
}

if (isset($_POST["rigctld"])) {
  $submit = $_POST["rigctld"];
  if ( $submit == 'on' ) {
      $output = shell_exec('sudo service rigctld start');
      echo $output;
  }
  if ( $submit == 'off' ) {
      $output = shell_exec('sudo service rigctld stop');
      echo $output;
  }
}



?>

<h3> System status:</h3>

<table bgcolor=#eeeeee>

<?php

echo "<tr><td>";
$output = shell_exec('systemctl is-active tnc');
echo "TNC 1200baud/VHF</td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active digipeater');
echo "APRS Digipeater</td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active tnc300b');
echo "TNC 300b/HF</td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active winlinkrms');
echo "VHF Winlink Gateway </td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active pat');
echo "Pat client  </td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active ardop');
echo "ARDOP modem  </td><td> <strong>$output</strong> </td></tr>";

echo "<tr><td>";
$output = shell_exec('systemctl is-active rigctld');
echo "Rig control daemon &nbsp;&nbsp; </td><td> <strong>$output</strong> </td></tr>";

?>

</table>
<br>
<a href=/index.php><strong>Refresh</strong></a>
&nbsp;&nbsp;
<script language="JavaScript">
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8080' + '" target="new"><strong>Pat Client</strong></a> ' );
</script>
&nbsp;&nbsp;
<a href=/help.html><strong>Help</strong></a>


</font>
</body>
</html>
