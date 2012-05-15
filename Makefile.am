bin_PROGRAMS = fuse-google-drive

fuse_google_drive_SOURCES = gd_fuse_operations.c \
                            gd_interface.c \
                            gd_cache.c \
                            stack.c \
                            functional_stack.c \
														str.c \
														curl_interface.c
fuse_google_drive_CFLAGS = -g $(AM_CFLAGS) $(fuse_CFLAGS) $(curl_CFLAGS) $(json_CFLAGS) $(xml_CFLAGS)
fuse_google_drive_LDADD = $(fuse_LIBS) $(curl_LIBS) $(json_LIBS) $(xml_LIBS)

EXTRA_DIST = COPYING README
