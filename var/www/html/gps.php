<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Packet Log</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<?php
      $output = shell_exec("pgrep -f 'ttyd -p 7689' 2> /dev/null ");
      if ( ! empty($output) ) {
         exec("pkill -f 'ttyd -p 7689' > /dev/null 2>&1");
         sleep(1);
      }
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi -p 7689 -t fontSize=14 -t 'theme={ \"foreground\": \"black\", \"background\": \"#eeeeee\" }' -o -s SIGTERM /usr/bin/cgps --silent > /dev/null 2> /dev/null &" );
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi -p 7689                 -t 'theme={ \"foreground\": \"black\", \"background\": \"#eeeeee\" }' -o -s SIGTERM /usr/bin/cgps --silent > /dev/null 2> /dev/null &" );
      exec("/usr/bin/ttyd -t titleFixed=GPS -p 7689                                                                                        -o -s SIGTERM /usr/bin/cgps --silent > /dev/null 2> /dev/null &" );
      sleep(1);
?>


<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7689' + '">' + window.location.protocol + '//' + window.location.hostname + ':7689' + '</a>' );
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7689' 
</script>


<br>
</font>
</body>
</html>

