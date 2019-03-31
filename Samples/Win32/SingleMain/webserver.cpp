// webserver.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

//#include "stdafx.h"
//#include <Windows.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#include "webserver_api_functions.h"

#pragma comment(lib, "Ws2_32.lib")

int main()
{
	printf("Hello Webserver!\n");
	

	if (0 == WebserverInit()) {



		WebserverConfigSetInt("use_csp", 0);

		WebserverAddFileDir("", "../testSite/www");
		WebserverAddFileDir("img", "../testSite/img");


		//WebserverLoadPlugin("TestPlugin", "../testSite/src/test_plugin.so");

		WebserverConfigSetInt("port", 8080);
		WebserverConfigSetInt("ssl_port", 4443);
		WebserverConfigSetText("ssl_file_path", "./");
		WebserverConfigSetText("ssl_key_file", "server.pem");
		WebserverConfigSetText("ssl_key_file_password", "password");
		WebserverConfigSetText("ssl_dh_file", "dh1024.pem");
		WebserverConfigSetText("ssl_ca_list_file", "root.pem");


		WebserverStart();
	}

	WebserverShutdown();



	HANDLE eventHandles[] = {
		GetStdHandle(STD_INPUT_HANDLE)
		// ... add more handles and/or sockets here
	};

	DWORD result = WSAWaitForMultipleEvents(sizeof(eventHandles) / sizeof(eventHandles[0]),
		&eventHandles[0],
		FALSE,
		1000,
		TRUE
	);
	while (1);
	/*
	switch (result) {
	case WSA_WAIT_TIMEOUT: // no I/O going on right now
		break;

	case WSA_WAIT_EVENT_0 + 0: // stdin at array index 0
		ProcessStdin();
		break;

	case WSA_WAIT_EVENT_0 + 1: // handle/socket at array index 1
		break;

	case WSA_WAIT_EVENT_0 + 2: // ... and so on
		break;

	default: // handle the other possible conditions
		break;
	} // end switch result
	*/

    return 0;
}

