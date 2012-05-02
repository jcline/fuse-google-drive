bin_PROGRAMS = fuse-google-drive

fuse_google_drive_SOURCES = gd_fuse_operations.c \
                            gd_interface.c \
                            gd_cache.c \
                            stack.c \
                            functional_stack.c
fuse_google_drive_CFLAGS = $(AM_CFLAGS) @FUSE_CFLAGS@ $(curl_CFLAGS) $(json_CFLAGS) $(xml_CFLAGS)
fuse_google_drive_LDADD = @FUSE_LIBS@ $(curl_LIBS) $(json_LIBS) $(xml_LIBS)

EXTRA_DIST = COPYING README
