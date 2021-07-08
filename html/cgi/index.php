<?php

$link = mysqli_connect("localhost", "root", "root", "quiz");

if (!$link)
	echo "Error occured";

$q = "SELECT * FROM quiz_list";
$r = mysqli_query($link, $q);
var_dump($r);

?>

