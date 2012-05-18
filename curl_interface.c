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

#include "str.h"
#include "curl_interface.h"

#include <curl/curl.h>
#include <curl/multi.h>

#include <string.h>

int ci_init(struct request_t* request, struct str_t* uri,
		size_t header_count, const struct str_t const headers[],
		enum request_type_e type)
{
	// TODO: Errors
	union func_u func;
	int ret = 0;

	memset(request, 0, sizeof(struct request_t));

	fstack_init(&request->cleanup, 10);

	CURL* handle = curl_easy_init();
	func.func1 = curl_easy_cleanup;
	fstack_push(&request->cleanup, handle, &func, 1);

	curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL

	curl_easy_setopt(handle, CURLOPT_HEADER, 1); // Enable headers, necessary?
	// set curl_post_callback for parsing the server response
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, ci_callback_controller);
	// set curl_post_callback's last parameter to state
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &request->response);

	ret = ci_create_header(request, header_count, headers);
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, request->headers); // Set headers
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "fuse-google-drive/0.1");

	request->handle = handle;

	ret = ci_set_uri(request, uri);

	return ret;
}

int ci_destroy(struct request_t* request)
{
	while(request->cleanup.size)
		fstack_pop(&request->cleanup);
	fstack_destroy(&request->cleanup);
	memset(request, 0, sizeof(struct request_t));
	return 0;
}

int ci_set_uri(struct request_t* request, struct str_t* uri)
{
	return curl_easy_setopt(request->handle, CURLOPT_URL, uri->str); // set URI
}

int ci_create_header(struct request_t* request,
		size_t header_count, const struct str_t headers[])
{
	union func_u func;
	struct curl_slist *header_list = NULL;

	size_t count = 0;
	for(; count < header_count; ++count)
	{
		printf("%s\n", headers[count].str);
		header_list = curl_slist_append(header_list, headers[count].str);
	}

	request->headers = header_list;
	func.func1 = curl_slist_free_all;
	fstack_push(&request->cleanup, header_list, &func, 1);

	return 0;
}

int ci_request(struct request_t* request)
{
	ci_reset_flags(request);
	return curl_easy_perform(request->handle);
}

void ci_clear_response(struct request_t* request)
{
	str_destroy(&request->response.body);
	str_destroy(&request->response.headers);
	str_init(&request->response.body);
	str_init(&request->response.headers);
	memset(&request->flags, 0, sizeof(struct request_flags_t));
}

/** Curl callback to handle Google's response when listing files.
 *
 *  Because Google's server returns the file listing in chunks, this function
 *  puts all those chunks together into one contiguous string.
 *
 *  @data  char*        the response from Google's server
 *  @size  size_t       size of one element in data
 *  @nmemb size_t       number of size chunks
 *  @store struct str_t our contiguous string
 *
 *  @returns the size of the data read, curl expects size*nmemb or it errors
 */
size_t ci_callback_controller(void *data, size_t size, size_t nmemb, void *store)
{
	struct request_t* req = (struct request_t*) store;
	struct request_flags_t* flags = &req->flags;

	// Store the header portion of the reqponse
	if(!flags->header)
	{
		char* iter = strstr((char*) data, "\r\n");
		if(iter == (char*)data)
			flags->header = 1;
		struct str_t *header = &req->response.headers;
		str_char_concat(header, (char*) data);
	}
	// If we are not in the header section of the response, then
	// we need to store the body portion.
	else
	{
		struct str_t *body = &req->response.body;
		str_char_concat(body, (char*) data);
	}

	return size*nmemb;
}

void ci_reset_flags(struct request_t* request)
{
	memset(&request->flags, 0, sizeof(struct request_flags_t));
}
