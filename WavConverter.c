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

#include <dirent.h>
#include <lame/lame.h>

#include "getopt/getopt.h"
#include "system_shims.h"


#include <pthread.h>

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
    char *input_dir;
    char *output_dir;
    int   quality_lvl;
    int   max_cores;
} parameters;


typedef struct thread_args {
    int thread_id;
} thread_args;

int done = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/*****************************************************************************************
 * Functions
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
\t%s --optimize=quality ~\n\
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

    while((opt = getopt_long(argc, argv, "huvo:O:n:", opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
            case 'u':
                usage();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                version(PROGRAM, VERSION, LICENSE, AUTHOR);
                break;
            case 'o':
            {
                size_t optlen = strlen(optarg);
                memcpy(params->output_dir, optarg, MIN(optlen, PATHNAME_MAX_SIZE - 1));
                params->output_dir[optlen] = 0;

                sync_out_dir = 0;
                break;
            }
            case 'O':
            {
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
            }
            case 'n':
            {
                int max_threads = atoi(optarg);
                if(max_threads && (max_threads < params->max_cores)) // test for sane values
                    params->max_cores = max_threads;
                break;
            }
            
            default:
                break;
          }
    }

    
    if(optind < argc) { 
        // TODO: Consider allowing multiple input dirs using a vector
        size_t optlen = strlen(argv[optind]);
        memcpy(params->input_dir, argv[optind], MIN(optlen, PATHNAME_MAX_SIZE - 1));
        params->input_dir[optlen] = 0;

        if(sync_out_dir) // We want to sync input & output dirs if -o isn't explicitly set
            memcpy(params->output_dir, params->input_dir, PATHNAME_MAX_SIZE);
    }
        
  
}

void *routine(void *arg)
{
    thread_args *args = arg;
    sleep(getNumCPUs() - args->thread_id);

    pthread_mutex_lock(&mutex);
    done++;
    pthread_cond_signal(&cond); 
    pthread_mutex_unlock(&mutex);

    free(arg); // Avoid a memory leak
    return 0;
}

/*****************************************************************************************
 * Main
 ****************************************************************************************/
int main (int argc, char *argv[]) {
    
    executable_name = argv[0];

    char *output_dir = malloc(256);
    output_dir = getCwd(output_dir, PATHNAME_MAX_SIZE);

    parameters params = {.input_dir   = calloc(PATHNAME_MAX_SIZE, 1), 
                         .output_dir  = calloc(PATHNAME_MAX_SIZE, 1),
                         .quality_lvl = OPTIMIZE_QUALITY,
                         .max_cores   = getNumCPUs() };

    params.input_dir = getCwd(params.input_dir, PATHNAME_MAX_SIZE - 1);
    memcpy(params.output_dir, params.input_dir, PATHNAME_MAX_SIZE - 1);

    if(argc == 1) {
        usage();
        exit(EXIT_SUCCESS);
    }

    parseOpts(&params, argc, argv);

    printf("\
    parameters params = {.input_dir   = %s,\n\
                         .output_dir  = %s,\n\
                         .quality_lvl = %i,\n\
                         .max_cores   = %i };\n", params.input_dir, params.output_dir, params.quality_lvl, params.max_cores);
    

    const int max_threads = params.max_cores;

    for (int i = 0; i < max_threads; i++) {
        thread_args *args = malloc(sizeof(thread_args));
        args->thread_id = i;

        pthread_t tid;
        pthread_create(&tid, NULL, routine, args);
        printf("created: %i\n", args->thread_id);
    }

    // we're going to test "done" so we need the mutex for safety
    pthread_mutex_lock( &mutex );

    while(done < max_threads) {
        
        printf("Waiting on cond\n");
        pthread_cond_wait( &cond, &mutex ); 
        printf("%i of %i threads are done\n", done, max_threads);
        /* we go around the loop with the lock held */
    }
  
    pthread_mutex_unlock( & mutex );

    exit(EXIT_SUCCESS);

    /*
    for(int i = 1; i < argc; i++) {
        //printf("arg %s %s\n", argv[i], strstr(argv[i], "-o"));
        if(strcmp(argv[i], "--help") == 0) {
            usage();
        } else if(strcmp(argv[i], "--usage") == 0) {
            usage();
        } else if(strstr(argv[i], "-v") != NULL) { // Also catches --version
            version(PROGRAM, VERSION, LICENSE, AUTHOR);
        } else if(strcmp(argv[i], "-o") < 2) { // Also catches --output
            printf("wow %s %li\n", argv[i], strspn(argv[i], "-o"));
            if(argc > ++i) {
                printf("i %i %i\n", argc, i);
                output_dir = argv[i];
            } else {
                printf("Output required with -o\n");
                exit(EXIT_FAILURE);
            }    
        } else if(strstr(argv[i], "-q") != NULL) { // Also catches --quality
            if(argc > ++i) {
                output_dir = argv[i];
            } else {
                printf("Quality level required\n");
                exit(EXIT_FAILURE);
            }    
        } else { // This argument must be a directory

        }
    }

    int len;
    struct dirent *pDirent;
    DIR *cwd;*/

    


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
        lame_set_in_samplerate(lame, 44100);
        lame_set_VBR(lame, vbr_default);
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
