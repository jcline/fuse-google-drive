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

#include <fuse.h>

int gd_getattr (const char *path, struct stat *)
{
	return 0;
}


int gd_readlink (const char *path, char *, size_t)
{
	return 0;
}


int gd_getdir (const char *path, fuse_dirh_t, fuse_dirfil_t)
{
	return 0;
}


int gd_mknod (const char *path, mode_t, dev_t)
{
	return 0;
}


int gd_mkdir (const char *path, mode_t)
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


int gd_symlink (const char *path, const char *)
{
	return 0;
}


int gd_rename (const char *path, const char *)
{
	return 0;
}


int gd_link (const char *path, const char *)
{
	return 0;
}


int gd_chmod (const char *path, mode_t)
{
	return 0;
}


int gd_chown (const char *path, uid_t, gid_t)
{
	return 0;
}


int gd_truncate (const char *path, off_t)
{
	return 0;
}


int gd_utime (const char *path, struct utimbuf *)
{
	return 0;
}


int gd_open (const char *path, struct fuse_file_info *)
{
	return 0;
}


int gd_read (const char *path, char *, size_t, off_t, struct fuse_file_info *)
{
	return 0;
}


int gd_write (const char *path, const char *, size_t, off_t,struct fuse_file_info *)
{
	return 0;
}


int gd_statfs (const char *path, struct statfs *)
{
	return 0;
}


int gd_flush (const char *path, struct fuse_file_info *)
{
	return 0;
}


int gd_release (const char *path, struct fuse_file_info *)
{
	return 0;
}


int gd_fsync (const char *path, int, struct fuse_file_info *)
{
	return 0;
}


int gd_setxattr (const char *path, const char *, const char *, size_t, int)
{
	return 0;
}


int gd_getxattr (const char *path, const char *, char *, size_t)
{
	return 0;
}


int gd_listxattr (const char *path, char *, size_t)
{
	return 0;
}


int gd_removexattr (const char *path, const char *)
{
	return 0;
}


// Only uncomment these assignments once an operation's function has been
// fleshed out.
struct fuse_operations gd_oper = {
	//.getattr = gd_getattr;
	//.readlink = gd_readlink;
	//.getdir = gd_getdir;
	//.mknod = gd_mknod;
	//.mkdir = gd_mkdir;
	//.unlink = gd_unlink;
	//.rmdir = gd_rmdir;
	//.symlink = gd_symlink;
	//.rename = gd_rename;
	//.link = gd_link;
	//.chmod = gd_chmod;
	//.chown = gd_chown;
	//.truncate = gd_truncate;
	//.utime = gd_utime;
	//.open = gd_open;
	//.read = gd_read;
	//.write = gd_write;
	//.statfs = gd_statfs;
	//.flush = gd_flush;
	//.release = gd_release;
	//.fsync = gd_fsync;
	//.setxattr = gd_setxattr;
	//.getxattr = gd_getxattr;
	//.listxattr = gd_listxattr;
	//.removexattr = gd_removexattr;
}


int main(int argc, char* argv[])
{
	return 0;
}
