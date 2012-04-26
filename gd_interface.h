/*
	fuse-google-drive: a fuse filesystem wrapper for Google Drive
	Copyright (C) 2012  James Cline

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
 	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef _GOOGLE_DRIVE_INTERFACE_H
#define _GOOGLE_DRIVE_INTERFACE_H

#include <stdlib.h>
#include <curl/curl.h>
#include <curl/multi.h>

struct gdi_state {
	char *clientsecrets;
	char *redirecturi;
	char *clientid;
	CURLM *curlmulti;
	char *code;
};

char* urlencode (const char *url, size_t* length);

int gdi_get_credentials();
char* gdi_load_clientsecrets(const char *path);
char* gdi_load_redirecturi(const char *path);

int gdi_init(struct gdi_state *state);
void gdi_destroy(struct gdi_state *state);

#endif
