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

const char auth_uri[] = "https://accounts.google.com/o/oauth2/auth";
const char token_uri[] = "https://accounts.google.com/o/oauth2/token";
const char drive_scope[] = "https://www.googleapis.com/auth/drive.file";
const char email_scope[] = "https://www.googleapis.com/auth/userinfo.email";
const char profile_scope[] = "https://www.googleapis.com/auth/userinfo.profile";

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

char* urlencode (const char *url, size_t *length)
{
	size_t i;
	size_t j;
	size_t count = 0;
	size_t size = *length;
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			if(url[i] == urlunsafe[j])
			{
				++count;
				break;
			}
		}
	}

	char *result = (char *) malloc( sizeof(char) * (size + count*3));
	if(result == NULL)
		return NULL;

	char *iter = result;
	for(i = 0; i < size; ++i)
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

	size = iter - result;
	result[size] = 0;
	*length = size;
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

	fclose(f);

	// Strip trailing newlines
	if(result[size-1] == '\n')
		result[size-1] = 0;

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

/** Reads in the client id from a file.
 *
 */
char* gdi_load_clientid(const char *path)
{
	return load_file(path);
}

size_t add_encoded_uri(char *buf, const char *uri, const size_t size)
{
	size_t length = size;
	char *encoded = urlencode(uri, &length);
	memmove(buf, encoded, length);
	free(encoded);
	return length-1;
}

size_t add_unencoded_str(char *buf, const char *str, const size_t size)
{
	memmove(buf, str, size);
	return size-1;
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

	state->clientid = gdi_load_clientid("clientid");
	if(state->clientid == NULL)
	{
		free(state->clientsecrets);
		free(state->redirecturi);
		return 1;
	}

	state->curlmulti = curl_multi_init();
	if(state->curlmulti == NULL)
	{
		free(state->clientsecrets);
		free(state->redirecturi);
		free(state->clientid);
		return 1;
	}

	/* Authenticate the application */
	char response_type[] = "?response_type=code";
	char redirect[] = "&redirect_uri=";
	char scope[] = "&scope=";
	char clientid[] = "&client_id=";

	// Calculating the correct value is more trouble than it is worth
	char *complete_authuri = (char *) malloc(sizeof(char) * 3000);
	char *iter = complete_authuri;

	// Create the authentication request URI
	// There might be a cleaner way to do this...
	iter += add_unencoded_str(iter, auth_uri, sizeof(auth_uri));
	iter += add_unencoded_str(iter, response_type, sizeof(response_type));
	iter += add_unencoded_str(iter, scope, sizeof(scope));

	iter += add_encoded_uri(iter, email_scope, sizeof(email_scope));

	*iter = '+';
	++iter;

	iter += add_encoded_uri(iter, profile_scope, sizeof(profile_scope));

	*iter = '+';
	++iter;

	iter += add_encoded_uri(iter, drive_scope, sizeof(drive_scope));

	iter += add_unencoded_str(iter, redirect, sizeof(redirect));

	iter += add_encoded_uri(iter, state->redirecturi, strlen(state->redirecturi)+1);

	iter += add_unencoded_str(iter, clientid, sizeof(clientid));

	iter += add_unencoded_str(iter, state->clientid, strlen(state->clientid)+1);

	printf("Please open this in a web browser and authorize fuse-google-drive:\n%s\n", complete_authuri);
	free(complete_authuri);

	return 0;
}

void gdi_destroy(struct gdi_state* state)
{
	free(state->clientsecrets);
	free(state->redirecturi);
}
