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
#include <sys/stat.h> // mkdir
#include <unistd.h>
#include <libxml/tree.h>

#include "gd_interface.h"
#include "gd_cache.h"

struct str_t {
	char *str;
	size_t len;
};

const char auth_uri[] = "https://accounts.google.com/o/oauth2/auth";
const char token_uri[] = "https://accounts.google.com/o/oauth2/token";
const char drive_scope[] = "https://www.googleapis.com/auth/drive.file";
const char email_scope[] = "https://www.googleapis.com/auth/userinfo.email";
const char docs_scope[] = "https://docs.google.com/feeds/";
const char docsguc_scope[] = "https://docs.googleusercontent.com/";
const char spreadsheets_scope[] = "https://spreadsheets.google.com/feeds/";
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

/** Escapes unsafe characters for adding to a URI.
 *
 *  @url the string to escape
 *  @length
 *          precondition:  strlen(url)
 *          postcondition: strlen(escaped url)
 *  @returns escaped, null terminated string
 */
char* urlencode (const char *url, size_t *length)
{
	size_t i;
	size_t j;
	size_t count = 0;
	size_t size = *length;
	// Count the number of characters that need to be escaped
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

	// Allocate the correct amount of memory for the escaped string
	char *result = (char *) malloc( sizeof(char) * (size + count*3));
	if(result == NULL)
		return NULL;

	// Copy old string into escaped string, escaping where necessary
	char *iter = result;
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < sizeof(urlunsafe); ++j)
		{
			// We found a character that needs escaping
			if(url[i] == urlunsafe[j])
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
			*iter = url[i];
			++iter;
		}
	}

	// Calculate the size of the final string, should be the same as (length+count*3)
	size = iter - result;
	// Make sure we null terminate
	result[size-1] = 0;
	// Save the new length
	*length = size;

	return result;
}


int gdi_get_credentials()
{
	return 0;
}

char* load_file(const char* path, const char* name)
{
	size_t pathlen = strlen(path);
	size_t namelen = strlen(name);
	size_t full_name_len = sizeof(char) * (pathlen + namelen) + 1;
	char *full_name = (char*) malloc(full_name_len);

	memset(full_name, 0, full_name_len);

	if(full_name == NULL)
	{
		perror("malloc");
		return NULL;
	}

	memcpy(full_name, path, pathlen);
	memcpy(full_name + pathlen, name, namelen);

	FILE *f = fopen(full_name, "rb");
	if(f == NULL)
	{
		printf("fopen(\"%s\"): %s\n", full_name, strerror(errno));
		return NULL;
	}

	// Find the size of the file
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	rewind(f);

	char *result = (char *) malloc(sizeof(char) * size);
	if(result == NULL)
	{
		perror("malloc");
		return NULL;
	}

	// Read the file in one chunk, possible issue for larger files
	fread(result, 1, size, f);
	if(ferror(f))
	{
		printf("fread(\"%s\"): %s\n", full_name, strerror(errno));
		return NULL;
	}

	fclose(f);

	// Strip trailing newlines
	if(result[size-1] == '\n')
		result[size-1] = 0;

	return result;
}

/** Callback for libcurl for parsing json auth tokens.
 *
 */
size_t curl_post_callback(void *data, size_t size, size_t nmemb, void *store)
{
	struct gdi_state *state = (struct gdi_state*) store;
	struct json_object *json = json_tokener_parse(data);
	
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

	// libcurl expects this to "read" as much data as it received from the server
	// since json-c is doing all the work here, unless it errors we may as well
	// return the size libcurl gave us for the data
	return size*nmemb;
}

void print_api_info(const char* path)
{
	printf("If you are seeing this then fuse-google-drive was unable to ");
	printf("find the files it needs in $XDG_CONFIG_HOME/fuse-google-drive/. ");
	printf("You need to go to https://code.google.com/apis/console/ then login ");
	printf("and create an installed application project. Select the Google Drive ");
	printf("API and generate the necessary tokens. Then, place the clientsecrets ");
	printf("code in $XDG_CONFIG_HOME/fuse-google-drive/clientsecrets and place ");
	printf("the clientid in the same folder, but in the file 'clientid'.\n");

	int ret = mkdir(path, S_IRWXU);
	if(ret == -1)
	{
		if(errno == ENOENT)
		{
			char *home = getenv("HOME");
			char *conf = "/.config/";
			char *full_path = (char*) malloc(sizeof(char) * (strlen(home) + strlen(conf)));
			memcpy(full_path, home, strlen(home));
			memcpy(full_path + strlen(home), conf, strlen(conf));
			printf("%s\n", full_path);
			ret = mkdir(full_path, S_IRWXU);
			if(ret == 0)
			{
				ret = mkdir(path, S_IRWXU);
				if(ret)
					rmdir(full_path);
			}
			free(full_path);
		}
	}
	if(ret == 0)
	{
		printf("\nWe have created the folder %s for you to place these files.", path);
		printf("Please ensure there are no newlines or whitespace before or after ");
		printf("the codes in these files.\n");
	}


}

/** Reads in the clientsecrets from a file.
 *
 */
char* gdi_load_clientsecrets(const char *path, const char *name)
{
	char *ret = load_file(path, name);
	if(ret == NULL)
		print_api_info(path);
	return ret;
}

/** Reads in the redirection URI from a file.
 *
 */
char* gdi_load_redirecturi(const char *path, const char *name)
{
	return load_file(path, name);
}

/** Reads in the client id from a file.
 *
 */
char* gdi_load_clientid(const char *path, const char *name)
{
	return load_file(path, name);
}

/** Concatenate an urlencoded string with buf.
 *
 */
size_t add_encoded_uri(char *buf, const char *uri, const size_t size)
{
	size_t length = size;
	char *encoded = urlencode(uri, &length);
	memcpy(buf, encoded, length);
	free(encoded);
	return length-1;
}

/** Effectively strcat() that doesn't need to call strlen(buf).
 *
 */
size_t add_unencoded_str(char *buf, const char *str, const size_t size)
{
	memcpy(buf, str, size);
	return size-1;
}

int gdi_init(struct gdi_state* state)
{
	state->head = NULL;

	char *xdg_conf = getenv("XDG_CONFIG_HOME");
	char *pname = "/fuse-google-drive/";
	if(xdg_conf == NULL)
	{
		xdg_conf = getenv("HOME");
		pname = "/.config/fuse-google-drive/";
	}

	char *full_path = (char*) malloc(sizeof(char) * (strlen(xdg_conf) + strlen(pname)));
	//TODO error
	memcpy(full_path, xdg_conf, strlen(xdg_conf));
	memcpy(full_path + strlen(xdg_conf), pname, strlen(pname));

	state->clientsecrets = gdi_load_clientsecrets(full_path, "clientsecrets");
	if(state->clientsecrets == NULL)
		goto init_fail;

	//state->redirecturi = gdi_load_redirecturi(full_path, "redirecturi");
	state->redirecturi = "urn:ietf:wg:oauth:2.0:oob";
	if(state->redirecturi == NULL)
		goto malloc_fail1;

	state->clientid = gdi_load_clientid(full_path, "clientid");
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

	iter += add_encoded_uri(iter, docs_scope, sizeof(docs_scope));
	*iter = '+';
	++iter;

	iter += add_encoded_uri(iter, docsguc_scope, sizeof(docsguc_scope));
	*iter = '+';
	++iter;

	iter += add_encoded_uri(iter, spreadsheets_scope, sizeof(spreadsheets_scope));
	*iter = '+';
	++iter;

	iter += add_encoded_uri(iter, drive_scope, sizeof(drive_scope));

	iter += add_unencoded_str(iter, redirect, sizeof(redirect));

	iter += add_encoded_uri(iter, state->redirecturi, strlen(state->redirecturi)+1);

	iter += add_unencoded_str(iter, clientid, sizeof(clientid));

	iter += add_unencoded_str(iter, state->clientid, strlen(state->clientid)+1);

	printf("Please open this in a web browser and authorize fuse-google-drive:\n%s\n", complete_authuri);

	printf("\n\nOnce you authenticate, Google should give you a code, please paste it here:\n");

	// Read in the code from Google
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

	code[i] = 0; // Null terminate code
	if(i!=30) // Is the code actually always this length?
	{
		printf("The code you entered, %s, is not the right length. Please retry mounting.\n", code);
		goto code_fail7;
	}

	// Prepare and make the request to exchange the code for an access token
	CURL *auth_handle = curl_easy_init();
	if(auth_handle == NULL)
		goto curl_fail7;

	// Using complete_authuri to store POST data
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

	// TODO: errors
	curl_easy_setopt(auth_handle, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL
	curl_easy_setopt(auth_handle, CURLOPT_URL, token_uri); // set URI
	curl_easy_setopt(auth_handle, CURLOPT_POSTFIELDS, complete_authuri); // BODY
	// set curl_post_callback for parsing the server response
	curl_easy_setopt(auth_handle, CURLOPT_WRITEFUNCTION, curl_post_callback);
	// set curl_post_callback's last parameter to state
	curl_easy_setopt(auth_handle, CURLOPT_WRITEDATA, state);
	curl_easy_perform(auth_handle); // POST
	curl_easy_cleanup(auth_handle); // cleanup

	printf("%s\n", state->access_token);

	gdi_get_file_list("/", state);


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
	free(full_path);

init_fail:
	return 1;

init_success:
	free(complete_authuri);
	free(full_path);
	return 0;
}

void gdi_destroy(struct gdi_state* state)
{
	free(state->clientsecrets);
	free(state->clientid);
	free(state->code);
	//TODO: make sure all easy handles were first removed
	curl_multi_cleanup(state->curlmulti);
	curl_global_cleanup();
}

size_t remove_newlines(char *str, size_t length)
{
	size_t scan_iter, move_iter;
	for(scan_iter = 0, move_iter = 0; scan_iter < length; ++scan_iter)
	{
		if(scan_iter == '\0')
		{
			str[move_iter] = 0;
			break;
		}
		else if(scan_iter != '\n')
		{
			str[move_iter] = str[scan_iter];
			++move_iter;
		}
	}

	return move_iter+1;
}

size_t curl_get_list_callback(void *data, size_t size, size_t nmemb, void *store)
{

	struct str_t *resp = (struct str_t*) store;
	resp->str = (char*) realloc(resp->str, resp->len + size*nmemb);
	memcpy(resp->str + resp->len, data, size*nmemb);
	resp->len += size*nmemb;

	return size*nmemb;
}

void gdi_get_file_list(const char *path, struct gdi_state *state)
{
	struct str_t resp;
	resp.len = 0;
	resp.str = NULL;

	char **list;
	char u[] = "https://docs.google.com/feeds/default/private/full?v=3&showfolders=true";
	char oauth_str[] = "Authorization: OAuth ";
	char *header_str = (char*) malloc(sizeof(char) * (strlen(state->access_token) +
				30));
	char *iter = header_str;
	iter += add_unencoded_str(iter, oauth_str, sizeof(oauth_str));
	iter += add_unencoded_str(iter, state->access_token, strlen(state->access_token));

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, header_str);

	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_VERBOSE,1);
	curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL
	curl_easy_setopt(handle, CURLOPT_URL, u); // set URI
	curl_easy_setopt(handle, CURLOPT_HEADER, 1); // Enable headers, necessary?
	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers); // Set headers
	// set curl_post_callback for parsing the server response
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_get_list_callback);
	// set curl_post_callback's last parameter to state
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &resp);


	curl_easy_perform(handle); // GET
	curl_easy_cleanup(handle); // cleanup
	curl_slist_free_all(headers);
	free(header_str);

	iter = strstr(resp.str, "<feed");
	//if(iter == NULL)
	// TODO: errors

	xmlDocPtr xmldoc = xmlParseMemory(iter, resp.len - (iter - resp.str));

	xmlNodePtr node;

	struct gd_fs_entry_t *tmp = NULL;

	for(node = xmldoc->children->children; node != NULL; node = node->next)
	{
		if(strcmp(node->name, "entry") == 0)
		{
			if(!tmp)
			{
				tmp = gd_fs_entry_from_xml(xmldoc, node);
				state->head = tmp;
			}
			else
			{
				tmp->next = gd_fs_entry_from_xml(xmldoc, node);
				tmp = tmp->next;
			}
		}
	}
}
