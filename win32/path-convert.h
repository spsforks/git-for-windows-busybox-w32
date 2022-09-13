/* vi: set sw=4 ts=4 expandtab: */
#ifndef PATH_CONVERT_H
#define PATH_CONVERT_H

enum path_convert_flag {
    PATH_CONVERT_ABSOLUTE  = (1 << 0),
    PATH_CONVERT_MIXED     = (1 << 1),
    PATH_CONVERT_UNIX      = (1 << 2),
    PATH_CONVERT_WINDOWS   = (1 << 3),
};

const char *path_convert(const char *path, char *output, size_t output_size,
                         enum path_convert_flag flags);

char *path_convert_path_list(const char *path_list,
                             enum path_convert_flag flags);

#endif
