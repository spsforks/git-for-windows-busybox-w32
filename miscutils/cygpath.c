/*
 * Convert MSYS-style paths to Win32-style ones
 *
 * Copyright (C) 2022 Johannes Schindelin <johannes.schindelin@gmx.de>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
//config:config CYGPATH
//config:	bool "cygpath"
//config:	default y
//config:	depends on PLATFORM_MINGW32
//config:	help
//config:	Convert Unix and Windows format paths

//applet:IF_CYGPATH(APPLET_NOEXEC(cygpath, cygpath, BB_DIR_USR_BIN, BB_SUID_DROP, cygpath))

//kbuild:lib-$(CONFIG_CYGPATH) += cygpath.o

//usage:#define cygpath_trivial_usage
//usage:       "ARG|ARGS"
//usage:#define cygpath_full_usage "\n\n"
//usage:       "Convert Unix and Windows format paths"

#include "libbb.h"
#include "path-convert.h"

enum {
    OPT_absolute  = (1 << 0),
    OPT_mixed     = (1 << 1),
    OPT_unix      = (1 << 2),
    OPT_windows   = (1 << 3),
    OPT_path_list = (1 << 4),
};

int cygpath_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int cygpath_main(int argc UNUSED_PARAM, char **argv)
{
    int i;
    enum path_convert_flag flags = 0;
#if ENABLE_LONG_OPTS
    static const char cygpath_longopts[] ALIGN1 =
        "absolute\0" No_argument "a"
        "mixed\0" No_argument "m"
        "unix\0" No_argument "u"
        "windows\0" No_argument "w"
        "path\0" No_argument "p"
        ;
#endif
    int opt = getopt32long(argv, "amuwp", cygpath_longopts);
    argv += optind;
    argc -= optind;

    if (opt & OPT_absolute)
        flags |= PATH_CONVERT_ABSOLUTE;
    if (opt & OPT_mixed)
        flags |= PATH_CONVERT_MIXED;
    if (opt & OPT_unix)
        flags |= PATH_CONVERT_UNIX;
    if (opt & OPT_windows)
        flags |= PATH_CONVERT_WINDOWS;

    for (i = 0; i < argc; i++) {
        char *to_free = NULL;
        const char *path = argv[i], *result;
        char buffer[PATH_MAX_LONG];

        if (!*argv[i]) {
            bb_error_msg("can't convert empty path");
            return EXIT_FAILURE;
        }

        if (opt & OPT_path_list)
            result = to_free = path_convert_path_list(path, flags);
        else
            result = path_convert(path, buffer, sizeof(buffer), flags);

        if (!result)
            return EXIT_FAILURE;

        printf("%s\n", result);

        free(to_free);
    }

    return EXIT_SUCCESS;
}
