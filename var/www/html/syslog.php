<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<?php
      $output = shell_exec("pgrep -f 'ttyd -p 7683' 2> /dev/null ");
      if ( ! empty($output) ) {
         exec("pkill -f 'ttyd -p 7683' > /dev/null 2>&1");
         sleep(1);
      }
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi  -p 7683 -t fontSize=20 -o  -s SIGTERM  sudo tail -f /var/log/syslog > /dev/null 2> /dev/null &" );
      exec("/usr/bin/ttyd -t titleFixed=SysLog -W -p 7683 -t fontSize=20 -o  -s SIGTERM  sudo /usr/bin/journalctl -f  > /dev/null 2> /dev/null &" );
      sleep(1);
?>

<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7683' + '">' + window.location.protocol + '//' + window.location.hostname + ':7683' + '</a>' );
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7683'      
</script>

<br>
</font>
</body>
</html>

