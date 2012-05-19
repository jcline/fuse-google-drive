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

#include <errno.h>
#include <stdio.h>
#include <search.h>
#include <string.h>

#include "gd_cache.h"
#include "str.h"

char filenameunsafe[] = 
{
	'%',
	'/'
};

/** Escapes unsafe characters for filenames.
 *
 *  @filename the string to escape
 *  @length length of filename
 *          precondition:  strlen(filename)
 *          postcondition: strlen(escaped filename)
 *
 *  @returns escaped, null terminated string
 */
char* filenameencode (const char *filename, size_t *length)
{
	size_t i;
	size_t j;
	size_t count = 0;
	size_t size = *length;
	// Count the number of characters that need to be escaped
	for(i = 0; i < size; ++i)
	{
		for(j = 0; j < sizeof(filenameunsafe); ++j)
		{
			if(filename[i] == filenameunsafe[j])
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
		for(j = 0; j < sizeof(filenameunsafe); ++j)
		{
			// We found a character that needs escaping
			if(filename[i] == filenameunsafe[j])
			{
				// Had a weird issue with sprintf(,"\%%02X,), so I do this instead
				*iter = '%';
				++iter;
				sprintf(iter, "%02X", filenameunsafe[j]);
				iter+=2;
				break;
			}
		}
		// We did not need to escape the current character in filename, so just copy it
		if(j == sizeof(filenameunsafe))
		{
			*iter = filename[i];
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

/** Creates and fills in a gd_fs_entry_t from an <entry>...</entry> in xml.
 *
 *  @xml  the xml containing the entry
 *  @node the node representing this <entry>...</entry> block
 *
 *  @returns pointer to gd_fs_entry_t with fields filled in as needed
 */
struct gd_fs_entry_t* gd_fs_entry_from_xml(xmlDocPtr xml, xmlNodePtr node)
{
	struct gd_fs_entry_t* entry;

	entry = (struct gd_fs_entry_t*) malloc(sizeof(struct gd_fs_entry_t));
	if(entry == NULL) {} // TODO: ERROR
	memset(entry, 0, sizeof(struct gd_fs_entry_t));

	size_t length;
	xmlNodePtr c1, c2;
	xmlChar *value = NULL;

	for(c1 = node->children; c1 != NULL; c1 = c1->next)
	{
		char const *name = c1->name;
		switch(*name)
		{
			case 'a': // 'author'
				for(c2 = c1->children; c2 != NULL; c2 = c2->next)
				{
					name = c2->name;
					value = xmlNodeListGetString(xml, c2->children, 1);
					switch(*name)
					{
						case 'n':
							str_init_create(&entry->author, value);
							break;
						case 'e':
							str_init_create(&entry->author_email, value);
							break;
						default:
							break;
					}
					xmlFree(value);
				}
				break;
			case 'c':
				if(strcmp(name, "content") == 0)
				{
					value = xmlGetProp(c1, "src");
					str_init_create(&entry->src, value);
					xmlFree(value);
				}
				break;
			case 'f':
				if(strcmp(name, "feedlink") == 0)
				{
					value = xmlGetProp(c1, "rel");
					if(strcmp(value, "http://schemas.google.com/acl/2007#accessControlList") == 0)
					{
						// Link for r/w access to ACLS for this entry
						// Do we care?
						// Can we expose this?
					}
					else if(strcmp(value, "http://schemas.google.com/docs/2007/revisions") == 0)
					{
						// Link for r/w access to revisions
						// It would be cool if we can expose these somehow
					}
				}
				break;
			case 'l':
				if(strcmp(name, "lastModifiedBy") == 0)
				{
					for(c2 = c1->children; c2 != NULL; c2 = c2->next)
					{
						name = c2->name;
						value = xmlNodeListGetString(xml, c2->children, 1);
						switch(*name)
						{
							case 'n':
								str_init_create(&entry->lastModifiedBy, value);
								break;
							case 'e':
								str_init_create(&entry->lastModifiedBy_email, value);
								break;
							default:
								break;
						}
						xmlFree(value);
					}
				}
				else if(strcmp(name, "link") == 0)
				{
					value = xmlGetProp(c1, "rel");
					if(strcmp(value, "http://schemas.google.com/docs/2007#parent") == 0)
					{
						// This entry is inside one (or more?) collections
						// These entries are the folders for this entry
					}
					else if(strcmp(value, "alternate") == 0)
					{
						// Link you can open this document in a web browser with
						// Do we care?
					}
					else if(strcmp(value, "self") == 0)
					{
						// Link to XML feed for just this entry
						// Might be useful for checking for updates instead of changesets
					}
					else if(strcmp(value, "edit") == 0)
					{
						// For writes?
					}
					/*
					else if(strcmp(value, "edit-media") == 0)
					{
						// deprecated, use 'resumeable'
					}
					*/
					else if(strcmp(value, "http://schemas.google.com/g/2005#resumable-edit-media") == 0)
					{
						// For resumeable writes?
						// This may be the one we *really* want to use, rather than 'edit'
					}
					else if(strcmp(value, "http://schemas.google.com/docs/2007/thumbnail") == 0)
					{
						// Might be a useful way to expose this for GUI file managers?
					}
				}
				break;
			case 'm':
				if(strcmp(name, "md5Checksum") == 0)
				{
					value = xmlNodeListGetString(xml, c1->children, 1);
					entry->md5set = 1;
					str_init_create(&entry->md5, value);
					xmlFree(value);
				}
				break;
			case 't': // 'title'
				if(strcmp(name, "title") == 0)
				{
					value = xmlNodeListGetString(xml, c1->children, 1);
					str_init_create(&entry->filename, value);
					entry->filename.str = filenameencode(value, &entry->filename.len);
					entry->filename.reserved = entry->filename.len;
					xmlFree(value);
				}
				break;
			case 's':
				if(strcmp(name, "size") == 0)
				{
					value = xmlNodeListGetString(xml, c1->children, 1);
					length = xmlStrlen(value);
					// TODO: errors?
					entry->size = strtol((char*)value, NULL, 10);
					xmlFree(value);
				}
				break;
			default:
				break;
		}
	}

	return entry;
}

void gd_fs_entry_destroy(struct gd_fs_entry_t* entry)
{
	str_destroy(&entry->author);
	str_destroy(&entry->author_email);
	str_destroy(&entry->lastModifiedBy);
	str_destroy(&entry->lastModifiedBy_email);
	str_destroy(&entry->filename);
	str_destroy(&entry->src);
	str_destroy(&entry->cache);
	str_destroy(&entry->md5);
}

/** Searches hash table for a filename.
 *
 *  @key the name of the file to find
 *
 *  @returns the gd_fs_entry_t representing that file
 */
struct gd_fs_entry_t* gd_fs_entry_find(const char* key)
{
	ENTRY keyentry;
	keyentry.key = key;
	keyentry.data = NULL;
	ENTRY* entry = hsearch(keyentry, FIND);
	if(entry == NULL)
		return NULL;
	return (struct gd_fs_entry_t*) entry->data;
}

/** Creates the hash table for speeding up file finding.
 *
 *  @size the size we want to make this hash table
 *  @head the first gd_fs_entry_t in the list of files
 *
 *  @returns 0 on success, 1 on failure
 */
int create_hash_table(size_t size, const struct gd_fs_entry_t* head)
{
	int ret = hcreate(size);
	if(!ret)
	{
		fprintf(stderr, "hcreate failed\n");
		return 1;
	}

	ENTRY entry;
	struct gd_fs_entry_t *iter = head;
	while(iter != NULL)
	{
		entry.key = iter->filename.str;
		entry.data = iter;

		ENTRY* entered = hsearch(entry, ENTER);
		if(0 && !entered)
		{
			fprintf(stderr, "hsearch: %s\n", strerror(errno));
			destroy_hash_table();
			return 1;
		}

		iter = iter->next;
	}

	return 0;
}

void destroy_hash_table()
{
	hdestroy();
}

