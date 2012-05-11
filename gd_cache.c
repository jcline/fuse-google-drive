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
		char *name = c1->name;
		switch(*name)
		{
			case 'a': // 'author'
				for(c2 = c1->children; c2 != NULL; c2 = c2->next)
				{
					name = c2->name;
					value = xmlNodeListGetString(xml, c2->children, 1);
					length = xmlStrlen(value);
					switch(*name)
					{
						case 'n':
							entry->author = (char*) malloc(sizeof(char)*length);
							memcpy(entry->author, value, length);
							break;
						case 'e':
							entry->author_email = (char*) malloc(sizeof(char)*length);
							memcpy(entry->author_email, value, length);
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
				}
				break;
			case 'g': // 'gd:*'
				// This doesn't seem to be working right
				if(0 && strcmp(name, "gd:lastModifiedBy") == 0)
				{
					for(c2 = c1->children; c2 != NULL; c2 = c2->next)
					{
						name = c2->name;
						value = xmlNodeListGetString(xml, c2->children, 1);
						length = xmlStrlen(value);
						switch(*name)
						{
							case 'n':
								entry->lastModifiedBy = (char*) malloc(sizeof(char)*length);
								memcpy(entry->lastModifiedBy, value, length);
								break;
							case 'e':
								entry->lastModifiedBy_email = (char*) malloc(sizeof(char)*length);
								memcpy(entry->lastModifiedBy_email, value, length);
								break;
							default:
								break;
						}
						xmlFree(value);
					}
				}
				break;
			case 't': // 'title'
				value = xmlNodeListGetString(xml, c1->children, 1);
				length = xmlStrlen(value);
				entry->filename = filenameencode(value, &length);
				xmlFree(value);
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
	return (struct gd_fs_entry*) entry->data;
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
		printf("hcreate failed\n");
		return 1;
	}

	ENTRY entry;
	struct gd_fs_entry_t *iter = head;
	while(iter != NULL)
	{
		entry.key = iter->filename;
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

