<html>
<meta name="viewport" content="width=device-width">

<title>DigiPi Webchat</title>

<body  style="position: relative; height: 100%; width: 100%; overflow: hidden;">

<font face="sans">


<p>
<br/>

<?php
$count = 0;
$hit = 0;
while ( $count < 8  ) {
   $output = shell_exec("netstat -nvat | grep LIST | grep :8055 | wc -l");
   if ( $output > 0 ) { 
      echo "<script>";
      echo "window.location.href =  window.location.protocol + '//' + window.location.hostname + ':8055'";
      echo "</script>";
      $hit = 1;
      break;
   }
   sleep(1);
   $count++;
}
if ( ! $hit ) {
   echo "<h3>Unable to connect to WebChat service, please ensure service is enabled.</h3>";
}
?>

<br>
<br>
Direct link:
<script>
document.write('<a href="' + window.location.protocol + '//' + window.location.hostname + ':8055' + '">'
+ window.location.protocol + '//' + window.location.hostname + ':8055' + '</a>' );
</script>

</p>
</font>
</body>
</html>
