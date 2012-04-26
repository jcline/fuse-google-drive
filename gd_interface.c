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

#include <curl/curl.h>
#include <curl/multi.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "gd_interface.h"

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
	'`',
	'\0'
};

char* urlencode (const char *url, size_t length)
{
	size_t i;
	size_t j;
	size_t count = 0;
	for(i = 0; i < length; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			if(url[i] == url[j])
			{
				++count;
				break;
			}
		}
	}

	char *result = (char *) malloc( sizeof(char) * (length + count*3));
	if(result == NULL)
		return NULL;

	char *iter = result;
	for(i = 0; i < length; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			if(url[i] == urlunsafe[j])
			{
				*iter = '%';
				++iter;
				sprintf(iter, "%02X", urlunsafe[j]);
				iter+=2;
				break;
			}
		}
		if(j == sizeof(urlunsafe))
		{
			*iter = url[i];
			++iter;
		}
	}

	result[(length+count*3)-1] = 0;
	return result;
}


int gdi_get_credentials()
{
	return 0;
}

char* load_file(const char* path)
{
	FILE *f = fopen(path, "rb");
	if(f == NULL)
	{
		printf("fopen(\"%s\"): %s\n", path, strerror(errno));
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);
	char *result = (char *) malloc(sizeof(char) * size);
	if(result == NULL)
	{
		perror("malloc");
		return NULL;
	}

	fread(result, 1, size, f);
	if(ferror(f))
	{
		printf("fread(\"%s\"): %s\n", path, strerror(errno));
		return NULL;
	}

	return result;
}

/** Reads in the clientsecrets from a file.
 *
 */
char* gdi_load_clientsecrets(const char *path)
{
	return load_file(path);
}

/** Reads in the redirection URI from a file.
 *
 */
char* gdi_load_redirecturi(const char *path)
{
	return load_file(path);
}

int gdi_init(struct gdi_state* state)
{
	state->clientsecrets = gdi_load_clientsecrets("clientsecrets");
	if(state->clientsecrets == NULL)
		return 1;

	state->redirecturi = gdi_load_redirecturi("redirecturi");
	if(state->redirecturi == NULL)
	{
		free(state->clientsecrets);
		return 1;
	}

	state->curlmulti = curl_multi_init();
	if(state->curlmulti == NULL)
	{
		free(state->clientsecrets);
		free(state->redirecturi);
		return 1;
	}

	return 0;
}

void gdi_destroy(struct gdi_state* state)
{
	free(state->clientsecrets);
	free(state->redirecturi);
}
