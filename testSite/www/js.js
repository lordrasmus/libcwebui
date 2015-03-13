function test(){
	alert("Hallo Button "+1);
}

function templateTest(){
	alert("{f:js_func}");
}

function createWindow(){
	/*console.log("ja");
	var mywindowWindow = function(){
		new MochaUI.Window({
			id: 'mywindow',
			title: 'My Window',
			loadMethod: 'xhr',
			contentURL: 'pages/lipsum.html',
			width: 340,
			height: 150
		});
	}
	mywindowWindow();*/
}
function createMultiSocket(url,element){
	var i=0
	for (i=0;i<500;i++)
	{
		createSocket(url,element);
	}

}

function createSocket(url,element){
	var ok=false;
	try {
		ws = new WebSocket(url);
		ok = true;
	}
	catch(err) {}

	try {
		ws = new MozWebSocket(url);
		ok = true;
	}
	catch(err) {}

	if(ok==false){
		alert("Keine Websockets im Browser ( Chrome ,Webkit , Firefox >= 6 benutzen)\n"+err );
		return null;
	}


//	if ( ("WebSocket" in window) || ("MozWebSocket" in window ) ) {		
	

	
		ws.onopen = function()
		{
		    console.log("Connected.");			
		};
	
		ws.onmessage = function(messageEvent)
		{
		    console.log("recv -> '" + messageEvent.data + "'");
			if( "closeconnection" == messageEvent.data){
				console.log("close");
				this.close();
				return;
			}
			if(element != 0)
				document.getElementById(element).innerHTML = messageEvent.data;			
		};
	
		ws.onclose = function()
		{
		    console.log("Closed.");		    
			console.log(ws.URL);		    
			//createSocket(ws.URL,0);
		};
		
		return ws;

}


function createCommandSocket(url){
	var ok=false;
	try {
		ws = new WebSocket("ws://"+url+"/CommandSocket");
		ok = true;
	}
	catch(err) {}

	try {
		ws = new MozWebSocket("ws://"+url+"/CommandSocket");
		ok = true;
	}
	catch(err) {}

	if(ok==false){
		alert("Keine Websockets im Browser ( Chrome ,Webkit , Firefox >= 6 benutzen)\n"+err );
		return null;
	}
	
		
	
		ws.onopen = function(){	    
			console.log("Connected.");	
			sendCommand("startUhr");
		};
		ws.onclose = function(){    console.log("Closed.");	};
	
		ws.onmessage = function(messageEvent)
		{		    
			console.log("recv -> '" + messageEvent.data + "'");
			parseCommand(messageEvent.data);
		    //document.getElementById(element).innerHTML = messageEvent.data;
			/*var r = JSON.parse("{ \"command\":\"as\"}");
			var tmp = JSON.parse(messageEvent.data);
			var t = 1;
			console.log("Command : " + tmp.command);
			for(var i=0;i<tmp.parameter[1].length;i++){
				//processGUI_Text(tmp.parameter[1][i][0],tmp.parameter[1][i][1]);
				setGUI_TextByTitle(tmp.parameter[1][i][0],tmp.parameter[1][i][1]);
				var status = document.getElementById("status");
				status.value ="got from Network " + tmp.parameter[1][i][0]; 
				if(tmp.parameter[1][i][1] != "UNKNOWN"){
					insertLangText(tmp.parameter[1][i][0],"DE",tmp.parameter[1][i][1]);
				}
			}*/
		};
		commad_socket = ws;
		return ws;
	
}
var cc=0;
function aberdas(){
	commad_socket.send("Test"+(cc++));
	commad_socket.send("Test"+(cc++));
	commad_socket.send("Test"+(cc++));
	commad_socket.send("Test"+(cc++));
}

function sendEcho(){
	var text_ele = document.getElementById("echo_text");
	var text = "Echo:"+text_ele.value;
	sendCommand(text);
	
}

function sendCommand(command){
	console.log("send -> '" + command + "'");
	commad_socket.send(command);
}

function parseCommand(text){
	var tmp = text.substring(0, 8);
	if( tmp == "UhrZeit:"){
		document.getElementById("uhr_zeit").innerHTML = text.substring(8);
		document.getElementById("uhr_zeit_div").innerHTML = text.substring(8);
		return;
	}
	tmp = text.substring(0, 4);
	if( tmp == "Pong"){
		//document.getElementById("uhr_zeit").innerHTML = text.substring(8);
		alert("Pong");
		return;
	}
	tmp = text.substring(0, 5);
	if( tmp == "Echo:"){
		document.getElementById("echo_antwort").value = text.substring(5);
		return;
	}
	
	console.log("Unknown Command Received: '" + text + "'");
}

































function jhsdgksdjhgksdjhgsdkjhfgksdjgh(){}
function jhsdgksdjhgksdjhgsdkjhfgksdjgh2(){}
function jhsdgksdjhgksdjhgsdkjhfgksdjgh3(){}
