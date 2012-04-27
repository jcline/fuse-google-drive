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

#define FUSE_USE_VERSION 26
#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <string.h>
#include <sys/stat.h>
#include "gd_interface.h"

/**
 *  Store any state about this mount in this structure.
 *
 *  root: The path to the mount point of the drive.
 *  credentials: Struct which stores necessary credentials for this mount.
 */
struct gd_state {
	char* root;
	struct gdi_state gdi_data;
};

/** Get file attributes.
 *
 */
int gd_getattr (const char *path, struct stat *statbuf)
{

	struct fuse_context *fc = fuse_get_context();
	memset(statbuf, 0, sizeof(struct stat));
	if( strcmp("/", path) == 0)
	{
		statbuf->st_mode = S_IFDIR | 0700;
		statbuf->st_nlink=2;
	}
	else
	{
		statbuf->st_mode = S_IFREG | 0700;
		statbuf->st_nlink=1;
	}
	statbuf->st_uid = fc->uid;
	statbuf->st_gid = fc->gid;

	return 0;
}

/** Read the target of a symbolic link.
 *
 */
int gd_readlink (const char *path, char *link, size_t size)
{
	return 0;
}

/** Create a regular file.
 *
 */
int gd_mknod (const char *path, mode_t mode, dev_t dev)
{
	return 0;
}

/** Create a directory.
 *
 */
int gd_mkdir (const char *path, mode_t mode)
{
	return 0;
}

/** Remove a file.
 *
 */
int gd_unlink (const char *path)
{
	return 0;
}

/** Remove a directory.
 *
 */
int gd_rmdir (const char *path)
{
	return 0;
}

/** Create a symbolic link.
 *
 *  Google Drive likely does not support an equivalent operation.
 *  We could allow this for a specific session, but it would be lost
 *  after an unmount.
 */
int gd_symlink (const char *path, const char *link)
{
	return 0;
}

/** Rename a file.
 *
 */
int gd_rename (const char *path, const char *newpath)
{
	return 0;
}

/** Create a hard link to a file.
 *
 *  Google Drive likely does not support an equivalent operation.
 *  We could allow this for a specific session, but it would be lost
 *  after an unmount.
 */
int gd_link (const char *path, const char *newpath)
{
	return 0;
}

/** Change the permission bits of a file.
 *
 *  Could this be used to share a document with others?
 *  Perhaps o+r would make visible to all, o+rw would be editable by all?
 *  I can't think of a way to do any sort of group level permissions sanely atm.
 *
 *  Alternatively, these permission settings could be used for local acces only,
 *  which would let you make a file locally readable or modifiable by another
 *  system user. This may violate the principle of least astonishment least.
 */
int gd_chmod (const char *path, mode_t mode)
{
	return 0;
}

/** Change the owner and group of a file.
 *
 *  Since this uses gid and uids, I cannot think of a way to easily use this for
 *  Google Drive sharing purposes with specific users right now.
 *  Perhaps some additional utility could be used to display a mapping of
 *  uid/gids and Google Drive users you can share with?
 *
 *  Alternatively, these uid/gid settings could be used for local acces only,
 *  which would let you make a file locally readable or modifiable by another
 *  system user. This may violate the principle of least astonishment least.
 */
int gd_chown (const char *path, uid_t uid, gid_t gid)
{
	return 0;
}

/** Change the size of a file.
 *
 */
int gd_truncate (const char *path, off_t newsize)
{
	return 0;
}

/** File open operation.
 *
 */
int gd_open (const char *path, struct fuse_file_info * fileinfo)
{
	return 0;
}

/** Read data from an open file.
 *
 */
int gd_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Write data to an open file.
 *
 */
int gd_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Get file system statistics.
 *
 */
int gd_statfs (const char *path, struct statvfs *statv)
{
	return 0;
}

/** Possibly flush cached data.
 *
 */
int gd_flush (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Release an open file.
 *
 */
int gd_release (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Synchronize file contents.
 *
 */
int gd_fsync (const char *path, int datasync, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Set extended attributes.
 *
 *  Does this mean anything for Google Drive?
 */
int gd_setxattr (const char *path, const char *name, const char *value, size_t size, int flags)
{
	return 0;
}

/** Get extended attributes.
 *
 *  Does this mean anything for Google Drive?
 */
int gd_getxattr (const char *path, const char *name, char *value, size_t size)
{
	return 0;
}

/** List extended attributes.
 *
 *  Does this mean anything for Google Drive?
 */
int gd_listxattr (const char *path, char *list, size_t size)
{
	return 0;
}

/** Remove extended attributes.
 *
 *  Does this mean anything for Google Drive?
 */
int gd_removexattr (const char *path, const char *name)
{
	return 0;
}

/** Open a directory.
 *
 */
int gd_opendir (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Read directory.
 *
 */
int gd_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileinfo)
{

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	struct gdi_state *state = &((struct gd_state*)fuse_get_context()->private_data)->gdi_data;
	char **list = gdi_get_file_list(path, state);
	char **iter = list;
	while((*iter) != NULL)
	{
		if(filler(buf, *iter, NULL, 0))
		{
			fprintf(stderr, "readdir() filler()\n");
			return -ENOMEM;
		}
		++iter;
	}
	return 0;
}

/** Release directory.
 *
 */
int gd_releasedir (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Synchronize directory contents.
 *
 */
int gd_fsyncdir (const char *path, int datasync, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Initialize filesystem
 *
 */
void *gd_init (struct fuse_conn_info *conn)
{
	return ((struct gd_state *) fuse_get_context()->private_data);
}

/** Clean up filesystem
 *
 *  Automatically called by fuse on filesystem exit (unmount).
 */
void gd_destroy (void *userdata)
{
}

/** Check file access permission.
 *
 */
int gd_access (const char *path, int mask)
{
	return 0;
}

/** Create and open a file.
 *
 */
int gd_create (const char *path, mode_t mode, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Change the size of an open file.
 *
 */
int gd_ftruncate (const char *path, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Get attributes from an open file.
 *
 */
int gd_fgetattr (const char *path, struct stat *statbuf, struct fuse_file_info *fileinfo)
{
	return 0;
}

/** Perform POSIX file locking operation.
 *
 *  Does this make any sense for Google Drive?
 */
int gd_lock (const char *path, struct fuse_file_info *fileinfo, int cmd, struct flock *lock)
{
	return 0;
}

/** Change the access and modification times of a file with nanosecond
 *  resoluton.
 *
 *  Does this make any sense for Google Drive?
 */
int gd_utimens (const char *path, const struct timespec tv[2])
{
	return 0;
}

/** Ioctl
 *
 *  Does this make any sense for Google Drive?
 */
int gd_ioctl (const char *path, int cmd, void *arg, struct fuse_file_info *fileinfo, unsigned int flags, void *data)
{
	return 0;
}

/** Poll for IO readiness events.
 *
 */
int gd_poll (const char *path, struct fuse_file_info *fileinfo, struct fuse_pollhandle *ph, unsigned *reventsp)
{
	return 0;
}


// Only uncomment these assignments once an operation's function has been
// fleshed out.
struct fuse_operations gd_oper = {
	.getattr     = gd_getattr,
	//.readlink    = gd_readlink,
	// getdir() deprecated, use readdir()
	.getdir        = NULL,
	//.mknod       = gd_mknod,
	//.mkdir       = gd_mkdir,
	//.unlink      = gd_unlink,
	//.rmdir       = gd_rmdir,
	//.symlink     = gd_symlink,
	//.rename      = gd_rename,
	//.link        = gd_link,
	//.chmod       = gd_chmod,
	//.chown       = gd_chown,
	//.truncate    = gd_truncate,
	// utime() deprecated, use utimens
	.utime         = NULL,
	.open        = gd_open,
	.read        = gd_read,
	//.write       = gd_write,
	//.statfs      = gd_statfs,
	//.flush       = gd_flush,
	//.release     = gd_release,
	//.fsync       = gd_fsync,
	//.setxattr    = gd_setxattr,
	//.getxattr    = gd_getxattr,
	//.listxattr   = gd_listxattr,
	//.removexattr = gd_removexattr,
	//.opendir     = gd_opendir,
	.readdir     = gd_readdir,
	//.releasedir  = gd_releasedir,
	//.fsyncdir    = gd_fsyncdir,
	.init        = gd_init,
	//.destroy     = gd_destroy,
	//.access      = gd_access,
	//.create      = gd_create,
	//.ftruncate   = gd_ftruncate,
	//.getattr     = gd_fgetattr,
	//.lock        = gd_lock,
	//.utimens     = gd_utimens,
	//.ioctl       = gd_ioctl,
	//.poll        = gd_poll,
};

int main(int argc, char* argv[])
{
	int fuse_stat;
	struct gd_state gd_data;

	int ret = gdi_init(&gd_data.gdi_data);
	if(ret != 0)
		return ret;

	// Start fuse
	fuse_stat = fuse_main(argc, argv, &gd_oper, &gd_data);
	/*  When we get here, fuse has finished.
	 *  Do any necessary cleanups.
	 */
	gdi_destroy(&gd_data.gdi_data);

	return fuse_stat;
}
