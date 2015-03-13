
{include:session_test_menu.inc}

<br>

<!--Session Var test2 :{get:session;"test2"} <- erst nach aufruf von startSession verf&uuml;gbar <br><br>-->

Session Var session-id :{get:session;"session-id"} <- kann nur von internen funktionen benutzt werden <br><br>

Session Var test2 :{get:session;"test2"} <- &auml;ndert sich erst nach aufruf von testfunction1 <br><br>

aufruf von &#123;f:testfunction1} Ausgabe : ->{f:testfunction1}<-<br>
aufruf von &#123;f:testfunction2} Ausgabe : ->{f:testfunction2}<- ( nur ein Funktionsaufruf )<br><br>

<table border=1><tr><td>
Session Var test1 :{get:session;"test1"}<br>
Session Var test2 :{get:session;"test2"}<br>
Session Var test3 :{get:session;"test3"}<br><br>
</table>
aufruf von &#123;f:testfunction3} Ausgabe : ->{f:testfunction3}<-<br>
<br>

{b:dumpSessionStore}<br>


aufruf von &#123;f:testfunction4}{f:testfunction4}<br>

{b:dumpSessionStore}<br>