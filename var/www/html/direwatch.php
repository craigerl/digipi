<?php include 'header.php' ?>

<title>Screen</title>

<body onload="JavaScript:init();">

<script type="text/JavaScript">
var url = "/direwatch.png"; //url to load image from
var refreshInterval = 1000; //in ms
var img;

function init() {
    var canvas = document.getElementById("canvas");
    var context = canvas.getContext("2d");
    img = new Image();

    img.onload = function() {
        var ratio = img.width / img.height;
        var width = 400;
        var height = width / ratio;
        canvas.setAttribute("width", width)
        canvas.setAttribute("height", height)
        context.drawImage(this, 0, 0, width, height);
    };
    refresh();
}
function refresh()
{
    img.src = url + "?t=" + new Date().getTime();
    setTimeout("refresh()",refreshInterval);
}

</script>

<canvas id="canvas">
</canvas>

<p>
&nbsp;
<table>
<tr>
  <td >
    <a href=/direwatch.php><strong>Refresh</strong></a>
  </td>
  <td colspan="1">
     <a href="JavaScript:window.close()"> <strong>Close</strong></a>
  </td>
</tr>
</table>
</p>


</body>
</html>
