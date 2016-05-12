/*

libCWebUI
Copyright (C) 2012  Ramin Seyed-Moussavi

Projekt URL : http://code.google.com/p/libcwebui/

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


#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#ifndef ALL_EXTRA
#define ALL_EXTRA
#endif

#ifndef ALL_ATTR
#define ALL_ATTR
#endif

#ifndef ALL_SRC
#define ALL_SRC
#endif


#include <math.h>

#ifdef USE_LIBEVENT
	#include <event2/event.h>
	#include <event2/thread.h>
#endif

#include "WebserverConfig.h"

#ifdef WEBSERVER_USE_WEBSOCKETS
	#ifndef WEBSERVER_USE_SSL
		#error "Webockets nur mit SSL support"
	#endif
#endif

#include "simclist.h"
#include "red_black_tree.h"
#include "is_utf8.h"

#include "intern/helper.h"
#include "intern/webserver_utils.h"


#include "intern/variable.h"
#include "intern/variable_store.h"
#include "intern/list.h"
#include "intern/dataTypes.h"
#include "intern/globals.h"
#include "intern/variables_globals.h"
#include "platform-defines.h"
#include "intern/message_queue.h"
#include "intern/server.h"
#include "intern/session.h"
#include "intern/webserver_log.h"
#include "intern/system.h"
#include "intern/websockets.h"
#include "intern/system_sockets.h"
#include "intern/system_sockets_events.h"
#include "intern/header.h"
#include "intern/convert.h"

#ifdef WEBSERVER_USE_SSL
#include "intern/webserver_ssl.h"
#endif

#include "intern/engine.h"
#include "intern/engine_parser.h"
#include "intern/variable_render.h"
#include "intern/builtinFunctions.h"
#include "intern/cookie.h"
#include "intern/system_sockets_container.h"
#include "intern/system_file_cache.h"
#include "intern/server_config.h"


#ifdef WEBSERVER_USE_PYTHON
	#include "intern/py_plugin.h"
#endif


#include "webserver_api_functions.h"


#endif
