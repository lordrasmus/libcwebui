/* webserver_events.c */

//#include "webserver_events.h"
#include "webserver.h" // Enthält socket_info und handleer()

#if defined ( USE_EPOLL )


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>

#define MAX_EVENTS 64

static int epoll_fd = -1;
static volatile int break_loop = 0;

void _set_main_thread( void ){}

void eventHandler(int fd, short flags, void *arg) {

    /* EPOLLRDHUP: Gegenstelle hat ihre Schreibrichtung geschlossen (FIN).
     * Wird wie ERR/HUP als EVENT_TIMEOUT gemeldet, damit der Handler die
     * Verbindung abbaut. Muss vor EPOLLIN geprueft werden: bei einem FIN
     * setzt der Kernel beide Flags, und ein Read wuerde hier nur 0 liefern. */
    if (flags & EPOLLERR || flags & EPOLLHUP || flags & EPOLLRDHUP) {
        handleer(fd, EVENT_TIMEOUT, arg);
        return;
    }

    /* READ und WRITE getrennt dispatchen, da handleer/handleWebsocket
     * ein switch(type) nutzt und kombinierte Flags nicht matcht. */
    if (flags & EPOLLIN) {
        handleer(fd, EVENT_READ, arg);
        return;
    }
    if (flags & EPOLLOUT) {
        handleer(fd, EVENT_WRITE, arg);
    }

    if (flags & ~(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        /* Barely reachable - the branches above return - but a printf would
         * bypass the log entirely and land on the console. */
        LOG(MESSAGE_LOG, ERROR_LEVEL, fd, "epoll reported unhandled flags 0x%X", flags);
    }
}

void initEvents(void) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        exit(1);
    }
    LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "%s", "event dispatcher: epoll");
}

void freeEvents(void) {
    if (epoll_fd >= 0) close(epoll_fd);
    epoll_fd = -1;
}

void breakEvents(void) {
    break_loop = 1;
}

/* The batch epoll_wait() has filled and how far it has been dispatched.
 *
 * No locking needed: both the dispatch loop and eventSocketFreed() run in the
 * event loop thread. Every socket_info is released from there - the websocket
 * threads only look sockets up by GUID and queue output, they never free one.
 * They do register events in parallel (see modifyEpoll), which is unrelated. */
static struct epoll_event *batch_events = NULL;
static int batch_len = 0;
static int batch_pos = 0;
static time_t stale_logged = 0;

void eventSocketFreed(socket_info *sock) {
    int i;

    for (i = batch_pos + 1; i < batch_len; ++i) {
        if (batch_events[i].data.ptr != sock) {
            continue;
        }

        /* Drop the entry before the socket is released, so the dispatch loop
         * skips it. Clearing it here rather than validating the pointer at
         * dispatch time is what makes this safe: the memory is still allocated
         * at this point, so no newly accepted connection can have taken the
         * block over yet. A check at dispatch time would confirm such a
         * recycled pointer as valid and deliver the event to a different,
         * live connection. */
        batch_events[i].data.ptr = NULL;

        /* At most one line per second - one hit per connection would flood the
         * log and push out whatever explains how it got there. */
        {
            time_t now = time(NULL);
            if (now != stale_logged) {
                stale_logged = now;
                LOG(MESSAGE_LOG, WARNING_LEVEL, sock->socket,
                    "socket released while its event was still queued, entry %d of %d skipped",
                    i, batch_len);
            }
        }
    }
}

char waitEvents(void) {
    struct epoll_event events[MAX_EVENTS];
    while (!break_loop) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0 && errno != EINTR) {
            perror("epoll_wait failed");
            return -1;
        }
        batch_events = events;
        batch_len = n;
        for (int i = 0; i < n; ++i) {
            socket_info *sock = (socket_info *)events[i].data.ptr;
            batch_pos = i;
            if (sock == NULL) {
                /* Released by a handler earlier in this same batch - see
                 * eventSocketFreed(). Dispatching would work on freed memory:
                 * epoll_ctl(DEL) cannot take back an event already delivered. */
                continue;
            }
            eventHandler(sock->socket, events[i].events, sock);
        }
        /* Outside the dispatch loop there is no batch to invalidate. */
        batch_len = 0;
        batch_events = NULL;
    }
    break_loop = 0;
    return 0;
}

static void modifyEpoll(socket_info *sock, uint32_t events, int persist) {
    struct epoll_event ev;
    int op;

    ev.data.ptr = sock;
    ev.events = events | (persist ? 0 : EPOLLONESHOT);

    op = sock->registered ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    if (epoll_ctl(epoll_fd, op, sock->socket, &ev) == 0) {
        sock->registered = 1;
        return;
    }

    /* sock->registered can disagree with what the kernel has: it is kept
     * without a lock while other threads (websocket_output_thread) register
     * in parallel. Retry with the opposite operation rather than losing the
     * requested events silently - a socket left without its registration
     * only shows up later as a connection that hangs. */
    if (errno == EEXIST) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock->socket, &ev) == 0) {
            sock->registered = 1;
            return;
        }
    } else if (errno == ENOENT) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock->socket, &ev) == 0) {
            sock->registered = 1;
            return;
        }
    }

    LOG(MESSAGE_LOG, ERROR_LEVEL, sock->socket, "epoll_ctl(%s, events=0x%X) failed: %s",
        (op == EPOLL_CTL_MOD) ? "MOD" : "ADD", ev.events, strerror(errno));
    sock->registered = 0;
}

void addEventSocketRead(socket_info *sock) {
    modifyEpoll(sock, EPOLLIN, 0);
}

void addEventSocketReadPersist(socket_info *sock) {
    modifyEpoll(sock, EPOLLIN, 1);
}

void addEventSocketWritePersist(socket_info *sock) {
    modifyEpoll(sock, EPOLLOUT, 1);
}

void addEventSocketReadWritePersist(socket_info *sock) {
    modifyEpoll(sock, EPOLLIN | EPOLLOUT, 1);
}

/* Nur auf das FIN der Gegenstelle horchen, nicht auf Daten.
 * Gedacht fuer Sockets, die gerade nicht bedient werden (z.B. der Client
 * waehrend der Reverse Proxy auf das Backend wartet): eingehende Daten
 * loesen kein Event aus, ein Verbindungsabbau dagegen sofort. Ohne das
 * bleibt ein solcher Socket voellig unbeobachtet und ein Abbau faellt
 * erst auf, wenn ihn zufaellig jemand anfasst.
 *
 * Achtung: EPOLLRDHUP feuert auch bei einem Half-Close (Gegenstelle hat nur
 * shutdown(SHUT_WR) gemacht und will noch lesen). Wer das nicht als Abbruch
 * werten will, muss das im Handler unterscheiden. */
void addEventSocketPeerClosePersist(socket_info *sock) {
    modifyEpoll(sock, EPOLLRDHUP, 1);
}

void delEventSocketReadPersist(socket_info *sock) {
    delEventSocketAll(sock);
}

void delEventSocketWritePersist(socket_info *sock) {
    delEventSocketAll(sock);
}

void delEventSocketAll(socket_info *sock) {
    if (sock->registered) {
        /* ENOENT and EBADF are expected here: the kernel drops a closed fd
         * from the set on its own, and an EPOLLONESHOT event may already have
         * disarmed it. Anything else means the socket keeps firing events. */
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock->socket, NULL) != 0
                && errno != ENOENT && errno != EBADF) {
            LOG(MESSAGE_LOG, ERROR_LEVEL, sock->socket, "epoll_ctl(DEL) failed: %s",
                strerror(errno));
        }
        sock->registered = 0;
    }
}

void activateEventSocketWrite(socket_info *sock) {
    /* epoll_ctl ist kernel-thread-safe, kein Deadlock moeglich.
     * WRITE zu den aktuellen Events hinzufuegen. */
    modifyEpoll(sock, EPOLLIN | EPOLLOUT, 1);
}

void updateWebsocketEventFlags(socket_info *sock, int list_empty) {
    if (list_empty) {
        modifyEpoll(sock, EPOLLIN, 1);
    } else {
        modifyEpoll(sock, EPOLLIN | EPOLLOUT, 1);
    }
}

void deleteEvent(socket_info *sock) {
    delEventSocketAll(sock);
}

#ifdef WEBSERVER_USE_SSL
#endif

#endif
