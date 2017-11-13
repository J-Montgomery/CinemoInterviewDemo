#ifndef FILESYSTEM_ACCESS_H_
#define FILESYSTEM_ACCESS_H_

typedef struct filepath {
    char *path;
    size_t path_len;
} filepath;


filepath set_path(filepath dest, filepath src);
filepath get_full_path(filepath dir, filepath file);
void traverse_dir(filepath cwd);


#endif /* FILESYSTEM_ACCESS_H_ */
