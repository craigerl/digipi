<!DOCTYPE html>
<html>
<head>
   <title>DigiPi</title>
</head>
<body>
<p>


<?php
$count = 0;
$hit = 0;
while ( $count < 8  ) {
   $output = shell_exec("netstat -nvat | grep LIST | grep :6080 | wc -l");
   if ( $output > 0 ) {
      echo "<script>";
      echo "window.location.href =  window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true";
      echo "</script>";
      $hit = 1;
      break;
   }
   sleep(1);
   $count++;
}
if ( $hit ) {
   echo "<script> window.location.href =  window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true'</script>";
}
else {
   echo "<h3>Unable to open display.  Please ensure service is enabled.</h3>"; 
}
?>


Forwarding to
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true' + '">' + window.location.protocol + '//' + window.location.hostname + ':6080/vnc.html?port=6080&password=test11&autoconnect=true' + '</a>' )
</script>
<br><br>

<a href="JavaScript:window.close()"> <strong>Close</strong></a>

</p>
</body>
</html>
