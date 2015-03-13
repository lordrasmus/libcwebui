/*

SPDX-License-Identifier: MPL-2.0

 libCWebUI
 Copyright (C) 2007 - 2019  Ramin Seyed-Moussavi

 Projekt URL : https://github.com/lordrasmus/libcwebui

 
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/. 

*/




void initConfig(void);
void freeConfig(void);

void setConfigText(const char* name, const char* text);
void setConfigInt(const char* name, const int value);

char* getConfigText(const char* name);
int   getConfigInt(const char* name) ;
