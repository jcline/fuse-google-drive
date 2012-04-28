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

#ifndef _GOOGLE_DRIVE_CACHE_H
#define _GOOGLE_DRIVE_CACHE_H

#include <pthread.h>

// Do we need to represent folders differently from files?
// For the time being, ignore folders
struct gd_fs_entry_t {
	char *author; // the file owner?
	char *author_email;

	char *lastModifiedBy; // do we even care about this?
	char *lastModifiedBy_email;

	char *filename; // 'title' in the XML from directory-list
	char *resourceID;

	size_t size; // file size in bytes, 'gd:quotaBytesUsed' in XML
	char *md5; // 'docs:md5Checksum' in XML, might be useful later


	// Add some data we can use in getattr()
};

// Since hsearch et al are likely not threadsafe we need to use a read write
// lock to prevent corruption. The write lock should only be taken when we
// need to add a new item.
// What to do about removing a file? Just mark the entry as invalid? There is
// no removal action for hsearch et al.
struct gd_fs_lock_t {
	pthread_rwlock_t *lock;
};



#endif
