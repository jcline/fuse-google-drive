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

#include "gd_cache.h"
#include <string.h>

struct gd_fs_entry_t* gd_fs_entry_from_xml(xmlDocPtr xml, xmlNodePtr node)
{
	struct gd_fs_entry_t* entry;

	entry = (struct gd_fs_entry_t*) malloc(sizeof(struct gd_fs_entry_t));
	entry->next = NULL;

	size_t length;
	xmlNodePtr c1, c2;
	xmlChar *value;

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
			default:
				break;
		}
	}

	//printf("%s\n", entry->filename);

	return entry;
}
