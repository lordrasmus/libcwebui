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


/******************************************************************************************
* buildinFunctions
* \brief Definiert Abfragefunktionen für Eigenschaften des Webservers für den Präprozessor.
*
* Mithilfe der Präprozessordirektive {b:<Funktionsname>} können die folgenden Eigenschaften
* des Webservers abgefragt werden
*	compiler: Compiler und dessen Versionsnummer, mit dem der Webserver kompiliert wurde
*	memoryInfos: Gibt die Anzahl der geladenen Dateien und den von Webserver belegten RAM
*				 in Bytes aus
*	build_time: Datum und Uhrzeit, wann der Webserver kompiliert wurde
*	ip: Die IP-Adresse des Webservers (z.B. 192.168.1.1)
*	netmask: Die Netzmaske des Webservers (z.B. 255.255.255.0)
*	MAC: Die MAC-Adresse des Webservers (z.B. **)
*	server_name: Der Name des Webeservers
*	server_port: Der Standardport des Webservers
*	server_ssl_port: Der SSL Port des Webservers
*	dumpSessionStore: Gibt eine Tabelle mit allen SessionStore Informationen aus
* Zusätzlich sind folgende Konditionen für die if-Abfrage vorhanden:
*	is_user_registered: Ist der Benutzer angemeldet (normal oder SSL)
*	is_user_registered_ssl: Ist der Benutzer über SSL angemeldet
*	is_ssl_active: Wird die Verbindung über SSL verschlüsselt
*	is_ssl_available: Ist SSL auf dem Webserver vorhanden
*	is_false: Liefert immer false zurück
*	is_true: Liefert immer true zurück
* Für den Rückgabewert alle Konditionen gilt: true = 1, false = -1, Kondition gibts es nicht = -2
******************************************************************************************/

#ifndef _BUILTIN_FUNCTION_
#define _BUILTIN_FUNCTION_

#include "dataTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int builtinFunction(http_request* s,FUNCTION_PARAS* func);
CONDITION_RETURN builtinConditions(http_request* s,FUNCTION_PARAS* func);

#ifdef __cplusplus
}
#endif

#endif
