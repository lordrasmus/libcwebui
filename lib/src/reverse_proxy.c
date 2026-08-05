/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include "webserver.h"
#include "intern/reverse_proxy.h"

/*
 * Streaming ist implementiert - Daten werden sofort an den Client weitergeleitet
 * während sie vom Backend empfangen werden. Der Buffer wird nur für Zwischenspeicherung
 * verwendet wenn der Client-Socket blockiert (STREAMING_WAIT_WRITE).
 */
#define PROXY_BUFFER_SIZE       (64 * 1024)
#define PROXY_MAX_HEADER_SIZE   (8 * 1024)

/* Debug Level System für Reverse Proxy
 * 0 oder undefiniert: Keine Ausgaben
 * 1: Nur Fehler
 * 2: Fehler + wichtige Zustandsänderungen (connect, done)
 * 3: + Request/Response Details
 * 4: + Alle Zustandsänderungen
 * 5: + Detaillierte Byte-Level Infos
 * 6: + HTTP Header Dumps (Request und Response)
 */
#ifndef _WEBSERVER_PROXY_DEBUG_
#define _WEBSERVER_PROXY_DEBUG_ 0
#endif

/*
 * Static Variables
 */
static reverse_proxy_registration_t* g_proxy_registrations = NULL;

/*
 * Forward Declarations
 */
static int proxy_process_state(reverse_proxy_connection_t* proxy);
static void proxy_cleanup_connection(reverse_proxy_connection_t* proxy);
static int proxy_connect_backend(reverse_proxy_connection_t* proxy);
static int proxy_parse_response_header(reverse_proxy_connection_t* proxy);
static int proxy_find_header_end(const unsigned char* buffer, uint32_t len);
/* Return values of proxy_parse_content_length() besides the length itself */
#define PROXY_CL_MISSING   (-1)
#define PROXY_CL_INVALID   (-2)

static int64_t proxy_parse_content_length(const unsigned char* header, uint32_t header_len);
static int proxy_check_chunked_encoding(const unsigned char* header, uint32_t header_len);
static int proxy_check_websocket_upgrade(const unsigned char* header, uint32_t header_len);
static int proxy_get_status_code(const unsigned char* header, uint32_t header_len);
static int proxy_inject_forwarded_headers(reverse_proxy_connection_t* proxy);
static int proxy_inject_connection_close(reverse_proxy_connection_t* proxy);
static int proxy_extract_host_header(const unsigned char* header, uint32_t header_len, char* host_buf, int host_buf_size);
static int proxy_request_is_upgrade(const unsigned char* header, uint32_t header_len);
static int proxy_force_connection_close(unsigned char* header, uint32_t header_len);

#if _WEBSERVER_PROXY_DEBUG_ >= 2
/*
 * Timing Helper - returns milliseconds between two timespecs
 */
static double proxy_time_diff_ms(struct timespec* start, struct timespec* end) {
    double diff = (end->tv_sec - start->tv_sec) * 1000.0;
    diff += (end->tv_nsec - start->tv_nsec) / 1000000.0;
    return diff;
}
#endif

#if _WEBSERVER_PROXY_DEBUG_ >= 6
/*
 * Debug Helper - dump HTTP header line by line
 */
static void proxy_dump_header(const char* label, const unsigned char* buffer, uint32_t header_end) {
    uint32_t line_start = 0;
    uint32_t i;
    char line_buf[512];

    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "=== %s ===", label);

    for (i = 0; i < header_end; i++) {
        if (buffer[i] == '\n') {
            uint32_t line_len = i - line_start;
            if (line_len > 0 && buffer[i-1] == '\r') {
                line_len--;
            }
            if (line_len > sizeof(line_buf) - 1) {
                line_len = sizeof(line_buf) - 1;
            }
            memcpy(line_buf, &buffer[line_start], line_len);
            line_buf[line_len] = '\0';
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "  %s", line_buf);
            line_start = i + 1;
        }
    }
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "=== END %s ===", label);
}
#endif

/*
 * Send 502 Bad Gateway error response directly on socket
 */
static void proxy_send_error_response(reverse_proxy_connection_t* proxy) {
    char body[512];
    char response[768];
    int body_len;
    int response_len;
    size_t total_sent = 0;
    size_t sent;
    SOCKET_SEND_STATUS status;
    const char* backend_path = (proxy != NULL && proxy->backend_path != NULL) ? proxy->backend_path : "unknown";

    body_len = snprintf(body, sizeof(body),
        "<html><head><title>502 Bad Gateway</title></head>"
        "<body><h1>502 Bad Gateway</h1>"
        "<p>Backend service unavailable: %s</p></body></html>",
        backend_path);

    response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 502 Bad Gateway\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        body_len, body);

    if (proxy == NULL || proxy->client_sock == NULL || proxy->client_sock->socket < 0) {
        return;
    }

    /* Ueber WebserverSend senden, nicht per rohem send(): auf einer
     * HTTPS-Verbindung ginge die Fehlerseite sonst unverschluesselt in den
     * TLS-Stream und der Browser meldet einen Protokollfehler statt die
     * Seite anzuzeigen. */
    while (total_sent < (size_t)response_len) {
        sent = 0;
        status = WebserverSend(proxy->client_sock,
                               (const unsigned char*)&response[total_sent],
                               (size_t)response_len - total_sent, 0, &sent);

        /* sent auch bei BUFFER_FULL mitzaehlen - mbedTLS liefert dort
         * bereits uebernommene Bytes, OpenSSL meldet 0 */
        total_sent += sent;

        if (status == SOCKET_SEND_CLIENT_DISCONNECTED) {
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->client_sock->socket, "%s",
                "client disconnected while sending 502");
            return;
        }

        if (status == SOCKET_SEND_SEND_BUFFER_FULL || sent == 0) {
            /* Sendepuffer voll. Die Verbindung wird gleich abgebaut, ein
             * Write-Event dafuer zu registrieren lohnt nicht. */
            LOG(PROXY_LOG, WARNING_LEVEL, proxy->client_sock->socket,
                "502 response only partially sent: %zu of %d bytes", total_sent, response_len);
            return;
        }
    }
}

/*
 * Registration Functions
 */

void reverse_proxy_register_uds_internal(const char* method, const char* url_prefix, const char* backend_path) {
    reverse_proxy_registration_t* reg;
    
    if (url_prefix == NULL || backend_path == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Invalid parameters for registration");
        return;
    }

    reg = WebserverMalloc(sizeof(reverse_proxy_registration_t));
    if (reg == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to allocate registration");
        return;
    }
    
    memset(reg, 0, sizeof(reverse_proxy_registration_t));
    
    if (method != NULL) {
        reg->method = strdup(method);
    } else {
        reg->method = strdup("*");
    }

    reg->url_prefix = strdup(url_prefix);
    reg->backend_path = strdup(backend_path);

    /* Without these strings the registration would be matched later and
     * dereferenced, so give up here instead. free() accepts NULL. */
    if (reg->method == NULL || reg->url_prefix == NULL || reg->backend_path == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "Failed to allocate registration strings for %s", url_prefix);
        free(reg->method);
        free(reg->url_prefix);
        free(reg->backend_path);
        WebserverFree(reg);
        return;
    }

    reg->url_prefix_len = strlen(url_prefix);
    reg->type = PROXY_TYPE_UDS;
    reg->forward_websocket = 1;  /* Default: WebSockets weiterleiten */
    reg->next = NULL;
    
    /* Ans Ende der Liste anhängen */
    if (g_proxy_registrations == NULL) {
        g_proxy_registrations = reg;
    } else {
        reverse_proxy_registration_t* p = g_proxy_registrations;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = reg;
    }
    
#if _WEBSERVER_PROXY_DEBUG_ >= 2
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Registered %s %s -> %s",
              reg->method, reg->url_prefix, reg->backend_path);
#endif
}

/*
 * Find matching proxy registration
 */
static reverse_proxy_registration_t* proxy_find_registration(const char* method, const char* url) {
    reverse_proxy_registration_t* reg = g_proxy_registrations;
    
    while (reg != NULL) {
        /* Check URL prefix */
        if (strncmp(url, reg->url_prefix, reg->url_prefix_len) == 0) {
            /* Check method (wenn nicht "*") */
            if (strcmp(reg->method, "*") == 0 || 
                strcasecmp(reg->method, method) == 0) {
                return reg;
            }
        }
        reg = reg->next;
    }
    
    return NULL;
}

/*
 * Extract method and URL from first line of HTTP request
 */
static int proxy_parse_request_line(const char* buffer, int len, 
                                    char* method, int method_size,
                                    char* url, int url_size) {
    int i = 0;
    int pos = 0;
    
    /* Parse method */
    while (i < len && buffer[i] != ' ' && pos < method_size - 1) {
        method[pos++] = buffer[i++];
    }
    method[pos] = '\0';
    
    if (i >= len || buffer[i] != ' ') {
        return -1;
    }
    i++;  /* Skip space */
    
    /* Parse URL */
    pos = 0;
    while (i < len && buffer[i] != ' ' && buffer[i] != '?' && pos < url_size - 1) {
        url[pos++] = buffer[i++];
    }
    url[pos] = '\0';
    
    return 0;
}

/*
 * Create new proxy connection
 */
static reverse_proxy_connection_t* proxy_create_connection(socket_info* client_sock, 
                                                           reverse_proxy_registration_t* reg) {
    reverse_proxy_connection_t* proxy;
    
    proxy = WebserverMalloc(sizeof(reverse_proxy_connection_t));
    if (proxy == NULL) {
        return NULL;
    }
    
    memset(proxy, 0, sizeof(reverse_proxy_connection_t));

#if _WEBSERVER_PROXY_DEBUG_ >= 2
    /* Timing starten */
    clock_gettime(CLOCK_MONOTONIC, &proxy->time_start);
#endif

    proxy->state = PROXY_STATE_RECV_CLIENT_HEADER;
    proxy->client_sock = client_sock;
    proxy->backend_fd = -1;
    proxy->backend_path = strdup(reg->backend_path);
    if (proxy->backend_path == NULL) {
        WebserverFree(proxy);
        return NULL;
    }

    /* Request Buffer allokieren */
    proxy->request_buffer = WebserverMalloc(PROXY_BUFFER_SIZE);
    if (proxy->request_buffer == NULL) {
        free(proxy->backend_path);
        WebserverFree(proxy);
        return NULL;
    }
    proxy->request_buffer_size = PROXY_BUFFER_SIZE;
    proxy->request_buffer_pos = 0;

    /* Response Buffer allokieren */
    proxy->response_buffer = WebserverMalloc(PROXY_BUFFER_SIZE);
    if (proxy->response_buffer == NULL) {
        WebserverFree(proxy->request_buffer);
        free(proxy->backend_path);
        WebserverFree(proxy);
        return NULL;
    }
    proxy->response_buffer_size = PROXY_BUFFER_SIZE;
    proxy->response_buffer_pos = 0;
    
    return proxy;
}

/*
 * Main entry point - called from header_parser.c
 */
int checkReverseProxy(socket_info* s, char* buffer, int length) {
    char method[16];
    char url[512];
    reverse_proxy_registration_t* reg;
    reverse_proxy_connection_t* proxy;
    
    /* Wurde bereits geprüft? */
    if (s->reverse_proxy_checked == 1) {
        return 1;  /* Kein Proxy */
    }
    
    /* Request Line parsen */
    if (proxy_parse_request_line(buffer, length, method, sizeof(method), 
                                  url, sizeof(url)) < 0) {
        return 1;  /* Parse Error, normales Handling */
    }
    
#if _WEBSERVER_PROXY_DEBUG_ >= 7
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "checkReverseProxy: %s %s", method, url);
#endif

    /* Passende Registration finden */
    reg = proxy_find_registration(method, url);
    if (reg == NULL) {
        s->reverse_proxy_checked = 1;
        return 1;  /* Keine Registration gefunden */
    }

#if _WEBSERVER_PROXY_DEBUG_ >= 3
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Found proxy: %s -> %s", url, reg->backend_path);
#endif

    /* Proxy Connection erstellen */
    proxy = proxy_create_connection(s, reg);
    if (proxy == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to create proxy connection");
        s->reverse_proxy_error = 1;
        return 1;
    }

    /* URL für Logging speichern */
    snprintf(proxy->request_url, sizeof(proxy->request_url), "%s", url);

    /* Bisherige Daten in Request Buffer kopieren */
    if (length > 0 && (uint32_t)length < proxy->request_buffer_size) {
        memcpy(proxy->request_buffer, buffer, length);
        proxy->request_buffer_pos = length;
    }

    /* Client Socket für Proxy konfigurieren */
    s->extern_handle = reverse_proxy_client_handler;
    s->extern_handle_data_ptr = proxy;
    s->reverse_proxy_checked = 1;

    /* Sofort den vorhandenen Request verarbeiten
     * proxy_process_state gibt -1 zurück wenn proxy/s nicht mehr safe zugreifbar sind
     * (ERROR/DONE: closeSocket gesetzt, Cleanup läuft deferred über den Event-Loop) */
    if (proxy_process_state(proxy) < 0) {
        return 0;
    }

    /* Falls noch nicht fertig, Event für weiteres Lesen registrieren */
    if (proxy->state == PROXY_STATE_RECV_CLIENT_HEADER ||
        proxy->state == PROXY_STATE_RECV_CLIENT_BODY) {
        addEventSocketReadPersist(s);
    }

    return 0;  /* Proxy übernimmt */
}

/*
 * Client Event Handler
 */
void reverse_proxy_client_handler(int fd, void* ptr) {
    reverse_proxy_connection_t* proxy = (reverse_proxy_connection_t*)ptr;
    int bytes_read;
    
    (void)fd;
    
    if (proxy == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "proxy is NULL in client handler");
        return;
    }

    switch (proxy->state) {
        case PROXY_STATE_RECV_CLIENT_HEADER:
        case PROXY_STATE_RECV_CLIENT_BODY:
            /* Weitere Daten vom Client lesen (mit SSL-Unterstützung für Client-Socket) */
            do {
                if (proxy->request_buffer_pos >= proxy->request_buffer_size) {
                    break;
                }
                bytes_read = WebserverRecv(proxy->client_sock,
                                  &proxy->request_buffer[proxy->request_buffer_pos],
                                  proxy->request_buffer_size - proxy->request_buffer_pos,
                                  0);

                if (bytes_read <= 0) {
                    /* WebserverRecv liefert feste Rueckgabecodes, kein errno:
                     * CLIENT_NO_MORE_DATA (0)  = wuerde blockieren, EAGAIN wird
                     *                            bereits in PlatformRecvSocketNonBlocking
                     *                            bzw. im SSL-Wrapper ausgewertet
                     * CLIENT_DISCONNECTED (-1) = Gegenstelle hat geschlossen
                     * < -1                     = echter Fehler
                     * Gilt fuer SSL und Plain gleichermassen. Wichtig fuer grosse
                     * Uploads, die ueber mehrere Read-Events kommen. */
                    if (bytes_read == CLIENT_NO_MORE_DATA) {
                        break;  /* auf naechstes Read-Event warten */
                    }
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }

                /* Defensive check: recv should never return more than requested */
                /* Also prevents overflow: if bytes_read fits in remaining space, addition is safe */
                if (proxy->request_buffer_pos > proxy->request_buffer_size ||
                    (uint32_t)bytes_read > proxy->request_buffer_size - proxy->request_buffer_pos) {
                    LOG(PROXY_LOG, ERROR_LEVEL, 0, "recv returned %d bytes, but only %u space available",
                        bytes_read, proxy->request_buffer_size - proxy->request_buffer_pos);
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }

                proxy->request_buffer_pos += (uint32_t)bytes_read;
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Received %d bytes from client, total=%d",
                          bytes_read, proxy->request_buffer_pos);
#endif
            } while (WebserverSSLPending(proxy->client_sock));
            break;

        case PROXY_STATE_STREAMING_WAIT_WRITE:
            /* Client-Socket ist wieder beschreibbar - Write-Event entfernen und Streaming fortsetzen */
#if _WEBSERVER_PROXY_DEBUG_ >= 4
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "STREAMING_WAIT_WRITE - client writable again");
#endif
            /* Write-Event ablegen und zurueck auf "nur Abbau beobachten" - die
             * add-Funktionen setzen den Event-Satz absolut, das ersetzt EPOLLOUT
             * direkt. */
            addEventSocketPeerClosePersist(proxy->client_sock);
            /* Backend-Read wieder scharf schalten, in WAIT_WRITE wurde es pausiert.
             * Muss vor proxy_process_state passieren, damit zwischenzeitlich
             * eingetroffene Backend-Daten ein Event ausloesen. */
            addEventSocketReadPersist(proxy->backend_sock);
            proxy->state = PROXY_STATE_STREAMING;
            proxy_process_state(proxy);
            return;

        case PROXY_STATE_RECV_BACKEND_HEADER:
        case PROXY_STATE_STREAMING:
            /* In diesen Zustaenden horcht der Client-Socket nur auf einen
             * Verbindungsabbau (addEventSocketPeerClosePersist in
             * SEND_TO_BACKEND), nicht auf Daten. Ein Event kann hier also nur
             * bedeuten: der Client ist weg. Sofort abbauen - epoll meldet das
             * level-triggered dauerhaft, ein Ignorieren waere eine Endlosschleife.
             *
             * Bewusste Einschraenkung: EPOLLRDHUP feuert auch bei einem
             * Half-Close, also wenn ein Client nach dem Request nur seine
             * Schreibrichtung schliesst (shutdown(SHUT_WR)) und die Antwort noch
             * lesen will. Das ist laut HTTP erlaubt, aber unueblich - Browser und
             * curl machen es nicht. Ein solcher Client bekommt hier keine Antwort
             * mehr. Falls das je auftritt (Symptom: abgebrochene Responses bei
             * einem exotischen Client, waehrend Browser normal funktionieren),
             * muesste an dieser Stelle zusaetzlich geprueft werden, ob die
             * Response schon vollstaendig raus ist, und nur dann abgebaut werden.
             * Der Fall wurde gegen den Dauer-Leak abgewogen: ein unbeobachteter
             * Client-Socket kostet zwei FDs pro Verbindung, dauerhaft. */
#if _WEBSERVER_PROXY_DEBUG_ >= 3
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "client closed while waiting for backend");
#endif
            proxy->state = PROXY_STATE_DONE;
            proxy_process_state(proxy);
            return;

        case PROXY_STATE_WEBSOCKET_TUNNEL:
            /* WebSocket Tunnel: Daten vom Client zum Backend */
            do {
                unsigned char tunnel_buf[4096];
                /* Client-Socket: WebserverRecv für SSL-Unterstützung */
                bytes_read = WebserverRecv(proxy->client_sock, tunnel_buf, sizeof(tunnel_buf), 0);

                if (bytes_read <= 0) {
                    /* Rueckgabecodes von WebserverRecv auswerten, kein errno.
                     * CLIENT_NO_MORE_DATA (0) heisst nur "gerade nichts da" -
                     * bei SSL kommt das z.B. bei einem unvollstaendigen TLS-Record,
                     * der Tunnel darf deswegen nicht abgebrochen werden. */
                    if (bytes_read == CLIENT_NO_MORE_DATA) {
                        break;  /* auf naechstes Read-Event warten */
                    }
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "WebSocket tunnel: client closed");
#endif
                    proxy->state = PROXY_STATE_DONE;
                    proxy_process_state(proxy);
                    return;
                }

                /* Direkt ans Backend weiterleiten (UDS - kein SSL nötig) */
                if (proxy->backend_fd >= 0) {
                    int sent = send(proxy->backend_fd, tunnel_buf, bytes_read, 0);
                    if (sent < 0) {
                        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "WebSocket tunnel: backend send error");
                        proxy->state = PROXY_STATE_ERROR;
                        proxy_process_state(proxy);
                        return;
                    }
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "WebSocket tunnel: forwarded %d bytes to backend", sent);
#endif
                }
            } while (WebserverSSLPending(proxy->client_sock));
            return;

        default:
            break;
    }
    
    proxy_process_state(proxy);
}

/*
 * Backend Event Handler
 */
void reverse_proxy_backend_handler(int fd, void* ptr) {
    reverse_proxy_connection_t* proxy = (reverse_proxy_connection_t*)ptr;
    int bytes_read;
    
    (void)fd;
    
    if (proxy == NULL) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "proxy is NULL in backend handler");
        return;
    }

#if _WEBSERVER_PROXY_DEBUG_ >= 5
    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "backend_handler called, state=%d", proxy->state);
#endif

    switch (proxy->state) {
        case PROXY_STATE_SEND_TO_BACKEND:
            /* Backend wieder schreibbar: restlichen Request-Body senden
             * (partieller Send bei grossen Uploads, siehe proxy_process_state). */
            proxy_process_state(proxy);
            break;

        case PROXY_STATE_RECV_BACKEND_HEADER:
            /* Daten vom Backend lesen - warten auf kompletten Header */
            if (proxy->response_buffer_pos < proxy->response_buffer_size) {
                bytes_read = recv(proxy->backend_fd,
                                  &proxy->response_buffer[proxy->response_buffer_pos],
                                  proxy->response_buffer_size - proxy->response_buffer_pos,
                                  0);
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "backend recv (header): %d bytes", bytes_read);
#endif
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Backend closed during header");
#endif
                        proxy->state = PROXY_STATE_ERROR;
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "Backend recv error: %s", strerror(errno));
                        proxy->state = PROXY_STATE_ERROR;
                    } else {
                        return;  /* EAGAIN - später nochmal */
                    }
                } else {
#if _WEBSERVER_PROXY_DEBUG_ >= 2
                    /* Timing: Erste Response-Bytes */
                    if (proxy->response_buffer_pos == 0) {
                        clock_gettime(CLOCK_MONOTONIC, &proxy->time_first_response);
                    }
#endif
                    /* Defensive check: recv should never return more than requested */
                    /* Also prevents overflow: if bytes_read fits in remaining space, addition is safe */
                    if (proxy->response_buffer_pos > proxy->response_buffer_size ||
                        (uint32_t)bytes_read > proxy->response_buffer_size - proxy->response_buffer_pos) {
                        LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "recv returned %d bytes, but only %u space available",
                            bytes_read, proxy->response_buffer_size - proxy->response_buffer_pos);
                        proxy->state = PROXY_STATE_ERROR;
                        proxy_process_state(proxy);
                        return;
                    }
                    proxy->response_buffer_pos += (uint32_t)bytes_read;
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "Received %d bytes from backend, total=%d",
                              bytes_read, proxy->response_buffer_pos);
#endif
                }
            }
            break;

        case PROXY_STATE_STREAMING:
            /* Streaming: Daten empfangen und sofort weiterleiten */
            {
                /* Noch ungesendete Daten im Buffer? Erst senden! */
                if (proxy->response_buffer_pos > proxy->send_pos) {
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "STREAMING: buffer not empty, sending first");
#endif
                    proxy_process_state(proxy);
                    return;
                }

                /* Bei CONTENT_LENGTH prüfen ob schon alles empfangen */
                if (proxy->response_body_mode == BODY_MODE_CONTENT_LENGTH) {
                    if (proxy->response_body_sent >= proxy->response_content_length) {
#if _WEBSERVER_PROXY_DEBUG_ >= 4
                        LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "STREAMING: all data sent, done");
#endif
                        proxy->state = PROXY_STATE_DONE;
                        proxy_process_state(proxy);
                        return;
                    }
                }

                /* Buffer ist leer, neu vom Backend lesen */
                proxy->response_buffer_pos = 0;
                proxy->send_pos = 0;

                bytes_read = recv(proxy->backend_fd,
                                  proxy->response_buffer,
                                  proxy->response_buffer_size, 0);
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "backend recv (streaming): %d bytes", bytes_read);
#endif
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
#if _WEBSERVER_PROXY_DEBUG_ >= 4
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Backend closed during streaming");
#endif
                        /* Prüfen ob alles empfangen wurde */
                        if (proxy->response_body_mode == BODY_MODE_CLOSE ||
                            proxy->response_body_sent >= proxy->response_content_length) {
                            proxy->state = PROXY_STATE_DONE;
                        } else {
                            LOG(PROXY_LOG, WARNING_LEVEL, proxy->backend_fd, "Backend closed prematurely, sent=%lu, expected=%lu",
                                      (unsigned long)proxy->response_body_sent,
                                      (unsigned long)proxy->response_content_length);
                            proxy->state = PROXY_STATE_ERROR;
                        }
                        proxy_process_state(proxy);
                        return;
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "Backend recv error: %s", strerror(errno));
                        proxy->state = PROXY_STATE_ERROR;
                        proxy_process_state(proxy);
                        return;
                    }
                    return;  /* EAGAIN - später nochmal */
                }

                proxy->response_buffer_pos = bytes_read;
                /* Sofort weiterleiten */
                proxy_process_state(proxy);
                return;
            }
            break;
            
        case PROXY_STATE_WEBSOCKET_TUNNEL:
            /* WebSocket Tunnel: Daten vom Backend zum Client */
            {
                unsigned char tunnel_buf[4096];
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: backend handler called");
#endif
                /* Backend: direktes recv (UDS - kein SSL) */
                bytes_read = recv(proxy->backend_fd, tunnel_buf, sizeof(tunnel_buf), 0);

                if (bytes_read <= 0) {
                    if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: backend closed");
#endif
                        proxy->state = PROXY_STATE_DONE;
                        proxy_process_state(proxy);
                        return;
                    }
                    return;
                }

                /* Client-Socket: WebserverSend für SSL-Unterstützung */
                size_t sent = 0;
                SOCKET_SEND_STATUS status = WebserverSend(proxy->client_sock,
                                                          tunnel_buf, bytes_read, 0, &sent);
                if (status == SOCKET_SEND_CLIENT_DISCONNECTED) {
                    LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: client disconnected");
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "WebSocket tunnel: forwarded %zu bytes to client", sent);
#endif
            }
            return;

        default:
            break;
    }

    proxy_process_state(proxy);
}

/*
 * State Machine Processing
 */
static int proxy_process_state(reverse_proxy_connection_t* proxy) {
    int ret;

    switch (proxy->state) {
        case PROXY_STATE_RECV_CLIENT_HEADER:
            /* Prüfen ob Header komplett */
            ret = proxy_find_header_end(proxy->request_buffer, proxy->request_buffer_pos);
            if (ret < 0) {
                /* Header noch nicht komplett, weiter lesen */
                return 0;
            }
            
            proxy->request_header_end = ret;
#if _WEBSERVER_PROXY_DEBUG_ >= 4
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Client header complete at pos %d", ret);
#endif
            /* Content-Length oder Chunked? */
            {
                int64_t parsed_content_length = proxy_parse_content_length(
                    proxy->request_buffer, proxy->request_header_end);

                if (parsed_content_length > 0) {
                    proxy->request_body_mode = BODY_MODE_CONTENT_LENGTH;
                    proxy->request_content_length = (uint64_t)parsed_content_length;
                    proxy->request_body_received = proxy->request_buffer_pos - proxy->request_header_end;
#if _WEBSERVER_PROXY_DEBUG_ >= 4
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Request Content-Length: %lu, received: %lu",
                              (unsigned long)proxy->request_content_length,
                              (unsigned long)proxy->request_body_received);
#endif
                    /* Request-Buffer auf die Upload-Groesse vergroessern (Default
                     * 64 KB), gedeckelt auf 512 MB. Der Request wird komplett
                     * gepuffert, bevor er ans Backend geht; ohne das Wachsen
                     * werden Bodies > 64 KB abgeschnitten/blockiert. */
                    {
                        uint64_t needed = (uint64_t)proxy->request_header_end
                                        + proxy->request_content_length + 1;
                        const uint64_t cap = (uint64_t)512 * 1024 * 1024;
                        if (needed > cap) needed = cap;
                        if (needed > proxy->request_buffer_size) {
                            unsigned char* nb = WebserverRealloc(proxy->request_buffer,
                                                                 (unsigned long)needed);
                            if (nb == NULL) {
                                LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s",
                                    "Failed to grow request buffer for upload");
                                proxy->state = PROXY_STATE_ERROR;
                                return proxy_process_state(proxy);
                            }
                            proxy->request_buffer = nb;
                            proxy->request_buffer_size = (uint32_t)needed;
                        }
                    }
                    if (proxy->request_body_received < proxy->request_content_length) {
                        proxy->state = PROXY_STATE_RECV_CLIENT_BODY;
                        return 0;
                    }
                } else if (proxy_check_chunked_encoding(proxy->request_buffer,
                                                         proxy->request_header_end)) {
                    proxy->request_body_mode = BODY_MODE_CHUNKED;
                    proxy->request_content_length = 0;
                    /* TODO: Chunked Request Body handling */
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "Request uses chunked encoding (not fully implemented)");
#endif
                } else {
                    /* Kein Body oder Content-Length: 0 */
                    proxy->request_body_mode = BODY_MODE_NONE;
                    proxy->request_content_length = 0;
                }
            }

            proxy->state = PROXY_STATE_CONNECT_BACKEND;
            return proxy_process_state(proxy);

        case PROXY_STATE_RECV_CLIENT_BODY:
            /* Prüfen ob Body komplett */
            proxy->request_body_received = proxy->request_buffer_pos - proxy->request_header_end;
#if _WEBSERVER_PROXY_DEBUG_ >= 5
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Body received: %lu of %lu",
                      (unsigned long)proxy->request_body_received,
                      (unsigned long)proxy->request_content_length);
#endif
            if (proxy->request_body_received >= proxy->request_content_length) {
                proxy->state = PROXY_STATE_CONNECT_BACKEND;
                return proxy_process_state(proxy);
            }
            break;

        case PROXY_STATE_CONNECT_BACKEND:
#if _WEBSERVER_PROXY_DEBUG_ >= 3
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Connecting to backend: %s", proxy->backend_path);
#endif
            ret = proxy_connect_backend(proxy);
            if (ret < 0) {
                proxy->state = PROXY_STATE_ERROR;
                return proxy_process_state(proxy);
            }
#if _WEBSERVER_PROXY_DEBUG_ >= 2
            /* Timing: Backend verbunden */
            clock_gettime(CLOCK_MONOTONIC, &proxy->time_backend_connected);
#endif
#if _WEBSERVER_PROXY_DEBUG_ >= 3
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Backend connected, fd=%d", proxy->backend_fd);
#endif
            /* Inject X-Forwarded-* headers before sending to backend */
            if (proxy_inject_forwarded_headers(proxy) < 0) {
                LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to inject forwarded headers");
                proxy->state = PROXY_STATE_ERROR;
                return proxy_process_state(proxy);
            }

#if _WEBSERVER_PROXY_DEBUG_ >= 6
            proxy_dump_header("REQUEST TO BACKEND", proxy->request_buffer, proxy->request_header_end);
#endif

            proxy->state = PROXY_STATE_SEND_TO_BACKEND;
            return proxy_process_state(proxy);
            
        case PROXY_STATE_SEND_TO_BACKEND:
            /* Request an Backend senden */
            {
                int to_send;
                int sent;

                /* connect() may have returned EINPROGRESS, in which case the
                 * connection can still have failed since. Collect the pending
                 * error before the first send, otherwise a failed connect only
                 * shows up as a misleading send error. Checked once per
                 * connection, on re-entry after a partial send send_pos is
                 * already non-zero. */
                if (proxy->send_pos == 0) {
                    int so_error = 0;
                    socklen_t so_error_len = sizeof(so_error);

                    if (getsockopt(proxy->backend_fd, SOL_SOCKET, SO_ERROR,
                                   &so_error, &so_error_len) == 0 && so_error != 0) {
                        LOG(PROXY_LOG, ERROR_LEVEL, 0, "Backend connect to %s failed: %s",
                            proxy->backend_path, strerror(so_error));
                        proxy->state = PROXY_STATE_ERROR;
                        return proxy_process_state(proxy);
                    }
                }

                to_send = proxy->request_buffer_pos - proxy->send_pos;
                sent = send(proxy->backend_fd,
                            &proxy->request_buffer[proxy->send_pos],
                            to_send, 0);

                /* Backend-Handler registrieren, damit Write-Events (partieller
                 * Send / EAGAIN bei grossen Uploads) wieder hier landen und der
                 * Rest gesendet wird (siehe backend_handler: SEND_TO_BACKEND). */
                proxy->backend_sock->extern_handle = reverse_proxy_backend_handler;
                proxy->backend_sock->extern_handle_data_ptr = proxy;

                if (sent < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        addEventSocketWritePersist(proxy->backend_sock);
                        return 0;  /* auf Schreibbarkeit warten */
                    }
                    LOG(PROXY_LOG, ERROR_LEVEL, 0, "Backend send error: %s", strerror(errno));
                    proxy->state = PROXY_STATE_ERROR;
                    return proxy_process_state(proxy);
                }

                proxy->send_pos += sent;
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Sent %d bytes to backend, total=%d/%d",
                          sent, proxy->send_pos, proxy->request_buffer_pos);
#endif

                if (proxy->send_pos < proxy->request_buffer_pos) {
                    /* Partieller Send (grosser Upload > Socket-Sendepuffer): Rest
                     * senden, sobald das Backend wieder schreibbar ist. */
                    addEventSocketWritePersist(proxy->backend_sock);
                    return 0;
                }

                /* Alles gesendet */
#if _WEBSERVER_PROXY_DEBUG_ >= 2
                /* Timing: Request vollständig gesendet */
                clock_gettime(CLOCK_MONOTONIC, &proxy->time_request_sent);
#endif
                delEventSocketWritePersist(proxy->backend_sock);
                proxy->send_pos = 0;
                proxy->state = PROXY_STATE_RECV_BACKEND_HEADER;
                addEventSocketReadPersist(proxy->backend_sock);

                /* Client-Socket nicht mehr auf Daten beobachten, aber weiter auf
                 * einen Verbindungsabbau horchen. Sonst ist der Client waehrend
                 * der gesamten Backend-Phase unbeobachtet und ein FIN des
                 * Browsers faellt nie auf - der Socket bleibt in CLOSE_WAIT
                 * haengen und mit ihm die Backend-Verbindung. */
                addEventSocketPeerClosePersist(proxy->client_sock);
            }
            break;
            
        case PROXY_STATE_RECV_BACKEND_HEADER:
            /* Prüfen ob Response Header komplett */
            ret = proxy_find_header_end(proxy->response_buffer, proxy->response_buffer_pos);
            if (ret < 0) {
                return 0;  /* Header noch nicht komplett */
            }

            proxy->response_header_end = ret;
            proxy->response_header_complete = 1;
#if _WEBSERVER_PROXY_DEBUG_ >= 4
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "Backend response header complete at pos %d", ret);
#endif
            /* Response Header parsen */
            proxy_parse_response_header(proxy);

#if _WEBSERVER_PROXY_DEBUG_ >= 6
            proxy_dump_header("RESPONSE FROM BACKEND", proxy->response_buffer, proxy->response_header_end);
#endif

            /* WebSocket Upgrade? -> Tunnel Mode */
            if (proxy->response_is_websocket_upgrade) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "WebSocket upgrade detected");
#endif
                /* Header senden, dann Tunnel - KEIN Connection: close bei WebSockets! */
                proxy->state = PROXY_STATE_STREAMING;
                return proxy_process_state(proxy);
            }

            /* Connection: close injizieren für sauberes Verbindungsende (nicht bei WebSockets).
             * Fails only when the buffer cannot be grown. Sending the response
             * without the header would tell the client to keep the connection
             * alive while the proxy closes it right after, so treat it as an
             * error - same as the forwarded headers on the request side. */
            if (proxy_inject_connection_close(proxy) < 0) {
                LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to inject Connection: close");
                proxy->state = PROXY_STATE_ERROR;
                return proxy_process_state(proxy);
            }

            /* Body Mode bestimmen */
            {
                int64_t parsed_content_length = proxy_parse_content_length(
                    proxy->response_buffer, proxy->response_header_end);

                /* Body Mode bestimmen
                 * HTTP 204 (No Content) und 304 (Not Modified) haben nie einen Body
                 * Content-Length: 0 bedeutet auch kein Body
                 * parsed_content_length >= 0 bedeutet Content-Length Header vorhanden
                 * PROXY_CL_MISSING means no Content-Length header,
                 * PROXY_CL_INVALID an unusable value - both fall through to
                 * chunked or read-until-close below */
                if (proxy->response_status_code == 204 || proxy->response_status_code == 304) {
                    proxy->response_body_mode = BODY_MODE_CONTENT_LENGTH;
                    proxy->response_content_length = 0;
                } else if (parsed_content_length >= 0) {
                    /* Content-Length Header vorhanden (auch wenn 0) */
                    proxy->response_body_mode = BODY_MODE_CONTENT_LENGTH;
                    proxy->response_content_length = (uint64_t)parsed_content_length;
                } else if (proxy_check_chunked_encoding(proxy->response_buffer,
                                                         proxy->response_header_end)) {
                    proxy->response_body_mode = BODY_MODE_CHUNKED;
                    proxy->response_content_length = 0;
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Response uses chunked encoding");
#endif
                } else {
                    /* Kein Content-Length, kein Chunked -> read until close */
                    proxy->response_body_mode = BODY_MODE_CLOSE;
                    proxy->response_content_length = 0;
                }
            }

            proxy->response_body_received = proxy->response_buffer_pos - proxy->response_header_end;
#if _WEBSERVER_PROXY_DEBUG_ >= 4
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "Response Content-Length: %lu, body in buffer: %lu",
                      (unsigned long)proxy->response_content_length,
                      (unsigned long)proxy->response_body_received);
#endif
            /* Wenn bei CONTENT_LENGTH alle Daten bereits empfangen, Backend-Events deregistrieren */
            if (proxy->response_body_mode == BODY_MODE_CONTENT_LENGTH &&
                proxy->response_body_received >= proxy->response_content_length) {
#if _WEBSERVER_PROXY_DEBUG_ >= 4
                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "All body data already received");
#endif
                delEventSocketReadPersist(proxy->backend_sock);
            }

            /* Streaming starten - Header und vorhandene Body-Daten sofort senden */
            proxy->state = PROXY_STATE_STREAMING;
            proxy->send_pos = 0;
            proxy->response_header_sent = 0;
            proxy->response_body_sent = 0;
            return proxy_process_state(proxy);

        case PROXY_STATE_STREAMING:
            /* Streaming: Daten sofort an Client weiterleiten */
            {
                size_t to_send;
                size_t sent = 0;
                SOCKET_SEND_STATUS status;

                /* Noch Header zu senden? */
                if (!proxy->response_header_sent) {
                    while (proxy->send_pos < proxy->response_header_end) {
                        to_send = proxy->response_header_end - proxy->send_pos;
                        status = WebserverSend(proxy->client_sock,
                                   &proxy->response_buffer[proxy->send_pos],
                                   to_send, 0, &sent);
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "STREAMING header: sent %zu/%zu bytes, status=%d", sent, to_send, status);
#endif
                        if (status == SOCKET_SEND_SEND_BUFFER_FULL) {
                            /* Account for what was already taken: mbedTLS reports
                             * accepted bytes together with BUFFER_FULL, OpenSSL
                             * reports 0. Skipping this would resend those bytes
                             * on the next attempt. */
                            proxy->send_pos += sent;
                            proxy->state = PROXY_STATE_STREAMING_WAIT_WRITE;
                            /* Backend-Read pausieren: in diesem Zustand liest niemand
                             * vom Backend, epoll ist level-triggered und wuerde den
                             * Backend-FD sonst endlos als lesbar melden (100 % CPU). */
                            delEventSocketAll(proxy->backend_sock);
                            addEventSocketWritePersist(proxy->client_sock);
                            return 0;
                        }
                        if (status == SOCKET_SEND_CLIENT_DISCONNECTED) {
                            proxy->state = PROXY_STATE_ERROR;
                            return proxy_process_state(proxy);
                        }

                        proxy->send_pos += sent;
                        if (sent == 0) {
                            /* Keine Daten gesendet, später nochmal versuchen */
                            proxy->state = PROXY_STATE_STREAMING_WAIT_WRITE;
                            delEventSocketAll(proxy->backend_sock);  /* Backend-Read pausieren */
                            addEventSocketWritePersist(proxy->client_sock);
                            return 0;
                        }
                    }
                    proxy->response_header_sent = 1;
#if _WEBSERVER_PROXY_DEBUG_ >= 4
                    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "STREAMING: header sent, starting body");
#endif
                    /* Bei WebSocket-Upgrade sofort in Tunnel-Mode wechseln */
                    if (proxy->response_is_websocket_upgrade) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Entering WebSocket tunnel mode");
#endif
                        proxy->state = PROXY_STATE_WEBSOCKET_TUNNEL;
                        /* Event-Zustand sauber zuruecksetzen, bevor fuer den Tunnel-Modus neu registriert wird */
                        delEventSocketAll(proxy->client_sock);
                        delEventSocketAll(proxy->backend_sock);
                        addEventSocketReadPersist(proxy->client_sock);
                        addEventSocketReadPersist(proxy->backend_sock);
                        return 0;
                    }
                }

                /* Body-Daten aus Buffer senden - in Schleife bis Buffer leer oder BUFFER_FULL */
                while (proxy->send_pos < proxy->response_buffer_pos) {
                    to_send = proxy->response_buffer_pos - proxy->send_pos;
                    status = WebserverSend(proxy->client_sock,
                               &proxy->response_buffer[proxy->send_pos],
                               to_send, 0, &sent);

                    if (status == SOCKET_SEND_SEND_BUFFER_FULL) {
                        /* Same as above, and response_body_sent has to follow too:
                         * it is what the Content-Length completion check compares
                         * against, so losing bytes here would keep the response
                         * from ever being recognised as finished. */
                        proxy->send_pos += sent;
                        proxy->response_body_sent += sent;
                        proxy->state = PROXY_STATE_STREAMING_WAIT_WRITE;
                        delEventSocketAll(proxy->backend_sock);  /* Backend-Read pausieren */
                        addEventSocketWritePersist(proxy->client_sock);
                        return 0;
                    }
                    if (status == SOCKET_SEND_CLIENT_DISCONNECTED) {
                        proxy->state = PROXY_STATE_ERROR;
                        return proxy_process_state(proxy);
                    }

                    if (sent == 0) {
                        /* Keine Daten gesendet, später nochmal versuchen */
                        proxy->state = PROXY_STATE_STREAMING_WAIT_WRITE;
                        delEventSocketAll(proxy->backend_sock);  /* Backend-Read pausieren */
                        addEventSocketWritePersist(proxy->client_sock);
                        return 0;
                    }

                    proxy->send_pos += sent;
                    proxy->response_body_sent += sent;
#if _WEBSERVER_PROXY_DEBUG_ >= 5
                    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "STREAMING body: sent %zu bytes, total: %lu",
                              sent, (unsigned long)proxy->response_body_sent);
#endif
                }

                /* Buffer ist leer - zurücksetzen für nächsten Backend-recv */
                proxy->response_buffer_pos = 0;
                proxy->send_pos = 0;

                /* Prüfen ob Response komplett */
                if (proxy->response_body_mode == BODY_MODE_CONTENT_LENGTH) {
                    if (proxy->response_body_sent >= proxy->response_content_length) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "STREAMING: all body sent (%lu bytes)",
                                  (unsigned long)proxy->response_body_sent);
#endif
                        if (proxy->response_is_websocket_upgrade) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Entering WebSocket tunnel mode");
#endif
                            proxy->state = PROXY_STATE_WEBSOCKET_TUNNEL;
                            addEventSocketReadPersist(proxy->client_sock);
                            addEventSocketReadPersist(proxy->backend_sock);
                            return 0;
                        }

                        proxy->state = PROXY_STATE_DONE;
                        return proxy_process_state(proxy);
                    }
                }
                /* Für BODY_MODE_CLOSE warten wir bis Backend schließt */
            }
            break;

        case PROXY_STATE_STREAMING_WAIT_WRITE:
            /* Warte auf Client Write Event - wird in client_handler behandelt */
            break;

        case PROXY_STATE_ERROR:
#if _WEBSERVER_PROXY_DEBUG_ >= 3
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "Proxy error state, cleaning up");
#endif
            {
                /* 502 Error senden wenn noch keine Response gesendet wurde */
                if (proxy->response_header_sent == 0) {
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Sending 502 Bad Gateway for %s", proxy->request_url);
                    proxy_send_error_response(proxy);
                }

                proxy->client_sock->reverse_proxy_error = 1;
                proxy->client_sock->closeSocket = 1;
                proxy->client_sock->extern_handle = NULL;
                /* Backend Events sofort deregistrieren */
                if (proxy->backend_sock != NULL) {
                    delEventSocketAll(proxy->backend_sock);
                }
                /* Read Event registrieren damit der Event-Handler closeSocket prüft */
                addEventSocketRead(proxy->client_sock);
                /* Restlicher Proxy Cleanup über reverse_proxy_cleanup() in WebserverFreeSocketInfo */
            }
            return -1;

        case PROXY_STATE_DONE:
#if _WEBSERVER_PROXY_DEBUG_ >= 2
            /* Timing: Fertig */
            clock_gettime(CLOCK_MONOTONIC, &proxy->time_done);
            {
                double t_connect = proxy_time_diff_ms(&proxy->time_start, &proxy->time_backend_connected);
                double t_send = proxy_time_diff_ms(&proxy->time_backend_connected, &proxy->time_request_sent);
                double t_wait = proxy_time_diff_ms(&proxy->time_request_sent, &proxy->time_first_response);
                double t_transfer = proxy_time_diff_ms(&proxy->time_first_response, &proxy->time_done);
                double t_total = proxy_time_diff_ms(&proxy->time_start, &proxy->time_done);

                LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd,
                    "TIMING %s: connect=%.1fms send=%.1fms wait=%.1fms transfer=%.1fms TOTAL=%.1fms",
                    proxy->request_url, t_connect, t_send, t_wait, t_transfer, t_total);
            }
#endif
#if _WEBSERVER_PROXY_DEBUG_ >= 2
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Proxy done, cleaning up");
#endif
            proxy->client_sock->closeSocket = 1;
            proxy->client_sock->extern_handle = NULL;
            /* Backend Events sofort deregistrieren */
            if (proxy->backend_sock != NULL) {
                delEventSocketAll(proxy->backend_sock);
            }
            /* Read Event registrieren damit der Event-Handler closeSocket prüft */
            addEventSocketRead(proxy->client_sock);
            /* Restlicher Proxy Cleanup über reverse_proxy_cleanup() in WebserverFreeSocketInfo */
            return -1;

        default:
            LOG(PROXY_LOG, ERROR_LEVEL, 0, "Unknown state: %d", proxy->state);
            break;
    }
    return 0;
}

/*
 * Connect to backend UDS
 */
static int proxy_connect_backend(reverse_proxy_connection_t* proxy) {
    struct sockaddr_un addr;
    int sock_fd;
    int flags;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "Failed to create UDS socket: %s", strerror(errno));
        return -1;
    }

    /* Non-blocking setzen */
    flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "Failed to set non-blocking on UDS socket: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    /* snprintf instead of strncpy: the return value reveals a truncation.
     * A silently truncated path would connect to a different socket instead
     * of reporting the error. Null termination is guaranteed by snprintf
     * (and additionally by the memset above). */
    if (snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", proxy->backend_path)
            >= (int)sizeof(addr.sun_path)) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "Backend path too long (max %d): %s",
            (int)sizeof(addr.sun_path) - 1, proxy->backend_path);
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno == EAGAIN) {
            /* A non blocking unix socket reports a full accept queue as EAGAIN,
             * not EINPROGRESS. The backend is running but not keeping up, so
             * this connection cannot be established right now. */
            LOG(PROXY_LOG, ERROR_LEVEL, 0, "Backend %s is not accepting connections (queue full)",
                proxy->backend_path);
            close(sock_fd);
            return -1;
        }
        if (errno != EINPROGRESS) {
            LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Failed to connect to backend %s: %s",
                      proxy->backend_path, strerror(errno));
            close(sock_fd);
            return -1;
        }
        /* EINPROGRESS: the connect is still running. Whether it succeeded is
         * checked via SO_ERROR before the first send in SEND_TO_BACKEND. */
    }

    proxy->backend_fd = sock_fd;

    /* Backend socket_info erstellen */
    proxy->backend_sock = WebserverMallocSocketInfo();
    if (proxy->backend_sock == NULL) {
        close(sock_fd);
        proxy->backend_fd = -1;
        return -1;
    }

    proxy->backend_sock->socket = sock_fd;
    proxy->backend_sock->active = 1;
    proxy->backend_sock->client = 1;

    /* Socket Container hinzufügen für Event-Handling */
    addSocketContainer(proxy->backend_sock);

#if _WEBSERVER_PROXY_DEBUG_ >= 3
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Connected to backend %s (fd=%d)", proxy->backend_path, sock_fd);
#endif

    return 0;
}

/*
 * Parse response header for status code and other info
 */
static int proxy_parse_response_header(reverse_proxy_connection_t* proxy) {
    proxy->response_status_code = proxy_get_status_code(proxy->response_buffer,
                                                       proxy->response_header_end);
    proxy->response_is_websocket_upgrade = proxy_check_websocket_upgrade(
        proxy->response_buffer, proxy->response_header_end);

#if _WEBSERVER_PROXY_DEBUG_ >= 4
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Response status: %d, websocket_upgrade: %d",
              proxy->response_status_code, proxy->response_is_websocket_upgrade);
#endif

    return 0;
}

/*
 * Find end of HTTP header (\r\n\r\n)
 */
static int proxy_find_header_end(const unsigned char* buffer, uint32_t len) {
    uint32_t i;

    if (len < 4) {
        return -1;
    }

    for (i = 0; i < len - 3; i++) {
        if (buffer[i] == '\r' && buffer[i+1] == '\n' &&
            buffer[i+2] == '\r' && buffer[i+3] == '\n') {
            return i + 4;
        }
    }

    return -1;
}

/*
 * Parse Content-Length header
 *
 * Returns the value, PROXY_CL_MISSING when there is no such header, or
 * PROXY_CL_INVALID when the value is not a number or does not fit.
 */
static int64_t proxy_parse_content_length(const unsigned char* header, uint32_t header_len) {
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;
    const char* cl_str = "Content-Length:";
    int cl_len = strlen(cl_str);
    uint64_t value;

    while (p < end - cl_len) {
        if (strncasecmp((const char*)p, cl_str, cl_len) == 0) {
            p += cl_len;
            while (p < end && (*p == ' ' || *p == '\t')) {
                p++;
            }

            /* Parse the digits by hand instead of using atoi()/strtoull():
             * the header buffer is not null terminated, so those could read
             * past its end, and atoi() has undefined behaviour on overflow.
             * A backend answering with a bogus length must not wrap around
             * into a small value here. */
            if (p >= end || *p < '0' || *p > '9') {
                LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Content-Length header without a number");
                return PROXY_CL_INVALID;
            }

            value = 0;
            while (p < end && *p >= '0' && *p <= '9') {
                uint64_t digit = (uint64_t)(*p - '0');
                if (value > ((uint64_t)INT64_MAX - digit) / 10) {
                    LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Content-Length value too large");
                    return PROXY_CL_INVALID;
                }
                value = value * 10 + digit;
                p++;
            }
            return (int64_t)value;
        }
        /* Zur nächsten Zeile */
        while (p < end && *p != '\n') {
            p++;
        }
        if (p < end) {
            p++;
        }
    }

    return PROXY_CL_MISSING;
}

/*
 * Check for Transfer-Encoding: chunked
 */
static int proxy_check_chunked_encoding(const unsigned char* header, uint32_t header_len) {
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;
    const char* te_str = "Transfer-Encoding:";
    int te_len = strlen(te_str);

    while (p < end - te_len) {
        if (strncasecmp((const char*)p, te_str, te_len) == 0) {
            p += te_len;
            while (p < end && *p != '\r' && *p != '\n') {
                if (strncasecmp((const char*)p, "chunked", 7) == 0) {
                    return 1;
                }
                p++;
            }
        }
        /* Zur nächsten Zeile */
        while (p < end && *p != '\n') {
            p++;
        }
        if (p < end) {
            p++;
        }
    }

    return 0;
}

/*
 * Check for WebSocket upgrade response (101 Switching Protocols)
 */
static int proxy_check_websocket_upgrade(const unsigned char* header, uint32_t header_len) {
    const unsigned char* p;
    const unsigned char* end;
    int status = proxy_get_status_code(header, header_len);

    if (status != 101) {
        return 0;
    }

    /* Check for Upgrade: websocket header */
    p = header;
    end = header + header_len;

    while (p < end - 8) {
        if (strncasecmp((const char*)p, "Upgrade:", 8) == 0) {
            p += 8;
            while (p < end && *p != '\r' && *p != '\n') {
                if (strncasecmp((const char*)p, "websocket", 9) == 0) {
                    return 1;
                }
                p++;
            }
        }
        while (p < end && *p != '\n') {
            p++;
        }
        if (p < end) {
            p++;
        }
    }

    return 0;
}

/*
 * Get HTTP status code from response
 *
 * Returns -1 when no status code can be read.
 */
static int proxy_get_status_code(const unsigned char* header, uint32_t header_len) {
    /* Format: HTTP/1.x SSS ... */
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;
    int status = 0;
    int digits = 0;

    /* Skip "HTTP/1.x " */
    while (p < end && *p != ' ') {
        p++;
    }
    if (p < end && *p == ' ') {
        p++;
    }

    /* Read at most three digits, bounded by the buffer. The response header
     * is not null terminated, so atoi() could read past its end - a backend
     * answering without a space in the status line was enough to trigger it. */
    while (p < end && digits < 3 && *p >= '0' && *p <= '9') {
        status = status * 10 + (*p - '0');
        p++;
        digits++;
    }

    if (digits == 0) {
        return -1;
    }

    return status;
}

/*
 * Extract Host header from HTTP request
 */
static int proxy_extract_host_header(const unsigned char* header, uint32_t header_len,
                                      char* host_buf, int host_buf_size) {
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;
    const char* host_str = "Host:";
    int host_str_len = 5;
    int pos = 0;

    while (p < end - host_str_len) {
        if (strncasecmp((const char*)p, host_str, host_str_len) == 0) {
            p += host_str_len;
            /* Skip whitespace */
            while (p < end && (*p == ' ' || *p == '\t')) {
                p++;
            }
            /* Copy until end of line */
            while (p < end && *p != '\r' && *p != '\n' && pos < host_buf_size - 1) {
                host_buf[pos++] = *p++;
            }
            host_buf[pos] = '\0';
            return pos;
        }
        /* Move to next line */
        while (p < end && *p != '\n') {
            p++;
        }
        if (p < end) {
            p++;
        }
    }

    host_buf[0] = '\0';
    return 0;
}

/*
 * Prueft ob der Request ein Protocol-Upgrade anfordert (z.B. WebSocket).
 * Bei einem Upgrade darf dem Backend kein "Connection: close" untergeschoben
 * werden, sonst kommt der Tunnel nicht zustande.
 */
static int proxy_request_is_upgrade(const unsigned char* header, uint32_t header_len) {
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;

    /* Erste Zeile ist die Request-Line, dort steht kein Header */
    while (p < end && *p != '\n') {
        p++;
    }
    if (p < end) {
        p++;
    }

    while (p + 8 <= end) {
        if (strncasecmp((const char*)p, "Upgrade:", 8) == 0) {
            return 1;
        }
        while (p < end && *p != '\n') {
            p++;
        }
        if (p >= end) {
            break;
        }
        p++;
    }

    return 0;
}

/*
 * Setzt einen bereits vorhandenen Connection-Header auf "close".
 *
 * Der Wert wird an Ort und Stelle ueberschrieben und mit Leerzeichen
 * aufgefuellt, sodass sich die Header-Laenge nicht aendert und der Buffer
 * nicht verschoben werden muss. Abschliessende Leerzeichen im Header-Wert
 * werden von HTTP-Parsern ignoriert (RFC 7230, optional whitespace).
 *
 * Rueckgabe: 1 wenn ein Header gesetzt wurde, 0 wenn keiner vorhanden ist
 * (dann muss der Aufrufer einen einfuegen).
 */
static int proxy_force_connection_close(unsigned char* header, uint32_t header_len) {
    static const char name[] = "Connection:";
    static const char value[] = " close";
    const size_t name_len = sizeof(name) - 1;
    const size_t value_len = sizeof(value) - 1;
    unsigned char* p = header;
    unsigned char* end = header + header_len;
    unsigned char* line_end;

    /* Erste Zeile ist Request-Line bzw. Status-Line, dort steht kein Header */
    while (p < end && *p != '\n') {
        p++;
    }
    if (p < end) {
        p++;
    }

    while (p + name_len <= end) {
        if (strncasecmp((const char*)p, name, name_len) == 0) {
            p += name_len;
            line_end = p;
            while (line_end < end && *line_end != '\r' && *line_end != '\n') {
                line_end++;
            }
            if ((size_t)(line_end - p) < value_len) {
                /* Wert zu kurz zum Ueberschreiben - Aufrufer fuegt selbst ein */
                return 0;
            }
            memcpy(p, value, value_len);
            memset(p + value_len, ' ', (size_t)(line_end - p) - value_len);
            return 1;
        }
        while (p < end && *p != '\n') {
            p++;
        }
        if (p >= end) {
            break;
        }
        p++;
    }

    return 0;
}

/*
 * Inject X-Forwarded-* headers into request before sending to backend
 * This modifies the request_buffer to add headers before the body
 *
 * Zusaetzlich wird dem Backend "Connection: close" aufgezwungen: der Proxy
 * baut ohnehin pro Request eine neue UDS-Verbindung auf, Keep-Alive bringt
 * also nichts. Ohne den Header beendet ein HTTP/1.1-Backend eine chunked
 * Response nicht mit einem Verbindungsabbau, und weil der Proxy den
 * abschliessenden Nullchunk nicht auswertet, bliebe die Verbindung fuer
 * immer im Zustand STREAMING stehen (Client- und Backend-FD lecken).
 */
static int proxy_inject_forwarded_headers(reverse_proxy_connection_t* proxy) {
    char host_buf[256];
    char forwarded_headers[512];
    int forwarded_len;
    unsigned char* new_buffer;
    uint32_t new_size;
    const char* proto = "http";
    const char* conn_close = "";

    /* Connection: close erzwingen. Ist bereits ein Connection-Header vorhanden,
     * wird dessen Wert ueberschrieben, sonst wird unten einer eingefuegt.
     * Bei einem Upgrade-Request bleibt der Header unangetastet. */
    if (proxy_request_is_upgrade(proxy->request_buffer, proxy->request_header_end) == 0) {
        if (proxy_force_connection_close(proxy->request_buffer, proxy->request_header_end) == 0) {
            conn_close = "Connection: close\r\n";
        }
    }

    /* Extract Host header from original request */
    proxy_extract_host_header(proxy->request_buffer, proxy->request_header_end,
                               host_buf, sizeof(host_buf));

    /* Determine protocol */
#ifdef WEBSERVER_USE_SSL
    if (proxy->client_sock->use_ssl) {
        proto = "https";
    }
#endif

    /* Build X-Forwarded-* headers */
    forwarded_len = snprintf(forwarded_headers, sizeof(forwarded_headers),
        "X-Forwarded-For: %s\r\n"
        "X-Forwarded-Host: %s\r\n"
        "X-Forwarded-Proto: %s\r\n"
        "%s",
        proxy->client_sock->client_ip_str,
        host_buf[0] ? host_buf : "unknown",
        proto,
        conn_close);

    if (forwarded_len <= 0 || forwarded_len >= (int)sizeof(forwarded_headers)) {
        LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to build forwarded headers");
        return -1;
    }

#if _WEBSERVER_PROXY_DEBUG_ >= 4
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Injecting X-Forwarded-For=%s, Proto=%s",
              proxy->client_sock->client_ip_str, proto);
#endif

    /* Calculate new buffer size */
    new_size = proxy->request_buffer_pos + forwarded_len;
    if (new_size > proxy->request_buffer_size) {
        /* Allocate larger buffer */
        new_buffer = WebserverMalloc(new_size + 1024);
        if (new_buffer == NULL) {
            LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to allocate new request buffer");
            return -1;
        }
    } else {
        /* Reuse existing buffer with memmove */
        new_buffer = proxy->request_buffer;
    }

    /* Insert headers before the \r\n\r\n (header_end - 2 is where we insert) */
    /* Format: [original_header_without_final_crlf][new_headers][\r\n][body] */

    if (new_buffer != proxy->request_buffer) {
        /* Copy header (without final \r\n) */
        memcpy(new_buffer, proxy->request_buffer, proxy->request_header_end - 2);
    }

    /* Move body data to make room for new headers */
    uint32_t body_start = proxy->request_header_end;
    uint32_t body_len = proxy->request_buffer_pos - body_start;

    if (new_buffer == proxy->request_buffer) {
        /* Move body data to the right */
        memmove(&new_buffer[proxy->request_header_end - 2 + forwarded_len + 2],
                &proxy->request_buffer[body_start],
                body_len);
    } else {
        /* Copy body to new buffer */
        memcpy(&new_buffer[proxy->request_header_end - 2 + forwarded_len + 2],
               &proxy->request_buffer[body_start],
               body_len);
    }

    /* Insert new headers */
    memcpy(&new_buffer[proxy->request_header_end - 2], forwarded_headers, forwarded_len);

    /* Add final \r\n after new headers */
    new_buffer[proxy->request_header_end - 2 + forwarded_len] = '\r';
    new_buffer[proxy->request_header_end - 2 + forwarded_len + 1] = '\n';

    /* Update proxy state */
    if (new_buffer != proxy->request_buffer) {
        WebserverFree(proxy->request_buffer);
        proxy->request_buffer = new_buffer;
        proxy->request_buffer_size = new_size + 1024;
    }

    proxy->request_buffer_pos += forwarded_len;
    proxy->request_header_end += forwarded_len;

    return 0;
}

/*
 * Inject Connection: close header into response to force connection close after response
 */
static int proxy_inject_connection_close(reverse_proxy_connection_t* proxy) {
    const char* conn_header = "Connection: close\r\n";
    int conn_len = strlen(conn_header);
    unsigned char* new_buffer;
    uint32_t new_size;

    /* Hat das Backend bereits einen Connection-Header gesetzt (was es tut,
     * seit der Request mit "Connection: close" rausgeht), nur dessen Wert
     * anpassen - sonst stuenden zwei Connection-Header in der Response. */
    if (proxy_force_connection_close(proxy->response_buffer, proxy->response_header_end) == 1) {
        return 0;
    }

    /* Calculate new buffer size */
    new_size = proxy->response_buffer_pos + conn_len;
    if (new_size > proxy->response_buffer_size) {
        /* Allocate larger buffer */
        new_buffer = WebserverMalloc(new_size + 1024);
        if (new_buffer == NULL) {
            LOG(PROXY_LOG, ERROR_LEVEL, 0, "%s", "Failed to allocate new response buffer");
            return -1;
        }
        proxy->response_buffer_size = new_size + 1024;
    } else {
        new_buffer = proxy->response_buffer;
    }

    /* Insert header before the \r\n\r\n (header_end - 2 is where we insert) */
    uint32_t body_start = proxy->response_header_end;
    uint32_t body_len = proxy->response_buffer_pos - body_start;

    if (new_buffer != proxy->response_buffer) {
        /* Copy header (without final \r\n) */
        memcpy(new_buffer, proxy->response_buffer, proxy->response_header_end - 2);
    }

    /* Move body data to make room for new header */
    if (new_buffer == proxy->response_buffer) {
        memmove(&new_buffer[proxy->response_header_end - 2 + conn_len + 2],
                &proxy->response_buffer[body_start],
                body_len);
    } else {
        memcpy(&new_buffer[proxy->response_header_end - 2 + conn_len + 2],
               &proxy->response_buffer[body_start],
               body_len);
    }

    /* Insert Connection: close header */
    memcpy(&new_buffer[proxy->response_header_end - 2], conn_header, conn_len);

    /* Add final \r\n after new header */
    new_buffer[proxy->response_header_end - 2 + conn_len] = '\r';
    new_buffer[proxy->response_header_end - 2 + conn_len + 1] = '\n';

    /* Update proxy state */
    if (new_buffer != proxy->response_buffer) {
        WebserverFree(proxy->response_buffer);
        proxy->response_buffer = new_buffer;
    }

    proxy->response_buffer_pos += conn_len;
    proxy->response_header_end += conn_len;

#if _WEBSERVER_PROXY_DEBUG_ >= 5
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "Injected Connection: close header");
#endif
    return 0;
}

/*
 * Cleanup proxy connection
 */
static void proxy_cleanup_connection(reverse_proxy_connection_t* proxy) {
    if (proxy == NULL) {
        return;
    }

#if _WEBSERVER_PROXY_DEBUG_ >= 4
    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Cleaning up proxy connection");
#endif
    
    /* Backend Socket schließen */
    if (proxy->backend_sock != NULL) {
        delEventSocketAll(proxy->backend_sock);
        deleteSocket(proxy->backend_sock);
        /* extern_handle_data_ptr vor WebserverFreeSocketInfo auf NULL setzen,
         * um Rekursion über reverse_proxy_cleanup zu vermeiden */
        proxy->backend_sock->extern_handle_data_ptr = NULL;
        if (proxy->backend_fd >= 0) {
            close(proxy->backend_fd);
            proxy->backend_fd = -1;
        }
        WebserverFreeSocketInfo(proxy->backend_sock);
        proxy->backend_sock = NULL;
    } else if (proxy->backend_fd >= 0) {
        close(proxy->backend_fd);
        proxy->backend_fd = -1;
    }
    
    /* Client Socket wieder normalisieren (wird von WebserverConnectionManagerCloseRequest geschlossen) */
    if (proxy->client_sock != NULL) {
        proxy->client_sock->extern_handle = NULL;
        proxy->client_sock->extern_handle_data_ptr = NULL;
        /* Client Socket wird vom Aufrufer geschlossen */
    }
    
    /* Buffer freigeben */
    if (proxy->request_buffer != NULL) {
        WebserverFree(proxy->request_buffer);
    }
    if (proxy->response_buffer != NULL) {
        WebserverFree(proxy->response_buffer);
    }
    if (proxy->backend_path != NULL) {
        free(proxy->backend_path);
    }
    
    WebserverFree(proxy);
}

/*
 * Error Response generieren
 */
void reverse_proxy_gen_error_msg(http_request* p) {
    if (p->socket->header->isHttp1_1) {
        printHeaderChunk(p->socket, "HTTP/1.1 ");
    } else {
        printHeaderChunk(p->socket, "HTTP/1.0 ");
    }
    
    printHeaderChunk(p->socket, "502 Bad Gateway\r\n");
    printHeaderChunk(p->socket, "Server: libCWebUI\r\n");
    printHeaderChunk(p->socket, "Content-Type: text/html\r\n");
    printHeaderChunk(p->socket, "Content-Length: 50\r\n");
    printHeaderChunk(p->socket, "Connection: close\r\n");
    printHeaderChunk(p->socket, "\r\n");
    printHeaderChunk(p->socket, "<html><body><h1>502 Bad Gateway</h1></body></html>");
}

/*
 * Cleanup bei Socket Close
 */
void reverse_proxy_cleanup(socket_info* s) {
    if (s == NULL || s->extern_handle_data_ptr == NULL) {
        return;
    }
    
    reverse_proxy_connection_t* proxy = (reverse_proxy_connection_t*)s->extern_handle_data_ptr;
    proxy_cleanup_connection(proxy);
    s->extern_handle_data_ptr = NULL;
}
