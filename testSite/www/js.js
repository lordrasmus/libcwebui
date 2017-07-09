
function upload_file(){
	
	var file_ele = document.getElementById("form_file");
	
	const formData = new FormData();
	if ( file_ele.files.length > 0 ){
		formData.append(file_ele.value,  file_ele.files[0]);
	}
          
	const client = new XMLHttpRequest();
	
	client.open("POST", "/file_upload.html" );
    client.withCredentials = true;
    client.send(formData);
    
}



// variable for the CommandSocket Websocket connection
var commad_socket = undefined;


// create Websocket Helper Function
function create_websocket_inst( handle ){

	var ws=undefined;

	if ( document.location.protocol == "http:" ) {
		var url  = "ws://" + document.location.host + "/" + handle
	}

	if ( document.location.protocol == "https:" ) {
		var url  = "wss://" + document.location.host + "/" + handle
	}

	try {
		ws = new WebSocket(url);
	}
	catch(err) {
		console.log(err)
	}

	if( ws == undefined ){
		alert("no websockets supported by the browser ( use Chrome , IE >= 11 or Firefox >= 6)\n");
		return null;
	}

	return ws;
}


// create CommandSocket Websocket connection
function createCommandSocket(){

	var ws = create_websocket_inst( "CommandSocket" )

	ws.onopen = function(){
		console.log("Connected CommandSocket");
		sendCommand("connect_clock")
	};

	ws.onclose = function(){                 console.log("Closed CommandSocket");	};
	ws.onmessage = function(messageEvent) {	 parseCommand( messageEvent.data ); };

	commad_socket = ws;
	return ws;

}


// parse Messages from Server on CommandSocket
function parseCommand(text){

	var tmp = text.substring(0, 5);
	if( tmp == "time:"){
		if ( document.getElementById("uhr_zeit") )
			document.getElementById("uhr_zeit").innerHTML = text.substring(5);
		if ( document.getElementById("uhr_zeit_div") )
			document.getElementById("uhr_zeit_div").innerHTML = text.substring(5);
		return;
	}

	console.log("recv -> " + text )
	tmp = text.substring(0, 4);
	if( tmp == "pong"){
		alert("Pong");
		return;
	}

	tmp = text.substring(0, 5);
	if( tmp == "echo:"){
		document.getElementById("echo_answer").value = text.substring(5);
		return;
	}

	console.log("unknown command received: '" + text + "'");
}

// send command on the CommandSocket to the Server
function sendCommand(command){
	console.log("send -> '" + command + "'");
	commad_socket.send(command);
}

// send echo command with text from DOM Element echo_text
function sendEcho(){
	var text_ele = document.getElementById("echo_text");
	var text = "echo:"+text_ele.value;
	sendCommand(text);
}



// create Websocket Helper Function for Simple Websockets
function createSocket( handle ,element){

	var ws = create_websocket_inst( handle )

	ws.onopen = function()
	{
		console.log("Connected to " + handle + " -> updating element id : " + element);
	};

	ws.onmessage = function(messageEvent)
	{
		if(element != undefined ){
			document.getElementById(element).innerHTML = messageEvent.data;
		}else{
			console.log("recv -> " + messageEvent.data )
		}
	};

	ws.onclose = function()
	{
		console.log("Closed: " + ws.URL);
	};

	return ws;

}

// Button onclick Handler
function start_simple_websocket( para ){

	// get sending DOM Element
	var ele = para.target

	createSocket( ele.dataset.handle ,ele.dataset.element)
}


// register Button onclick Events JS Handler
function testsite_init(){

	var ele = document.getElementById("ws_create1")
	if ( ele ){
		ele.onclick = start_simple_websocket
	}

	var ele = document.getElementById("ws_create2")
	if ( ele ){
		ele.onclick = start_simple_websocket
	}

	var ele = document.getElementById("ws_create3")
	if ( ele ){
		ele.onclick = start_simple_websocket
	}

	var ele = document.getElementById("echo_text_button")
	if ( ele ){
		ele.onclick = sendEcho
	}

	var ele = document.getElementById("ws_send_ping")
	if ( ele ){
		ele.onclick = function() { sendCommand("ping"); }
	}

	createCommandSocket()
}

// register DOM ready Event
document.addEventListener("DOMContentLoaded", testsite_init, false )
























function jhsdgksdjhgksdjhgsdkjhfgksdjgh(){}
function jhsdgksdjhgksdjhgsdkjhfgksdjgh2(){}
function jhsdgksdjhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhgksdjhgsdkjhfgksdjgh3(){}
