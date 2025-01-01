<?php include 'header.php' ?>

<h3>Audio Setup</h3>


<form id="audioform" action="audio.php" method="post">

<?php

// Get all the available controls
$output = shell_exec("sudo amixer -M scontrols");
preg_match_all('/\'.*\'/', $output, $scontrols);

// Handle POST
if (isset($_POST["audio"])) {
    $submit = $_POST["audio"];
    if ( $submit == 'Submit' ) {
	foreach ($scontrols[0] as $scontrol) {
            $name = substr($scontrol, 1, -1);
            $phpname = str_replace(" ", "_", $name);
            $plevel = $_POST["pl$phpname"];
            $clevel = $_POST["cl$phpname"];
            $eselect = $_POST["e$phpname"];
	    $mute = 'mute';
	    $cap = 'nocap';

	    if($_POST["ps$phpname"] == 'on')
                $mute = 'unmute';
	    if($_POST["cs$phpname"] == 'on')
                $cap = 'cap';

            if($eselect)
                shell_exec("sudo amixer -M set $scontrol $eselect");
            else
                shell_exec("sudo amixer -M set $scontrol $mute playback $plevel% capture $clevel% $cap");
	}
    }
}

// Create the Slider table
echo "<table>";

// Create the Playback sliders
echo "<tr><td><h3>Playback</h3></td></tr>";
foreach ($scontrols[0] as $scontrol) {
    $name = substr($scontrol, 1, -1);

    // Get the current control settings and capabilties
    $output = shell_exec("sudo amixer -M get $scontrol");

    $checked = 0;
    foreach(preg_split("/((\r?\n)|(\r\n?))/", $output) as $line) {
        if(preg_match('/Playback [0-9]+ \[[0-9]+%\] \[-*[0-9]+\.[0-9]+dB\] \[on\]/', $line, $matches))
	    $checked = 1;
    }

    if (preg_match_all('/Capabilities:.*pvolume/', $output)) {
        // Grab the first level. Not supporting stereo.
        preg_match_all('/Playback [0-9]+ \[([0-9]+)%\]/', $output, $levels);
        $level = $levels[1][0];

        echo "<tr>";
        echo "<td>$name</td>";
        // mmmmm sliders
        echo "<td><input type='range' min='0' max='100' name='pl$name' value='$level' oninput='document.getElementById(\"pv$name\").value = this.value'></td><td><output id='pv$name'>$level</output></td>";
	if (preg_match_all('/Capabilities:.*pswitch/', $output)) {
            echo "<td><input type='checkbox' name='ps$name'";
            if($checked)
                echo " checked";
            echo "></td>";
	}
        else
            echo "<td></td>";
	echo "</tr>";
    }
    elseif (preg_match_all('/Capabilities:.*pswitch/', $output)) {
        $checked = preg_match_all('/Playback.*\[on\]/', $output);
        echo "<tr>";
        echo "<td>$name</td><td></td><td></td>";
	echo "<td><input type='checkbox' name='ps$name'";
        if($checked)
            echo " checked";
        echo "></td>";
	echo "</tr>";
    }
    elseif (preg_match_all('/Capabilities:.*enum/', $output)) {
        preg_match_all('/Items: (.*)/', $output, $sitems);
	$items = preg_split("/('[^']*')|\h+/", $sitems[1][0], -1, PREG_SPLIT_NO_EMPTY|PREG_SPLIT_DELIM_CAPTURE);
	preg_match_all('/Item0: (.*)/', $output, $mitem0);
	$item0 = $mitem0[1][0];

        echo "<tr>";
        echo "<td>$name</td><td></td><td></td>";
	echo "<td><select name='e$name'>";
	foreach($items as $item) {
            $itemname = substr($item, 1, -1);
	    echo '<option value="'.$item.'"';
            if($item0 == $item)
                echo 'selected';
	    echo '>'.$itemname.'</option>';
        }
        echo "</select></td>";
	echo "</tr>";
    }
}

// Create the Capture sliders
echo "<tr><td><h3>Capture</h3></td></tr>";

foreach ($scontrols[0] as $scontrol) {
    $name = substr($scontrol, 1, -1);

    // Get the current control settings and capabilties
    $output = shell_exec("sudo amixer -M get $scontrol");

    $checked = 0;
    foreach(preg_split("/((\r?\n)|(\r\n?))/", $output) as $line) {
        if(preg_match('/Capture [0-9]+ \[[0-9]+%\] \[-*[0-9]+\.[0-9]+dB\] \[on\]/', $line, $matches))
	    $checked = 1;
    }

    if (preg_match_all('/Capabilities:.*[c ]volume/', $output)) {
	$level = 0;

        // Grab the first level. Not supporting stereo.
        if(preg_match_all('/Capture [0-9]+ \[([0-9]+)%\]/', $output, $levels))
            $level = $levels[1][0];
        else {
            // Handle generic volume control as capture device
            preg_match_all('/ [0-9]+ \[([0-9]+)%\]/', $output, $levels);
            $level = $levels[1][0];
        }

        echo "<tr>";
        echo "<td>$name</td>";
        // mmmmm sliders
        echo "<td><input type='range' min='0' max='100' name='cl$name' value='$level' oninput='document.getElementById(\"cv$name\").value = this.value'></td><td><output id='cv$name'>$level</output></td>";
	if (preg_match_all('/Capabilities:.*cswitch/', $output)) {
            echo "<td><input type='checkbox' name='cs$name'";
            if($checked)
                echo " checked";
            echo "></td>";
	}
        else
            echo "<td></td>";
	echo "</tr>";
    }
    elseif (preg_match_all('/Capabilities:.*cswitch/', $output)) {
        $checked = preg_match_all('/Capture.*\[on\]/', $output);
        echo "<tr>";
        echo "<td>$name</td><td></td><td></td>";
	echo "<td><input type='checkbox' name='cs$name'";
        if($checked)
            echo " checked";
        echo "></td>";
	echo "</tr>";
    }
}
echo "</table>";


?>
<br>
<input type="submit" name="audio" value="Submit"> &nbsp; &nbsp; &nbsp; &nbsp;
</form>

</body>
</html>
