<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Shell</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<?php
      exec("/usr/bin/ttyd -p 7681 -t fontSize=20 -o  -s SIGTERM sudo /bin/login > /dev/null 2> /dev/null &" );
      sleep(2);
?>

<p>
<br/>

Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7681' + '">' + window.location.protocol + '//' + window.location.hostname + ':7681' + '</a>' );
window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7681'      
</script>

<br>
</font>
</body>
</html>

