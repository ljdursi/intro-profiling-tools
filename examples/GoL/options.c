#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <mpi.h>
#include "rundata.h"
#include "options.h"

int get_options(int argc, char **argv, rundata_t *rundata) {
    
                                        
    int overlap;
    struct option long_options[] = {
        {"npr",        required_argument, 0, 'R'},
        {"npc",        required_argument, 0, 'C'},
        {"niters",     required_argument, 0, 'n'},
        {"outevery",   required_argument, 0, 'o'},
        {"outfilename",required_argument, 0, 'f'},
        {"infilename", required_argument, 0, 'i'},
        {"overlap",    no_argument, &overlap, 1},
        {"nooverlap",  no_argument, &overlap, 0},
        {"help",       no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int c;
    int option_index;
    int tempint;
    FILE *tst;
    char *defaultfname=rundata->filename;


    while (1) { 
        c = getopt_long(argc, argv, "R:C:n:o:f:i:h", long_options, 
                        &option_index);
        if (c == -1) break;

        switch (c) { 
            case 0: if (long_options[option_index].flag != 0)
                    break;

            case 'n': tempint = atoi(optarg);
                      if (tempint < 1 || tempint > 500000) {
                          fprintf(stderr,
                                  "%s: Cannot use number of timesteps %s;\n",
                                  argv[0], optarg);
                          fprintf(stderr,"  Using %d\n", rundata->niters);
                      } else {
                          rundata->niters = tempint;
                      }
                      break;


            case 'o': tempint = atoi(optarg);
                      if (tempint < 1 || tempint > 500) {
                          fprintf(stderr,
                                  "%s: Cannot output every %s timesteps;\n",
                                  argv[0], optarg);
                          fprintf(stderr,"  Using %d\n", rundata->outevery);
                      } else {
                          rundata->outevery = tempint;
                      }
                      break;


            case 'R': tempint = atoi(optarg);
                      if (tempint < 1 || tempint > rundata->nprocs) {
                          fprintf(stderr,
                                  "%s: Cannot use number of processors in row direction %s;\n",
                                  argv[0], optarg);
                          fprintf(stderr,"  Using %d\n", rundata->npr);
                      } else if (rundata->nprocs % tempint != 0) {
                          fprintf(stderr,
                                  "%s: Number of processors in row direction %s does not divide %d;\n",
                                  argv[0], optarg, rundata->nprocs);
                          fprintf(stderr,"  Using %d\n", rundata->npr);
                      } else {
                          rundata->npr = tempint;
                          rundata->npc = rundata->nprocs / tempint;
                      }
                      break;

            case 'C': tempint = atoi(optarg);
                      if (tempint < 1 || tempint > rundata->nprocs) {
                          fprintf(stderr,
                                  "%s: Cannot use number of processors in column direction %s;\n",
                                  argv[0], optarg);
                          fprintf(stderr,"  Using %d\n", rundata->npc);
                      } else if (rundata->nprocs % tempint != 0) {
                          fprintf(stderr,
                                  "%s: Number of processors column y direction %s does not divide %d;\n",
                                  argv[0], optarg, rundata->nprocs);
                          fprintf(stderr,"  Using %d\n", rundata->npc);
                      } else {
                          rundata->npc = tempint;
                          rundata->npr = rundata->nprocs / tempint;
                      }
                      break;

            case 'f': if (!(tst=fopen(optarg,"w"))) {
                          fprintf(stderr,
                                  "%s: Cannot use filename %s;\n",
                                  argv[0],rundata->filename);
                          fprintf(stderr, "  Using %s\n",defaultfname);
                          strcpy(rundata->filename, defaultfname);
                      } else {
                          fclose(tst);
                          strncpy(rundata->filename, optarg, MAXFILENAME-1); 
                      }
                      break; 

            case 'i': if (strlen(optarg) >= MAXFILENAME) {
                         fprintf(stderr, 
                                  "%s: input filename %s too long\n", 
                                   argv[0], optarg);
                         fprintf(stderr,"Using %s\n",rundata->infilename);
                      } else if (!(tst=fopen(optarg,"r"))) {
                              fprintf(stderr,
                                  "%s: Cannot open input filename %s;\n",
                                  argv[0],rundata->filename);
                              fprintf(stderr, "  Using %s\n",rundata->infilename);
                      } else  {
                          fclose(tst);
                          strncpy(rundata->infilename, optarg, MAXFILENAME-1);
                      }
                      break; 

            case 'h':
                  puts("Options: ");
                  puts("    --niters=N      (-n N): Set the number of iterations to run.");
                  puts("    --outevery=N    (-o N): Set the frequency of outputs.");
                  puts("    --npr=N         (-R N): Set the number of processors across rows.");
                  puts("    --npc=N         (-C N): Set the number of processors across columns.");
                  puts("    --outfilename=S (-f S): Set the output base filename.");
                  puts("    --infilename=S  (-i S): Set the input filename.");
                  puts("    --[no]overlap        : Overlap (or not) communications.");
                  puts("");
                  return +1;

            default: printf("Invalid option %s\n", optarg);
                 break;
        }
    }

    rundata->overlap = overlap;
    return 0;
}
