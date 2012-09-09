fuse-google-drive is a fuse filesystem wrapper for Google Drive released under GPLv2

Currently in alpha stages. Do not trust this for anything important.

A detailed guide to get running on Ubuntu: http://jason-king.info/mount-your-google-drive-readonly-on-linux/

Status:

* read() works, cache not freed until unmount, should detect file updates
* directory listing works, no heirarchy
* incorrect stat() info, filesize is correct, fails (as it should) on nonexistant files
* redirecturi is now hardcoded -- you do not need the file
* clientsecrets and client id should now be in `$XDG_CONFIG_HOME/fuse-google-drive/`

Discussion:

* #fuse-google-drive on irc.freenode.net

Dependencies:

* fuse
* libcurl
* json-c aka libjson
* libxml2

Build Dependencies:

* autotools
* make

If you are on one of the systems that does not include development files with
packages, then make sure you install the development packages for each of the
dependencies.

Compilation:

```
$ ./autogen.sh
$ ./configure
$ make
```

Usage:

Right now you need to go to http://code.google.com/apis/console and create
a new app and generate a client id and client secret for an install application.
The clientid value and clientsecrets value should each go into:

```
$XDG_CONFIG_HOME/fuse-google-drive/clientid
$XDG_CONFIG_HOME/fuse-google-drive/clientsecrets
```

resepectively. You should `chmod 700 $XDG_CONFIG_HOME/fuse-google-drive` as well.
If the folder does not exist at runtime, a helpful message is printed and the
directory is created with the correct permissions if possible.
Note: If `$XDG_CONFIG_HOME` is unset on your system, it defaults to `~/.config/`.

```
$ mkdir mountpoint
$ ./fuse-google-drive mountpoint
```

Thanks to:

* http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/
* https://www.ibm.com/developerworks/linux/library/l-fuse/
* Gregor on FreeNode
