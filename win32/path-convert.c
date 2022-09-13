/* vi: set sw=4 ts=4 expandtab: */
#include "libbb.h"
#include "path-convert.h"

static const char *path_convert_internal(const char *path,
                                         char *output, size_t output_size,
                                         size_t *output_size_hint,
                                         enum path_convert_flag flags)
{
    char *p;
    DWORD len;

    if ((flags & PATH_CONVERT_ABSOLUTE) ||
		((flags & (PATH_CONVERT_WINDOWS | PATH_CONVERT_MIXED)) && *path == '/')) {
        wchar_t *wpath = mingw_pathconv(path);

        if (!_wcsnicmp(wpath, L"\\\\?\\", 4))
            wpath += 4;

        len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
        if (len > output_size) {
            *output_size_hint = len;
            return NULL;
        }
        if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, output, output_size, NULL, NULL) == 0) {
            *output_size_hint = len;
            return NULL;
        }
    } else if ((len = strlen(path)) + 1 > output_size) {
        *output_size_hint = len + 1;
        return NULL;
    } else
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

const char *path_convert(const char *path, char *output, size_t output_size,
                         enum path_convert_flag flags)
{
    size_t unused;

    return path_convert_internal(path, output, output_size, &unused, flags);
}

static void grow_result(char **result, size_t count, size_t actual, size_t *alloc)
{
    if (actual + count <= *alloc)
        return;
    *alloc = MAX(actual + count + 64, *alloc + (*alloc >> 1));
    *result = xrealloc(*result, *alloc);
}

char *path_convert_path_list(const char *path_list,
                             enum path_convert_flag flags)
{
    size_t alloc = strlen(path_list), count = 0, len, hint;
    char *result = xmalloc(alloc);
    const char *path_start = path_list, *p = path_list, *p2, *p3;

    if (!path_list || !*path_list) {
        bb_error_msg("cannot convert empty path");
        return NULL;
    }

    /* find path separator, if any */
    for (;;) {
        if (*p && *p != ';' &&
            /*
             * Be nice and handle colon-separated path lists, too, but
             * take care of _not_ mishandling a colon after a drive letter.
             */
            (*p != ':' || (p == path_start + 1 && isalpha(*path_start)))) {
            p++;
            continue;
        }

        if (p == path_start)
            p2 = ".";
        else if (!*p)
            p2 = path_start;
        else {
            grow_result(&result, p + 1 - path_start, count, &alloc);
            safe_strncpy(result + count, path_start, p + 1 - path_start);
            p2 = NULL; /* do not point into `result` as it might be realloced, use `result + count` instead */
        }

        p3 = path_convert_internal(p2 ? p2 : result + count, result + count, alloc - count, &hint, flags);
        if (!p3) {
            grow_result(&result, hint, count, &alloc);
            p3 = path_convert_internal(p2 ? p2 : result + count, result + count, alloc - count, &hint, flags);
            if (!p3)
                bb_error_msg_and_die("tried to realloc, but still %lu bytes missing", (unsigned long)hint);
        }

        len = strlen(p3);
        if (p3 != result + count)
            memmove(result + count, p3, len + 1);

        count += len;
        if (!*p) {
            result[count] = '\0';
            return result;
        }

        grow_result(&result, 2, count, &alloc);
        result[count++] = flags & PATH_CONVERT_UNIX ? ':' : ';';
        path_start = ++p;
    }
}
