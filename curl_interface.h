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

#ifndef _CURL_INTERFACE_H
#define _CURL_INTERFACE_H

#include <curl/curl.h>
#include <curl/multi.h>

#include "str.h"
#include "stack.h"
#include "functional_stack.h"

/** The type of an HTTP request
 */
enum request_type_e {
	POST,
	GET,
};

struct request_flags_t {
	int init    : 1;
	int header  : 1;
	int success : 1;

	// If the request failed, this holds the code
	int failure_code;
};

/** A structure containing headers and body sections of an HTTP message
 */
struct message_t {
	// The body part of an HTTP message, follows CRLF separator
	struct str_t body;
	// The headers part of an HTTP message
	struct str_t headers;
};

/** A structure for the state of an HTTP request.
 */
struct request_t {
	// The response from the server
	struct message_t response;
	// The request to send to the server
	struct message_t request;

	// The handle for libcurl
	CURL* handle;
	// The header list for the handle
	struct curl_slist* headers;

	// Callback for this request
	size_t (*callback) (void *data, size_t size, size_t nmemb, void *store);

	// Stack for cleanups
	struct stack_t cleanup;

	// What type of request this is.
	enum request_type_e type;

	// Any bit flags for control purposes (parsing etc)
	struct request_flags_t flags;
};

int ci_init(struct request_t* request, struct str_t* uri,
		size_t header_count, const struct str_t headers[],
		enum request_type_e type);
int ci_destroy(struct request_t* request);

int ci_create_header(struct request_t* request,
		size_t header_count, const struct str_t headers[]);
int ci_set_uri(struct request_t* request, struct str_t* uri);

int ci_request(struct request_t* request);

void ci_clear_response(struct request_t* request);

size_t ci_callback_controller(void *data, size_t size, size_t nmemb, void *store);

void ci_reset_flags(struct request_t* request);
#endif
