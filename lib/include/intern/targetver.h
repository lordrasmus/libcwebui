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


#pragma once

// Die folgenden Makros definieren die mindestens erforderliche Plattform. Die mindestens erforderliche Plattform
// ist die fr�heste Windows-, Internet Explorer-Version usw., die �ber die erforderlichen Features zur Ausf�hrung 
// Ihrer Anwendung verf�gt. Die Makros aktivieren alle Funktionen, die auf den Plattformversionen bis 
// einschlie�lich der angegebenen Version verf�gbar sind.

// �ndern Sie folgende Definitionen f�r Plattformen, die �lter als die unten angegebenen sind.
// Unter MSDN finden Sie die neuesten Informationen �ber die entsprechenden Werte f�r die unterschiedlichen Plattformen.
#ifndef _WIN32_WINNT            // Gibt an, dass Windows Vista die mindestens erforderliche Plattform ist.
#define _WIN32_WINNT 0x0600     // �ndern Sie den entsprechenden Wert, um auf andere Versionen von Windows abzuzielen.
#endif

