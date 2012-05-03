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
				entry->filename = (char*) malloc(sizeof(char*)*length);
				memcpy(entry->filename, value, length);
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

	//printf("%s\n", entry->filename);

	return entry;
}

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

int create_hash_table(size_t size, const struct gd_fs_entry_t* head)
{
	int ret = hcreate(size);
	if(!ret)
	{
		printf("hcreate failed\n");
		return 1;
	}

	size_t debugcount = 0;
	ENTRY entry;
	struct gd_fs_entry_t *iter = head;
	while(iter != NULL)
	{
		entry.key = iter->filename;
		entry.data = iter;
		printf("%s\n", entry.key);

		ENTRY* entered = hsearch(entry, ENTER);
		if(0 && !entered)
		{
			fprintf(stderr, "hsearch(%ld): %s\n", debugcount, strerror(errno));
			destroy_hash_table();
			return 1;
		}

		iter = iter->next;
		++debugcount;
	}

	return 0;
}

void destroy_hash_table()
{
	hdestroy();
}

