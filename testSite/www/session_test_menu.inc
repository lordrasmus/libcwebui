{f:sessionmenustart}
<table>
<tr><th>Normale Links<th>SSL Links
<tr>
<td>
{if:eq:render;"session_menu":0}
<a class=aktive href=http://{b:ip}:{b:server_port}/session_storage.html?t=>Normal</a><br>
{else}
<a class=inaktive href=http://{b:ip}:{b:server_port}/session_storage.html?t=>Normal</a><br>
{endif}

{if:eq:render;"session_menu":1}
<a class=aktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=ja&testpara2=1>Normal ({b:ip}:{b:server_port}) ja,1</a><br>
{else}
<a class=inaktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=ja&testpara2=1>Normal ({b:ip}:{b:server_port}) ja,1</a><br>
{endif}

{if:eq:render;"session_menu":2}
<a class=aktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=nein&testpara2=2>Normal ({b:ip}:{b:server_port}) nein,2</a><br>
{else}
<a class=inaktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=nein&testpara2=2>Normal ({b:ip}:{b:server_port}) nein,2</a><br>
{endif}

{if:eq:render;"session_menu":3}
<a class=aktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=hmm&testpara2=3>Normal ({b:ip}:{b:server_port}) hmm,3</a><br>
{else}
<a class=inaktive href=http://{b:ip}:{b:server_port}/session_storage.html?nopar&testpara1=hmm&testpara2=3>Normal ({b:ip}:{b:server_port}) hmm,3</a><br>
{endif}


{if:is_ssl_available}

<td>

{if:eq:render;"session_menu":4}
<a class=aktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?t=>SSL</a><br>
{else}
<a class=inaktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?t=>SSL</a><br>
{endif}

{if:eq:render;"session_menu":5}
<a class=aktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=ja&testpara2=1>SSL ({b:ip}:{b:server_ssl_port}) ja,1</a><br>
{else}
<a class=inaktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=ja&testpara2=1>SSL ({b:ip}:{b:server_ssl_port}) ja,1</a><br>
{endif}

{if:eq:render;"session_menu":6}
<a class=aktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=nein&testpara2=2>SSL ({b:ip}:{b:server_ssl_port}) nein,2</a><br>
{else}
<a class=inaktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=nein&testpara2=2>SSL ({b:ip}:{b:server_ssl_port}) nein,2</a><br>
{endif}

{if:eq:render;"session_menu":7}
<a class=aktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=hmm&testpara2=3>SSL ({b:ip}:{b:server_ssl_port}) hmm,3</a><br>
{else}
<a class=inaktive href=https://{b:ip}:{b:server_ssl_port}/session_storage.html?nopar&testpara1=hmm&testpara2=3>SSL ({b:ip}:{b:server_ssl_port}) hmm,3</a><br>
{endif}

{endif}
</table>
