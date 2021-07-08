<?php

var_dump($_SERVER['argv']);
echo "<table border=1>";
foreach($_SERVER as $n => $s)
{
	echo "<tr>";
	echo "<td>". $n . "</td>";
	echo "<td>". $s . "</td>";
	echo "</tr>";
}


?>

<a href="../">Go back</a>
