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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "str.h"

void str_init(struct str_t* str)
{
	str->str = NULL;
	str->len = 0;
	str->reserved = 0;
}

int str_init_create(struct str_t* str, const char const* value, size_t size)
{
	str_init(str);
	if(!size)
		size = strlen(value);
	return str_char_concat(str, value, size);
}

int str_destroy(struct str_t* str)
{
	if(!str)
		return 0;
	free(str->str);
	memset(str, 0, sizeof(struct str_t));
	return 0;
}

int str_concat(struct str_t* str, size_t str_count, const struct str_t const* strings[])
{
	size_t count;
	size_t alloc = 0;
	// Count the amount of memory needed
	for(count = 0; count < str_count; ++count)
		alloc += strings[count]->len;
	// Allocate enough memory for current contents and new contents in one go
	if(str_resize(str, str->len + alloc + 1))
		return 1;

	// Copy strings[] values into str
	for(count = 0; count < str_count; ++count)
	{
		memcpy(str->str + ((str->len) ? str->len - 1 : 0), strings[count]->str, strings[count]->len);
		str->len += strings[count]->len;
	}
	// Terminate with null
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

int str_char_concat(struct str_t* str, const char const* value, size_t size)
{
	// Create array of size 1 with a str_t wrapping the passed char*
	const struct str_t const* array[] = { &(struct str_t) { .str = value, .len = size, .reserved = size }};
	return str_concat(str, 1, array);
}

int str_resize(struct str_t *str, size_t new_size)
{
	// We don't need to do anything if called on a smaller amount of space
	if(new_size <= str->reserved)
		return 0;

	char* result = (char*) realloc(str->str, sizeof(char) * new_size);
	if(!result)
		return 1;

	// If realloc() succeeded, set str and reserved size values
	str->str = result;
	str->reserved = new_size;

	return 0;
}

char urlunsafe[] =
{
	'$',
	'&',
	'+',
	',',
	'/',
	':',
	';',
	'=',
	'?',
	'@',
	' ',
	'"',
	'<',
	'>',
	'#',
	'%',
	'{',
	'}',
	'|',
	'\\',
	'^',
	'~',
	'[',
	']',
	'`'
};

/** Escapes unsafe characters for adding to a URI.
 *
 *  This function simply wraps a char* in a str_t* and calls str_urlencode_str.
 *
 *  @url the string to escape
 *
 *  @returns url encoded str or NULL if error
 */
struct str_t *str_urlencode_char (const char* url, size_t length)
{
	if(!length)
		length = strlen(url);
	const struct str_t tmp = { .str = url, .len = length, .reserved = length };
	return str_urlencode_str(&tmp);
}

/** Escapes unsafe characters for adding to a URI.
 *
 *  @url the string to escape
 *
 *  @returns url encoded str or NULL if error
 */
struct str_t *str_urlencode_str (const struct str_t const* url)
{
	size_t i;
	size_t j;
	size_t count = 0;
	size_t size = url->len;

	// Count the number of characters that need to be escaped
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			if(url->str[i] == urlunsafe[j])
			{
				++count;
				break;
			}
		}
	}

	// Allocate and correctly size the str
	struct str_t *result = (struct str_t*) malloc(sizeof(struct str_t));
	if(result == NULL)
		return NULL;
	str_init(result);
	str_resize(result, sizeof(char) * (size + count*2 + 1));

	// Copy old string into escaped string, escaping where necessary
	char *iter = result->str;
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			// We found a character that needs escaping
			if(url->str[i] == urlunsafe[j])
			{
				// Had a weird issue with sprintf(,"\%%02X,), so I do this instead
				*iter = '%';
				++iter;
				sprintf(iter, "%02X", urlunsafe[j]);
				iter+=2;
				break;
			}
		}
		// We did not need to escape the current character in url, so just copy it
		if(j == sizeof(urlunsafe))
		{
			*iter = url->str[i];
			++iter;
		}
	}

	// Calculate the size of the final string, should be the same as (length+count*2)
	size = iter - result->str;
	// Make sure we null terminate
	result->str[size] = 0;

	// Update result's length
	result->len = size;

	return result;
}
