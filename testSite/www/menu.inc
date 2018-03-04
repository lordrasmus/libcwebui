TEMPLATE_V1


{f:menustart}
<table>
	<tr><td>
		<table width=200>
			<tr><td><a class=inaktive href={b:link}/index.html>Index ( Auto Link )</a>
			<tr><td><a class={get:render;"mp1_class"} href={b:link_std}/index.html>Normal</a>
{if:is_ssl_available}
			<a class={get:render;"mp2_class"} href={b:link_ssl}/index.html>SSL</a>
			<tr><td><a class={get:render;"mp3_class"} href={b:link_ssl}/register.html>Register SSL</a>
			<tr><td><a class={get:render;"mp4_class"} href={b:link_ssl}/unregister.html>Unregister SSL</a>
{endif}

			<tr><td><a class={get:render;"mp5_class"} href={b:link_std}/render_variablen.html>Render Variablen</a>
{if:is_ssl_available}
			<a class={get:render;"mp6_class"} href={b:link_ssl}/render_variablen.html>SSL</a>
{endif}

			<tr><td><a class={get:render;"mp7_class"} href={b:link_std}/session_storage.html>Session Storage</a>
{if:is_ssl_available}
			<a class={get:render;"mp8_class"} href={b:link_ssl}/session_storage.html>SSL</a>
{endif}
			
			<tr><td><a class={get:render;"mp11_class"} href={b:link_std}/memory_info.html?menu=0>Memory Info</a>

{if:is_websockets_available}
			<tr><td><a class={get:render;"mp12_class"} href={b:link_std}/websocket.html>Websocket Test</a>
{endif}
			
			<tr><td><a class={get:render;"mp13_class"} href={b:link_std}/ajax_test.html>AJAX Test</a>
{if:is_ssl_available}
			<a class={get:render;"mp14_class"} href={b:link_ssl}/ajax_test.html>SSL</a>
{endif}
			<tr><td><a class={get:render;"mp15_class"} href={b:link_std}/if_test.html>IF Test</a>
{if:is_ssl_available}
			<a class={get:render;"mp16_class"} href={b:link_ssl}/if_test.html>SSL</a>
{endif}

			<tr><td><a class={get:render;"mp17_class"} href={b:link_std}/python.html>Python API</a>
			
			<tr><td><a class={get:render;"mp18_class"} href={b:link_std}/file_upload.html>File Upload</a>

		</table>
</table>
<br>
