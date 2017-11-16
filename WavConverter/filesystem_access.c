#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dirent.h>
#include "filesystem_access.h"


filepath set_path(filepath dest, filepath src) {
    if(dest.path_len < src.path_len || dest.path == NULL) {
        dest.path = realloc(dest.path, src.path_len);
    }

    snprintf(dest.path, src.path_len + 1, "%s", src.path);
    dest.path_len = src.path_len;

    return dest;
}

filepath get_full_path(filepath dir, filepath file) {
    size_t len = dir.path_len + file.path_len;
    filepath full_path = { .path     = calloc(len + 1, 1),
                           .path_len = len };

    snprintf(full_path.path, full_path.path_len + 1, "%s%s", dir.path, file.path);
    return full_path;
}

filepath normalize_filepath(filepath path) {
    if(path.path == NULL || path.path_len == 0)
        return (filepath) { NULL, 0 };

    /* Normalizing by calling GetFullPathName or realpath would be better but significantly more complicated */
    /* Instead we append a system-specific directory separator */
    size_t new_len = strlen(path.path);

    if(path.path[new_len - 1] != '\\') {
        if(path.path_len < (new_len + 1))
            realloc(path.path, new_len + 2);

        path.path[new_len] = '\\';
        path.path[new_len + 1] = '\0';
        new_len += 1;
    }

    path.path_len = new_len;

    return path;
}

bool match_extension(char *filename, char *extension) {
    return (strstr(filename, extension) != NULL);
}

bool traverse_dir(filepath cwd, char *extension, callback cb) {
    struct dirent *p_dir;
    DIR *dir;

    dir = opendir(cwd.path);
    if(dir == NULL) {
        printf("Cannot open directory '%s'\n", cwd.path);
        return FALSE;
    }

    while((p_dir = readdir(dir)) != NULL) {
        if(match_extension(p_dir->d_name, extension)) {
            cb.func(cwd, (filepath) { p_dir->d_name, p_dir->d_namlen }, cb.args);
        }
    }
    closedir(dir);

    return TRUE;
}