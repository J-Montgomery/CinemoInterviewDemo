/* Write a C/C++ commandline application that encodes a set of WAV files to MP3
Requirements:
    (1) application is called with pathname as argument, e.g.
    <applicationname> F:\MyWavCollection all WAV-files contained directly in that folder are to be encoded to MP3

    (2) use all available CPU cores for the encoding process in an efficient way by utilizing multi-threading
    (3) statically link to lame encoder library
    (4) application should be compilable and runnable on Windows and Linux
    (5) the resulting MP3 files are to be placed within the same directory as the source WAV files, the filename extension should be changed appropriately to .MP3
    (6) non-WAV files in the given folder shall be ignored
    (7) multithreading shall be implemented by means of using Posix Threads (there exist implementations for Windows)
    (8) the Boost library shall not be used
    (9) the LAME encoder should be used with reasonable standard settings (e.g. quality based encoding with quality level "good") 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <dirent.h>
#include <lame/lame.h>
#include <getopt.h>

#include "system_shims.h"
#include "filesystem_access.h"

#define PROGRAM "WavConverter"
#define VERSION "v0.1"
#define LICENSE "Some license copyright (C) 2017"
#define AUTHOR "Jordan Montgomery"

/*****************************************************************************************
* Configuration defines
****************************************************************************************/
#define DEFAULT_Q_LVL (5)
#define PCM_SIZE      (8192)
#define MP3_SIZE      (8192)

/*****************************************************************************************
 * Structs and types
 ****************************************************************************************/
enum quality_lvl {
    OPTIMIZE_QUALITY_HIGH = 2,
    OPTIMIZE_QUALITY_MID = 5,
    OPTIMIZE_QUALITY_LOW = 7
};

struct option opts[] = {
    {"help",        no_argument, 0, 'h'},
    {"usage",       no_argument, 0, 'u'},
    {"version",     no_argument, 0, 'v'},
    {"output",      required_argument, 0, 'o'},
    {"quality",     required_argument, 0, 'q'},
    {"max-cores",   required_argument, 0, 'n'},
    {0, 0, 0, 0}
  };

typedef struct parameters {
    filepath input_dir;
    filepath output_dir;

    int   quality_lvl;
    int   max_cores;
} parameters;

typedef struct thread_args {
    filepath in_file;
    filepath out_file;

    int quality;
} thread_args;

/*! A straight semaphore would be less clear than this struct */
struct sync_block {
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
    int counter;
};

/*****************************************************************************************
* Globals
****************************************************************************************/
struct sync_block sem = { .counter = 0 };
char *executable_name;

/*****************************************************************************************
* Function prototypes
****************************************************************************************/
/* Argument parsing protytypes*/
void usage(void);
void version(char *name, char *version, char *license, char *author);
void parseOpts(parameters *params, int argc, char *argv[]);

/* Misc. function prototypes */
void encode(filepath input, filepath output, int quality);
void *convert_wav(void *arg);
void wav_file_found(filepath dir, filepath file, void *args);

/*****************************************************************************************
 * Parameter parsing Functions
 ****************************************************************************************/
void usage(void) {
    printf("\
Usage: %s [OPTION]... [DIR]\n\
or:  %s [DIR]\n\
Convert WAV files to MP3 via LAME\n\
\n\
Examples:\n\
\t%s F:\\MyWavCollection\n\
\t%s . -o output\n\
\t%s --quality mid ~/\n\
Options:\n\
\t-o, --output    [DIR]\n\
\t-n, --max-cores [N]\n\
\t-q, --quality   [high|mid|low]\n\
\t-v, --version\n\
\t-h, --help\n\
\t    --usage\
\n", executable_name, executable_name, executable_name, executable_name, executable_name);
}

void version(char *name, char *version, char *license, char *author) {
    printf("\
%s %s\n\
%s\n\
Written by %s\
\n", name, version, license, author);
}

//! Parse command line options
void parseOpts(parameters *params, int argc, char *argv[]) {
    int opt;
    int sync_out_dir = 1;

    while(1) {
        opt = getopt_long(argc, argv, "hvo:q:n:", opts, NULL);
        if(opt != -1) {
            switch(opt) {
            case 'h':
            case 'u':
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                version(PROGRAM, VERSION, LICENSE, AUTHOR);
                exit(EXIT_SUCCESS);
                break;
            case 'o':
            {
                filepath opt_dir = { optarg, strlen(optarg) };
                params->output_dir = set_path(params->output_dir, opt_dir);
                sync_out_dir = 0;
                break;
            }
            case 'q':
                if(strcmp(optarg, "high") == 0)
                    params->quality_lvl = OPTIMIZE_QUALITY_HIGH;
                else if(strcmp(optarg, "mid") == 0)
                    params->quality_lvl = OPTIMIZE_QUALITY_MID;
                else if(strcmp(optarg, "low") == 0)
                    params->quality_lvl = OPTIMIZE_QUALITY_LOW;
                else {
                    puts("Unknown quality level");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'n':
            {
                int max_threads = atoi(optarg);
                if(max_threads && (max_threads < 2*params->max_cores)) // test for sane values
                    params->max_cores = max_threads;
                break;
            }
            case '?':
            {
                int ind = optind - (int)(optopt == 0); // If given unknown short commands (e.g. -abc), optind will remain 
                                                       // indexed at the current index. Decrement optind otherwise. 
                printf("Unknown option %s\n", argv[ind]);
                exit(EXIT_FAILURE);
                break;
            }
            default:
                break;
            }
        }
        else
            break;      
    }

    if(optind < argc) { // Handle non-option arguments (esp. input dir)
        filepath opt_dir = { argv[optind], strlen(argv[optind]) };
        params->input_dir = set_path(params->input_dir, opt_dir);

        if(sync_out_dir) // We want to sync input & output dirs if -o isn't explicitly set
            params->output_dir = set_path(params->output_dir, params->input_dir);
        optind++;
    }  
}

/*****************************************************************************************
* Encoding Functions
****************************************************************************************/
//! Transcode the input WAV into an MP3 file in the output directory
void encode(filepath input, filepath output, int quality) {
    int read, write;

    FILE *pcm = fopen(input.path, "rb");
    FILE *mp3 = fopen(output.path, "wb+");

    if(pcm == NULL || mp3 == NULL) {
        printf("Could not open files\n");
        return;
    }

    // These would be better malloc'd on an MCU with a small stack
    short int *pcm_buffer = calloc(2 * PCM_SIZE, sizeof(short int));
    unsigned char *mp3_buffer = calloc(MP3_SIZE, sizeof(short int));

    lame_t lame = lame_init(); // No need to check for null ptrs yet, the config functions check internally
    lame_set_VBR(lame, vbr_default);
    lame_set_quality(lame, quality);
    int lame_success = lame_init_params(lame);

    if((lame_success < 0) || (pcm_buffer == NULL) || (mp3_buffer == NULL)) {
        printf("Encoder failed to init %s\n", input.path);
        fclose(pcm);
        fclose(mp3);
        return;
    }

    do {
        read = fread(pcm_buffer, 2 * sizeof(short int), PCM_SIZE, pcm);
        if(read != 0)
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
        else
            write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
        fwrite(mp3_buffer, write, 1, mp3);
    } while(read != 0);

    lame_mp3_tags_fid(lame, mp3);
    lame_close(lame);

    fclose(mp3);
    fclose(pcm);

    free(pcm_buffer);
    free(mp3_buffer);
}

//! Handle the busy-work of running the thread, including modifying the counter mutex and freeing passed arguments
void *convert_wav(void *arg)
{
    thread_args *args = arg;

    printf("encoding %s\n", args->out_file.path);
    encode(args->in_file, args->out_file, args->quality);

    pthread_mutex_lock(&sem.mutex);
    sem.counter--;
    pthread_cond_signal(&sem.cond_var);
    pthread_mutex_unlock(&sem.mutex);

    free(args->in_file.path);
    free(args->out_file.path);
    free(args); // Avoid memory leaks
    return 0;
}

/*! For every WAV found, spawn a thread to transcode it. If too many threads are currently running, block until we can
 *  safely spawn another.
 */
void wav_file_found(filepath dir, filepath file, void *args) {
    parameters params = *(parameters *)args;

    struct thread_args *t_params = malloc(sizeof(thread_args)); // This will be freed by the spawned thread
    if(t_params == NULL) {
        puts("Could not start thread");
        return;
    }

    t_params->in_file = get_full_path(dir, file);

    memcpy(&file.path[file.path_len-3], "MP3", 3);
    t_params->out_file = get_full_path(params.output_dir, file);

    t_params->quality = params.quality_lvl;

    pthread_t tid;
    pthread_mutex_lock(&sem.mutex);
    if(sem.counter >= params.max_cores)
        pthread_cond_wait(&sem.cond_var, &sem.mutex);

    sem.counter++;
    pthread_mutex_unlock(&sem.mutex);
    pthread_create(&tid, NULL, convert_wav, t_params);   
}

/*****************************************************************************************
 * Main
 ****************************************************************************************/
int main (int argc, char *argv[]) {
    
    executable_name = argv[0];

    parameters params = { .input_dir   = (filepath) {NULL, 0},
                          .output_dir  = (filepath) {NULL, 0},
                          .quality_lvl = OPTIMIZE_QUALITY_MID,
                          .max_cores   = getNumCPUs() };

    params.input_dir.path = getCwd(NULL, INITIAL_SYS_PATH_LEN); // getcwd() will malloc enough memory. If it cannot, there's no hope anyway.
    params.input_dir.path_len = MAX(strlen(params.input_dir.path), INITIAL_SYS_PATH_LEN); 

    // Copy the dir over in case we receive no arguments on the command line
    params.output_dir = set_path(params.output_dir, params.input_dir);

    if(argc == 1) {
        usage();
        exit(EXIT_SUCCESS);
    }
    else if(params.input_dir.path == NULL || params.output_dir.path == NULL) {
        puts("Could not allocate memory");
        exit(EXIT_FAILURE);
    }
        
    parseOpts(&params, argc, argv);

    params.input_dir = normalize_filepath(params.input_dir);
    params.output_dir = normalize_filepath(params.output_dir);

    int i_exist = f_access(params.input_dir.path, test_existence);
    int o_exist = f_access(params.output_dir.path, test_existence);

    if(i_exist == -1) {
        printf("Input dir %s does not exist\n", params.input_dir.path);
        exit(EXIT_FAILURE);
    } else if(o_exist == -1) { 
        printf("Output dir %s does not exist\n", params.output_dir.path);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&sem.mutex, NULL);
    pthread_cond_init(&sem.cond_var, NULL);

    callback cb = { .func = &wav_file_found,
                    .args = &params };
    traverse_dir(params.input_dir, ".wav", cb);

    pthread_mutex_lock(&sem.mutex);
    while(sem.counter > 0) // Idle while the threads do their work
        pthread_cond_wait(&sem.cond_var, &sem.mutex);

    pthread_mutex_unlock(&sem.mutex);

    pthread_mutex_destroy(&sem.mutex);
    pthread_cond_destroy(&sem.cond_var);

    // The OS will deallocate params.input_dir.path and params.output_dir.path automatically
    // On bare-metal embedded systems they should be deallocated for sanitation reasons
    return 0;
}
