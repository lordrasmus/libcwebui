

void WebserverGenerateGUID(char* buf, int length){ 	
	generateGUID(buf,length); 
}

void setRenderVar(dummy_handler* s, char* name, char* text) { 
	ws_set_render_var( s, name, text ); 
}

dummy_var* getRenderVar(dummy_handler* s, const char* name,WS_VAR_FLAGS flags) { 
	return ws_get_render_var( s, name, flags ); 
}
