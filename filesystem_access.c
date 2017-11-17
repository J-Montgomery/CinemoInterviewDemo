#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <dirent.h>
#include "filesystem_access.h"

// Specific incompatibilities between *nix and windows
#if defined(_WIN32)
    #define SYS_PATH_SEPARATOR '\\'
    #define lc_strstr(str1, str2) _stricmp((str1), (str2))
#else
    #define SYS_PATH_SEPARATOR '/'
    #define lc_strstr(str1, str2) (strcasestr((str1), (str2)) == NULL)
#endif


filepath set_path(filepath dest, filepath src) {
    if(dest.path_len < src.path_len || dest.path == NULL) {
        char *tmp = realloc(dest.path, src.path_len);
        if(tmp != NULL)
            dest.path = tmp;
        else
            return src;
    }

    snprintf(dest.path, src.path_len + 1, "%s", src.path);
    dest.path_len = src.path_len;

    return dest;
}

filepath get_full_path(filepath dir, filepath file) {
    size_t len = dir.path_len + file.path_len;
    filepath full_path = { .path     = calloc(len + 1, 1),
                           .path_len = len };

    if(full_path.path != NULL)
        snprintf(full_path.path, full_path.path_len + 1, "%s%s", dir.path, file.path);
    return full_path;
}

filepath normalize_filepath(filepath path) {
    if(path.path == NULL || path.path_len == 0)
        return (filepath) { NULL, 0 };

    /* Normalizing by calling GetFullPathName or realpath would be better but significantly more complicated */
    /* Instead we append a system-specific directory separator */
    size_t new_len = strlen(path.path);

    if(path.path[new_len - 1] != SYS_PATH_SEPARATOR) {
        if(path.path_len < (new_len + 1)) {
            char *tmp = realloc(path.path, new_len + 2);
            if(tmp != NULL)
                path.path = tmp;
            else
                return path; // It may still be a valid path, but we're out of memory to normalize
        }

        path.path[new_len] = SYS_PATH_SEPARATOR;
        path.path[new_len + 1] = '\0';
        new_len += 1;
    }

    path.path_len = new_len;

    return path;
}

char *get_ext(char *filename) {
    char *ptr = strrchr(filename, '.');
    return ptr;
}

int match_extension(char *filename, char *extension) {
    char *file_ext = get_ext(filename);
    if(file_ext != NULL)
        return (lc_strstr(file_ext, extension) == 0); // case-insensitive comparison
    else
        return 0;
}

bool traverse_dir(filepath cwd, char *extension, callback cb) {
    struct dirent *p_dir;
    DIR *dir;

    dir = opendir(cwd.path);
    if(dir == NULL) {
        printf("Cannot open directory '%s'\n", cwd.path);
        return false;
    }

    while((p_dir = readdir(dir)) != NULL) {
        if(match_extension(p_dir->d_name, extension)) {
            cb.func(cwd, (filepath) { p_dir->d_name, strlen(p_dir->d_name) }, cb.args);
        }
    }
    closedir(dir);

    return true;
}

bool parse_wav(wav_header *params, FILE *wav) {
    int errors = 0;
    int f_ret = fread(params, 1, sizeof(wav_header), wav);
    errors += (f_ret != 0);
    errors += (memcmp(&params->chunk_id   , "RIFF", 4) != 0);
    errors += (memcmp(&params->format     , "WAVE", 4) != 0);
    errors += (memcmp(&params->subchunk_id, "fmt ", 4) != 0);
    errors += (params->format_type != 1);

    if(!errors) {
        fseek(wav, params->subchunk_len + 28, SEEK_SET);
        return true;
    }
    else 
        return false;
}