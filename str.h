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

#ifndef _STR_H
#define _STR_H

#include <stdlib.h>

struct str_t {
	char *str;
	size_t len;
	size_t reserved;
};

void str_init(struct str_t* str);
int str_init_create(struct str_t* str, const char const* value, size_t size);
int str_destroy(struct str_t* str);
int str_clear(struct str_t* str);

int str_resize(struct str_t *str, size_t new_size);

int str_concat(struct str_t* str, size_t str_count, const struct str_t const* strings[]);
void str_swap(struct str_t* a, struct str_t* b);

int str_char_concat(struct str_t* str, const char const* value, size_t size);

struct str_t *str_urlencode_str (const struct str_t const* url);
struct str_t *str_urlencode_char (const char* url, size_t length);

#endif

