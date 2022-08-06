/* vi: set sw=4 ts=4 expandtab: */
#include "libbb.h"
#include "path-convert.h"

const char *path_convert(const char *path, char *output, size_t output_size,
                         enum path_convert_flag flags)
{
    char *p;
    DWORD len;

    if (flags & PATH_CONVERT_ABSOLUTE) {
        wchar_t *wpath = mingw_pathconv(path);

        if (!_wcsnicmp(wpath, L"\\\\?\\", 4))
            wpath += 4;

        len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
        if (len > output_size)
            return NULL;
        if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, output, output_size, NULL, NULL) == 0)
            return NULL;
    } else if ((len = strlen(path)) + 1 > output_size)
        return NULL;
    else
        safe_strncpy(output, path, output_size);

    if ((flags & (PATH_CONVERT_WINDOWS & PATH_CONVERT_MIXED)) &&
        output[0] == '/' && isalpha(output[1]) && output[2]) {
        output[0] = toupper(output[1]);
        output[1] = ':';
    } else if ((flags & PATH_CONVERT_UNIX) &&
               isalpha(output[0]) && output[1] == ':' && is_dir_sep(output[2])) {
        output[1] = tolower(output[0]);
        output[0] = output[2] = '/';
    }

    if (flags & PATH_CONVERT_WINDOWS) {

        for (p = output; *p; p++)
            if (*p == '/')
                *p = '\\';
    } else
        bs_to_slash(output);

    if (flags & PATH_CONVERT_UNIX) {
        static char *pseudo_root;
        static size_t pseudo_root_len;

        if (!pseudo_root) {
            wchar_t *wpath = mingw_pathconv("/");

            if (!_wcsnicmp(wpath, L"\\\\?\\", 4))
                wpath += 4;

            pseudo_root_len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
            pseudo_root = xmalloc(pseudo_root_len);
            if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, pseudo_root, pseudo_root_len, NULL, NULL) == 0)
                bb_error_msg_and_die("Could not convert '%ls' to UTF-8", wpath);
            pseudo_root_len--;

            bs_to_slash(pseudo_root);

            if (isalpha(pseudo_root[0]) && pseudo_root[1] == ':' && is_dir_sep(pseudo_root[2])) {
                pseudo_root[1] = tolower(pseudo_root[0]);
                pseudo_root[0] = pseudo_root[2] = '/';
            }
        }

        if (!strncmp(output, pseudo_root, pseudo_root_len)) {
            if (!output[pseudo_root_len])
                return "/";
            if (is_dir_sep(output[pseudo_root_len]))
                output += pseudo_root_len;
        }
    }

    return output;
}
