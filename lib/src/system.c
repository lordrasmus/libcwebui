/*

 libCWebUI
 Copyright (C) 2007 - 2016  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "webserver.h"

#ifdef DMALLOC
#include <dmalloc/dmalloc.h>
#endif

#ifndef DISABLE_OLD_TEMPLATE_SYSTEM
ws_variable* template_file_postfix;
#endif

inline global_vars* initWebserver(void) {
	initMemoryManager();
	memset(&globals, 0, sizeof(globals));

	init_extension_api();

	init_file_access();

	#ifndef DISABLE_OLD_TEMPLATE_SYSTEM
	template_file_postfix = newWSVariable("postfix");
	setWSVariableArray(template_file_postfix);
	addTemplateFilePostfix(".html");
	addTemplateFilePostfix(".inc");
	#endif

	globals.config.connections = 128;
	globals.init_called = 0xAB;

#ifdef __GUNC__
#warning "Config Daten auf standart wert setzen"
#endif

	initGlobalVariable();
	initConfig();

#ifdef WEBSERVER_USE_SESSIONS
	initSessions();
#endif


	register_internal_funcs();

	PlatformInitNetwork();

	initEvents();

	if (!initFilesystem()) {
		return 0;
	}

	return &globals;
}

void startWebServer(void) {



#ifdef WEBSERVER_USE_IPV6
	PlatformGetIPv6 ( ( char* ) ip );
	LOG ( MESSAGE_LOG,NOTICE_LEVEL,"IP : %s:%d",ip,config->port );
#endif

#ifdef WEBSERVER_USE_SSL

	int ssl_port = getConfigInt("ssl_port");

	if ( ( ssl_port > 0 ) && (initOpenSSL() < 0) ) {
		LOG( CONNECTION_LOG, ERROR_LEVEL, 0, "Error : SSL konnte nicht gestartet werden", "");
		return;
	}
#endif



	WebserverConnectionManagerStartLoop();
	/* shutdownWebserver(); */
}

int handleWebRequest(socket_info* sock) {
	if (sock == 0) return -1;

	if (sock->header == 0) return -1;

	if (checkHeaderComplete(sock->header) == false) {
		sendMethodBadRequestMissingHeaderLines(sock);
		return 0;
	}

	getHttpRequest(sock);

	return 0;
}

void noRamFree(socket_info* newsockfd) {
	unsigned char buf[50];
#ifdef _MSC_VER
	int len = snprintf ( buf,50,"<html><body>Kein RAM verf&uuml;gbar</body></html>" );
#endif
#ifdef __GNUC__
	int len = snprintf((char*) buf, 50, "<html><body>Kein RAM verf&uuml;gbar</body></html>");
#endif
	WebserverSend(newsockfd, buf, len, 0, 0);
	WebserverCloseSocket(newsockfd);
}

void shutdownWebserverHandler(void) {
	LOG(CONNECTION_LOG, NOTICE_LEVEL, 0, "Webserver Shutdown Handler Invoked", "");
	breakEvents();
}

void shutdownWebserver(void) {
	LOG(CONNECTION_LOG, NOTICE_LEVEL, 0, "Webserver Shutdown", "");
	freeSocketContainer();

	free_file_access();

	free_extension_api();
	freeSessions();
	freeEvents();
	freeConfig();
	freeGlobalVariable();
	freeMemoryManager();
	PlatformEndNetwork();
}


#ifndef DISABLE_OLD_TEMPLATE_SYSTEM

void addTemplateFilePostfix(const char* postfix) {
	ws_variable *tmp;

	tmp = getWSVariableArray(template_file_postfix, postfix);
	if (tmp == 0) {
		tmp = addWSVariableArray(template_file_postfix, postfix);
	}
	setWSVariableInt(tmp, 1);
}

void addTemplateIgnoreFilePostfix(const char* postfix) {
	ws_variable *tmp;

	tmp = getWSVariableArray(template_file_postfix, postfix);
	if (tmp == 0) {
		tmp = addWSVariableArray(template_file_postfix, postfix);
	}
	setWSVariableInt(tmp, 0);
}



char isTemplateFile(const char* file) {

	ws_variable *tmp;
	int i, i2;
	int ret = 0;

	/* prüfen ob file postfix in der liste ist */
	tmp = getWSVariableArrayFirst(template_file_postfix);
	while (tmp != 0) {
		for (i2 = strlen(file), i = tmp->name_len; (i >= 0) && (i2 >= 0); i--, i2--) {
			if (file[i2] != tmp->name[i]) {
				break;
			}
		}
		if ((i == -1) && (tmp->val.value_int == 1)) {
			ret = 1;
		}
		tmp = getWSVariableArrayNext(template_file_postfix);
	}
	stopWSVariableArrayIterate(template_file_postfix);

	if (ret == 0) return 0;

	/* template ignores suchen */
	tmp = getWSVariableArrayFirst(template_file_postfix);
	while (tmp != 0) {
		for (i2 = strlen(file), i = tmp->name_len; (i >= 0) && (i2 >= 0); i--, i2--) {
			if (file[i2] != tmp->name[i]) {
				break;
			}
		}
		if ((i == -1) && (tmp->val.value_int == 0)) {
			ret = 0;
		}
		tmp = getWSVariableArrayNext(template_file_postfix);
	}
	stopWSVariableArrayIterate(template_file_postfix);

	return ret;


}

#endif

