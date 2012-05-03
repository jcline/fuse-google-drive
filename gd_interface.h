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

#include <curl/curl.h>
#include <curl/multi.h>
#include <fuse.h>
#include <stdlib.h>

#include "gd_cache.h"
#include "stack.h"

struct gdi_state {
	char *clientsecrets;
	char *redirecturi;
	char *clientid;
	CURLM *curlmulti;
	char *code;

	// So we can identify files owned by this user
	char *email;

	char *access_token;
	char *id_token;
	char *refresh_token;
	long token_expiration;
	char *token_type;

	struct gd_fs_entry_t *head;
	struct gd_fs_entry_t *tail;
	size_t num_files;

	struct stack_t *stack;

	int callback_error;
};

char* urlencode (const char *url, size_t* length);

int gdi_get_credentials();

int gdi_init(struct gdi_state *state);
void gdi_destroy(struct gdi_state *state);

/* Interface for various operations */
void gdi_get_file_list(struct gdi_state *state);

#endif
