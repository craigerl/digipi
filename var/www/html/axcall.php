<?php include 'header.php' ?>

<h3>AX.25 Utilities</h3>

<form action="axcall.php" method="post">
  <p>
  Enter callsign to connect (via optional digipeater)
  </p>
  <b>axcall -r radio <input type="text" name="target" size="8"> via <input type="text" size="8" name="via"> </b>
  <br/>
  </p>
  <p>
  <input type="submit" name="connect" value="Connect"> 
  <br/>

  <hr align="left" width="400">
  Connect to your node
  <br/>
  <br/>
  <b>telnet localhost 4444 </b>  
  <br/>
  <br/>
  <input type="submit" name="telnet" value="Connect"> 
  <hr align="left" width="400">

  Keyboard-to-Keyboard over AX.25
  <br/>
  <br/>
  <b>linpac</b>
  <br/>
  <br/>
  <input type="submit" name="linpac" value="Run linpac"> 
  <hr align="left" width="400">

  </p>
</form>


<?php   

$submit = "none";

if (isset($_POST["telnet"])) {
      exec("/usr/bin/ttyd -t titleFixed=BBS -W -p 7685 -t fontSize=20 -o  -s SIGTERM telnet localhost 4444   > /dev/null 2> /dev/null &" );
      echo "Connecting...";
      echo <<<JS
      <script>
      document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7685' + '">' + window.location.protocol + '//' + window.location.hostname + ':7685' + '</a>' );
      window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7685'
      </script>
      JS;
}

if (isset($_POST["linpac"])) {
      exec("export HOME=/home/pi/; sudo su -c \"/usr/bin/ttyd -t titleFixed=Linpac -W -p 7687 -t fontSize=20 -o  -s SIGTERM /home/pi/linpac.sh  > /dev/null 2> /dev/null &\" pi  " );
      echo "Starting linpac...";
      echo <<<JS
      <script>
      document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7687' + '">' + window.location.protocol + '//' + window.location.hostname + ':7687' + '</a>' );
      window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7687'
      </script>
      JS;
}


if (isset($_POST["connect"])) {
  $submit = $_POST["connect"];
  if ( $submit == 'Connect' ) {

      $target = $_POST["target"];
      $via = $_POST["via"];

      echo "target: $target";
      echo "via: $via";

      if(!isset($via) || trim($via) == '')
      {
         exec("/usr/bin/ttyd -t titleFixed=AXCall -W -p 7686 -t fontSize=20 -o  -s SIGTERM /home/pi/axcall.sh -r radio $target          > /dev/null 2> /dev/null &" );
      }
      else
      {
         #exec("/usr/bin/ttyd -t titleFixed=DigiPi -W -p 7686 -t fontSize=20 -o  -s SIGTERM axcall -r radio $target via $via > /dev/null 2> /dev/null &" );
         exec("/usr/bin/ttyd -t titleFixed=AXCall -W -p 7686 -t fontSize=20 -o  -s SIGTERM /home/pi/axcall.sh -r radio $target via $via > /dev/null 2> /dev/null &" );
      }  
      echo "Connecting...";
      echo <<<JS
      <script>
      document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':7686' + '">' + window.location.protocol + '//' + window.location.hostname + ':7686' + '</a>' );
      window.location.href =  window.location.protocol + '//' + window.location.hostname + ':7686'
      </script>
      JS;
      exit();
  }
}

?>

<br>


</font>
</body>
</html>
