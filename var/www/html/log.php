<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Packet Log</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<?php
      $output = shell_exec("pgrep -f 'ttyd.*[p] 7682' 2> /dev/null "); #brackets prevent matching pgrep itself
      if ( empty($output) ) {
         exec("/usr/bin/ttyd -P 19 -t titleFixed=PktLog -p 7682 -W -t fontSize=20    -s SIGTERM /usr/local/bin/direlog.sh > /dev/null 2> /dev/null &" );
         sleep(1);
      }
         echo $output;
?>


<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7682' + '">' + window.location.protocol + '//' + window.location.hostname + ':7682' + '</a>' );
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7682' 
</script>


<br>
</font>
</body>
</html>

