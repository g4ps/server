<?php

require_once '../database/db.php';
$quiz_id = $_POST["id"]; 
$key = $_POST["key"]; 
$date = date("Y-m-d H:i:s");


$quiz1 = "SELECT start_time FROM quiz_list WHERE id = '" . $quiz_id ."'"; 
$result1 = mysqli_query($link, $quiz1) or die("Ошибка: " . mysqli_error($link)); 

$row = mysqli_fetch_array($result1);
$row = ($row[0]);

$quiz = "SELECT * FROM teams WHERE team_key = '" . $key ."'"; 
$result = mysqli_query($link, $quiz) or die("Ошибка: " . mysqli_error($link)); 

if ($result->num_rows == 1){
	foreach ($result as $key => $value)	{
		$team_id = $value['id'];
	}
	$access_q = "SELECT * FROM access_control WHERE team_id = " . $team_id; //checking access control
	$access = mysqli_query($link, $access_q) or die("Ошибка: " . mysqli_error($link));
	$access = $access->fetch_assoc();
	$access_times = $access['access_times'];    //getting number of uncookied accesses;
	$cookie = $access['cookie'];               //getting cookie value;
	$team_q = "SELECT * FROM students WHERE team_id = " . $team_id;
	$team_res = mysqli_query($link, $team_q) or die("Ошибка: " . mysqli_error($link));
	$team_num = mysqli_num_rows($team_res);        //getting number of people in the team
	$cookie_control = $cookie == $_COOKIE['access'];   //we have two different checks for the purpose of not incrementing
	//access times in case of cookie access
	$num_control = $team_num > $access_times;
	$access_control = $cookie_control || $num_control;
	$end = strtotime($row ." +1 hours 30 minutes");
	if ($date >= $row && strtotime($date) <= $end && $access_control){
		setcookie('access', $cookie, $end + 100) or die("Ошибка: вы должны подключить доступ к 
			cookie, чтобы пройти викторину");
		if (!$cookie_control) { //in case, if we have uncookied access
			$access_times++;
			$access_q = 'UPDATE access_control SET access_times = ' . $access_times . ' WHERE team_id = ' . $team_id;
			echo $access_q . '<br>';
			mysqli_query($link, $access_q) or die('Ошибка: ' . mysqli_error($link));
		}
		header('Location: /partisipation/game/index.php?id=' . $quiz_id . '&team_id=' . $team_id);
	}
	else if (!$access_control) {
		echo '<h1>Нельзя подключиться к викторине; количество человек, проходящих викторину 
			превысило количество человек в команде</h1>';
	}
	else {
		if ($date < $row){
			$dt = (strtotime($row) - strtotime($date));	
		
			define('SECONDS_PER_MINUTE', 60); // секунд в минуте
			define('SECONDS_PER_HOUR', 3600); // секунд в часу
			define('SECONDS_PER_DAY', 86400); // секунд в дне

			$daysRemaining = floor($dt / SECONDS_PER_DAY); //дни, до даты
			$dt -= ($daysRemaining * SECONDS_PER_DAY);     //обновляем переменную

			$hoursRemaining = floor($dt / SECONDS_PER_HOUR); // часы до даты
			$dt -= ($hoursRemaining * SECONDS_PER_HOUR);     //обновляем переменную

			$minutesRemaining = floor($dt / SECONDS_PER_MINUTE); //минуты до даты
			$dt -= ($minutesRemaining * SECONDS_PER_MINUTE);     //обновляем переменную

			echo("<h3 style='text-align: center;'>До начала викторины осталось $daysRemaining дней, $hoursRemaining часов, $minutesRemaining минут, $dt секунд</h3>"); //печатаем сообщение
			echo "<script>window.location.reload()</script>";
			if ($daysRemaining == 0 && $hoursRemaining == 0 && $minutesRemaining == 0 && $dt == 0 ) {
				header('Location: /partisipation/game/index.php?id=' . $quiz_id . '&team_id=' . $team_id);
			}
		} else {
			header("Location: ../partisipation/end/index.php?id=". $quiz_id ."&team_id=". $team_id);
		} 		
	}	
}
else {
	header('Location: /');
}
