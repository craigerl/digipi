<!DOCTYPE html>
<html>
   <head>
      <title>DigiPi JS8Call redirect</title>
   </head>
   <body>
      <p>

<?php
sleep(2);
?>
Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true' + '">' + window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true' + '</a>' )
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true'
</script>

      </p>
   </body>
</html>


