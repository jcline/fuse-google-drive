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
	 	enum request_type_e type)
{
	int ret = 0;

	ret = ci_set_uri(request, uri);

	return ret;
}

int ci_destroy(struct request_t* request)
{
	return 0;
}

int ci_set_uri(struct request_t* request, struct str_t* uri)
{
	return 0;
}
