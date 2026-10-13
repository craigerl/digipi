<html>
<meta name="viewport" content="width=device-width">
<LINK href="/styles/simple.css" rel="stylesheet" type="text/css">
<LINK href="/styles/switch.css" rel="stylesheet" type="text/css">

<title>DigiPi</title>
<?php
putenv(shell_exec("grep ^NEWCALL= /home/pi/localize.env | tail -1"));
?>
<body style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<table width="400">
<tr>
  <td width=90% style="padding-bottom: 10px; padding-left: 7px; padding-top: 7px; background-image: linear-gradient(to top, white 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;"  >
    <a style="color:#000"  href="/"><font color="#333333" size=+3><strong>DigiPi</a></strong></font>
    <font color="grey" size="+3">
    &nbsp;
    <strong>
    <?php 
        putenv(shell_exec("grep ^NEWCALL= /home/pi/localize.env | tail -1"));
        $callsign = chop(getenv("NEWCALL"));
        if ( $callsign != 'KX6XXX' ) {
           echo $callsign ;
        } 
    ?>
    </strong>
  </td>
  </strong></font>
  <td width=5% style="padding-bottom: 10px; padding-top: 7px; background-image: linear-gradient(to top, white 0%, lightgrey 90%); background-origin: border-box; border-spacing: 0px; border: 0px solid transparent;" >
    <a href="https://digipi.org" target="craiger">
        <img align="right" height="40px" src="/km6lyw.png">
    </a>
  </td>
</tr>
</table>

