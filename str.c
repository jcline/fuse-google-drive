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

int str_destroy(struct str_t* str)
{
	if(!str)
		return 0;
	free(str->str);
	memset(str, 0, sizeof(struct str_t));
	return 0;
}

int str_concat(struct str_t* str, size_t str_count, struct str_t* strings[])
{
	size_t count = 0;
	for(; count < str_count; ++count)
	{
		str->str = (char*) realloc(str->str, str->len + strings[count]->len + 1);
		str->reserved = str->len + strings[count]->len + 1;
		memcpy(str->str + str->len, strings[count]->str, strings[count]->len);
		str->len += strings[count]->len;
	}
	str->str[str->len] = 0;
	return 0;
}

void str_swap(struct str_t* a, struct str_t* b)
{
	char* tmp = a->str;
	const size_t len = a->len;
	a->str = b->str;
	a->len = b->len;
	b->str = tmp;
	b->len = len;
}
