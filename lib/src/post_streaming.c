/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2024  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui


 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

*/

/*
 * Streaming Empfang von multipart/form-data POST Uploads.
 *
 * Ohne Streaming laeuft ein Upload so:
 *
 *   1. recv_post_payload() alloziert post_buffer in Groesse Content-Length
 *   2. parsePostData()     kopiert den Datei Part noch einmal heraus
 *   3. das Webmodul        schreibt die Kopie in eine Datei
 *
 * Kopie 1 und 2 leben gleichzeitig, der Spitzenbedarf ist also das Doppelte
 * der Dateigroesse. Mit einem registrierten Streaming Handler entfaellt
 * beides: der Body wird nie gepuffert sondern in Chunks durchgereicht.
 *
 * Der Multipart Rahmen muss dafuer inkrementell geparst werden. Ein Boundary
 * kann auf einer Chunk Grenze liegen, darum werden Bytes die ein Praefix des
 * Boundaries sein koennten im Rueckhalte Puffer zurueckgehalten und erst
 * ausgegeben wenn feststeht dass sie zu den Nutzdaten gehoeren.
 */

#include "webserver.h"

#ifndef POST_STREAM_MAX_URL_LENGTH
    #define POST_STREAM_MAX_URL_LENGTH 1024
#endif

/* Ein Part Header ( Content-Disposition / Content-Type ) ist wenige hundert
 * Byte gross. Alles daroeber ist kein legitimer Upload. */
#ifndef POST_STREAM_MAX_PART_HEADER
    #define POST_STREAM_MAX_PART_HEADER (8 * 1024)
#endif

/* Formularfelder ohne filename= werden weiter normal gepuffert damit
 * getFileName()/getFileData() im Webmodul unveraendert funktionieren. */
#ifndef POST_STREAM_MAX_FIELD_SIZE
    #define POST_STREAM_MAX_FIELD_SIZE (64 * 1024)
#endif

/* Summe aller Formularfelder eines Requests. Ohne diese Grenze koennte ein
 * Upload fast vollstaendig aus vielen kleinen Feldern bestehen und damit am
 * Gesamtbudget max_post_memory_total vorbei Speicher binden - der Streaming
 * Pfad reserviert dort ja nichts. */
#ifndef POST_STREAM_MAX_FIELDS_TOTAL
    #define POST_STREAM_MAX_FIELDS_TOTAL (256 * 1024)
#endif

/* Laenge eines Part Namens ( Feldname bzw. Dateiname ). Der Name kommt aus dem
 * Part Header und waere sonst nur durch dessen 8 KiB begrenzt. Alle Namen
 * bleiben bis zum Ende des Requests liegen: bei POST_STREAM_MAX_PARTS Parts
 * waeren das ueber 500 KiB, die am Gesamtbudget max_post_memory_total vorbei
 * Speicher binden. Ein echter Feld- oder Dateiname bleibt weit darunter.
 * Der Handler bekommt den ungekuerzten Namen weiterhin ueber ctx->file_name. */
#ifndef POST_STREAM_MAX_NAME
    #define POST_STREAM_MAX_NAME 256
#endif

/* Handler registry - simple linked list
 * Entries persist for the lifetime of the application.
 * Per-stream state is in socket_info, not here.
 */
static list_t post_stream_handler_list;
static int post_streaming_initialized = 0;

/*
 * Initialize streaming subsystem
 */
void initPostStreaming(void) {
    if (post_streaming_initialized) return;
    ws_list_init(&post_stream_handler_list);
    post_streaming_initialized = 1;
}

/*
 * Cleanup streaming subsystem (called at shutdown)
 */
void cleanupPostStreaming(void) {
    if (!post_streaming_initialized) return;

    post_stream_handler_entry* entry;
    ws_list_iterator_start(&post_stream_handler_list);
    while ((entry = (post_stream_handler_entry*)ws_list_iterator_next(&post_stream_handler_list))) {
        if (entry->url) WebserverFree(entry->url);
        WebserverFree(entry);
    }
    ws_list_iterator_stop(&post_stream_handler_list);
    ws_list_clear(&post_stream_handler_list);
    post_streaming_initialized = 0;
}

/*
 * Der Header Parser legt die URL ohne fuehrenden Slash ab ( header_parser.c
 * kopiert ab &line[6] hinter "POST /" ). Registrierung und Suche werden darum
 * beide normalisiert, damit "/upload.html" und "upload.html" gleich wirken.
 */
static const char* skipLeadingSlash(const char* url) {
    if (url && url[0] == '/') {
        return &url[1];
    }
    return url;
}

/*
 * Register a streaming handler for a URL
 * Internal, der exportierte Wrapper steht in webserver_api_functions.c
 */
void registerPostStreamHandlerInternal(
    const char* url,
    post_stream_auth_handler auth_check,
    uint64_t max_size,
    post_stream_start_handler on_start,
    post_stream_chunk_handler on_chunk,
    post_stream_end_handler on_end
) {
    if (!post_streaming_initialized) {
        initPostStreaming();
    }

    /* Validate parameters with specific error messages */
    if (!url) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "%s", "RegisterPostStreamHandler: url is NULL");
        return;
    }
    if (!on_start) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: on_start callback is NULL for url '%s'", url);
        return;
    }
    if (!on_chunk) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: on_chunk callback is NULL for url '%s'", url);
        return;
    }
    if (!on_end) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: on_end callback is NULL for url '%s'", url);
        return;
    }

    const char* url_norm = skipLeadingSlash(url);

    size_t url_len = strlen(url_norm);
    if (url_len == 0) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "%s", "RegisterPostStreamHandler: url is empty");
        return;
    }
    if (url_len >= POST_STREAM_MAX_URL_LENGTH) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: url too long (%zu >= %d)", url_len, POST_STREAM_MAX_URL_LENGTH);
        return;
    }

    if (findPostStreamHandler(url)) {
        LOG(CONNECTION_LOG, WARNING_LEVEL, 0, "RegisterPostStreamHandler: url '%s' already registered", url);
        return;
    }

    post_stream_handler_entry* entry = WebserverMalloc(sizeof(post_stream_handler_entry));
    if (!entry) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: failed to allocate handler entry for url '%s'", url);
        return;
    }

    entry->url = WebserverMalloc(url_len + 1);
    if (!entry->url) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "RegisterPostStreamHandler: failed to allocate url string for '%s'", url);
        WebserverFree(entry);
        return;
    }
    memcpy(entry->url, url_norm, url_len);
    entry->url[url_len] = '\0';

    entry->auth_check = auth_check;
    entry->max_size = max_size;
    entry->on_start = on_start;
    entry->on_chunk = on_chunk;
    entry->on_end = on_end;

    ws_list_append(&post_stream_handler_list, entry, 0);

#ifdef _WEBSERVER_POST_STREAMING_DEBUG_
    LOG(CONNECTION_LOG, INFO_LEVEL, 0, "Registered POST streaming handler for URL: %s", url);
#endif
}

/*
 * Find streaming handler for URL
 * Internal
 */
post_stream_handler_entry* findPostStreamHandler(const char* url) {
    if (!post_streaming_initialized || !url) return NULL;

    const char* url_norm = skipLeadingSlash(url);

    /* Direkt traversieren statt ws_list_iterator_*: deren Zustand liegt in der
     * Liste selbst ( list_t::cur_iter ). Zwei gleichzeitige Iterationen -
     * verschachtelt oder aus einem zweiten Thread, etwa wenn ein Webmodul
     * GetMaxPostStreamSize() aus einem Websocket Callback ruft - wuerden sich
     * gegenseitig ueberschreiben. */
    struct list_entry* e;
    for (e = post_stream_handler_list.first; e != NULL; e = e->next) {
        post_stream_handler_entry* entry = (post_stream_handler_entry*)e->data;
        if (entry && (strcmp(entry->url, url_norm) == 0)) {
            return entry;
        }
    }
    return NULL;
}

/*
 * Free stream context (per-stream state, not the handler entry)
 * Internal
 */
static void freePostStreamContext(post_stream_context* ctx) {
    if (!ctx) return;
    if (ctx->url)        WebserverFree(ctx->url);
    if (ctx->field_name) WebserverFree(ctx->field_name);
    if (ctx->file_name)  WebserverFree(ctx->file_name);
    WebserverFree(ctx);
}

/*
 * Alle Streaming Ressourcen des Sockets freigeben
 */
void cleanupSocketPostStreamState(socket_info* sock) {
    if (!sock) return;

    if (sock->post_stream_ctx) {
        freePostStreamContext(sock->post_stream_ctx);
        sock->post_stream_ctx = NULL;
    }
    if (sock->post_stream_hold) {
        WebserverFree(sock->post_stream_hold);
        sock->post_stream_hold = NULL;
    }
    if (sock->post_stream_delim) {
        WebserverFree(sock->post_stream_delim);
        sock->post_stream_delim = NULL;
    }
    if (sock->post_stream_read_buf) {
        WebserverFree(sock->post_stream_read_buf);
        sock->post_stream_read_buf = NULL;
    }
    if (sock->post_stream_part_header) {
        WebserverFree(sock->post_stream_part_header);
        sock->post_stream_part_header = NULL;
    }

    /* Gepufferte Formularfelder. Im Normalfall hat postStreamFillUploadInfo()
     * sie bereits an den Request uebergeben, bei einem Abbruch vor der
     * Auswertung haengen sie hier noch. */
    post_stream_part* part = sock->post_stream_parts;
    while (part) {
        post_stream_part* next = part->next;
        if (part->name) WebserverFree(part->name);
        if (part->data) WebserverFree(part->data);
        WebserverFree(part);
        part = next;
    }
    sock->post_stream_parts = NULL;
    sock->post_stream_current_part = NULL;
    sock->post_stream_parts_count = 0;
    sock->post_stream_fields_total = 0;
    sock->post_stream_field_limit_logged = 0;

    sock->post_streaming_active      = 0;
    sock->post_stream_state          = POST_STREAM_STATE_PREAMBLE;
    sock->post_stream_handler        = NULL;  /* Just a pointer, handler stays in registry */
    sock->post_stream_hold_len       = 0;
    sock->post_stream_hold_size      = 0;
    sock->post_stream_delim_len      = 0;
    sock->post_stream_read_buf_size  = 0;
    sock->post_stream_part_header_len = 0;
    sock->post_stream_part_is_file   = 0;
    sock->post_stream_started        = 0;
    sock->post_stream_discard        = 0;
}

/*
 * Groesstes Limit ueber alle registrierten Streaming Handler.
 *
 * Das Frontend fragt vor einem Upload "wie gross darf die Datei sein" und
 * bekam bisher max_post_size. Seit das global klein gehalten wird und nur
 * einzelne URLs mehr duerfen, ist die ehrliche Antwort auf diese Frage das
 * groesste Handler-Limit - sonst lehnt die UI eine Firmware ab, die der
 * Server durchaus annehmen wuerde.
 */
uint64_t maxPostStreamSizeInternal( void ) {

    uint64_t max_size = 0;
    struct list_entry* e;

    if (!post_streaming_initialized) return 0;

    /* Zustandslos traversieren, siehe findPostStreamHandler() */
    for (e = post_stream_handler_list.first; e != NULL; e = e->next) {
        post_stream_handler_entry* entry = (post_stream_handler_entry*)e->data;
        if (entry && (entry->max_size > max_size)) {
            max_size = entry->max_size;
        }
    }

    return max_size;
}

/*
 * Prueft ob der Request von einer angemeldeten Session kommt.
 *
 * Laeuft aus check_post_header(), also bevor ein Byte des Bodys gelesen wurde.
 * Der Header ist zu diesem Zeitpunkt vollstaendig, die Cookies sind geparst.
 * restoreSession() wird bewusst mit lock_stores == 0 und create_session == 0
 * gerufen: es soll nur ein bereits bestehender Store gefunden werden, ohne
 * einen anzulegen und ohne ein Lock zu halten, das hier niemand freigeben
 * wuerde.
 *
 * Rueckgabe: 1 = angemeldet, 0 = nicht angemeldet
 */
int postStreamRequestAuthorized(socket_info* sock, post_stream_auth_handler auth_check) {

    http_request tmp;

    if (!sock || !sock->header) return 0;

    memset(&tmp, 0, sizeof(http_request));
    tmp.socket = sock;
    tmp.header = sock->header;

#ifdef WEBSERVER_USE_SESSIONS
    /* Ohne Cookie erzeugt das eine frische GUID und setzt create_cookie,
     * restoreSession sucht dann gar nicht erst - Store bleibt 0. */
    checkSessionCookie(&tmp);
    restoreSession(&tmp, 0, 0);
#endif

    /* Der Consumer entscheidet: nur er weiss, welches Recht sein Upload
     * verlangt. Er sieht hier den vollstaendigen Header samt URL Parametern -
     * die entstehen beim Parsen der Request Zeile - und die
     * wiederhergestellte Session. Damit kann er alle Anmeldewege pruefen,
     * auch die ohne Session ( auth_user/auth_pass, Basic Authorization ). */
    return (auth_check((dummy_handler*)&tmp) > 0) ? 1 : 0;
}

/*
 * Start a new streaming session
 * Called from check_post_header() when a stream handler exists for the URL
 * Internal
 */
int startPostStream(socket_info* sock, post_stream_handler_entry* handler, int discard) {

    if (!sock || !sock->header || !handler) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, 0, "startPostStream: invalid parameters (sock=%p, handler=%p)",
            (void*)sock, (void*)handler);
        return -1;
    }

    if (sock->header->boundary == 0) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: no boundary");
        return -1;
    }

    post_stream_context* ctx = WebserverMalloc(sizeof(post_stream_context));
    if (!ctx) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: failed to allocate stream context");
        return -1;
    }
    memset(ctx, 0, sizeof(post_stream_context));

    /* Copy URL */
    size_t url_len = strlen(sock->header->url);
    ctx->url = WebserverMalloc(url_len + 1);
    if (!ctx->url) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: failed to allocate url");
        WebserverFree(ctx);
        return -1;
    }
    memcpy(ctx->url, sock->header->url, url_len);
    ctx->url[url_len] = '\0';

    ctx->total_length    = sock->header->contentlenght;
    ctx->received_length = 0;
    ctx->aborted         = 0;
    ctx->user_data       = NULL;

    /* Gesuchtes Muster ist "\r\n" + boundary. Das erste Boundary im Body hat
     * kein fuehrendes CRLF, dafuer wird einfach delim + 2 verwendet. */
    uint32_t bound_len = (uint32_t)strlen(sock->header->boundary);
    sock->post_stream_delim_len = bound_len + 2;
    sock->post_stream_delim = WebserverMalloc(sock->post_stream_delim_len + 1);
    if (!sock->post_stream_delim) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: failed to allocate delimiter");
        freePostStreamContext(ctx);
        sock->post_stream_delim_len = 0;
        return -1;
    }
    sock->post_stream_delim[0] = '\r';
    sock->post_stream_delim[1] = '\n';
    memcpy(&sock->post_stream_delim[2], sock->header->boundary, bound_len);
    sock->post_stream_delim[sock->post_stream_delim_len] = '\0';

    /* Rueckhalte Puffer: das laengste Muster ist "\r\n" + boundary + "--" */
    sock->post_stream_hold_size = bound_len + 8;
    sock->post_stream_hold = WebserverMalloc(sock->post_stream_hold_size);
    if (!sock->post_stream_hold) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: failed to allocate hold buffer");
        WebserverFree(sock->post_stream_delim);
        sock->post_stream_delim = NULL;
        sock->post_stream_delim_len = 0;
        freePostStreamContext(ctx);
        sock->post_stream_hold_size = 0;
        return -1;
    }

    sock->post_stream_read_buf_size = POST_STREAM_READ_CHUNK_SIZE;
    sock->post_stream_read_buf = WebserverMalloc(sock->post_stream_read_buf_size);
    if (!sock->post_stream_read_buf) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s", "startPostStream: failed to allocate read buffer");
        WebserverFree(sock->post_stream_delim);
        sock->post_stream_delim = NULL;
        sock->post_stream_delim_len = 0;
        WebserverFree(sock->post_stream_hold);
        sock->post_stream_hold = NULL;
        sock->post_stream_hold_size = 0;
        sock->post_stream_read_buf_size = 0;
        freePostStreamContext(ctx);
        return -1;
    }

    sock->post_stream_ctx             = ctx;
    sock->post_stream_handler         = handler;
    sock->post_streaming_active       = 1;
    sock->post_stream_state           = POST_STREAM_STATE_PREAMBLE;
    sock->post_stream_hold_len        = 0;
    sock->post_stream_part_header_len = 0;
    sock->post_stream_part_is_file    = 0;
    sock->post_stream_current_part    = NULL;
    sock->post_stream_parts_count     = 0;
    sock->post_stream_fields_total    = 0;
    sock->post_stream_field_limit_logged = 0;
    sock->post_stream_started         = 0;
    sock->post_stream_discard         = (char)(discard ? 1 : 0);

#ifdef _WEBSERVER_POST_STREAMING_DEBUG_
    LOG(CONNECTION_LOG, DEBUG_LEVEL, sock->socket, "Started POST streaming for url '%s', content length: %llu",
        ctx->url, (unsigned long long)ctx->total_length);
#endif

    return 0;
}

/***********************************************************************************************
*                                                                                              *
*          Multipart Statemachine                                                              *
*                                                                                              *
************************************************************************************************/

/*
 * Nutzdaten des aktuellen Parts weitergeben.
 *
 * Datei Parts gehen direkt an den Handler, Formularfelder werden gepuffert
 * damit sie das Webmodul spaeter wie gewohnt auslesen kann.
 */
static void emitPartData(socket_info* sock, const unsigned char* data, uint32_t len) {

    post_stream_context* ctx = sock->post_stream_ctx;

    if (len == 0 || ctx == NULL || ctx->aborted) return;

    if (sock->post_stream_part_is_file) {
        ctx->received_length += len;
        /* Ohne Berechtigung wird der Body zwar gelesen - sonst bliebe die
         * Verbindung haengen - aber nichts davon gespeichert. Der Request
         * laeuft danach normal weiter und der Handler antwortet wie gewohnt. */
        if (sock->post_stream_discard == 0) {
            sock->post_stream_handler->on_chunk(ctx, data, len);
        }
        return;
    }

    /* Formularfeld puffern.
     * Der Eintrag des laufenden Parts, nicht der Listenkopf: gibt es fuer
     * diesen Part keinen ( Discard, Part Limit, Abbruch ), duerfen seine Daten
     * nicht am vorherigen Part landen. */
    post_stream_part* part = sock->post_stream_current_part;
    if (part == NULL) return;   /* kein offener Part */
    if (part->is_file) return;  /* Dateien sind gestreamt, nichts zu puffern */

    /* Einmal ueber der Grenze bleibt der Wert abgeschnitten. Sonst wuerde ein
     * spaeterer kleiner Chunk wieder angehaengt und der Wert haette ein Loch
     * in der Mitte - schlimmer als ein sauber gekuerzter Wert. */
    if (part->truncated) {
        return;
    }

    /* Nur einmal pro Request loggen, nicht pro Part.
     *
     * emitPartData() wird beim Boundary Backtracking auch byteweise gerufen,
     * ein Guard je Aufruf reicht also nicht. Ein Guard je Part reicht aber
     * auch nicht: die Gesamtsumme wird bei Ueberschreitung nicht weiter
     * hochgezaehlt, also reisst sie jedes Feld erneut, das groesser ist als
     * die verbliebene Restkapazitaet - gemessen sechs Zeilen fuer einen Body
     * aus zwoelf Feldern. Auf einem Geraet mit NOR Flash Log ist jede
     * vermeidbare Zeile Verschleiss, und der Vorgang ist derselbe: die
     * Feldgrenzen dieses Requests sind erreicht. */

    /* Gesamtsumme ueber alle Felder des Requests, nicht nur pro Feld */
    if (sock->post_stream_fields_total + len > POST_STREAM_MAX_FIELDS_TOTAL) {
        part->truncated = 1;
        if (sock->post_stream_field_limit_logged == 0) {
            sock->post_stream_field_limit_logged = 1;
            LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket,
                "POST stream: form fields exceed %d bytes in total, affected fields are dropped",
                POST_STREAM_MAX_FIELDS_TOTAL);
        }
        return;
    }

    if (part->length + len > POST_STREAM_MAX_FIELD_SIZE) {
        part->truncated = 1;
        if (sock->post_stream_field_limit_logged == 0) {
            sock->post_stream_field_limit_logged = 1;
            LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket,
                "POST stream: form field '%s' exceeds %d bytes, field is dropped",
                part->name ? part->name : "", POST_STREAM_MAX_FIELD_SIZE);
        }
        return;
    }

    /* Kapazitaet verdoppeln statt pro Chunk exakt neu zu allokieren.
     *
     * Mit exakter Neuallokation kopiert jeder Chunk den bisherigen Wert
     * komplett um. Das faellt nicht auf, solange die Chunks gross sind - der
     * Boundary Scanner gibt die Nutzdaten aber byteweise heraus, sobald sie wie
     * ein Musteranfang aussehen ( needle[0] ist '\r' ). Ein Feldwert aus lauter
     * '\r' erzwingt das fuer jedes Byte, und aus 60 KiB Eingabe werden 60000
     * Allokationen mit im Mittel 30 KiB Kopie: gemessen 27 ms allein fuer die
     * Pufferarbeit gegenueber 0,03 ms mit Verdopplung. */
    if (part->length + len + 1 > part->capacity) {

        uint64_t neu = (part->capacity == 0) ? 256 : part->capacity * 2;

        while (neu < part->length + len + 1) {
            neu *= 2;
        }

        /* Oberhalb der Feldgrenze waechst nichts mehr - dort waere die
         * Verdopplung nur noch Verschnitt. */
        if (neu > (uint64_t)POST_STREAM_MAX_FIELD_SIZE + 1) {
            neu = part->length + len + 1;
        }

        char* grown = WebserverMalloc((SIZE_TYPE)neu);
        if (!grown) return;

        if (part->data) {
            memcpy(grown, part->data, (size_t)part->length);
            WebserverFree(part->data);
        }
        part->data = grown;
        part->capacity = neu;
    }

    memcpy(&part->data[part->length], data, len);
    part->length += len;
    sock->post_stream_fields_total += len;
    part->data[part->length] = '\0';
}

/*
 * Nach einem Mismatch den Rueckhalte Puffer neu ausrichten.
 *
 * Es wird solange das erste Byte als Nutzdaten ausgegeben bis der Rest wieder
 * ein Praefix des gesuchten Musters ist. Der Puffer ist nur wenige Dutzend
 * Byte gross, das ist damit unkritisch und kommt ohne KMP aus.
 */
static void rebaseHold(socket_info* sock, const unsigned char* needle) {

    while (sock->post_stream_hold_len > 0 &&
           memcmp(sock->post_stream_hold, needle, sock->post_stream_hold_len) != 0) {

        emitPartData(sock, sock->post_stream_hold, 1);
        sock->post_stream_hold_len--;
        memmove(sock->post_stream_hold, &sock->post_stream_hold[1], sock->post_stream_hold_len);
    }
}

/*
 * Im Datenstrom nach dem Muster suchen.
 *
 * Alles vor dem Muster wird als Nutzdaten ausgegeben ( ausser emit == 0 ).
 * Bytes die ein Praefix des Musters sein koennten werden zurueckgehalten und
 * erst ausgegeben wenn feststeht dass sie nicht zum Muster gehoeren. Damit
 * funktioniert die Erkennung auch wenn ein Boundary auf einer Chunk Grenze
 * liegt.
 *
 * Rueckgabe: 1 = Muster gefunden, *pos steht dahinter
 *            0 = Chunk zuende, Zustand steckt im Rueckhalte Puffer
 */
static int scanForNeedle(socket_info* sock, const unsigned char* needle, uint32_t needle_len,
                         const unsigned char* data, uint32_t len, uint32_t* pos, int emit) {

    while (*pos < len) {

        if (sock->post_stream_hold_len == 0) {
            /* Schnellpfad: bis zum naechsten moeglichen Musteranfang springen */
            const unsigned char* p = memchr(&data[*pos], needle[0], len - *pos);
            if (p == NULL) {
                if (emit) emitPartData(sock, &data[*pos], len - *pos);
                *pos = len;
                return 0;
            }
            uint32_t skip = (uint32_t)(p - &data[*pos]);
            if (emit && skip > 0) emitPartData(sock, &data[*pos], skip);
            *pos += skip;
        }

        /* Rueckhalte Puffer weiter fuellen solange die Bytes passen */
        while (sock->post_stream_hold_len < needle_len && *pos < len &&
               data[*pos] == needle[sock->post_stream_hold_len]) {
            sock->post_stream_hold[sock->post_stream_hold_len++] = data[*pos];
            (*pos)++;
        }

        if (sock->post_stream_hold_len == needle_len) {
            sock->post_stream_hold_len = 0;
            return 1;
        }

        if (*pos >= len) {
            /* Chunk zuende mitten im Muster, Rest beim naechsten Aufruf */
            return 0;
        }

        /* Mismatch: erstes Byte gehoert doch zu den Nutzdaten */
        if (emit) {
            emitPartData(sock, sock->post_stream_hold, 1);
        }
        sock->post_stream_hold_len--;
        memmove(sock->post_stream_hold, &sock->post_stream_hold[1], sock->post_stream_hold_len);

        if (emit) {
            rebaseHold(sock, needle);
        } else {
            /* Preamble wird verworfen, Puffer einfach leeren */
            sock->post_stream_hold_len = 0;
        }
    }

    return 0;
}

/*
 * filename= / name= aus der Content-Disposition holen
 */
static char* extractDispositionValue(const char* disposition, const char* key) {

    const char* p;
    const char* start;
    const char* end;
    size_t key_len;
    size_t len;
    char* value;

    if (disposition == NULL) return NULL;

    key_len = strlen(key);

    /* Auf Parametergrenze pruefen: "name=" ist ein Teilstring von "filename=".
     * Eine reine Substringsuche liefert sonst bei
     *   Content-Disposition: form-data; filename="a.bin"; name="datei"
     * fuer "name=" den Wert von filename - und da post_stream_part_is_file an
     * file_name haengt, kippt das die Einordnung Datei/Formularfeld. */
    /* Der Treffer muss an einer Parametergrenze stehen UND von einem
     * Anfuehrungszeichen gefolgt sein. Beides in der Schleife pruefen, sonst
     * bricht ein Dateiname wie filename="mein name=x.bin" die Suche nach dem
     * echten name=" ab, statt weiterzusuchen. Die Pruefung auf das
     * Anfuehrungszeichen faengt zugleich einen abgeschnittenen Header ab
     * ( '... name=' am Ende ), der sonst ueber das String-Ende laese. */
    p = disposition;
    while ((p = strstr(p, key)) != NULL) {
        if (((p == disposition) || (p[-1] == ' ') || (p[-1] == ';')) &&
            (p[key_len] == '"')) {
            break;
        }
        p += key_len;
    }

    if (p == NULL) return NULL;

    start = &p[key_len + 1];
    end   = strchr(start, '"');
    if (end == NULL) return NULL;

    len = (size_t)(end - start);
    value = WebserverMalloc(len + 1);

    memcpy(value, start, len);
    value[len] = '\0';
    return value;
}

/*
 * Gesammelten Part Header auswerten und den Part eroeffnen.
 *
 * Rueckgabe -1, wenn der Part Header nicht auswertbar ist. Der Request ist
 * damit hin: die Statemachine kann ohne verlaessliche Disposition nicht
 * entscheiden, ob der Part eine Datei oder ein Formularfeld ist.
 */
static int beginPart(socket_info* sock) {

    post_stream_context* ctx = sock->post_stream_ctx;
    HttpRequestHeader* header = sock->header;

    /* Kein Eintrag, solange keiner angelegt wurde. endPart() darf sich nicht
     * auf den Listenkopf verlassen - das ist der vorherige, fertige Part,
     * sobald fuer den aktuellen keiner entsteht ( Discard, Part Limit,
     * Abbruch ). Sonst bekommt eine bereits vollstaendig gestreamte Datei
     * nachtraeglich Laenge 0 und failed = 1 und verschwindet aus
     * getFileName()/getFileSize(). */
    sock->post_stream_current_part = NULL;

    /* aborted gilt laut API fuer den restlichen Request. Ab hier wird der Body
     * nur noch ueberlesen: kein Header wird mehr ausgewertet ( der Puffer ist
     * nach dem Abbruch ohnehin nur noch ein Fenster, siehe collectPartHeader ),
     * kein Eintrag entsteht und kein on_start() laeuft an. Beim echten
     * Consumer legt on_start() die Zieldatei an und loescht dabei die
     * vorhandene: nach dem Abbruch waere das reine Zerstoerung, und ohne
     * on_chunk() dazwischen bliebe sie leer. */
    if (ctx->aborted) {
        sock->post_stream_part_is_file = 0;
        return 0;
    }

    /* Content_Disposition des vorherigen Parts verwerfen */
    if (header->Content_Disposition) {
        WebserverFree(header->Content_Disposition);
        header->Content_Disposition = NULL;
    }
    if (header->Content_Type) {
        WebserverFree(header->Content_Type);
        header->Content_Type = NULL;
    }

    /* analyseFormDataLine() erwartet den Header inklusive des fuehrenden
     * "\r\n" hinter dem Boundary, genau so wie er gesammelt wurde. */
    analyseFormDataLine(sock, sock->post_stream_part_header,
                        sock->post_stream_part_header_len, header);

    /* Ein kaputter Part Header - etwa mit einem NUL Byte im Wert - wird vom
     * Parser als Fehler gemeldet. Weiterlaufen hiesse mit Content_Disposition
     * aus dem vorherigen Part arbeiten. */
    if (header->error) {
        LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket, "%s",
            "POST stream: malformed part header, rejecting request");
        return -1;
    }

    if (ctx->field_name) { WebserverFree(ctx->field_name); ctx->field_name = NULL; }
    if (ctx->file_name)  { WebserverFree(ctx->file_name);  ctx->file_name  = NULL; }

    ctx->field_name = extractDispositionValue(header->Content_Disposition, "name=");
    ctx->file_name  = extractDispositionValue(header->Content_Disposition, "filename=");

    sock->post_stream_part_is_file = (ctx->file_name != NULL) ? 1 : 0;
    ctx->received_length = 0;

    /* Ohne Berechtigung entsteht gar kein Part: weder Daten noch Namen. Sonst
     * sickern die Feldnamen eines unangemeldeten Requests in upload_files, und
     * ein Handler, der nur ws_list_size() prueft, haelt den Upload faelschlich
     * fuer vorhanden. */
    if (sock->post_stream_discard == 1) {
        return 0;
    }

    /* Fuer jeden Part einen Eintrag anlegen - auch fuer Dateien. Der ctx wird
     * ueber alle Parts wiederverwendet, seine file_name/field_name sind nach
     * dem naechsten beginPart() ueberschrieben. Ohne eigenen Eintrag pro Part
     * verschwindet eine Datei aus getFileName()/getFileSize(), sobald ihr ein
     * Formularfeld folgt, und bei mehreren Dateien bliebe nur die letzte.
     * ( Das Part Limit greift schon in AFTER_BOUNDARY, hier ist es damit
     *   bereits ueber ctx->aborted erledigt. ) */
    {
        post_stream_part* part = WebserverMalloc(sizeof(post_stream_part));

        memset(part, 0, sizeof(post_stream_part));
        part->is_file = sock->post_stream_part_is_file;

        {
            /* Dateien werden unter ihrem Dateinamen gefuehrt, so wie es der
             * gepufferte Weg in parsePostData() auch tut. */
            /* Nie NULL lassen: der gepufferte Pfad in parsePostData() legt bei
             * fehlendem Namen einen Leerstring an, und getFileName() Aufrufer
             * rechnen nicht mit NULL. */
            const char* nm = part->is_file ? ctx->file_name : ctx->field_name;
            size_t nlen = nm ? strlen(nm) : 0;

            if (nlen > POST_STREAM_MAX_NAME) {
                nlen = POST_STREAM_MAX_NAME;
                if (sock->post_stream_field_limit_logged == 0) {
                    sock->post_stream_field_limit_logged = 1;
                    LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket,
                        "POST stream: part name exceeds %d bytes, name is shortened",
                        POST_STREAM_MAX_NAME);
                }
            }

            part->name = WebserverMalloc(nlen + 1);
            if (nlen) memcpy(part->name, nm, nlen);
            part->name[nlen] = '\0';
        }

        part->next = sock->post_stream_parts;
        sock->post_stream_parts = part;
        sock->post_stream_current_part = part;
        sock->post_stream_parts_count++;
    }

    if (sock->post_stream_part_is_file) {
        sock->post_stream_handler->on_start(ctx);
        sock->post_stream_started = 1;
    }

    return 0;
}

/*
 * Aktuellen Part abschliessen
 */
static void endPart(socket_info* sock, int success) {

    if (sock->post_stream_part_is_file && sock->post_stream_started) {
        post_stream_context* ctx = sock->post_stream_ctx;
        sock->post_stream_handler->on_end(ctx, ctx->aborted ? 0 : success);
        sock->post_stream_started = 0;

#ifdef _WEBSERVER_POST_STREAMING_DEBUG_
        LOG(CONNECTION_LOG, DEBUG_LEVEL, sock->socket, "POST stream part '%s' finished, %llu bytes",
            ctx->file_name ? ctx->file_name : "", (unsigned long long)ctx->received_length);
#endif
    }

    /* Ergebnis erst nach on_end() vermerken: der Handler darf dort noch
     * abbrechen, weil ihm zum Beispiel die Pruefsumme ueber die fertige Datei
     * nicht passt. Ein abgebrochener Part darf spaeter nicht als vorhandene
     * Datei gemeldet werden - der Handler hat seine Teildatei bereits
     * weggeraeumt. */
    if (sock->post_stream_current_part != NULL) {
        post_stream_part* cur = sock->post_stream_current_part;
        if (cur->is_file) {
            post_stream_context* c = sock->post_stream_ctx;
            cur->length = c ? c->received_length : 0;
            if ((success == 0) || (c && c->aborted)) {
                cur->failed = 1;
            }
        }
    }

    sock->post_stream_current_part = NULL;
    sock->post_stream_part_is_file = 0;
}

/*
 * Part Header Bytes sammeln bis "\r\n\r\n" erreicht ist
 *
 * Rueckgabe: 1 = Header vollstaendig, 0 = weitere Daten noetig, -1 = Fehler
 */
static int collectPartHeader(socket_info* sock, const unsigned char* data, uint32_t len, uint32_t* pos) {

    post_stream_context* ctx = sock->post_stream_ctx;

    /* Nach dem Abbruch ( PostStreamAbort() oder Part Limit ) wird der Rest des
     * Bodys nur noch ueberlesen. Die Groessengrenze hier weiter scharf zu
     * stellen hiesse, einen bereits verworfenen Rest doch noch mit 400 zu
     * beantworten - der Client bekaeme statt der Antwort seines URL Handlers
     * eine generische Fehlerseite, und beim Part Limit waeren auch die zuvor
     * akzeptierten Parts weg. */
    const int header_is_discarded = (ctx != NULL) && (ctx->aborted != 0);

    while (*pos < len) {

        if (sock->post_stream_part_header_len + 1 >= POST_STREAM_MAX_PART_HEADER) {

            if (header_is_discarded) {
                /* Die letzten 3 Bytes bleiben stehen, damit ein "\r\n\r\n"
                 * ueber die Puffergrenze hinweg noch erkannt wird. */
                memmove(sock->post_stream_part_header,
                        &sock->post_stream_part_header[sock->post_stream_part_header_len - 3],
                        3);
                sock->post_stream_part_header_len = 3;
            } else {
                LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket,
                    "POST stream: part header exceeds %d bytes", POST_STREAM_MAX_PART_HEADER);
                return -1;
            }
        }

        sock->post_stream_part_header[sock->post_stream_part_header_len++] = (char)data[*pos];
        (*pos)++;

        if (sock->post_stream_part_header_len >= 4 &&
            0 == memcmp(&sock->post_stream_part_header[sock->post_stream_part_header_len - 4], "\r\n\r\n", 4)) {
            sock->post_stream_part_header[sock->post_stream_part_header_len] = '\0';
            return 1;
        }
    }

    return 0;
}

/*
 * Body Bytes in die Statemachine geben
 *
 * Rueckgabe: 1 = Body vollstaendig, 0 = weitere Daten erwartet, -1 = Fehler
 */
int postStreamFeed(socket_info* sock, const unsigned char* data, uint32_t len) {

    uint32_t pos = 0;

    if (!sock || !sock->post_streaming_active || !sock->post_stream_ctx) {
        LOG(CONNECTION_LOG, ERROR_LEVEL, sock ? sock->socket : 0, "%s", "postStreamFeed: no active stream");
        return -1;
    }

    sock->header->post_buffer_pos += len;

    while (pos < len) {

        switch (sock->post_stream_state) {

        case POST_STREAM_STATE_PREAMBLE:
            /* Das erste Boundary steht ohne fuehrendes CRLF am Body Anfang,
             * darum delim + 2. Alles davor ist Preamble und wird verworfen. */
            if (1 == scanForNeedle(sock, &sock->post_stream_delim[2], sock->post_stream_delim_len - 2,
                                   data, len, &pos, 0)) {
                sock->post_stream_delim_in_data = 0;
                sock->post_stream_state = POST_STREAM_STATE_DELIM_CHECK;
            }
            break;

        case POST_STREAM_STATE_AFTER_BOUNDARY:
            /* Part Limit hier und nicht erst in beginPart(): sonst wird der
             * Header des ueberzaehligen Parts noch mit scharfer Groessengrenze
             * gesammelt, und ein ueberlanger Header macht aus dem laengst
             * verworfenen Rest einen Parserfehler fuer den ganzen Request -
             * samt der Parts, die bis zum Limit angenommen wurden.
             * Nur einmal geloggt: ab hier bleibt aborted gesetzt. Sonst
             * schreibt ein Body mit zehntausend Parts ebenso viele Zeilen ins
             * Flash Log. */
            if ((sock->post_stream_ctx != NULL) &&
                (sock->post_stream_ctx->aborted == 0) &&
                (sock->post_stream_parts_count >= POST_STREAM_MAX_PARTS)) {
                LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket,
                    "POST stream: more than %d parts, dropping the rest of the request",
                    POST_STREAM_MAX_PARTS);
                sock->post_stream_ctx->aborted = 1;
            }

            /* Das CRLF hinter dem Boundary hat DELIM_CHECK bereits geprueft und
             * verbraucht. analyseFormDataLine() erwartet es als Teil des Headers */
            if (sock->post_stream_part_header == NULL) {
                sock->post_stream_part_header = WebserverMalloc(POST_STREAM_MAX_PART_HEADER);
                if (sock->post_stream_part_header == NULL) {
                    LOG(CONNECTION_LOG, ERROR_LEVEL, sock->socket, "%s",
                        "POST stream: failed to allocate part header buffer");
                    sock->post_stream_state = POST_STREAM_STATE_ERROR;
                    return -1;
                }
            }
            sock->post_stream_part_header[0] = '\r';
            sock->post_stream_part_header[1] = '\n';
            sock->post_stream_part_header_len = 2;

            sock->post_stream_hold_len = 0;
            sock->post_stream_state = POST_STREAM_STATE_PART_HEADER;
            break;

        case POST_STREAM_STATE_PART_HEADER: {
            int ret = collectPartHeader(sock, data, len, &pos);
            if (ret < 0) {
                sock->post_stream_state = POST_STREAM_STATE_ERROR;
                return -1;
            }
            if (ret == 1) {
                if (beginPart(sock) < 0) {
                    sock->post_stream_state = POST_STREAM_STATE_ERROR;
                    return -1;
                }
                sock->post_stream_state = POST_STREAM_STATE_PART_DATA;
            }
            break;
        }

        case POST_STREAM_STATE_PART_DATA:
            /* Der Part wird hier noch nicht beendet: erst die zwei Bytes hinter
             * dem Boundary entscheiden, ob es ueberhaupt ein Delimiter ist.
             * Sonst meldet on_end() Erfolg fuer Nutzdaten, die zufaellig oder
             * absichtlich "\r\n--<boundary>" enthalten. */
            if (1 == scanForNeedle(sock, sock->post_stream_delim, sock->post_stream_delim_len,
                                   data, len, &pos, 1)) {
                sock->post_stream_delim_in_data = 1;
                sock->post_stream_state = POST_STREAM_STATE_DELIM_CHECK;
            }
            break;

        case POST_STREAM_STATE_DELIM_CHECK: {
            /* Nach RFC 2046 folgt auf das Boundary entweder CRLF ( naechster
             * Part ) oder "--" ( Ende ). Alles andere ist kein Delimiter. */
            while (sock->post_stream_hold_len < 2 && pos < len) {
                sock->post_stream_hold[sock->post_stream_hold_len++] = data[pos++];
            }
            if (sock->post_stream_hold_len < 2) break;

            if (0 == memcmp(sock->post_stream_hold, "--", 2)) {
                sock->post_stream_hold_len = 0;
                if (sock->post_stream_delim_in_data) endPart(sock, 1);
                sock->post_stream_state = POST_STREAM_STATE_EPILOGUE;
                break;
            }

            if (0 == memcmp(sock->post_stream_hold, "\r\n", 2)) {
                sock->post_stream_hold_len = 0;
                if (sock->post_stream_delim_in_data) endPart(sock, 1);
                sock->post_stream_state = POST_STREAM_STATE_AFTER_BOUNDARY;
                break;
            }

            /* Kein gueltiger Delimiter. In der Preamble einfach weitersuchen,
             * in den Nutzdaten gehoeren die zurueckgehaltenen Bytes zum Payload. */
            if (sock->post_stream_delim_in_data) {
                emitPartData(sock, sock->post_stream_delim, sock->post_stream_delim_len);
                /* Die zwei gelesenen Bytes koennen selbst ein neuer Delimiter
                 * Anfang sein - rebaseHold gibt aus was nicht passt und behaelt
                 * den Rest als Praefix. */
                rebaseHold(sock, sock->post_stream_delim);
                sock->post_stream_state = POST_STREAM_STATE_PART_DATA;
            } else {
                sock->post_stream_hold_len = 0;
                sock->post_stream_state = POST_STREAM_STATE_PREAMBLE;
            }
            break;
        }

        case POST_STREAM_STATE_EPILOGUE:
            /* Nach dem End Boundary folgen keine Nutzdaten mehr */
            pos = len;
            break;

        default:
            return -1;
        }
    }

    if (sock->header->post_buffer_pos >= sock->header->contentlenght) {

        if (sock->post_stream_state != POST_STREAM_STATE_EPILOGUE) {
            LOG(CONNECTION_LOG, WARNING_LEVEL, sock->socket, "%s",
                "POST stream: body ended before the closing boundary");
            postStreamFinish(sock, 0);
            return -1;
        }

        return 1;
    }

    return 0;
}

/*
 * Laufenden Stream beenden, z.B. bei Verbindungsabbruch
 */
void postStreamFinish(socket_info* sock, int success) {

    if (!sock || !sock->post_streaming_active) return;

    if (sock->post_stream_started) {
        /* Ein Part war noch offen, der Handler muss aufraeumen koennen */
        endPart(sock, success);
    }
}

/*
 * Metadaten des gestreamten Parts in den http_request uebertragen.
 *
 * Der Datei Part hat kein data, seine Nutzdaten sind ja bereits beim Handler
 * gelandet. getFileName() und getFileSize() liefern trotzdem die gewohnten
 * Werte, getFileData() gibt NULL zurueck.
 */
void postStreamFillUploadInfo(http_request* s) {

    socket_info* sock = s->socket;
    post_stream_part* part;
    post_stream_part* reversed = NULL;

    if (!sock || !sock->post_streaming_active) return;

    /* Die Liste wurde beim Empfang vorne angehaengt - fuer die Reihenfolge im
     * Request umdrehen, damit getFileName(s,0) den ersten Part des Bodys
     * liefert und nicht den letzten. */
    part = sock->post_stream_parts;
    while (part) {
        post_stream_part* next = part->next;
        part->next = reversed;
        reversed = part;
        part = next;
    }
    sock->post_stream_parts = NULL;
    /* Die Eintraege sind unten gleich freigegeben - ein current_part von einem
     * noch offenen Part waere danach ein Zeiger ins Nichts, in den endPart()
     * schreibt. */
    sock->post_stream_current_part = NULL;
    sock->post_stream_parts_count = 0;

    part = reversed;
    while (part) {
        post_stream_part* next = part->next;
        int keep = 1;

        /* Nur Dateien. getFileName()/getFileData()/getFileSize() liefern
         * Dateien - das sagt schon der Name. Landen Formularfelder in
         * derselben Liste, kann ein Konsument beides nicht auseinanderhalten:
         * ein Request ganz ohne Datei liefert dann trotzdem einen Namen an
         * Index 0. Ein Handler, der nur prueft ob ueberhaupt etwas da ist,
         * haelt den Upload damit faelschlich fuer vorhanden. */
        if (part->is_file == 0) {
            keep = 0;
        }

        /* Verworfene Requests melden gar nichts, und ein abgebrochener Datei
         * Part darf nicht als vorhandene Datei erscheinen - der Handler hat
         * seine Teildatei bereits entfernt, ein Konsument wuerde auf etwas
         * zugreifen, das es nicht mehr gibt.
         * ( Im Discard Fall entstehen ohnehin keine Parts, das hier ist das
         *   Sicherheitsnetz. ) */
        if (sock->post_stream_discard || (part->is_file && part->failed)) {
            keep = 0;
        }

        /* Ein an der Groessengrenze gekuerzter Wert ist kein gueltiger Wert.
         * ( Formularfelder sind oben schon raus, das hier bleibt als Regel
         *   fuer alles stehen, was ueber die Groessengrenzen laeuft. )
         * Ohne eigene Meldung: emitPartData() hat den Vorgang beim Reissen der
         * Grenze bereits gemeldet. */
        if (part->truncated) {
            keep = 0;
        }

        if (keep) {
            upload_file_info* file_info = WebserverMalloc(sizeof(upload_file_info));

            file_info->name   = part->name;   /* Besitz geht ueber */
            file_info->data   = part->data;   /* bei Dateien NULL, sie sind gestreamt */
            file_info->length = part->length;
            ws_list_append(&s->upload_files, file_info, 0);

            part->name = NULL;
            part->data = NULL;
        }

        if (part->name) WebserverFree(part->name);
        if (part->data) WebserverFree(part->data);
        WebserverFree(part);

        part = next;
    }
}
