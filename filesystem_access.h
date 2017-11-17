#ifndef FILESYSTEM_ACCESS_H_
#define FILESYSTEM_ACCESS_H_

#include <stdbool.h>

/*
 * Thin wrapper around strings representing filepaths, with a defined length.
 */
typedef struct filepath {
    char *path;
    size_t path_len;
} filepath;

typedef void(*file_found_cb)(filepath dir, filepath file, void *args);

typedef struct callback {
    file_found_cb func;
    void *args;
} callback;

//! Realloc dest and set it to the value of src
filepath set_path(filepath dest, filepath src);

//! Concatenate paths of dir and file to create a full filepath.
filepath get_full_path(filepath dir, filepath file);

//! Put the directory path into a valid form for iteration and eventual concatenation
filepath normalize_filepath(filepath path);

//! Get the extension out of a filename
char *get_ext(char *filename);
//! Returns true if the file has the extension given
int match_extension(char *file, char *extension);

//! Iterate over a directory, calling cb.func with cb.args when a file is found with the specified extension
bool traverse_dir(filepath cwd, char *extension, callback cb);

#endif /* FILESYSTEM_ACCESS_H_ */
