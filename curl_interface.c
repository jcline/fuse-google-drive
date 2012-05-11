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

int ci_init(struct request_t* request, struct str_t* uri,
	 	size_t header_count, const struct str_t const* headers[],
		enum request_type_e type,
		size_t (*callback) (void *data, size_t size, size_t nmemb, void *store))
{
	int ret = 0;


	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL

	//curl_easy_setopt(handle, CURLOPT_HEADER, 1); // Enable headers, necessary?
	// set curl_post_callback for parsing the server response
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback);
	// set curl_post_callback's last parameter to state
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &request->response);

	request->handle = handle;

	curl_easy_perform(handle); // GET
	curl_easy_cleanup(handle); // cleanup

	ret = ci_set_uri(request, uri);

	return ret;
}

int ci_destroy(struct request_t* request)
{
	curl_slist_free_all(request->headers);
	return 0;
}

int ci_set_uri(struct request_t* request, struct str_t* uri)
{
	return curl_easy_setopt(request->handle, CURLOPT_URL, uri->str); // set URI
}

int ci_create_header(struct request_t* request,
		size_t header_count, const struct str_t headers[])
{
	struct curl_slist *header_list = NULL;

	size_t count = 0;
	for(; count < header_count; ++count)
	{
		header_list = curl_slist_append(header_list, headers[count].str);
	}

	request->headers = header_list;

	return 0;
}
