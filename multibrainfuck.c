#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>

#define CELL_SIZE (1024*1024*16)

static unsigned char cells[CELL_SIZE] = {0};
static pthread_mutex_t mutexes[CELL_SIZE] = PTHREAD_MUTEX_INITIALIZER;
static unsigned char* prog = NULL;
static size_t prog_size = 0;
typedef struct {
    size_t prog_index;
    size_t cell_index;
} brainstate;

static size_t inc_prog_index(size_t idx){
    ++idx;
    if(idx>=prog_size){
        fprintf(stderr, "\nend of program\n");
        exit(0);
    }
    return idx;
}

static brainstate skip_to_matching(brainstate brain, char inc, char dec){
    //for(brain.prog_index = inc_prog_index(brain.prog_index);prog[brain.prog_index]!='/';brain.prog_index = inc_prog_index(brain.prog_index));
    size_t depth = 1;
    do {
        brain.prog_index = inc_prog_index(brain.prog_index);
        unsigned char i = prog[brain.prog_index];
        if(i==dec) --depth;
        else if(i==inc) ++depth;
    } while(depth != 0);
    return brain;
}

static brainstate string_literal(brainstate brain){
    bool is_esc = false;
    cells[brain.cell_index] = 0;
    ++brain.cell_index;
    for(brain.prog_index = inc_prog_index(brain.prog_index);true;brain.prog_index = inc_prog_index(brain.prog_index)){
        char inst = prog[brain.prog_index];
        if(is_esc){
            switch(inst){
                default:
                    cells[brain.cell_index] = inst;
                    is_esc = false;
                    break;
            }
        } else {
            switch(inst){
                case '/':
                    is_esc = true;
                    break;
                case '"':
                    cells[brain.cell_index] = 0;
                    return brain;
                default:
                    cells[brain.cell_index] = inst;
                    break;
            }
        }
        brain.cell_index++;
    }
}


static brainstate interp(brainstate brain);
static void* interp_th_rt(void* arg){
    interp(*((brainstate*)arg));
    //printf("thread done\n");
    free(arg);
    return NULL;
}
static void interp_th(brainstate b){
    brainstate* brain = malloc(sizeof(brainstate));
    memcpy(brain, &b, sizeof(brainstate));
    pthread_t th;
    pthread_create(&th, NULL, interp_th_rt, brain);
}
static brainstate interp(brainstate brain){
    while(true){
        char inst = prog[brain.prog_index];
        switch(inst){
            case '$':
                size_t start = brain.prog_index+1;
                brainstate b = skip_to_matching(brain, '$', '$');
                size_t end = b.prog_index-1;
                unsigned char* str = calloc(sizeof(unsigned char), end-start+1);
                memcpy(str, prog+start, end-start+1 * sizeof(unsigned char));
                int v = atoi(str);
                free(str);
                if(v>=0 && v <=255){
                    cells[brain.cell_index] = v;
                } else {
                    fprintf(stderr, "out of bounds value %d in instruction %lu\n", v, brain.prog_index);
                }
                brain.prog_index=end+1;
                break;
            case '!':
                size_t last = 0;
                for(size_t i=0;i<CELL_SIZE;i++){
                    if(cells[i]!=0)last = i;
                }
                for(size_t i=0;i<=last;i++){
                    printf("CELL[%d]=%d, %c\n", i, cells[i], cells[i]);
                }
                break;
            case '>':
                brain.cell_index = (brain.cell_index+1)%CELL_SIZE;
                break;
            case '<':
                brain.cell_index = (brain.cell_index-1)%CELL_SIZE;
                break;
            case '+':
                ++cells[brain.cell_index];
                break;
            case '-':
                --cells[brain.cell_index];
                break;
            case '^':
                pthread_mutex_lock(mutexes+brain.cell_index);
                ++cells[brain.cell_index];
                pthread_mutex_unlock(mutexes+brain.cell_index);
                break;
            case 'v':
                pthread_mutex_lock(mutexes+brain.cell_index);
                --cells[brain.cell_index];
                pthread_mutex_unlock(mutexes+brain.cell_index);
                break;
            case '?':
                while(true){
                    pthread_mutex_lock(mutexes+brain.cell_index);
                    if(cells[brain.cell_index] == 0){
                        pthread_mutex_unlock(mutexes+brain.cell_index);
                        break;
                    } else {
                        pthread_mutex_unlock(mutexes+brain.cell_index);
                        sched_yield();
                    }
                }
            case 'z':
                sched_yield();
                break;
            case 'b':
                printf("\n");
                break;
            case ':':
                printf("%d", cells[brain.cell_index]);
                break;
            case '.':
                write(STDOUT_FILENO, cells+brain.cell_index, sizeof(unsigned char));
                break;
            case ',':
                read(STDIN_FILENO, cells+brain.cell_index, sizeof(unsigned char));
                break;
            case '[':
                if(cells[brain.cell_index]==0){
                    brain = skip_to_matching(brain, '[', ']');
                } else {
                    brain.prog_index = inc_prog_index(brain.prog_index);
                    brainstate b = interp(brain);
                    brain.cell_index = b.cell_index;
                    brain.prog_index--;
                    goto skip_inc;
                }
                break;
            case ']':
                return brain;
            case '{':
                brain.prog_index = inc_prog_index(brain.prog_index);
                interp_th(brain);
                brain = skip_to_matching(brain, '{', '}');
                break;
            case '}':
                return brain;
            case '/':
                brain = skip_to_matching(brain, '/', '/');
                break;
            case '"':
                brain = string_literal(brain);
                break;
            default:
                break;
        }
        brain.prog_index = inc_prog_index(brain.prog_index);
        skip_inc:
    }
    return brain;
}

int main(int argc, char *argv[]){
    const char* infilename = NULL;
    if(argc<2){
        fprintf(stderr, "missing filename\n");
        exit(1);
    }
    infilename = argv[1];
    FILE* infile = fopen(infilename, "rb");
    
    size_t prog_offset = 0;
    while(true){
        size_t nsize = 1024;
        size_t rsize = 1024;
        if(prog_size){
            nsize = prog_size * 2;
            rsize = prog_size;
        }
        prog = realloc(prog, sizeof(unsigned char) * nsize);
        if(prog==NULL){
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        int r = fread(prog+prog_offset, sizeof(unsigned char), rsize, infile);
        if(r==0){
            if(feof(infile)!=0){
                break;
            }
            if(ferror(infile)!=0){
                fprintf(stderr, "error reading file: %d\n", ferror(infile));
                exit(1);
            } else {
                break;
            }
        }
        prog_offset += r;
        prog_size += r;
    }
    fclose(infile);

    brainstate brain = {0};
    brain = interp(brain);
}
