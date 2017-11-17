#ifndef FILESYSTEM_ACCESS_H_
#define FILESYSTEM_ACCESS_H_

#include <stdbool.h>

typedef struct filepath {
    char *path;
    size_t path_len;
} filepath;

typedef void(*file_found_cb)(filepath dir, filepath file, void *args);

typedef struct callback {
    file_found_cb func;
    void *args;
} callback;

filepath set_path(filepath dest, filepath src);
filepath get_full_path(filepath dir, filepath file);
filepath normalize_filepath(filepath path);
int match_extension(char *file, char *extension);
bool traverse_dir(filepath cwd, char *extension, callback cb);

#endif /* FILESYSTEM_ACCESS_H_ */
