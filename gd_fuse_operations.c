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
#include <fuse.h>

int gd_getattr (const char *path, struct stat *statbuf)
{
	return 0;
}


int gd_readlink (const char *path, char *link, size_t size)
{
	return 0;
}


int gd_mknod (const char *path, mode_t mode, dev_t dev)
{
	return 0;
}


int gd_mkdir (const char *path, mode_t mode)
{
	return 0;
}


int gd_unlink (const char *path)
{
	return 0;
}


int gd_rmdir (const char *path)
{
	return 0;
}


int gd_symlink (const char *path, const char *link)
{
	return 0;
}


int gd_rename (const char *path, const char *newpath)
{
	return 0;
}


int gd_link (const char *path, const char *newpath)
{
	return 0;
}


int gd_chmod (const char *path, mode_t mode)
{
	return 0;
}


int gd_chown (const char *path, uid_t uid, gid_t gid)
{
	return 0;
}


int gd_truncate (const char *path, off_t newsize)
{
	return 0;
}


int gd_open (const char *path, struct fuse_file_info * fileinfo)
{
	return 0;
}


int gd_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_statfs (const char *path, struct statvfs *statv)
{
	return 0;
}


int gd_flush (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_release (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_fsync (const char *path, int datasync, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_setxattr (const char *path, const char *name, const char *value, size_t size, int flags)
{
	return 0;
}


int gd_getxattr (const char *path, const char *name, char *value, size_t size)
{
	return 0;
}


int gd_listxattr (const char *path, char *list, size_t size)
{
	return 0;
}


int gd_removexattr (const char *path, const char *name)
{
	return 0;
}


int gd_opendir (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_releasedir (const char *path, struct fuse_file_info *fileinfo)
{
	return 0;
}


void *gd_init (struct fuse_conn_info *conn)
{
	return NULL;
}


void gd_destroy (void *userdata)
{
}


int gd_access (const char *path, int mask)
{
	return 0;
}


int gd_create (const char *path, mode_t mode, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_ftruncate (const char *path, off_t offset, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_fgetattr (const char *path, struct stat *statbuf, struct fuse_file_info *fileinfo)
{
	return 0;
}


int gd_lock (const char *path, struct fuse_file_info *fileinfo, int cmd, struct flock *lock)
{
	return 0;
}


int gd_utimens (const char *path, const struct timespec tv[2])
{
	return 0;
}


int gd_ioctl (const char *path, int cmd, void *arg, struct fuse_file_info *fileinfo, unsigned int flags, void *data)
{
	return 0;
}


int gd_poll (const char *path, struct fuse_file_info *fileinfo, struct fuse_pollhandle *ph, unsigned *reventsp)
{
	return 0;
}


// Only uncomment these assignments once an operation's function has been
// fleshed out.
struct fuse_operations gd_oper = {
	//.getattr     = gd_getattr,
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
	//.open        = gd_open,
	//.read        = gd_read,
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
	//.readdir     = gd_readdir,
	//.releasedir  = gd_releasedir,
	//.fsyncdir    = gd_fsyncdir,
	//.init        = gd_init,
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

/**
 *  Store any state about this mount in this structure.
 *
 *  root: The path to the mount point of the drive.
 *  TODO:
 *    Tokens - should they be char*s? 
 */
struct gd_state {
	char* root;
};

int main(int argc, char* argv[])
{
	int fuse_stat;
	struct gd_state gd_data;

	fuse_stat = fuse_main(argc, argv, &gd_oper, &gd_data);

	return fuse_stat;
}
