TEMPLATE_V1
<table>
<tr><td>

	Server:<td>{f:server_name}

<tr><td>

<table>

	<tr><td>Compiler:<td>{f:compiler}
	<tr><td>Build Time:<td>{f:build_time}
	<tr><td>64 Bit:<td>{f:64bit_avaible}
	<tr><td>Websocket:<td>{f:websocket_avaible}
</table>

<td>

<table>
	<tr><td>Host:<td>{f:host}
	<tr><td>Port:<td>{f:server_port} {if:ssl_supported} / {f:server_ssl_port} {endif}
	<tr><td>SSL:<td>{f:ssl_avaible}
	<tr><td>OS:<td>{f:build_os}
	
</table>

</table>
