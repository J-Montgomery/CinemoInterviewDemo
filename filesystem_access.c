#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dirent.h>
#include "filesystem_access.h"
#include "system_shims.h"


static filepath get_system_full_path(filepath path) {

}

filepath set_path(filepath dest, filepath src) {
    if(dest.path_len <= src.path_len) {
        dest.path = realloc(dest.path, src.path_len);
        dest.path_len = src.path_len;
    }

    memcpy(dest.path, src.path, src.path_len);
    dest.path[src.path_len] = 0;

    return dest;
}

filepath dup_filepath(filepath old_path) {
    filepath new_path;
    new_path.path = calloc(old_path.path_len + 1, 1);
    memcpy(new_path.path, old_path.path, old_path.path_len + 1);
    new_path.path_len = old_path.path_len;

    return new_path;
}

filepath get_full_path(filepath dir, filepath file) {
    size_t len = dir.path_len + file.path_len + 1;

    filepath full_path = { .path     = calloc(len, 1),
                           .path_len = len - 1 };

    strncat(full_path.path, dir.path,  dir.path_len);
    strncat(full_path.path, file.path, file.path_len);
    full_path.path[full_path.path_len] = '\0';

    return full_path;
}

filepath normalize_filepath(filepath path) {
    size_t new_len = strlen(path.path);

    if(path.path[new_len - 1] != '\\') {
        if(path.path_len < (new_len + 1))
            realloc(path.path, new_len + 1);
        path.path[new_len] = '\0';
        strncat(path.path, "\\", 1); // Normalize by appending a system-specific directory separator
        new_len += 1;
        
    }
    path.path_len = new_len;

    /* In an ideal world, we would continue normalizing by calling GetFullPathName or realpath */
    return path;
}

bool match_extension(char *filename, char *extension) {
    // TODO: Rewrite using system-specific code
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
    printf("MAX_PATH %li\nMAX_NAME %lu\n", INITIAL_SYS_PATH_LEN, getSystemNameMax(cwd.path));

    while((p_dir = readdir(dir)) != NULL) {
        if(match_extension(p_dir->d_name, extension)) {
            cb.func(cwd, (filepath) { p_dir->d_name, p_dir->d_namlen }, cb.args);
        }
    }
    closedir(dir);

    return FALSE;
}