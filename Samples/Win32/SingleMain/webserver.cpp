// webserver.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

//#include "stdafx.h"
//#include <Windows.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#include "webserver_api_functions.h"

#include "linked_list.h"

#pragma comment(lib, "Ws2_32.lib")

#ifdef __CDT_PARSER__
	#define STD_INPUT_HANDLE
	#define FALSE
	#define DWORD
	#define HANDLE
	#define sprintf_s
#endif

typedef struct 
{
	char* key;
	uint32_t key_length;

	list_t* childen;

	void* value;

}trie_node;

trie_node* trie_init_node( void ) {
	trie_node* ret = (trie_node*)malloc(sizeof(trie_node));
	memset(ret, 0, sizeof(trie_node));
	ret->childen = (list_t*)malloc(sizeof(list_t));
	ws_list_init(ret->childen);
	return ret;
}

static void set_key(trie_node* node, const char* key, uint32_t len) {
	node->key = (char*)malloc(len + 1);
	memcpy(node->key, key, len);
	node->key[len] = '\0';
	node->key_length = len;
}

void trie_insert(trie_node* tie, const char* key, void* value) {
	uint32_t inserted = 0;
	uint32_t len = (uint32_t)strlen(key);

	

	if (ws_list_size(tie->childen) == 0) {
		trie_node* value_node = trie_init_node();
		value_node->value = value;
		set_key( value_node , key, len );
		ws_list_append(tie->childen, value_node);
		return;
	}

	trie_node* remove_node = 0;

	ws_list_iterator_start(tie->childen);
	while (ws_list_iterator_hasnext(tie->childen)) {
		trie_node* node = (trie_node*)ws_list_iterator_next(tie->childen);
		if (node->key[0] != key[0]) continue;

		uint32_t same_chars = 0;
		for ( ; ( same_chars < len ) && ( same_chars < node->key_length); same_chars++) {
			if (node->key[same_chars] != key[same_chars])
				break;
		}

		ws_list_iterator_start(node->childen);
		while (ws_list_iterator_hasnext(node->childen)) {
			trie_node* child_node = (trie_node*)ws_list_iterator_next(node->childen);
			if (child_node->key[0] != key[same_chars]) continue;

			// einer der children hat den gleichen anfangsbuchstaben wie der rest vom key
			// node als neue root verwenden und wert dort einsetzen
			trie_insert(node, &key[same_chars], value);
			ws_list_iterator_stop(node->childen);
			ws_list_iterator_stop(tie->childen);
			return;
		}
		ws_list_iterator_stop(node->childen);

		if ((same_chars == node->key_length ) && (len > node->key_length)) {
			// neuer nur ein zeichen länger darum als child anhängen
			trie_node* value_node = trie_init_node(); 
			value_node->value = value;
			set_key(value_node, &key[same_chars], len - same_chars);
			ws_list_append(node->childen, value_node);
			inserted = 1;
			break;
		}

		// key des nodes ist gleich dem insert key und in node ist noch kein wert eingetragen
		if ((same_chars == node->key_length) && (len == node->key_length) && (node->value == 0)) {
			node->value = value;
			inserted = 1;
			break;
		}

		trie_node* new_node = trie_init_node();
		set_key( new_node, key, same_chars);
		
		remove_node = node; // diesen key aus children entfernen
		char* bak = node->key;
		set_key(node, &bak[same_chars], len - same_chars);
		free(bak);
		ws_list_append(new_node->childen, node);

		trie_node* value_node = trie_init_node(); 
		value_node->value = value;
		set_key( value_node, &key[same_chars], len - same_chars);
		ws_list_append(new_node->childen, value_node);

		ws_list_append(tie->childen, new_node);

		inserted = 1; // wurde eingesetzt
		break;
		
	}
	ws_list_iterator_stop(tie->childen);

	if (remove_node != 0) {
		ws_list_delete(tie->childen, remove_node);
	}

	if (inserted == 0) {
		trie_node* value_node = trie_init_node();
		value_node->value = value;
		value_node->key = (char*)malloc(len + 1);
		memcpy(value_node->key, key, len);
		value_node->key[len] = '\0';
		value_node->key_length = len;
		ws_list_append(tie->childen, value_node);
	}
}

void trie_dump(const char* prefix,trie_node* node) {
	/*if (node->value != 0) {
		printf("%s%s\n", prefix,node->key);
	}*/

	ws_list_iterator_start(node->childen);
	while (ws_list_iterator_hasnext(node->childen)) {
		trie_node* child_node = (trie_node*)ws_list_iterator_next(node->childen);
		if (child_node->value != 0) {
			printf("%s%s\n", prefix, child_node->key);
		}

		if (ws_list_size(child_node->childen) > 0) {
			char pre[1000];
			sprintf_s(pre, "%s%s", prefix, child_node->key);
			trie_dump(pre,child_node);
		}
	}
	ws_list_iterator_stop(node->childen);
}

extern "C" {

	void init_testsite(void);

}

int main()
{
	printf("Hello Webserver!\n");

	
	/*
	trie_node *root = trie_init_node();

	trie_insert(root, "atest1", (void*)100);
	trie_insert(root, "btest1", (void*)1);
	trie_insert(root, "ctest1", (void*)2);

	trie_insert(root, "atest2", (void*)3);
	trie_insert(root, "atest3", (void*)4);
	trie_insert(root, "atest4", (void*)5);
	trie_insert(root, "atee1", (void*)5);
	trie_insert(root, "atee2", (void*)6);

	trie_insert(root, "atest5", (void*)7);

	trie_insert(root, "ctest2", (void*)8);

	trie_dump("1-", root);
	
	// die beiden noch prüfen
	trie_insert(root, "ctest", (void*)9);
	//trie_insert(root, "ctes", (void*)9);

	trie_dump("3-",root);
	
	*/
	if (0 == WebserverInit()) {



		WebserverConfigSetInt("use_csp", 0);

		WebserverAddFileDir("", "../../testSite/www");
		WebserverAddFileDir("css", "../../testSite/css"); 
		WebserverAddFileDir("img", "../../testSite/img");
		


		//WebserverLoadPlugin("TestPlugin", "../testSite/src/test_plugin.so");

		WebserverConfigSetInt("port", 8081);
		WebserverConfigSetInt("ssl_port", 4443);
		WebserverConfigSetText("ssl_file_path", "./");
		WebserverConfigSetText("ssl_key_file", "server.pem");
		WebserverConfigSetText("ssl_key_file_password", "password");
		WebserverConfigSetText("ssl_dh_file", "dh1024.pem");
		WebserverConfigSetText("ssl_ca_list_file", "root.pem");

		//startApiThreads();
		init_testsite();

		WebserverStart();
	}

	WebserverShutdown();
	
	


	HANDLE eventHandles[] = {
		GetStdHandle(STD_INPUT_HANDLE)
		// ... add more handles and/or sockets here
	};

	DWORD result = WSAWaitForMultipleEvents(sizeof(eventHandles) / sizeof(eventHandles[0]),
		&eventHandles[0],
		FALSE,
		1000,
		TRUE
	);
	while (1);
	/*
	switch (result) {
	case WSA_WAIT_TIMEOUT: // no I/O going on right now
		break;

	case WSA_WAIT_EVENT_0 + 0: // stdin at array index 0
		ProcessStdin();
		break;

	case WSA_WAIT_EVENT_0 + 1: // handle/socket at array index 1
		break;

	case WSA_WAIT_EVENT_0 + 2: // ... and so on
		break;

	default: // handle the other possible conditions
		break;
	} // end switch result
	*/

    return 0;
}

