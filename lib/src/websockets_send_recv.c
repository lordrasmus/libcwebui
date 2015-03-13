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




#include "webserver.h"

#ifdef WEBSERVER_USE_WEBSOCKETS

#include <limits.h>

#ifndef WEBSOCKET_MAX_INBUFFER_SIZE
	#define WEBSOCKET_MAX_INBUFFER_SIZE 65000
	#warning #define WEBSOCKET_MAX_INBUFFER_SIZE x missing, assuming 65000 byte
#endif

#ifdef WEBSOCKET_INBUFFER_SIZE
	#warning WEBSOCKET_INBUFFER_SIZE depricated
#endif

typedef enum {
	WSF_CONTINUE = 0x0,
	WSF_TEXT = 0x1,
	WSF_BINARY = 0x2,
	WSF_RESERVED1 = 0x3,
	WSF_RESERVED2 = 0x4,
	WSF_RESERVED3 = 0x5,
	WSF_RESERVED4 = 0x6,
	WSF_RESERVED5 = 0x7,
	WSF_CLOSE = 0x8,
	WSF_PING = 0x9,
	WSF_PONG = 0xA,
	WSF_RESERVED6 = 0xB,
	WSF_RESERVED7 = 0xC,
	WSF_RESERVED8 = 0xD,
	WSF_RESERVED9 = 0xE,
	WSF_RESERVED10 = 0xF
} Opcodes;

typedef struct {
	char fin;
	/*char rsv1:1;
	 char rsv2:1;
	 char rsv3:1;*/
	Opcodes opcode;
	char mask_bit;
	unsigned char playload_length;
	WEBSOCK_LEN_T real_length;
	unsigned char *mask; //[4];
} websocket_frame;


int sendWebsocketFrame(const Opcodes op_code, const char* guid,const  char* in, const WEBSOCK_LEN_T length);

int sendWebsocketTextFrame(const char* guid,const  char* in, const WEBSOCK_LEN_T length) {
	return sendWebsocketFrame(WSF_TEXT, guid, in, length );
}

int sendWebsocketBinaryFrame(const char* guid,const unsigned  char* in, const WEBSOCK_LEN_T length) {
	return sendWebsocketFrame(WSF_BINARY, guid,(const  char* ) in, length );
}

int sendWebsocketFrame(const Opcodes op_code, const char* guid,const  char* in, const WEBSOCK_LEN_T length) {
	//
	//	Daten im Text Frame nicht 0 terminieren
	//
	websocket_queue_msg* msg;
	if (length < 126) {
		msg = create_websocket_output_queue_msg(WEBSOCKET_SIGNAL_MSG, guid, length + 2);
		msg->msg[0] = (unsigned char)(0x80 + op_code);
		msg->msg[1] = (unsigned char)(length);
		memcpy(&msg->msg[2], in, length);
		insert_websocket_output_queue(msg);
		return 0;
	} else {
		if ( ( length >= 126) && ( length < 65556) ){
			msg = create_websocket_output_queue_msg(WEBSOCKET_SIGNAL_MSG, guid, length + 4);
			msg->msg[0] = (unsigned char)(0x80 + op_code);
			msg->msg[1] = 126;
			msg->msg[2] = (unsigned char)(((unsigned int)length) >> 8);
			msg->msg[3] = (unsigned char)(((unsigned int)length) >> 0);
			memcpy(&msg->msg[4], in, length);
			insert_websocket_output_queue(msg);
			return 0;
		}
		msg = create_websocket_output_queue_msg( WEBSOCKET_SIGNAL_MSG, guid, length + 10);
		msg->msg[0] = (unsigned char)(0x80 + op_code);
		msg->msg[1] = 127;
		#ifdef SHORT_WS_FRAMES_T
			msg->msg[2] = 0;
			msg->msg[3] = 0;
			msg->msg[4] = 0;
			msg->msg[5] = 0;
		#else
			msg->msg[2] = ((WEBSOCK_LEN_T)length) >> 56;
			msg->msg[3] = ((WEBSOCK_LEN_T)length) >> 48;
			msg->msg[4] = ((WEBSOCK_LEN_T)length) >> 40;
			msg->msg[5] = ((WEBSOCK_LEN_T)length) >> 32;
		#endif
		msg->msg[6] = (unsigned char)(((WEBSOCK_LEN_T)length) >> 24);
		msg->msg[7] = (unsigned char)(((WEBSOCK_LEN_T)length) >> 16);
		msg->msg[8] = (unsigned char)(((WEBSOCK_LEN_T)length) >> 8);
		msg->msg[9] = (unsigned char)(((WEBSOCK_LEN_T)length) >> 0);
		memcpy(&msg->msg[10], in, length);
		insert_websocket_output_queue(msg);
		return 0;
	}
	return 0;
}

void insert_websocket_output_chunk(socket_info *sock, const unsigned char* in, const WEBSOCK_LEN_T length){
	int ret;
	ret = ws_list_empty(&sock->websocket_chunk_list);

	sendWebsocketChunk(sock, in, length);

	// Write Event nur hinzufuegen wenn noch keine frames in der liste waren
	if (ret == 1) {
		delEventSocketAll(sock);
		addEventSocketReadWritePersist(sock);
	}
}

//
// Schreibt Daten aus der output queue in die Socket chunk liste
//
int sendWebsocketFrameReal(const char* guid, const unsigned char* in, const WEBSOCK_LEN_T length) {

	// getSocketByGUID locked den socket mutex
	socket_info *sock = getSocketByGUID(guid);
	if (sock == 0) return -1;

	if (sock->closeSocket == 1) {
		PlatformUnlockMutex(&sock->socket_mutex);
		return -1;
	}

	if (sock->header->SecWebSocketVersion > 6) {
		insert_websocket_output_chunk(sock,in,length);
	}else{
		printf("Error : Websocket Version %d < 6 \n",sock->header->SecWebSocketVersion);
	}

	PlatformUnlockMutex(&sock->socket_mutex);

	return 0;

}

void sendCloseFrame2(const char* guid) {
	unsigned char buffer[2];
	buffer[0] = WSF_CLOSE;
	buffer[1] = 0;
	sendWebsocketBinaryFrame(guid, buffer, 2);
}

void sendCloseFrame(socket_info *sock) {
	sendCloseFrame2(sock->websocket_guid);}


int recFrameV8(socket_info *sock) {
	int last_frame_start;
	unsigned int offset = 0;
	unsigned int offset2 = 0;
	unsigned int i;
	unsigned int ui = 0;
	int ret;
	unsigned int diff = 0;
	unsigned int extra_bytes;
	int max_read;
	websocket_queue_msg* msg;
	websocket_frame wsf;
	unsigned char *tmp;


	while (1) {
		max_read = WebserverMallocedSize(sock->websocket_buffer) - sock->websocket_buffer_offset;
		//max_read = 18;
		ret = WebserverRecv(sock, &sock->websocket_buffer[sock->websocket_buffer_offset], max_read, 0);
		if (ret <= 0) return ret;

		sock->websocket_buffer_offset += ret;

		while (1) {

			extra_bytes = 0;
			last_frame_start = offset;
			if ( sock->websocket_buffer_offset > offset ){
				diff = sock->websocket_buffer_offset - offset;
			}else{
				printf("sock->websocket_buffer_offset ( %d ) <=  offset ( %d ) \n", sock->websocket_buffer_offset, offset);
			}

			if (diff < 2)
				goto recopy_buffer;

			// Reserved Bits must be 0
			if ( ( sock->websocket_buffer[offset] & 0x70 ) != 0 ){
				goto close_socket_error;
			}

			wsf.fin = 0;
			if (sock->websocket_buffer[offset] & 0x80){
				wsf.fin = 1;
			}
			wsf.opcode = (Opcodes) (sock->websocket_buffer[offset++] & 0x0F);

			if (sock->websocket_buffer[offset] & 0x80){
				wsf.mask_bit = 1;
				// 4 Byte Mask
				extra_bytes += 4;
			}
			wsf.playload_length = sock->websocket_buffer[offset++] & 0x7F;


			if (wsf.playload_length == 126) {
				// 2 byte extended payload
				offset += 2;
				extra_bytes += 2;
			}
			if (wsf.playload_length == 127) {
				// 8 byte extended payload
				offset += 8;
				extra_bytes += 8;
			}

			if (diff < (2 + extra_bytes )) goto recopy_buffer;

			wsf.real_length = wsf.playload_length;

			if (wsf.playload_length == 126) {
				wsf.real_length  = ((uint32_t)sock->websocket_buffer[offset - 1]) << 0;
				wsf.real_length += ((uint32_t)sock->websocket_buffer[offset - 2]) << 8;
			}

			if (wsf.playload_length == 127) {
				if ( ( sock->websocket_buffer[offset - 8] & 0x80 ) == 0x80 ) {
					printf("recFrameV8: the most significant bit MUST be 0\n");
					goto close_socket_error;
				}

				wsf.real_length  = ((uint32_t)sock->websocket_buffer[offset - 1]) << 0;
				wsf.real_length += ((uint32_t)sock->websocket_buffer[offset - 2]) << 8;
				wsf.real_length += ((uint32_t)sock->websocket_buffer[offset - 3]) << 16;
				wsf.real_length += ((uint32_t)sock->websocket_buffer[offset - 4]) << 24;
				#ifndef SHORT_WS_FRAMES_T
				wsf.real_length += ((uint64_t)sock->websocket_buffer[offset - 5]) << 32;
				wsf.real_length += ((uint64_t)sock->websocket_buffer[offset - 6]) << 40;
				wsf.real_length += ((uint64_t)sock->websocket_buffer[offset - 7]) << 48;
				wsf.real_length += ((uint64_t)sock->websocket_buffer[offset - 8]) << 56;
				#else
				if (
					( sock->websocket_buffer[offset - 5] != 0 ) ||
					( sock->websocket_buffer[offset - 6] != 0 ) ||
					( sock->websocket_buffer[offset - 7] != 0 ) ||
					( sock->websocket_buffer[offset - 8] != 0 )
					){
					printf("Error SHORT_WS_FRAMES_T defined Max Frame size 4G\n");
					goto close_socket_error;
				}

				#endif
			}


			if (diff < (2 + extra_bytes + wsf.real_length)){
				if ( ( last_frame_start == 0 ) && ( wsf.real_length > WebserverMallocedSize(sock->websocket_buffer) ) ){

					if ( wsf.real_length > WEBSOCKET_MAX_INBUFFER_SIZE  ){
						printf("Websocket Frame ( %d ) > Max Input Buffer ( %d ) \n", wsf.real_length, WEBSOCKET_MAX_INBUFFER_SIZE);
						goto close_socket_error;
					}

					sock->websocket_buffer = WebserverRealloc( sock->websocket_buffer, 2 + extra_bytes + wsf.real_length + 10 );
					offset = 0;
					goto read_more_data;
				}
				goto recopy_buffer;
			}

			if (wsf.mask_bit == 1) {
				wsf.mask = &sock->websocket_buffer[offset];
				offset += 4;
			}

			switch (wsf.opcode) {
			case WSF_TEXT:
			case WSF_BINARY:

				if ( wsf.fin == 1 ){
					if ( wsf.opcode == WSF_TEXT ){
						msg = create_websocket_input_queue_msg(WEBSOCKET_SIGNAL_MSG, sock->websocket_guid, sock->header->url, wsf.real_length + 1 );
					}else{
						msg = create_websocket_input_queue_msg(WEBSOCKET_SIGNAL_MSG, sock->websocket_guid, sock->header->url, wsf.real_length );
					}

					for (ui = 0; ui < wsf.real_length; ui++){
						if (wsf.mask_bit == 1) {
							msg->msg[ui] = sock->websocket_buffer[offset + ui] ^ wsf.mask[ui % 4];
						}else{
							msg->msg[ui] = sock->websocket_buffer[offset + ui];
						}
					}
					offset += ui;

					if ( wsf.opcode == WSF_TEXT ){
						msg->msg[wsf.real_length] = '\0';
					}

					insert_websocket_input_queue(msg);
				}else{
					tmp = WebserverMalloc( wsf.real_length ) ;

					ws_list_append(&sock->websocket_fragments, tmp);
					for (ui = 0; ui < wsf.real_length; ui++){
						if (wsf.mask_bit == 1) {
							tmp[ui] = sock->websocket_buffer[offset + ui] ^ wsf.mask[ui % 4];
						}else{
							tmp[ui] = sock->websocket_buffer[offset + ui];
						}
					}
					offset += ui;

					sock->websocket_fragment_start_opcode = wsf.opcode;
					sock->websocket_fragments_length = wsf.real_length;
				}
				break;

			case WSF_CONTINUE:
				tmp = WebserverMalloc( wsf.real_length ) ;

				ws_list_append(&sock->websocket_fragments, tmp);
				for (ui = 0; ui < wsf.real_length; ui++){
					if (wsf.mask_bit == 1) {
						tmp[ui] = sock->websocket_buffer[offset + ui] ^ wsf.mask[ui % 4];
					}else{
						tmp[ui] = sock->websocket_buffer[offset + ui];
					}
				}
				offset += ui;
				sock->websocket_fragments_length += ui;

				if ( wsf.fin == 1 ){
					if ( sock->websocket_fragment_start_opcode == WSF_TEXT ){
						msg = create_websocket_input_queue_msg(WEBSOCKET_SIGNAL_MSG, sock->websocket_guid, sock->header->url, sock->websocket_fragments_length + 1 );
					}else{
						msg = create_websocket_input_queue_msg(WEBSOCKET_SIGNAL_MSG, sock->websocket_guid, sock->header->url, sock->websocket_fragments_length );
					}

					offset2=0;
					ws_list_iterator_start(&sock->websocket_fragments);
					while ( ( tmp = (unsigned char*)ws_list_iterator_next(&sock->websocket_fragments) )) {
						memcpy( &msg->msg[offset2] , tmp , WebserverMallocedSize(tmp) );
						offset2 += WebserverMallocedSize(tmp);
						WebserverFree(tmp);
					}
					ws_list_iterator_stop(&sock->websocket_fragments);
					ws_list_clear(&sock->websocket_fragments);


					if ( sock->websocket_fragment_start_opcode == WSF_TEXT ){
						msg->msg[sock->websocket_fragments_length] = '\0';
					}

					insert_websocket_input_queue(msg);

				}

				break;

			case WSF_CLOSE:
				sendCloseFrame(sock);
				sock->closeSocket = 1;
				return 0;

			case WSF_PONG:
				offset += ui;
				break;

			case WSF_PING:
				if ( wsf.fin == 0 ){
					printf("Error : Websocket Ping Control Frame must not be fragmented \n");
					return -1;
				}
				for (ui = 0; ui < wsf.real_length; ui++){
					if (wsf.mask_bit == 1) {
						sock->websocket_buffer[offset + ui] = sock->websocket_buffer[offset + ui] ^ wsf.mask[ui % 4];
					}
				}
				sendWebsocketFrame(WSF_PONG, sock->websocket_guid,(const  char* ) &sock->websocket_buffer[offset] , wsf.real_length );
				offset += ui;
				break;

			case WSF_RESERVED1:
			case WSF_RESERVED2:
			case WSF_RESERVED3:
			case WSF_RESERVED4:
			case WSF_RESERVED5:
			case WSF_RESERVED6:
			case WSF_RESERVED7:
			case WSF_RESERVED8:
			case WSF_RESERVED9:
			case WSF_RESERVED10:
				printf("Websocket Frame Reserved Opcode %d received\n", wsf.opcode);
				return -1;

			/*default:
				printf("Websocket Frame V8 Opcode %d nicht behandelt\n", wsf.opcode);
				return 0;*/
			}

			// Wenn alle Bytes aus dem websocket_buffer verarbeitet wurden
			// beide offset wieder auf anfang setzen
			if (offset == sock->websocket_buffer_offset) {
				sock->websocket_buffer_offset = 0;
				offset = 0;
				break;
			}
		}
		continue;

		recopy_buffer:

			// Wenn bearbeitete Frames im Buffer sind letztes Frame nach vorne kopieren
			if ( last_frame_start > 0 ) {
				diff = sock->websocket_buffer_offset - last_frame_start;
				for (i = 0; i < diff; i++) {
					sock->websocket_buffer[i] = sock->websocket_buffer[last_frame_start + i];
				}
				sock->websocket_buffer_offset = diff;
			}else{
				printf("recopy_buffer last_frame_start = 0 \n");
			}
			offset = 0;

		read_more_data:
			continue;
	}

	return 0;

close_socket_error:
	sendCloseFrame(sock);
	sock->closeSocket = 1;
	return -1;
}

int recFrame(socket_info *sock) {

	if (sock == 0) return -1;

	if (sock->header->SecWebSocketVersion > 6) return recFrameV8(sock);

	LOG( WEBSOCKET_LOG, ERROR_LEVEL, sock->socket, "Websocket Frame < 7 not handled", "");
	return -1;
}

#endif
