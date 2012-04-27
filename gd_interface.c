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
#include <json.h>
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
	result[size-1] = 0;
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

size_t curl_post_callback(void *data, size_t size, size_t nmemb, void *store)
{
	struct gdi_state *state = (struct gdi_state*) store;
	struct json_object *json = json_tokener_parse(data);
	printf("json: %s\n", json_object_to_json_string(json)); 
	
	// TODO: Errors
	struct json_object *tmp = json_object_object_get(json, "access_token");
	state->access_token = json_object_get_string(tmp);
	
	tmp = json_object_object_get(json, "token_type");
	state->token_type = json_object_get_string(tmp);

	tmp = json_object_object_get(json, "refresh_token");
	state->refresh_token = json_object_get_string(tmp);

	tmp = json_object_object_get(json, "id_token");
	state->id_token = json_object_get_string(tmp);

	tmp = json_object_object_get(json, "expires_in");
	state->token_expiration = json_object_get_int(tmp);

	return size*nmemb;
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
		goto init_fail;

	state->redirecturi = gdi_load_redirecturi("redirecturi");
	if(state->redirecturi == NULL)
		goto malloc_fail1;

	state->clientid = gdi_load_clientid("clientid");
	if(state->clientid == NULL)
		goto malloc_fail2;

	if(curl_global_init(CURL_GLOBAL_SSL) != 0)
		goto curl_init_fail3;

	state->curlmulti = curl_multi_init();
	if(state->curlmulti == NULL)
		goto multi_init_fail4;

	/* Authenticate the application */
	char response_type_code[] = "?response_type=code";
	char codeparameter[] = "code=";
	char redirect[] = "&redirect_uri=";
	char scope[] = "&scope=";
	char clientid[] = "&client_id=";
	char clientsecret[] = "&client_secret=";
	char granttype[] = "&grant_type=authorization_code";

	// Calculating the correct value is more trouble than it is worth
	char *complete_authuri = (char *) malloc(sizeof(char) * 3000);
	if(complete_authuri == NULL)
		goto malloc_fail5;
	char *iter = complete_authuri;

	// Create the authentication request URI
	// There might be a cleaner way to do this...
	iter += add_unencoded_str(iter, auth_uri, sizeof(auth_uri));
	iter += add_unencoded_str(iter, response_type_code, sizeof(response_type_code));
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
	//memset((void*)auth_uri, 0, iter - auth_uri);

	printf("\n\nOnce you authenticate, Google should give you a code, please paste it here:\n");

	size_t length = 45;
	size_t i = 0;
	size_t done = 0;
	state->code = (char *) malloc(sizeof(char)*length);
	if(state->code == NULL)
		goto malloc_fail6;
	char *code = state->code;
	while(i < length && !done)
	{
		char c = getc(stdin);
		switch(c)
		{
			case ' ':
				break;
			case '\t':
				break;
			case '\n':
				done = 1;
				break;
			default:
				code[i] = c;
				++i;
				break;
		}
	}

	code[i] = 0;
	if(i!=30) // Is the code actually always this length?
	{
		printf("The code you entered, %s, is not the right length. Please retry mounting.\n", code);
		goto code_fail7;
	}

	CURL *auth_handle = curl_easy_init();
	if(auth_handle == NULL)
		goto curl_fail7;

	// TODO: errors
	curl_easy_setopt(auth_handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);

	// Using complete_authuri to store post data
	iter = complete_authuri;
	iter += add_unencoded_str(iter, codeparameter, sizeof(codeparameter));
	iter += add_encoded_uri(iter, state->code, strlen(state->code)+1);
	iter += add_unencoded_str(iter, clientid, sizeof(clientid));
	iter += add_encoded_uri(iter, state->clientid, strlen(state->clientid)+1);
	iter += add_unencoded_str(iter, clientsecret, sizeof(clientsecret));
	iter += add_encoded_uri(iter, state->clientsecrets, strlen(state->clientsecrets)+1);
	iter += add_unencoded_str(iter, redirect, sizeof(redirect));
	iter += add_encoded_uri(iter, state->redirecturi, strlen(state->redirecturi)+1);
	iter += add_unencoded_str(iter, granttype, sizeof(granttype));
	printf("\n%s\n", complete_authuri);

	curl_easy_setopt(auth_handle, CURLOPT_URL, token_uri);
	printf("%s\n", auth_uri);
	curl_easy_setopt(auth_handle, CURLOPT_POSTFIELDS, complete_authuri);
	curl_easy_setopt(auth_handle, CURLOPT_WRITEFUNCTION, curl_post_callback);
	curl_easy_setopt(auth_handle, CURLOPT_WRITEDATA, state);
	curl_easy_perform(auth_handle);
	curl_easy_cleanup(auth_handle);


	goto init_success;
// These labels are the name of what failed followed by number of things to clean
curl_fail7:
code_fail7:
	free(state->code);
malloc_fail6: // malloc state->code failed
	free(complete_authuri);
malloc_fail5: // malloc complete_authuri failed
	curl_multi_cleanup(state->curlmulti);
multi_init_fail4: // curl_multi_init() failed
	curl_global_cleanup();
curl_init_fail3:
	free(state->clientid);
malloc_fail2: // malloc clientid failed
	free(state->redirecturi);
malloc_fail1: // malloc redirecturi failed
	free(state->clientsecrets);

init_fail:
	return 1;

init_success:
	free(complete_authuri);
	return 0;
}

void gdi_destroy(struct gdi_state* state)
{
	free(state->clientsecrets);
	free(state->redirecturi);
	free(state->clientid);
	free(state->code);
	//TODO: make sure all easy handles were first removed
	curl_multi_cleanup(state->curlmulti);
	curl_global_cleanup();
}
