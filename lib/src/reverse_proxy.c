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
static int proxy_get_upgrade_value(const unsigned char* header, uint32_t header_len,
                                   char* buf, int buf_size);
static int proxy_get_status_code(const unsigned char* header, uint32_t header_len);
static int proxy_inject_forwarded_headers(reverse_proxy_connection_t* proxy);
static int proxy_inject_connection_close(reverse_proxy_connection_t* proxy);
static int proxy_extract_host_header(const unsigned char* header, uint32_t header_len, char* host_buf, int host_buf_size);
static int proxy_request_is_upgrade(const unsigned char* header, uint32_t header_len);
static int proxy_force_connection_close(unsigned char* header, uint32_t header_len);
static void proxy_shutdown_client(reverse_proxy_connection_t* proxy);
static void proxy_tunnel_update_events(reverse_proxy_connection_t* proxy);
static int proxy_tunnel_flush_c2b(reverse_proxy_connection_t* proxy);
static int proxy_tunnel_flush_b2c(reverse_proxy_connection_t* proxy);

/*
 * Follow a chunked response body to find its end.
 *
 * The proxy forwards the body byte for byte, so this only tracks framing - it
 * never rewrites or de-chunks anything. Call it with every stretch of body
 * bytes in order, exactly once each; the state persists across calls because a
 * chunk header can be split across two recv() calls.
 *
 * Returns 1 once the null chunk and its trailer are complete.
 *
 * On broken framing the state goes to PROXY_CHUNK_ERROR and stays there, and
 * the caller falls back to waiting for the backend to close - which is exactly
 * how it behaved before this parser existed. A bug in here can therefore not
 * cut a valid response short, only miss the early end.
 *
 * That property only holds because the framing is checked strictly: every
 * deviation has to end in ERROR rather than in a guess. Two ways to get it
 * wrong, both found by review:
 *   - a size line without any hex digit would otherwise read as size 0, i.e.
 *     as the null chunk, and end the response early (hence
 *     PROXY_CHUNK_SIZE_START)
 *   - a bare LF instead of CRLF is rejected, as in nginx and apache; tolerating
 *     it would leave the parser sitting in a line state forever, and a lone LF
 *     is a known request smuggling vector
 *
 * Chunk sizes are capped well below the point where the accumulator could
 * overflow; anything that large is a broken backend, not a real body.
 */
#define PROXY_CHUNK_SIZE_MAX  0x7FFFFFFFFFFFFFFFULL

static uint64_t proxy_hex_value(unsigned char c) {
    if (c <= '9') {
        return (uint64_t)(c - '0');
    }
    if (c <= 'F') {
        return (uint64_t)(c - 'A' + 10);
    }
    return (uint64_t)(c - 'a' + 10);
}

static int proxy_chunked_scan(reverse_proxy_connection_t* proxy,
                              const unsigned char* data, uint32_t len) {
    uint32_t i = 0;

    if (proxy->chunk_state == PROXY_CHUNK_DONE) {
        return 1;
    }
    if (proxy->chunk_state == PROXY_CHUNK_ERROR) {
        return 0;
    }

    while (i < len) {
        unsigned char c = data[i];

        switch (proxy->chunk_state) {
            case PROXY_CHUNK_SIZE_START:
                /* A size line must start with a hex digit. Without this state
                 * an empty line would be read as size 0, i.e. as the null
                 * chunk, and end the response early - the one way a parser
                 * error could truncate instead of merely miss the end.
                 * nginx has the same dedicated state (sw_chunk_start), apache
                 * names the case in a comment ("this also works for empty
                 * chunk size lines"). */
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    proxy->current_chunk_size = proxy_hex_value(c);
                    proxy->chunk_state = PROXY_CHUNK_SIZE;
                } else {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                i++;
                break;

            case PROXY_CHUNK_SIZE:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    uint64_t digit = proxy_hex_value(c);

                    if (proxy->current_chunk_size > (PROXY_CHUNK_SIZE_MAX - digit) / 16) {
                        proxy->chunk_state = PROXY_CHUNK_ERROR;
                        return 0;
                    }
                    proxy->current_chunk_size = proxy->current_chunk_size * 16 + digit;
                } else if ((c == ';') || (c == ' ') || (c == '\t')) {
                    /* Extension, or whitespace before the CRLF - nginx enters
                     * the same state for all three. */
                    proxy->chunk_state = PROXY_CHUNK_EXT;
                } else if (c == '\r') {
                    proxy->chunk_state = PROXY_CHUNK_SIZE_LF;
                } else {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                i++;
                break;

            case PROXY_CHUNK_EXT:
                if (c == '\r') {
                    proxy->chunk_state = PROXY_CHUNK_SIZE_LF;
                } else if (c == '\n') {
                    /* Bare LF instead of CRLF. Rejected everywhere here, as in
                     * nginx (no option at all) and apache (strict is the
                     * default, only HttpProtocolOptions Unsafe allows it) -
                     * a lone LF is a known request smuggling vector. */
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                i++;
                break;

            case PROXY_CHUNK_SIZE_LF:
                if (c != '\n') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                /* size 0 is the null chunk, only the trailer follows */
                proxy->chunk_state = (proxy->current_chunk_size == 0)
                                   ? PROXY_CHUNK_TRAILER : PROXY_CHUNK_DATA;
                i++;
                break;

            case PROXY_CHUNK_DATA: {
                /* just skip the payload - it is already on its way out */
                uint64_t remaining = (uint64_t)(len - i);
                uint64_t skip = (proxy->current_chunk_size < remaining)
                              ? proxy->current_chunk_size : remaining;

                i += (uint32_t)skip;
                proxy->current_chunk_size -= skip;

                if (proxy->current_chunk_size == 0) {
                    proxy->chunk_state = PROXY_CHUNK_DATA_CR;
                }
                break;
            }

            case PROXY_CHUNK_DATA_CR:
                if (c != '\r') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                proxy->chunk_state = PROXY_CHUNK_DATA_LF;
                i++;
                break;

            case PROXY_CHUNK_DATA_LF:
                if (c != '\n') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                proxy->chunk_state = PROXY_CHUNK_SIZE_START;
                proxy->current_chunk_size = 0;
                i++;
                break;

            case PROXY_CHUNK_TRAILER:
                /* start of a line: an immediate CRLF means no trailers and
                 * ends the response */
                if (c == '\r') {
                    proxy->chunk_state = PROXY_CHUNK_TRAILER_LF;
                } else if (c == '\n') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;   /* bare LF */
                    return 0;
                } else {
                    proxy->chunk_state = PROXY_CHUNK_TRAILER_LINE;
                }
                i++;
                break;

            case PROXY_CHUNK_TRAILER_LF:
                if (c != '\n') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                proxy->chunk_state = PROXY_CHUNK_DONE;
                return 1;

            case PROXY_CHUNK_TRAILER_LINE:
                if (c == '\r') {
                    proxy->chunk_state = PROXY_CHUNK_TRAILER_LINE_LF;
                } else if (c == '\n') {
                    /* Without this the parser stayed in this state for the
                     * rest of the stream and never found the end. */
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                i++;
                break;

            case PROXY_CHUNK_TRAILER_LINE_LF:
                if (c != '\n') {
                    proxy->chunk_state = PROXY_CHUNK_ERROR;
                    return 0;
                }
                proxy->chunk_state = PROXY_CHUNK_TRAILER;
                i++;
                break;

            case PROXY_CHUNK_DONE:
                return 1;

            case PROXY_CHUNK_ERROR:
            default:
                return 0;
        }
    }

    return (proxy->chunk_state == PROXY_CHUNK_DONE) ? 1 : 0;
}

/*
 * WebSocket tunnel: event registration follows the backlog
 *
 * Read from a side only while the opposite direction has nothing pending -
 * otherwise we would keep pulling in data we cannot pass on and would have to
 * drop it. Ask for writability exactly while something is pending.
 *
 * The add... calls set the complete event set, so one call per socket is both
 * necessary and sufficient.
 */
static void proxy_tunnel_update_events(reverse_proxy_connection_t* proxy) {
    int client_read  = (proxy->tunnel_c2b_len == 0);
    int client_write = (proxy->tunnel_b2c_len > 0);
    int backend_read  = (proxy->tunnel_b2c_len == 0);
    int backend_write = (proxy->tunnel_c2b_len > 0);

    if (proxy->client_sock != NULL) {
        if (client_read && client_write) {
            addEventSocketReadWritePersist(proxy->client_sock);
        } else if (client_write) {
            addEventSocketWritePersist(proxy->client_sock);
        } else if (client_read) {
            addEventSocketReadPersist(proxy->client_sock);
        } else {
            /* Both directions parked: nothing to send, and reading is blocked
             * by the backlog in the other direction. Register nothing at all -
             * READ would spin (the peer has data the handler must not take,
             * level triggered epoll reports it again immediately) and so would
             * EPOLLRDHUP once the peer is gone. The socket is re-armed from
             * proxy_tunnel_update_events() as soon as the other side's write
             * event drained the backlog. */
            delEventSocketAll(proxy->client_sock);
        }
    }

    if (proxy->backend_sock != NULL) {
        if (backend_read && backend_write) {
            addEventSocketReadWritePersist(proxy->backend_sock);
        } else if (backend_write) {
            addEventSocketWritePersist(proxy->backend_sock);
        } else if (backend_read) {
            addEventSocketReadPersist(proxy->backend_sock);
        } else {
            delEventSocketAll(proxy->backend_sock);   /* siehe oben */
        }
    }
}

/*
 * Push whatever is pending towards the backend.
 * Returns 0 on success (backlog may still be non-empty), -1 if the connection
 * is broken.
 */
static int proxy_tunnel_flush_c2b(reverse_proxy_connection_t* proxy) {
    while (proxy->tunnel_c2b_sent < proxy->tunnel_c2b_len) {
        int sent = send(proxy->backend_fd,
                        &proxy->request_buffer[proxy->tunnel_c2b_sent],
                        proxy->tunnel_c2b_len - proxy->tunnel_c2b_sent, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;   /* stays pending, retried on the next write event */
            }
            LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd,
                "WebSocket tunnel: backend send failed: %s", strerror(errno));
            return -1;
        }
        if (sent == 0) {
            return 0;
        }
        proxy->tunnel_c2b_sent += (uint32_t)sent;
    }

    proxy->tunnel_c2b_len = 0;
    proxy->tunnel_c2b_sent = 0;
    return 0;
}

/*
 * Push whatever is pending towards the client. WebserverSend is required here
 * so the payload goes through TLS on an https connection.
 */
static int proxy_tunnel_flush_b2c(reverse_proxy_connection_t* proxy) {
    while (proxy->tunnel_b2c_sent < proxy->tunnel_b2c_len) {
        size_t sent = 0;
        SOCKET_SEND_STATUS status = WebserverSend(proxy->client_sock,
                            &proxy->response_buffer[proxy->tunnel_b2c_sent],
                            proxy->tunnel_b2c_len - proxy->tunnel_b2c_sent,
                            0, &sent);

        /* Count what was taken even on BUFFER_FULL - mbedTLS reports accepted
         * bytes there, OpenSSL reports 0. */
        proxy->tunnel_b2c_sent += (uint32_t)sent;

        /* Alles ausser NO_MORE_DATA und BUFFER_FULL beendet den Tunnel. Der
         * SSL-Pfad meldet einen abgerissenen Client auch als
         * SSL_PROTOCOL_ERROR, SOCKET_SEND_SSL_ERROR oder CLIENT_DISCONNECTED
         * (-1) - keiner davon ist SOCKET_SEND_CLIENT_DISCONNECTED. */
        if (status != SOCKET_SEND_NO_MORE_DATA
                && status != SOCKET_SEND_SEND_BUFFER_FULL) {
            return -1;
        }
        if (status == SOCKET_SEND_SEND_BUFFER_FULL || sent == 0) {
            return 0;   /* stays pending, retried on the next write event */
        }
    }

    proxy->tunnel_b2c_len = 0;
    proxy->tunnel_b2c_sent = 0;
    return 0;
}

/*
 * Shut the client connection down so the pending close actually happens.
 *
 * DONE and ERROR only set closeSocket and arm a read event; the teardown then
 * waits for that event to fire. For a plain request it does, because the
 * response carries Connection: close and the client closes. A websocket tunnel
 * ended by the backend is different: the browser is not told anything, an idle
 * tunnel sends nothing and closes nothing, so no event ever arrives and both
 * descriptors stay around indefinitely.
 *
 * shutdown() makes the socket readable at EOF right away, so the armed event
 * fires on the next loop iteration and handleer runs the normal teardown. It
 * also tells the peer the connection is over instead of leaving it waiting.
 *
 * Data already handed to the kernel is still sent - shutdown does not discard
 * the send buffer, the FIN goes out behind it. ENOTCONN is expected when the
 * peer is already gone.
 */
static void proxy_shutdown_client(reverse_proxy_connection_t* proxy) {
    if (proxy->client_sock == NULL || proxy->client_sock->socket < 0) {
        return;
    }

    if (shutdown(proxy->client_sock->socket, SHUT_RDWR) != 0 && errno != ENOTCONN) {
        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->client_sock->socket,
            "shutdown of client socket failed: %s", strerror(errno));
    }
}

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
 * Send a minimal error response directly on the client socket
 *
 * detail may be NULL. Only usable while no response header has gone out yet -
 * the caller has to make sure of that.
 */
static void proxy_send_error_response(reverse_proxy_connection_t* proxy, int code,
                                      const char* reason, const char* detail) {
    char body[512];
    char response[768];
    int body_len;
    int response_len;
    size_t total_sent = 0;
    size_t sent;
    SOCKET_SEND_STATUS status;

    body_len = snprintf(body, sizeof(body),
        "<html><head><title>%d %s</title></head>"
        "<body><h1>%d %s</h1>"
        "<p>%s</p></body></html>",
        code, reason, code, reason, (detail != NULL) ? detail : "");

    response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        code, reason, body_len, body);

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

        /* Alles ausser NO_MORE_DATA und BUFFER_FULL ist toedlich - der
         * SSL-Pfad liefert auch SSL_PROTOCOL_ERROR, SOCKET_SEND_SSL_ERROR und
         * CLIENT_DISCONNECTED (-1). */
        if (status != SOCKET_SEND_NO_MORE_DATA
                && status != SOCKET_SEND_SEND_BUFFER_FULL) {
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->client_sock->socket,
                "client gone while sending the error page (status %d)", status);
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

        case PROXY_STATE_CONNECT_BACKEND:
        case PROXY_STATE_SEND_TO_BACKEND:
        case PROXY_STATE_RECV_BACKEND_HEADER:
        case PROXY_STATE_STREAMING:
            /* In these states the client socket only listens for a connection
             * teardown (addEventSocketPeerClosePersist in CONNECT_BACKEND),
             * not for data. An event here can therefore only mean the client
             * is gone. Tear down right away - epoll reports this level
             * triggered forever, ignoring it would be an endless loop.
             *
             * CONNECT_BACKEND and SEND_TO_BACKEND have to be part of this:
             * without them the event falls into default, is never consumed and
             * the event loop keeps spinning at 100% CPU until the process is
             * restarted.
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
            /* WebSocket Tunnel, Client-Seite.
             *
             * The handler gets no event type, so both directions are attempted:
             * on a write event the flush makes progress and the read returns
             * NO_MORE_DATA, on a read event the flush is a no-op. */

            /* Pending data towards the client first - that is what a write
             * event on this socket is for. */
            if (proxy->tunnel_b2c_len > 0) {
                if (proxy_tunnel_flush_b2c(proxy) < 0) {
                    proxy->state = PROXY_STATE_DONE;
                    proxy_process_state(proxy);
                    return;
                }
            }

            /* Only read while nothing is queued towards the backend, otherwise
             * there would be nowhere to put it. */
            while (proxy->tunnel_c2b_len == 0) {
                bytes_read = WebserverRecv(proxy->client_sock, proxy->request_buffer,
                                           proxy->request_buffer_size, 0);

                if (bytes_read <= 0) {
                    /* Rueckgabecodes von WebserverRecv auswerten, kein errno.
                     * CLIENT_NO_MORE_DATA (0) heisst nur "gerade nichts da" -
                     * bei SSL kommt das z.B. bei einem unvollstaendigen TLS-Record,
                     * der Tunnel darf deswegen nicht abgebrochen werden. */
                    if (bytes_read == CLIENT_NO_MORE_DATA) {
                        break;
                    }
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "%s", "WebSocket tunnel: client closed");
#endif
                    proxy->state = PROXY_STATE_DONE;
                    proxy_process_state(proxy);
                    return;
                }

                proxy->tunnel_c2b_len = (uint32_t)bytes_read;
                proxy->tunnel_c2b_sent = 0;

                if (proxy->backend_fd >= 0 && proxy_tunnel_flush_c2b(proxy) < 0) {
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }

                if (!WebserverSSLPending(proxy->client_sock)) {
                    break;
                }
            }

            /* Returning with a backlog and pending SSL data is a legitimate
             * state: reading stopped because there is nowhere to put the data.
             * run_extern_handler() will not call us again for it - the wakeup
             * comes from the backend write event, which drives the read path
             * from reverse_proxy_backend_handler() once the backlog drains. */
            proxy_tunnel_update_events(proxy);
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
            if (proxy->response_buffer_pos >= proxy->response_buffer_size) {
                /* Buffer full and still no complete header. Without this the
                 * branch below is skipped, nothing is read, and the armed read
                 * event keeps firing - the same endless loop as on the request
                 * side. A backend whose header does not fit is broken. */
                LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd,
                    "Backend response header exceeds %u bytes, giving up",
                    proxy->response_buffer_size);
                proxy->state = PROXY_STATE_ERROR;
                proxy_process_state(proxy);
                return;
            }
            {
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

                /* Follow the framing before the data goes out. Every body byte
                 * exactly once - the state survives across recv boundaries,
                 * a chunk header can arrive split in two. */
                if (proxy->response_body_mode == BODY_MODE_CHUNKED) {
                    proxy_chunked_scan(proxy, proxy->response_buffer,
                                       (uint32_t)bytes_read);
                }

                /* Sofort weiterleiten */
                proxy_process_state(proxy);
                return;
            }
            break;
            
        case PROXY_STATE_WEBSOCKET_TUNNEL:
            /* WebSocket tunnel, backend side. Symmetric to the client side:
             * drain the backlog first, then read. */
#if _WEBSERVER_PROXY_DEBUG_ >= 5
            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: backend handler called");
#endif
            /* Pending data towards the backend - that is what a write event on
             * this socket is for. */
            if (proxy->tunnel_c2b_len > 0) {
                if (proxy_tunnel_flush_c2b(proxy) < 0) {
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }

                /* Backlog gone: the client socket may still hold decrypted
                 * bytes inside the SSL layer. Those are invisible to epoll -
                 * they left the kernel buffer when the record was read - so no
                 * read event will ever announce them. While the backlog
                 * existed the client read path declined to take them, which is
                 * what kept the connection alive instead of dropping data.
                 * Drive that path from here now that there is room again.
                 *
                 * This is the wakeup path that lets handleer() stop looping on
                 * WebserverSSLPending() without the bytes starving.
                 *
                 * The client handler re-arms both sockets and may tear the
                 * connection down, so proxy must not be touched afterwards. */
                if ((proxy->tunnel_c2b_len == 0) && (proxy->client_sock != NULL) &&
                    WebserverSSLPending(proxy->client_sock)) {
                    reverse_proxy_client_handler(proxy->client_sock->socket, proxy);
                    return;
                }
            }

            /* Only read while nothing is queued towards the client. */
            if (proxy->tunnel_b2c_len == 0) {
                bytes_read = recv(proxy->backend_fd, proxy->response_buffer,
                                  proxy->response_buffer_size, 0);

                if (bytes_read <= 0) {
                    if (bytes_read == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: backend closed");
#endif
                        proxy->state = PROXY_STATE_DONE;
                        proxy_process_state(proxy);
                        return;
                    }
                    proxy_tunnel_update_events(proxy);
                    return;
                }

                proxy->tunnel_b2c_len = (uint32_t)bytes_read;
                proxy->tunnel_b2c_sent = 0;

                if (proxy_tunnel_flush_b2c(proxy) < 0) {
                    LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "%s", "WebSocket tunnel: client disconnected");
                    proxy->state = PROXY_STATE_ERROR;
                    proxy_process_state(proxy);
                    return;
                }
            }

            proxy_tunnel_update_events(proxy);
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
                /* Header not complete yet. Enforce PROXY_MAX_HEADER_SIZE before
                 * reading on: without that limit the buffer eventually fills up,
                 * the client read loop then breaks without calling recv() and the
                 * read event stays armed - level triggered epoll keeps reporting
                 * the fd, which spins the event loop. */
                if (proxy->request_buffer_pos > PROXY_MAX_HEADER_SIZE) {
                    LOG(PROXY_LOG, ERROR_LEVEL, 0, "Request header exceeds %d bytes, rejecting",
                        PROXY_MAX_HEADER_SIZE);
                    proxy_send_error_response(proxy, 431, "Request Header Fields Too Large", NULL);
                    proxy->response_header_sent = 1;   /* stops ERROR from sending a 502 on top */
                    proxy->state = PROXY_STATE_ERROR;
                    return proxy_process_state(proxy);
                }
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
                int request_is_chunked = proxy_check_chunked_encoding(
                    proxy->request_buffer, proxy->request_header_end);

                /*
                 * Both framings at once is the classic request smuggling
                 * pattern: whoever reads by Content-Length and whoever reads by
                 * Transfer-Encoding disagree about where the request ends, and
                 * the bytes in between become a second request for one of them.
                 * RFC 9112 6.3 - "ought to be handled as an error". Unlike the
                 * response side the sender here is a client, so this is not a
                 * bug report, it is an attempt; reject it outright.
                 */
                if ((parsed_content_length >= 0) && request_is_chunked) {
                    LOG(PROXY_LOG, WARNING_LEVEL, 0, "%s",
                        "Request has both Content-Length and Transfer-Encoding, rejecting");
                    proxy_send_error_response(proxy, 400, "Bad Request", NULL);
                    proxy->response_header_sent = 1;
                    proxy->state = PROXY_STATE_ERROR;
                    return proxy_process_state(proxy);
                }

                /*
                 * A chunked request body is not implemented. Passing it on as
                 * BODY_MODE_NONE - which is what used to happen - sends the
                 * request without its body and leaves the backend waiting for
                 * data that never arrives. Say so instead.
                 *
                 * Browsers do not produce this over HTTP/1.1: they know the
                 * length and send Content-Length, and the one streaming case
                 * (fetch with a ReadableStream body) is restricted to HTTP/2
                 * and HTTP/3. It comes from tooling - curl with unknown length,
                 * programmatic clients.
                 */
                if (request_is_chunked) {
                    LOG(PROXY_LOG, WARNING_LEVEL, 0, "%s",
                        "Chunked request body is not supported, rejecting");
                    proxy_send_error_response(proxy, 411, "Length Required", NULL);
                    proxy->response_header_sent = 1;
                    proxy->state = PROXY_STATE_ERROR;
                    return proxy_process_state(proxy);
                }

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

            /* Buffer full but the body is still short of Content-Length. That
             * happens when the announced size did not fit and the buffer was
             * capped while growing. Reading cannot make progress from here, so
             * the read loop would break without recv() and spin the event loop.
             * Reject instead of waiting for data there is no room for. */
            if (proxy->request_buffer_pos >= proxy->request_buffer_size) {
                LOG(PROXY_LOG, ERROR_LEVEL, 0,
                    "Request body of %lu bytes does not fit into the buffer (%u), rejecting",
                    (unsigned long)proxy->request_content_length, proxy->request_buffer_size);
                proxy_send_error_response(proxy, 413, "Payload Too Large", NULL);
                proxy->response_header_sent = 1;
                proxy->state = PROXY_STATE_ERROR;
                return proxy_process_state(proxy);
            }
            break;

        case PROXY_STATE_CONNECT_BACKEND:
            /* The request is fully read - from here on the client socket must
             * no longer be registered for data. It used to stay that way until
             * the request had been sent in full, and in that window a client
             * FIN or RST produces a read event that no state of this handler
             * picks up: level triggered epoll redelivers it immediately and
             * the event loop spins at 100% CPU. Measured against a backend
             * that does not take the body (test_uds_server) - the spin did not
             * even stop once the backend was already dead.
             *
             * No further data can arrive anyway: the body is complete and
             * there is no pipelining (the backend is forced to Connection:
             * close). The connection teardown still has to be watched, else a
             * vanished client leaves its backend connection behind as a pair
             * of descriptors. */
            addEventSocketPeerClosePersist(proxy->client_sock);
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

                /* The client socket has been listening for the teardown only
                 * since CONNECT_BACKEND, nothing left to switch over here. */
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
            /* Parse the response header. A rejected upgrade (see there) leaves
             * nothing usable behind, so the connection is torn down. */
            if (proxy_parse_response_header(proxy) < 0) {
                proxy->state = PROXY_STATE_ERROR;
                return proxy_process_state(proxy);
            }

#if _WEBSERVER_PROXY_DEBUG_ >= 6
            proxy_dump_header("RESPONSE FROM BACKEND", proxy->response_buffer, proxy->response_header_end);
#endif

            /* WebSocket Upgrade? -> Tunnel Mode */
            if (proxy->response_is_upgrade) {
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
                /*
                 * Content-Length and Transfer-Encoding at the same time is
                 * refused rather than repaired.
                 *
                 * RFC 9112 6.3 puts the error first - "ought to be handled as
                 * an error" - and only then allows forwarding, and that with a
                 * MUST to strip Content-Length beforehand. Otherwise we would
                 * count by Content-Length while the client parses the chunked
                 * framing, and both headers reach it unchanged: the response
                 * gets truncated at whatever the smaller of the two says.
                 *
                 * The references disagree here, so the reasoning matters more
                 * than the vote: nginx rejects (ngx_http_upstream.c: "upstream
                 * sent Content-Length and Transfer-Encoding headers at the same
                 * time"), apache and go strip Content-Length and continue.
                 * Both of those proxy arbitrary third-party upstreams, where
                 * leniency is the only option. Our backend ships with the same
                 * product - if it sends both it is broken, and that should be
                 * visible as a 502 with a log line instead of silently patched
                 * over.
                 */
                if ((parsed_content_length >= 0) &&
                    proxy_check_chunked_encoding(proxy->response_buffer,
                                                 proxy->response_header_end)) {
                    LOG(PROXY_LOG, ERROR_LEVEL, proxy->backend_fd, "%s",
                        "Backend sent Content-Length and Transfer-Encoding at the same time");
                    proxy->state = PROXY_STATE_ERROR;
                    return proxy_process_state(proxy);
                }

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

            /* Body bytes that arrived together with the header have to go
             * through the chunk scanner just like everything later - for a
             * small response the complete body including the null chunk is
             * already sitting here. */
            if (proxy->response_body_mode == BODY_MODE_CHUNKED &&
                proxy->response_body_received > 0) {
                proxy_chunked_scan(proxy,
                                   &proxy->response_buffer[proxy->response_header_end],
                                   (uint32_t)proxy->response_body_received);
            }
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
                        sent = 0;
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
                        /* Alles ausser NO_MORE_DATA ist toedlich. WebserverSend
                         * liefert auf dem SSL-Pfad auch SSL_PROTOCOL_ERROR (-22),
                         * SOCKET_SEND_SSL_ERROR (4) und CLIENT_DISCONNECTED (-1);
                         * wer nur auf SOCKET_SEND_CLIENT_DISCONNECTED prueft, haelt
                         * einen abgerissenen TLS-Client faelschlich fuer "spaeter
                         * nochmal" und dreht endlos zwischen WAIT_WRITE und
                         * STREAMING. */
                        if (status != SOCKET_SEND_NO_MORE_DATA) {
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
                    if (proxy->response_is_upgrade) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                        LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Entering WebSocket tunnel mode");
#endif
                        proxy->state = PROXY_STATE_WEBSOCKET_TUNNEL;
                        /* Event-Zustand sauber zuruecksetzen, bevor fuer den Tunnel-Modus neu registriert wird */
                        delEventSocketAll(proxy->client_sock);
                        delEventSocketAll(proxy->backend_sock);

                        /* Anything the backend already sent past the header is
                         * payload and has to reach the client. Hand it over as
                         * the initial backlog instead of dropping it. */
                        proxy->tunnel_b2c_len = proxy->response_buffer_pos;
                        proxy->tunnel_b2c_sent = proxy->send_pos;
                        proxy->tunnel_c2b_len = 0;
                        proxy->tunnel_c2b_sent = 0;
                        if (proxy->tunnel_b2c_sent >= proxy->tunnel_b2c_len) {
                            proxy->tunnel_b2c_len = 0;
                            proxy->tunnel_b2c_sent = 0;
                        } else if (proxy_tunnel_flush_b2c(proxy) < 0) {
                            proxy->state = PROXY_STATE_ERROR;
                            return proxy_process_state(proxy);
                        }

                        proxy_tunnel_update_events(proxy);
                        return 0;
                    }
                }

                /* Body-Daten aus Buffer senden - in Schleife bis Buffer leer oder BUFFER_FULL */
                while (proxy->send_pos < proxy->response_buffer_pos) {
                    sent = 0;
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
                    /* Alles ausser NO_MORE_DATA ist toedlich, siehe Header-Schleife */
                    if (status != SOCKET_SEND_NO_MORE_DATA) {
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
                        if (proxy->response_is_upgrade) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                            LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd, "%s", "Entering WebSocket tunnel mode");
#endif
                            proxy->state = PROXY_STATE_WEBSOCKET_TUNNEL;
                            proxy->tunnel_c2b_len = 0;
                            proxy->tunnel_c2b_sent = 0;
                            proxy->tunnel_b2c_len = 0;
                            proxy->tunnel_b2c_sent = 0;
                            proxy_tunnel_update_events(proxy);
                            return 0;
                        }

                        proxy->state = PROXY_STATE_DONE;
                        return proxy_process_state(proxy);
                    }
                }

                /* Chunked: the null chunk and its trailer are the end. Without
                 * this the only marker would be the backend closing, which
                 * never happens for an upgrade request - no "Connection:
                 * close" is forced on the backend there - so the connection
                 * used to hang on to two descriptors.
                 *
                 * On broken framing chunk_state is PROXY_CHUNK_ERROR and we
                 * wait for the close as before. */
                if (proxy->response_body_mode == BODY_MODE_CHUNKED &&
                    proxy->chunk_state == PROXY_CHUNK_DONE) {
#if _WEBSERVER_PROXY_DEBUG_ >= 3
                    LOG(PROXY_LOG, NOTICE_LEVEL, proxy->backend_fd,
                        "STREAMING: chunked response complete (%lu bytes)",
                        (unsigned long)proxy->response_body_sent);
#endif
                    proxy->state = PROXY_STATE_DONE;
                    return proxy_process_state(proxy);
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
                    proxy_send_error_response(proxy, 502, "Bad Gateway",
                        (proxy->backend_path != NULL) ? proxy->backend_path : "unknown");
                }

                proxy->client_sock->reverse_proxy_error = 1;
                proxy->client_sock->closeSocket = 1;
                proxy->client_sock->extern_handle = NULL;
                /* Backend Events sofort deregistrieren */
                if (proxy->backend_sock != NULL) {
                    delEventSocketAll(proxy->backend_sock);
                }
                proxy_shutdown_client(proxy);
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
            proxy_shutdown_client(proxy);
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
    proxy->response_is_upgrade = 0;

#if _WEBSERVER_PROXY_DEBUG_ >= 4
    LOG(PROXY_LOG, NOTICE_LEVEL, 0, "Response status: %d",
              proxy->response_status_code);
#endif

    /*
     * Status 101 means the backend is switching protocols - from here on the
     * only question is whether it was allowed to. That is independent of which
     * protocol it names, so the check hangs on the status code alone.
     *
     * Hanging it on "the response says websocket" instead would skip exactly
     * the cases it exists for: a 101 carrying Upgrade: h2c, or none at all,
     * would sail past unchecked.
     *
     * Two things are verified, both taken from how established proxies handle
     * it: the request must have carried an Upgrade header at all (nginx,
     * ngx_http_proxy_module.c: "if (r->headers_in.upgrade) u->upgrade = 1"),
     * and the protocol named in the response must be the one that was asked
     * for (apache mod_proxy_http.c rejects a mismatch with 502, caddy
     * streaming.go refuses it - "backend tried to switch to unexpected
     * protocol"). Without the first check a backend can open a tunnel that
     * nobody requested.
     *
     * A rejected upgrade tears the connection down instead of continuing: a
     * 101 carries no Content-Length, so the response path would fall back to
     * BODY_MODE_CLOSE and wait for a backend that is itself waiting for tunnel
     * traffic - two descriptors held for nothing.
     *
     * What passes is tunnelled whatever the protocol is called. The tunnel
     * forwards raw bytes and does not interpret them, so it is not websocket
     * specific.
     */
    if (proxy->response_status_code == 101) {
        char req_upgrade[64];
        char res_upgrade[64];

        if (proxy_get_upgrade_value(proxy->request_buffer, proxy->request_header_end,
                                    req_upgrade, sizeof(req_upgrade)) == 0) {
            LOG(PROXY_LOG, WARNING_LEVEL, proxy->backend_fd, "%s",
                "Backend sent 101 for a request without Upgrade header, dropping connection");
            return -1;
        }

        if ((proxy_get_upgrade_value(proxy->response_buffer, proxy->response_header_end,
                                     res_upgrade, sizeof(res_upgrade)) == 0) ||
            (strcasecmp(req_upgrade, res_upgrade) != 0)) {
            LOG(PROXY_LOG, WARNING_LEVEL, proxy->backend_fd,
                "Backend switched to '%s' but '%s' was requested, dropping connection",
                res_upgrade[0] != '\0' ? res_upgrade : "(none)", req_upgrade);
            return -1;
        }

        proxy->response_is_upgrade = 1;
    }

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
/*
 * Extract the value of the Upgrade header, trimmed, into buf.
 *
 * Returns 1 if a non-empty value was found. Used to compare what the client
 * asked for against what the backend switched to.
 */
static int proxy_get_upgrade_value(const unsigned char* header, uint32_t header_len,
                                   char* buf, int buf_size) {
    const unsigned char* p = header;
    const unsigned char* end = header + header_len;
    int i;

    buf[0] = '\0';

    while (p + 8 <= end) {
        /* Only match at the start of a header line, otherwise a value
         * containing "upgrade:" would be picked up as a header name. */
        if ((p == header || *(p - 1) == '\n') &&
            (strncasecmp((const char*)p, "Upgrade:", 8) == 0)) {

            p += 8;
            while (p < end && (*p == ' ' || *p == '\t')) {
                p++;
            }

            i = 0;
            while ((p < end) && (*p != '\r') && (*p != '\n') && (i < buf_size - 1)) {
                buf[i++] = (char)*p;
                p++;
            }
            /* trailing whitespace */
            while ((i > 0) && ((buf[i - 1] == ' ') || (buf[i - 1] == '\t'))) {
                i--;
            }
            buf[i] = '\0';
            return (i > 0) ? 1 : 0;
        }
        p++;
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
