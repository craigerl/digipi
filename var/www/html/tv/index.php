<!DOCTYPE html>
<html>
   <?php
   $IP = $_SERVER['SERVER_ADDR'];
   ?>
   <meta http-equiv = "refresh" content = "2; url = http://<?php echo $IP;?>:6080/vnc.html?host=<?php echo $IP;?>&port=6080&password=test11&autoconnect=true" />
   <head>
      <title>DigiPi SSTV redirect</title>
   </head>
   <body>
      <p>Please wait, SSTV starting up...
      <br/>
      <a href="http://<?php echo $IP;?>:6080/vnc.html?host=<?php echo $IP;?>&port=6080&password=test11&autoconnect=true">Click here to redirect now</a>
      </p>
   </body>
</html>


<!--
<script language="JavaScript">
function sleep(milliseconds) {
  const date = Date.now();
  let currentDate = null;
  do {
    currentDate = Date.now();
  } while (currentDate - date < milliseconds);
}

sleep(10000);

window.location.replace(window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?host=digipi&port=6080&password=test11&autoconnect=true');
</script>
-->
        
