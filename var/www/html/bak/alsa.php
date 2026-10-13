<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<?php
      $output = shell_exec("pgrep -f 'ttyd -p 7690' 2> /dev/null ");
      if ( ! empty($output) ) {
         exec("pkill -f 'ttyd -p 7690' > /dev/null 2>&1");
         sleep(1);
      }
      exec("/usr/bin/ttyd -t titleFixed=Audio -W -p 7690 -t fontSize=14 -t 'theme={ \"foreground\": \"white\", \"background\": \"black\" }' -o  -s SIGTERM  sudo alsamixer > /dev/null 2> /dev/null &" );
      sleep(1);
?>

<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7690' + '">' + window.location.protocol + '//' + window.location.hostname + ':7690' + '</a>' );
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7690'      
</script>

<br>
</font>
</body>
</html>

