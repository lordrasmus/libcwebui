
#include <signal.h>

#include "webserver_api_functions.h"

void termination_handler(int signum) {
	WebserverShutdownHandler();
}

void sig_pipe_hanler(int signum) {}



int main(int argc, char **argv) {


	if (signal(SIGINT, termination_handler) == SIG_IGN )  signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, termination_handler) == SIG_IGN )  signal(SIGHUP, SIG_IGN);
	if (signal(SIGTERM, termination_handler) == SIG_IGN ) signal(SIGTERM, SIG_IGN);
	if (signal(SIGPIPE, sig_pipe_hanler) == SIG_IGN	)     signal(SIGPIPE, SIG_IGN);

	if (0 == WebserverInit()) {

		setGlobalVar( "global1", "value1");

		WebserverConfigSetInt("port",8080);
		WebserverAddFileDir("", "www");
		WebserverAddFileDir("img", "img");

		WebserverInitPython();
		WebserverLoadPyPlugin( "plugin.py" );

		REGISTER_FUNCTION ( func2 );

		WebserverStart();
	}
	return 0;
}
