

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

#ifndef _WEBSERVER_SYSTEM_FILE_ACCESS_H_
#define _WEBSERVER_SYSTEM_FILE_ACCESS_H_

#include "webserver.h"

#ifdef WEBSERVER_USE_WNFS

	extern int wnfs_socket;
#endif


#include "intern/system_file_access_utils.h"

#include "intern/system_file_access_fs.h"
#include "intern/system_file_access_binary.h"
#include "intern/system_file_access_wnfs.h"




#endif