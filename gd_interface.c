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
#include "stack.h"
#include "functional_stack.h"
#include "str.h"
#include "curl_interface.h"


const char auth_uri[] = "https://accounts.google.com/o/oauth2/auth";
const char token_uri[] = "https://accounts.google.com/o/oauth2/token";
//const char drive_scope[] = "https://www.googleapis.com/auth/drive.file";
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
 *  @length length of the url
 *          precondition:  strlen(url)
 *          postcondition: strlen(escaped url)
 *
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
	char *result = (char *) malloc( sizeof(char) * (size + count*3 + 1));
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
	result[size] = 0;
	// Save the new length
	*length = size;

	return result;
}

int create_oauth_header(struct gdi_state* state)
{
	union func_u func;
	int ret = 0;
	struct str_t oauth;
	struct str_t oauth_header;

	str_init(&oauth_header);
	str_init_create(&oauth, "Authorization: OAuth ");

	struct str_t* concat[2] = {&oauth, &state->access_token};

	str_init(&state->oauth_header);
	func.func3 = str_destroy;
	fstack_push(state->stack, &state->oauth_header, &func, 3);

	ret = str_concat(&state->oauth_header, 2, concat);
	if(ret)
		fstack_pop(state->stack);

	str_destroy(&oauth);
	str_destroy(&oauth_header);
	return ret;
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
	if(full_name == NULL)
	{
		perror("malloc");
		return NULL;
	}

	memset(full_name, 0, full_name_len);

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

	char *result = (char *) malloc(sizeof(char) * (size+1));
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
	result[size] = 0;

	char *to = result;
	char *from = result;
	for(; *from != 0; ++from)
	{
		*to = *from;
		if(*from != '\n')
			++to;
	}
	*to = 0;

	return result;
}

/** Callback for libcurl for parsing json auth tokens.
 *
 */
size_t curl_post_callback(void *data, size_t size, size_t nmemb, void *store)
{
	struct gdi_state *state = (struct gdi_state*) store;
	struct json_object *json = json_tokener_parse(data);
	union func_u func;

	struct json_object *tmp = json_object_object_get(json, "error");
	if(tmp != NULL)
	{
		fprintf(stderr, "Error: %s\n", json_object_get_string(tmp));
		state->callback_error = 1;
		return size*nmemb;
	}

	tmp = json_object_object_get(json, "access_token");
	const char* val = json_object_get_string(tmp);

	str_init_create(&state->access_token, val);
	func.func1 = str_destroy;
	fstack_push(state->stack, &state->access_token, &func, 1);

	tmp = json_object_object_get(json, "token_type");
	val = json_object_get_string(tmp);

	str_init_create(&state->token_type, val);
	func.func1 = str_destroy;
	fstack_push(state->stack, &state->token_type, &func, 1);

	tmp = json_object_object_get(json, "refresh_token");
	val = json_object_get_string(tmp);

	str_init_create(&state->refresh_token, val);
	func.func1 = free;
	fstack_push(state->stack, &state->refresh_token, &func, 1);

	tmp = json_object_object_get(json, "id_token");
	val = json_object_get_string(tmp);

	str_init_create(&state->id_token, val);
	func.func1 = free;
	fstack_push(state->stack, &state->id_token, &func, 1);

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
	union func_u func;

	struct stack_t *estack = (struct stack_t*)malloc(sizeof(struct stack_t));
	if(!estack)
		return 1;
	if(fstack_init(estack, 20))
		return 1;
	struct stack_t *gstack = (struct stack_t*)malloc(sizeof(struct stack_t));
	if(!gstack)
		return 1;
	if(fstack_init(gstack, 20))
		return 1;

	state->stack = estack;
	state->head = NULL;
	state->tail = NULL;
	state->callback_error = 0;
	state->num_files = 0;

	char *xdg_conf = getenv("XDG_CONFIG_HOME");
	char *pname = "/fuse-google-drive/";
	if(xdg_conf == NULL)
	{
		xdg_conf = getenv("HOME");
		pname = "/.config/fuse-google-drive/";
	}

	char *full_path = (char*) malloc(sizeof(char) * (strlen(xdg_conf) +
			 	strlen(pname) + 1));
	if(full_path == NULL)
		goto init_fail;
	func.func1 = free;
	fstack_push(gstack, full_path, &func, 1);

	memcpy(full_path, xdg_conf, strlen(xdg_conf) + 1);
	memcpy(full_path + strlen(xdg_conf), pname, strlen(pname) + 1);

	state->clientsecrets = gdi_load_clientsecrets(full_path, "clientsecrets");
	if(state->clientsecrets == NULL)
		goto init_fail;
	func.func1 = free;
	fstack_push(estack, state->clientsecrets, &func, 1);

	char redirecturi[] = "urn:ietf:wg:oauth:2.0:oob";
	state->redirecturi = (char*) malloc(sizeof(char) * sizeof(redirecturi));
	if(state->redirecturi == NULL)
		goto init_fail;
	func.func1 = free;
	fstack_push(estack, state->redirecturi, &func, 1);
	memcpy(state->redirecturi, redirecturi, sizeof(redirecturi));

	state->clientid = gdi_load_clientid(full_path, "clientid");
	if(state->clientid == NULL)
		goto init_fail;
	func.func1 = free;
	fstack_push(estack, state->clientid, &func, 1);

	if(curl_global_init(CURL_GLOBAL_SSL) != 0)
		goto init_fail;
	func.func2 = curl_global_cleanup;
	fstack_push(estack, NULL, &func, 2);

	state->curlmulti = curl_multi_init();
	if(state->curlmulti == NULL)
		goto init_fail;
	func.func3 = curl_multi_cleanup;
	fstack_push(estack, state->curlmulti, &func, 3);

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
		goto init_fail;
	func.func1 = free;
	fstack_push(gstack, complete_authuri, &func, 1);

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
	state->code = (char *) malloc(sizeof(char)*(length+1));
	if(state->code == NULL)
		goto init_fail;
	func.func1 = free;
	fstack_push(estack, state->code, &func, 1);

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

	if(i < 5)
	{
		printf("You did not enter a correct authentication code.\n");
		goto init_fail;
	}

	// Prepare and make the request to exchange the code for an access token
	CURL *auth_handle = curl_easy_init();
	if(auth_handle == NULL)
		goto init_fail;
	func.func1 = curl_easy_cleanup;
	fstack_push(gstack, auth_handle, &func, 1);

	// Using complete_authuri to store POST data rather than allocating more space
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
	//curl_easy_setopt(auth_handle, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(auth_handle, CURLOPT_USE_SSL, CURLUSESSL_ALL); // SSL
	curl_easy_setopt(auth_handle, CURLOPT_URL, token_uri); // set URI
	curl_easy_setopt(auth_handle, CURLOPT_POSTFIELDS, complete_authuri); // BODY
	// set curl_post_callback for parsing the server response
	curl_easy_setopt(auth_handle, CURLOPT_WRITEFUNCTION, curl_post_callback);
	// set curl_post_callback's last parameter to state
	curl_easy_setopt(auth_handle, CURLOPT_WRITEDATA, state);
	curl_easy_perform(auth_handle); // POST
	if(state->callback_error)
		goto init_fail;

	create_oauth_header(state);
	gdi_get_file_list(state);
	if(create_hash_table(state->num_files*4, state->head))
	{
		printf("create_hash failed\n");
		goto init_fail;
	}
	func.func2 = destroy_hash_table;
	fstack_push(estack, NULL, &func, 2);


	goto init_success;

init_fail:
	while(estack->size)
		fstack_pop(estack);
	while(gstack->size)
		fstack_pop(gstack);
	fstack_destroy(estack);
	free(estack);
	fstack_destroy(gstack);
	free(gstack);
	return 1;

init_success:
	while(gstack->size)
		fstack_pop(gstack);
	fstack_destroy(gstack);
	free(gstack);
	return 0;
}

void gdi_destroy(struct gdi_state* state)
{
	printf("Cleaning up...\n");
	fflush(stdout);

	struct gd_fs_entry_t *iter = state->head;
	struct gd_fs_entry_t *tmp = iter;
	while(iter != NULL)
	{
		gd_fs_entry_destroy(iter);
		iter = iter->next;
		free(tmp);
		tmp = iter;
	}

	while(state->stack->size)
		fstack_pop(state->stack);
	fstack_destroy(state->stack);
	free(state->stack);
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

/** Build linked list of files.
 *
 *  Calls the XML parsing code to get gd_fs_entry_ts for creating a list of files.
 *
 *  @xml   a string containing the xml to parse, this is what we get from curl
 *  @len   the length of xml
 *  @state the variable we store this mount's state in
 *
 *  @returns the link to the  next page of the directory listing, as found in xml
 */
struct str_t* xml_parse_file_list(struct str_t* xml, struct gdi_state *state)
{
	char* iter = strstr(xml->str, "<feed");
	xmlDocPtr xmldoc = xmlParseMemory(iter, xml->len - (iter - xml->str));

	xmlNodePtr node;

	size_t count = 0;
	struct gd_fs_entry_t *tmp = state->tail;

	struct str_t* next = NULL;

	if(xmldoc == NULL || xmldoc->children == NULL || xmldoc->children->children == NULL)
		return NULL;
	for(node = xmldoc->children->children; node != NULL; node = node->next)
	{
		if(strcmp(node->name, "entry") == 0)
		{
			if(!tmp)
			{
				tmp = gd_fs_entry_from_xml(xmldoc, node);
				state->head = tmp;
				state->tail = tmp;
			}
			else
			{
				tmp->next = gd_fs_entry_from_xml(xmldoc, node);
				tmp = tmp->next;
				state->tail = tmp;
			}
			++count;
		}
		if(strcmp(node->name, "link") == 0)
		{
			char *prop = xmlGetProp(node, "rel");
			if(prop != NULL)
			{
				if(strcmp(prop, "next") == 0)
				{
					next = (struct str_t*) malloc(sizeof(struct str_t));
					str_init(next);
					next->str = xmlGetProp(node, "href");
					next->len = strlen(next->str);
				}
			}
		}
	}
	xmlFreeDoc(xmldoc);
	state->num_files += count;
	return next;
}

/** Gets a listing of all the files for this mount.
 *
 *  Calls curl repeatedly to get each page of xml from the directory-listing
 *  Google API. Then parses those pages into a list of gd_fs_entry_ts, stored
 *  in state.
 *
 *  @state the state for this mount
 *
 *  @returns void
 */
void gdi_get_file_list(struct gdi_state *state)
{
	struct str_t resp;
	resp.len = 0;
	resp.str = NULL;

	struct str_t uri;
	str_init_create(&uri, "https://docs.google.com/feeds/default/private/full?v=3&showfolders=true&max-results=1000");

	struct str_t* next = NULL;

	struct request_t request;
	ci_init(&request, &uri, 1, &state->oauth_header, GET);
	do
	{

		ci_request(&request);

		str_destroy(next);
		free(next);
		next = xml_parse_file_list(&request.response.body, state);
		if(next)
			ci_set_uri(&request, next);

		ci_clear_response(&request);
	} while(next);

	ci_destroy(&request);
	str_destroy(&uri);
}

const char* gdi_strip_path(const char* path)
{
	char *filename = strrchr(path, '/');
	++filename;
	return filename;
}

int gdi_check_update(struct gdi_state* state, struct gd_fs_entry_t* entry)
{
	int ret = 0;

	if(entry->md5set)
	{
		struct request_t request;
		ci_init(&request, &entry->feed, 1, &state->oauth_header, GET);
		ci_request(&request);

		struct str_t* md5 = xml_get_md5sum(&request.response.body);
		if(md5 == NULL)
			ret = -1;
		if(!ret && strcmp(md5->str, entry->md5.str))
			ret = 1;

		ci_destroy(&request);
	}

	return ret;
}

int gdi_load(struct gdi_state* state, struct gd_fs_entry_t* entry)
{
	int ret = 0;
	if(entry->cached)
	{
		int updated = gdi_check_update(state, entry);
		switch(updated)
		{
			case -1:
				ret = 1;
				break;
			case 0:
				ret = 0;
				break;
			case 1:
				struct request_t request;
				ci_init(&request, &entry->src, 1, &state->oauth_header, GET);
				ci_request(&request);

				str_swap(&request.response.body, &entry->cache);

				ci_destroy(&request);
				break;
		}
	}
	else
	{
		struct request_t request;
		ci_init(&request, &entry->src, 1, &state->oauth_header, GET);
		ci_request(&request);

		str_swap(&request.response.body, &entry->cache);

		ci_destroy(&request);
		entry->cached = 1;
	}

	return ret;
}

const char* gdi_read(size_t *size, struct gd_fs_entry_t* entry, off_t offset)
{
	size_t remaining = (entry->cache.len < offset) ? 0 : entry->cache.len - offset;
	*size = (remaining < *size) ? remaining : *size;
	if(*size == 0)
		return NULL;
	return entry->cache.str + offset;
}
