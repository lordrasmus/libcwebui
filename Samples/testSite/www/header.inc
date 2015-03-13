TEMPLATE_V1
<html>

<head>
<title>{get:render;"title"}</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
<!--<meta http-equiv="refresh" content="2">-->
<link rel="stylesheet" href="/css/test.css" type="text/css" />    <!-- muss nicht verschluesselt werden -->
<script type="text/javascript" src="js.js"></script>		   <!-- muss nicht verschluesselt werden, ab IE6 kein zugriff auf cookies von js -->
<script type="text/javascript" src="mootools-core-1.4.1-full-nocompat.js"></script>
</head>

<body>
	<table>
		<tr>
			<td><div style="height: 30px" id=uhr_zeit_div>{get:global;"uhr_zeit"}</div>{f:memoryInfos}
			<td><H1>Webserver Testseite</H1>
		<tr>
			<td valign="top" >{include:menu.inc}
			<td valign=top >
