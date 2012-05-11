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
#include <stdlib.h>
#include <string.h>

#include "str.h"

int str_init(struct str_t* str)
{
	str->str = NULL;
	str->len = 0;
	str->reserved = 0;
}

int str_concat(struct str_t* str, size_t str_count, struct str_t* strings[])
{
	size_t count = 0;
	for(; count < str_count; ++count)
	{
		str->str = (char*) realloc(str->str, str->len + strings[count]->len);
		str->reserved = str->len + strings[count]->len;
		memcpy(str->str + str->len, strings[count]->str, strings[count]->len);
		str->len += strings[count]->len;
	}
	return 0;
}

int str_destroy(struct str_t* str)
{
	free(str->str);
	memset(str, 0, sizeof(struct str_t));
	return 0;
}
