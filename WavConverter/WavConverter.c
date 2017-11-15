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

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dirent.h>
#include <lame.h>

#include <getopt.h>
#include "system_shims.h"
#include "filesystem_access.h"

#define PROGRAM "WavConverter"
#define VERSION "v0.1"
#define LICENSE "Some license copyright (C) 2017"
#define AUTHOR "Jordan Montgomery"


/*****************************************************************************************
 * Defined parameters and globals needed for init
 ****************************************************************************************/
/* There is no way to safely use getcwd() without a max path size */
#define PATHNAME_MAX_SIZE 256 

char *executable_name;

struct option opts[] = {
    {"help",        no_argument, 0, 'h'},
    {"usage",       no_argument, 0, 'u'},
    {"version",     no_argument, 0, 'v'},
    {"output",      required_argument, 0, 'o'},
    {"optimize",    required_argument, 0, 'O'},
    {"max-cores",   required_argument, 0, 'n'},
    {0, 0, 0, 0}
  };

enum quality_lvl {
    OPTIMIZE_QUALITY = 0,
    OPTIMIZE_SIZE    = 1,
    OPTIMIZE_SPEED   = 2
};


typedef struct parameters {
    filepath input_dir;
    filepath output_dir;

    int   quality_lvl;
    int   max_cores;
} parameters;


typedef struct thread_args {
    int thread_id;
    filepath in_file;
    filepath out_file;
} thread_args;



/*! This struct is an alternative to a semaphore, which would be less clear here */
struct sync_block {
    MUTEX_T mutex;
    COND_T cond_var;
    int counter;
};

struct sync_block sem = { .counter = 0 };


/*****************************************************************************************
* Function prototypes
****************************************************************************************/
/* Argument parsing protytypes*/
void usage(void);
void version(char *name, char *version, char *license, char *author);
void parseOpts(parameters *params, int argc, char *argv[]);


/* Misc. function prototypes */

void *convert_wav(void *arg);


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
\t%s --optimize=quality ~/\n\
Options:\n\
\t-o, --output    [DIR]\n\
\t-n, --max-cores [N]\n\
\t-O, --optimize  [quality|size|speed]\n\
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

void parseOpts(parameters *params, int argc, char *argv[]) {
    int opt;
    int sync_out_dir = 1;

    while(optind < argc) {
        opt = getopt_long(argc, argv, "huvo:O:n:", opts, NULL);
        if(opt == -1) { // Handle non-option arguments (esp. input dir)
            filepath opt_dir = { argv[optind], strlen(argv[optind]) };
            params->input_dir = set_path(params->input_dir, opt_dir);

            if(sync_out_dir) // We want to sync input & output dirs if -o isn't explicitly set
                params->output_dir = set_path(params->output_dir, params->input_dir);
            optind++;
        }
        else {
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
                printf("-o %s\n", optarg);
                filepath opt_dir = { optarg, strlen(optarg) };
                params->output_dir = set_path(params->output_dir, opt_dir);
                sync_out_dir = 0;
                break;
            }
            case 'O':
                if(strcmp(optarg, "quality") == 0)
                    params->quality_lvl = OPTIMIZE_QUALITY;
                else if(strcmp(optarg, "size") == 0)
                    params->quality_lvl = OPTIMIZE_SIZE;
                else if(strcmp(optarg, "speed") == 0)
                    params->quality_lvl = OPTIMIZE_SPEED;
                else {
                    puts("Unknown quality");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'n':
            {
                int max_threads = atoi(optarg);
                if(max_threads && (max_threads < params->max_cores)) // test for sane values
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
    }
}

/*****************************************************************************************
* Misc. Functions
****************************************************************************************/

void encode(filepath input, filepath output) {
    int read, write;

    FILE *pcm = fopen(input.path, "rb");
    FILE *mp3 = fopen(output.path, "wb");


    if(pcm == NULL || mp3 == NULL) {
        printf("Could not open files\n");
    }
    else {
        printf("Opened both files\n");
    }

    const int PCM_SIZE = 8192;
    const int MP3_SIZE = 8192;

    short int pcm_buffer[8192 * 2];
    unsigned char mp3_buffer[8192];

    lame_t lame = lame_init();
    //lame_set_in_samplerate(lame, 44100);
    lame_set_VBR(lame, vbr_default);
    //lame_set_quality(
    lame_init_params(lame);

    do {
        read = fread(pcm_buffer, 2 * sizeof(short int), 8192, pcm);
        if(read == 0)
            write = lame_encode_flush(lame, mp3_buffer, 8192);
        else
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, 8192);
        fwrite(mp3_buffer, write, 1, mp3);
    } while(read != 0);

    lame_close(lame);

    fclose(mp3);
    fclose(pcm);
}

void *convert_wav(void *arg)
{
    thread_args *args = arg;

    printf("searching for file %s | %s\n", args->in_file.path, args->out_file.path);
    encode(args->in_file, args->out_file);

    mutex_lock(&sem.mutex);
    sem.counter--;
    cond_signal(&sem.cond_var); 
    mutex_unlock(&sem.mutex);

    free(args->in_file.path);
    free(args->out_file.path);
    free(args); // Avoid a temporary memory leak
    return 0;
}

void wav_file_found(filepath dir, filepath file, void *args) {
    parameters params = *(parameters *)args;

    struct thread_args *t_params = malloc(sizeof(thread_args));
    t_params->thread_id = rand();

    t_params->in_file = get_full_path(dir, file);
    memcpy(&file.path[file.path_len-3], "mp3", 3);
    t_params->out_file = get_full_path(params.output_dir, file);

    TID_T tid;

    mutex_lock(&sem.mutex);

    if(sem.counter >= params.max_cores) {
        printf("Waiting\n");
        cond_wait(&sem.cond_var, &sem.mutex);
        printf("Starting thread\n");
    }
        

    create_thread(tid, convert_wav, t_params);

    sem.counter++;
    mutex_unlock(&sem.mutex);

    printf("created: %i\n", t_params->thread_id);
}

/*****************************************************************************************
 * Main
 ****************************************************************************************/
int main (int argc, char *argv[]) {
    
    executable_name = argv[0];

    parameters params = { .input_dir   = (filepath) {NULL, 0},
                          .output_dir  = (filepath) {NULL, 0},
                          .quality_lvl = OPTIMIZE_QUALITY,
                          .max_cores   = getNumCPUs() };

    params.input_dir.path = getCwd(NULL, INITIAL_SYS_PATH_LEN); // getcwd() will malloc enough memory
    params.input_dir.path_len = max(strlen(params.input_dir.path), INITIAL_SYS_PATH_LEN);

    // Copy the dir over in case we receive no arguments on the command line
    params.output_dir.path = calloc(params.input_dir.path_len, 1);
    memcpy(params.output_dir.path, params.input_dir.path, params.input_dir.path_len + 1);
    params.output_dir.path_len = params.input_dir.path_len;

    if(argc == 1) {
        usage();
        exit(EXIT_SUCCESS);
    }


    parseOpts(&params, argc, argv);

    params.input_dir = normalize_filepath(params.input_dir);
    params.output_dir = normalize_filepath(params.output_dir);

    printf("\
    parameters params = {.input_dir   = %s,\n\
                         .output_dir  = %s,\n\
                         .quality_lvl = %i,\n\
                         .max_cores   = %i };\n", params.input_dir.path, params.output_dir.path, params.quality_lvl, params.max_cores);

    int i_exist = f_access(params.input_dir.path, test_existence);
    int o_exist = f_access(params.output_dir.path, test_existence);

    if(i_exist == -1) {
        printf("Input dir %s does not exist\n", params.input_dir.path);
        exit(EXIT_FAILURE);
    } else if(o_exist == -1) { // Should we try to create the dir?
        printf("Output dir %s does not exist\n", params.output_dir.path);
        exit(EXIT_FAILURE);
    }

    printf("f_exist %i | %i\n", i_exist, o_exist);

    mutex_init(&sem.mutex);
    cond_init(&sem.cond_var);

    callback cb = { .func = &wav_file_found,
                    .args = &params };
    traverse_dir(params.input_dir, "wav", cb);

    mutex_lock(&sem.mutex);

    while(sem.counter > 0) // Idle while the threads do their work
        cond_wait(&sem.cond_var, &sem.mutex);

    mutex_unlock(&sem.mutex);

    mutex_destroy(&sem.mutex);
    cond_destroy(&sem.cond_var);

    /*const int max_threads = params.max_cores;
    mutex_init(&sem.mutex);
    cond_init(&sem.cond_var);

    for (int i = 0; i < max_threads; i++) {
        thread_args *args = malloc(sizeof(thread_args));
        args->thread_id = i;

        // TODO: Rewrite to use params.input_dir
        args->in_file = (filepath) {NULL, 0};
        args->out_file = (filepath) {NULL, 0};

        TID_T tid;
        create_thread(tid, convert_wav, args);
        printf("created: %i\n", args->thread_id);
    }

    mutex_unlock(&sem.mutex);

    while(sem.counter < max_threads) {
        cond_wait(&sem.cond_var, &sem.mutex); 
        printf("%i of %i threads are done\n", sem.counter, max_threads);
    }
  
    printf("All %i threads finished\n", sem.counter);
    mutex_unlock(&sem.mutex);

    mutex_destroy(&sem.mutex);
    cond_destroy(&sem.cond_var);*/

    exit(EXIT_SUCCESS);


    /*

    int len;
    struct dirent *pDirent;
    DIR *cwd;*/

    
    /*
    FILE *pcm = fopen(wav_fullpath.path, "rb");
    if(pcm != NULL) {
    printf("File %s opened!\n", pDirent->d_name);
    fclose(pcm);
    }
    else {
    printf("File could not be opened\n");
    }*/

    /*
    cwd = opendir (argv[1]);
    if (cwd == NULL) {
        printf ("Cannot open directory '%s'\n", argv[1]);
        return 1;
    }

    while ((pDirent = readdir(cwd)) != NULL) {
        printf ("[%s]\n", pDirent->d_name);
        if(strstr(pDirent->d_name, ".wav") != NULL) {
            char *filepath = malloc(strlen(pDirent->d_name) + strlen(argv[1]) + 1);
            strcpy(filepath, argv[1]);
            strcat(filepath, "/");
            strcat(filepath, pDirent->d_name);
            printf("Filepath %s \n",  filepath);

            FILE *pcm = fopen(filepath, "rb");
            if(pcm != NULL) {
                printf("File %s opened!\n", pDirent->d_name);
                fclose(pcm);

                char *help = getCwd(filepath, strlen(filepath));
                printf("cwd %s\n", help);
            } else {
                printf("File could not be opened\n");
            }

        }
    }
    closedir (cwd);
    exit(EXIT_SUCCESS);*/

        /*int read, write;
    
        FILE *pcm = fopen("samples/s1.wav", "rb");
        FILE *mp3 = fopen("output/s1.mp3", "wb");


        if( pcm == NULL || mp3 == NULL) {
            printf("Null\n");
        }
        else {
            printf("wow\n");
        }
    
        const int PCM_SIZE = 8192;
        const int MP3_SIZE = 8192;
    
        short int pcm_buffer[PCM_SIZE*2];
        unsigned char mp3_buffer[MP3_SIZE];
    
        lame_t lame = lame_init();
        //lame_set_in_samplerate(lame, 44100);
        lame_set_VBR(lame, vbr_default);
        lame_set_quality(
        lame_init_params(lame);
    
        do {
            read = fread(pcm_buffer, 2*sizeof(short int), PCM_SIZE, pcm);
            if (read == 0)
                write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
            else
                write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
            fwrite(mp3_buffer, write, 1, mp3);
        } while (read != 0);
    
        lame_close(lame);
        fclose(mp3);
        fclose(pcm);
    
        return 0;*/
    
}
