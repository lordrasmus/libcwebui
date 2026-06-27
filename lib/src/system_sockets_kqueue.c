/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui


 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

*/

#include "webserver.h" // Enthält socket_info und handleer()

#if defined ( USE_KQUEUE )

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#define MAX_EVENTS 64

/* Bitfeld in sock->registered: welche Filter aktuell beim kqueue angemeldet sind.
 * kqueue verwaltet READ und WRITE als getrennte Filter (anders als epoll, das
 * eine kombinierte Bitmaske pro fd nutzt). */
#define KQ_READ  0x01
#define KQ_WRITE 0x02

static int kq = -1;
static volatile int break_loop = 0;

void _set_main_thread( void ){}

void eventHandler(int fd, short flags, void *arg) {
    handleer(fd, flags, arg);
}

void initEvents(void) {
    kq = kqueue();
    if (kq < 0) {
        perror("kqueue failed");
        exit(1);
    }
    LOG(MESSAGE_LOG, NOTICE_LEVEL, 0, "%s", "event dispatcher: kqueue");
}

void freeEvents(void) {
    if (kq >= 0) close(kq);
    kq = -1;
}

void breakEvents(void) {
    break_loop = 1;
}

char waitEvents(void) {
    struct kevent evlist[MAX_EVENTS];
    while (!break_loop) {
        int n = kevent(kq, NULL, 0, evlist, MAX_EVENTS, NULL);
        if (n < 0 && errno != EINTR) {
            perror("kevent failed");
            return -1;
        }
        for (int i = 0; i < n; ++i) {
            socket_info *sock = (socket_info *)evlist[i].udata;

            /* Fehler beim Verarbeiten dieses Events -> wie EPOLLERR/EPOLLHUP
             * im epoll-Backend als TIMEOUT (Cleanup) dispatchen. */
            if (evlist[i].flags & EV_ERROR) {
                eventHandler(sock->socket, EVENT_TIMEOUT, sock);
                continue;
            }

            /* READ und WRITE getrennt dispatchen, da handleer/handleWebsocket
             * ein switch(type) nutzt und kombinierte Flags nicht matcht.
             * EV_EOF (Peer hat geschlossen) wird als READ dispatcht; handleer
             * erkennt den Disconnect beim recv. */
            if (evlist[i].filter == EVFILT_READ) {
                eventHandler(sock->socket, EVENT_READ, sock);
            } else if (evlist[i].filter == EVFILT_WRITE) {
                eventHandler(sock->socket, EVENT_WRITE, sock);
            }
        }
    }
    break_loop = 0;
    return 0;
}

/* Einen einzelnen Filter-Change anwenden. Rueckgabe wird bewusst ignoriert:
 * ein EV_DELETE auf einen nicht (mehr) registrierten Filter liefert ENOENT,
 * was harmlos ist (z.B. nach EV_ONESHOT, das der Kernel selbst entfernt). */
static void kq_change(socket_info *sock, int filter, int flags) {
    struct kevent kev;
    EV_SET(&kev, sock->socket, filter, flags, 0, 0, sock);
    kevent(kq, &kev, 1, NULL, 0, NULL);
}

/* Setzt den gewuenschten Event-Zustand. want = KQ_READ|KQ_WRITE Bitmaske.
 * persist=0 -> EV_ONESHOT (Filter feuert einmal, dann automatisch entfernt).
 * kevent() ist ein lockfreier Kernel-Syscall und kann (anders als libevents
 * event_active) gefahrlos innerhalb des socket_mutex aufgerufen werden. */
static void modifyKqueue(socket_info *sock, int want, int persist) {
    int oneshot = persist ? 0 : EV_ONESHOT;

    if (want & KQ_READ)
        kq_change(sock, EVFILT_READ, EV_ADD | oneshot);
    else if (sock->registered & KQ_READ)
        kq_change(sock, EVFILT_READ, EV_DELETE);

    if (want & KQ_WRITE)
        kq_change(sock, EVFILT_WRITE, EV_ADD | oneshot);
    else if (sock->registered & KQ_WRITE)
        kq_change(sock, EVFILT_WRITE, EV_DELETE);

    sock->registered = want;
}

void addEventSocketRead(socket_info *sock) {
    modifyKqueue(sock, KQ_READ, 0);
}

void addEventSocketReadPersist(socket_info *sock) {
    modifyKqueue(sock, KQ_READ, 1);
}

void addEventSocketWritePersist(socket_info *sock) {
    modifyKqueue(sock, KQ_WRITE, 1);
}

void addEventSocketReadWritePersist(socket_info *sock) {
    modifyKqueue(sock, KQ_READ | KQ_WRITE, 1);
}

void delEventSocketReadPersist(socket_info *sock) {
    delEventSocketAll(sock);
}

void delEventSocketWritePersist(socket_info *sock) {
    delEventSocketAll(sock);
}

void delEventSocketAll(socket_info *sock) {
    if (sock->registered & KQ_READ)
        kq_change(sock, EVFILT_READ, EV_DELETE);
    if (sock->registered & KQ_WRITE)
        kq_change(sock, EVFILT_WRITE, EV_DELETE);
    sock->registered = 0;
}

void activateEventSocketWrite(socket_info *sock) {
    /* WRITE zu den aktuellen Events hinzufuegen (kein Deadlock, s.o.). */
    modifyKqueue(sock, KQ_READ | KQ_WRITE, 1);
}

void updateWebsocketEventFlags(socket_info *sock, int list_empty) {
    if (list_empty) {
        modifyKqueue(sock, KQ_READ, 1);
    } else {
        modifyKqueue(sock, KQ_READ | KQ_WRITE, 1);
    }
}

void deleteEvent(socket_info *sock) {
    delEventSocketAll(sock);
}

#ifdef WEBSERVER_USE_SSL
void commitSslEventFlags(socket_info *sock) {
    /* Der ssl_block_event_flags / ssl_event_flags-Mechanismus ist ein Relikt
     * aus dem libevent/select-Backend und wird von kqueue (wie epoll) nicht
     * benoetigt: kevent() ist nicht blockierend und jederzeit aufrufbar. Diese
     * Funktion wird im aktuellen Code nirgends aufgerufen und existiert nur zur
     * Vollstaendigkeit des Backend-Interfaces. */
    modifyKqueue(sock, KQ_READ | KQ_WRITE, 1);
}
#endif

#endif
