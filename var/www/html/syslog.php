<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<?php

      exec("/usr/local/bin/ttyd -p 7683 -t fontSize=20 -o  -s SIGTERM  sudo tail -f /var/log/syslog > /dev/null 2> /dev/null &" );
#      exec("/usr/local/bin/ttyd -t font=Arial -t fontSize=20 -o  -s SIGTERM tail -f /run/direwolf.log > /dev/null 2> /dev/null &" );
      echo "Opening direwolf log...";
      time.sleep(2);
      $IP = $_SERVER['SERVER_ADDR'];
      echo "<script> window.location.href = \"http://$IP:7683/\" </script>";
      exit();

?>

<br>


</font>
</body>
</html>

