TEMPLATE_V1

<h2>Debugging</h2>

	<div style="margin:0 10px 10px 0">
	<table>
	<caption>Memory</caption>
	<th>Typ<th>Count<th>Bytes
	{b:memoryInfosDetail}
	</table>
	</div>


	<div style="margin:0 10px 10px 0">
	<table>
	<caption>Sockets</caption>
	<tr><th>Typ<th>Port<th>Size<th>SSL<th>Status<th>Websocket
	{b:dumpSockets}
	</table>
	</div>

	<div style="margin:0 10px 10px 0;">
	<table>
	<caption>Files</caption>
	<tr><th>File<th>Bytes<th>File Sys<th>Ram Cached<th>Compressed<th>Template</th>
	{b:printLoadedFiles}
	</table>
	</div>

	<div style="margin:0 10px 10px 0;">
	<table>
	<caption>Plugins</caption>
	<tr><th>Plugins<th>File<th>Type</th><th>Status
	{b:registeredPlugins}
	</table>
	</div>

	<div style="margin:0 10px 10px 0;">
	<table>
	<caption>Functions</caption>
	<tr><th>Plugin</th><th>User Function<th>Type</th><th>File<th>Line<th>Status</th>
	{b:registeredFunctionsDetail}
	</table>
	</div>


