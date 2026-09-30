/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2024  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui


 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

*/

#ifndef POST_STREAMING_H
#define POST_STREAMING_H

/* Anzahl der Parts eines Requests. Jeder Part kostet einen Eintrag und seinen
 * Namen, unabhaengig davon ob er Nutzdaten enthaelt. Ein leerer Part braucht im
 * Body nur ein paar Dutzend Bytes - ohne Grenze bindet ein Upload in
 * Firmwaregroesse damit ueber hunderttausend Eintraege, am Gesamtbudget
 * max_post_memory_total vorbei. Ein Upload Formular hat eine Handvoll Felder.
 * Hier und nicht in der .c, damit die Unittests dieselbe Zahl sehen wie der
 * Build - auch wenn sie per -D ueberschrieben wird. */
#ifndef POST_STREAM_MAX_PARTS
    #define POST_STREAM_MAX_PARTS 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Der normale Empfangsweg liest den Body durch den Header Buffer, der mit
 * WEBSERVER_MAX_HEADER_LINE_LENGHT ( 2 KB ) fuer Header Zeilen dimensioniert
 * ist. Fuer 14 MB sind das ueber 7000 recv() Aufrufe, gemessen 82 % der
 * CPU Zeit im Kernel. Beim Streaming wird der Header Buffer nicht gebraucht,
 * darum ein eigener, deutlich groesserer Puffer.
 */
#ifndef POST_STREAM_READ_CHUNK_SIZE
    #define POST_STREAM_READ_CHUNK_SIZE (32 * 1024)
#endif

/* Nach so vielen Bytes zurueck in den Event Loop, damit andere Sockets
 * drankommen. Gegenstueck zum 20000er Limit des normalen Weges. */
#ifndef POST_STREAM_MAX_READ_PER_EVENT
    #define POST_STREAM_MAX_READ_PER_EVENT (128 * 1024)
#endif

/* Internal streaming functions */
void initPostStreaming(void);
void cleanupPostStreaming(void);

post_stream_handler_entry* findPostStreamHandler(const char* url);

/* Gegenstueck zu RegisterPostStreamHandler(), der exportierte Wrapper steht
 * in webserver_api_functions.c */
void registerPostStreamHandlerInternal(const char* url,
                                       post_stream_auth_handler auth_check,
                                       uint64_t max_size,
                                       post_stream_start_handler on_start,
                                       post_stream_chunk_handler on_chunk,
                                       post_stream_end_handler on_end);

/* Groesstes Limit ueber alle registrierten Handler, 0 = keiner registriert */
uint64_t maxPostStreamSizeInternal( void );

/* 1 = Request kommt von einer angemeldeten Session, 0 = nicht angemeldet */
int postStreamRequestAuthorized(socket_info* sock, post_stream_auth_handler auth_check);

/* Streaming fuer diesen Request aufsetzen. Wird aus check_post_header()
 * gerufen sobald URL, Content-Type und Boundary bekannt sind. */
/* discard == 1: Body lesen aber verwerfen ( fehlende Berechtigung ). Der
 * Request laeuft danach normal weiter, damit die Antwort dieselbe ist wie
 * ohne Streaming. */
int startPostStream(socket_info* sock, post_stream_handler_entry* handler, int discard);

/* Body Bytes in die Statemachine geben.
 * Rueckgabe: 1 = Body vollstaendig, 0 = weitere Daten erwartet, -1 = Fehler */
int postStreamFeed(socket_info* sock, const unsigned char* data, uint32_t len);

/* Laufenden Stream beenden, z.B. bei Verbindungsabbruch.
 * Ruft on_end( ctx, success ) falls noch ein Part offen ist. */
void postStreamFinish(socket_info* sock, int success);

/* Alle Streaming Ressourcen des Sockets freigeben */
void cleanupSocketPostStreamState(socket_info* sock);

/* Metadaten des gestreamten Parts in den http_request uebertragen,
 * damit getFileName()/getFileSize() im Webmodul weiter funktionieren. */
void postStreamFillUploadInfo(http_request* s);

#ifdef __cplusplus
}
#endif

#endif
