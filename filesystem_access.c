#include <string.h>
#include <stdlib.h>

#include <dirent.h>
#include "filesystem_access.h"


filepath set_path(filepath dest, filepath src) {
    if(dest.path_len <= src.path_len) {
        dest.path = realloc(dest.path, src.path_len);
        dest.path_len = src.path_len;
    }

    memcpy(dest.path, src.path, src.path_len);
    dest.path[src.path_len] = 0;

    return dest;
}


filepath get_full_path(filepath dir, filepath file) {
    size_t len = dir.path_len + file.path_len + 2;
    filepath full_path = { .path = calloc(len, 1),
        .path_len = len };

    strncat(full_path.path, dir.path, full_path.path_len);
    strncat(full_path.path, "\\", full_path.path_len);
    strncat(full_path.path, file.path, full_path.path_len);

    return full_path;
}

void traverse_dir(filepath cwd) {

}