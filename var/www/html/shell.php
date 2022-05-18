<html>
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>DigiPi Shell </title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<?php

      exec("/usr/local/bin/ttyd -p 7681 -t fontSize=20 -o  -s SIGTERM sudo /bin/login > /dev/null 2> /dev/null &" );
      echo "Shell access...";
      time.sleep(2);
      $IP = $_SERVER['SERVER_ADDR'];
      echo "<script> window.location.href = \"http://$IP:7681/\" </script>";
      exit();

?>

<br>


</font>
</body>
</html>

