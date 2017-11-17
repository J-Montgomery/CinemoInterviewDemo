#ifndef FILESYSTEM_ACCESS_H_
#define FILESYSTEM_ACCESS_H_

#include <stdbool.h>
#include <stdint.h>

/*
 * Thin wrapper around strings representing filepaths, with a defined length.
 */
typedef struct filepath_t {
    char *path;
    size_t path_len;
} filepath;

typedef void(*file_found_cb)(filepath dir, filepath file, void *args);

typedef struct callback_t {
    file_found_cb func;
    void *args;
} callback;

typedef struct wav_header_t {
    uint8_t  chunk_id[4];            // "RIFF"
    uint32_t chunk_size;             // filesize - 8 bytes
    uint8_t  format[4];              // "WAVE"
	uint8_t  subchunk_id[4];         // "fmt "
	uint32_t subchunk_len;           // length of the format data
	uint16_t format_type;            // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16_t n_channels;             // no.of channels
	uint32_t sample_rate;            // sampling rate (blocks per second)
	uint32_t byte_rate;              // sample_rate * n_channels * bits_per_sample/8
	uint16_t block_align;            // n_channels * bits_per_sample/8
	uint16_t bits_per_sample;        // bits per sample, 8- 8bits, 16- 16 bits etc
} wav_header;

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




bool parse_wav(wav_header *params, FILE *wav);

#endif /* FILESYSTEM_ACCESS_H_ */
