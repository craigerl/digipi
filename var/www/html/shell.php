<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Shell</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">

<?php
      exec("kill `ps ax | grep 'ttyd -p 7681' | grep -v grep | cut -f 1 -d\  ` >/dev/null 2>/dev/null ");
      sleep(1);
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi -W -p 7681 -t fontSize=20 -o  -s SIGTERM /usr/bin/sudo /bin/login > /dev/null 2> /dev/null &" );
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi -W -p 7681 -t fontSize=20 -o  -s SIGTERM /usr/bin/sudo /bin/login > /tmp/out 2> /tmp/out &" );
#      exec("/usr/bin/ttyd -t titleFixed=DigiPi -W -p 7681 -t fontSize=20 -o  -s SIGTERM /bin/login > /tmp/out 2> /tmp/out &" );
      exec("/usr/bin/ttyd -t titleFixed=Shell -W -p 7681 -t fontSize=20 -o  -s SIGTERM ssh pi@localhost > /dev/null 2> /dev/null &" );

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

