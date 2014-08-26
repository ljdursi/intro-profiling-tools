#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>

int get_options(int argc, char **argv, int *nsize, int *nsteps, int *dobin);
int alloc2d(float ***a, int n);
int free2d(float ***a, int n);
int alloc1d(float **x, int n);
int free1d(float **x, int n);

void tick(struct timeval *t);
double tock(struct timeval *t);

int main(int argc, char **argv) {

    float **a;
    float *x, *y;
    int size=2500;
    int binoutput=0;
    int transpose=0;
    FILE *out;
    struct timeval init, calc, io;
    double inittime, calctime, iotime;
    
    struct timeval t;
    unsigned int seed;

    /* get sizes */
    if (get_options(argc, argv, &size, &binoutput, &transpose)) 
        return -1;
    
    /* allocate data */
    if (alloc2d(&a,size))
        return -2;

    if (alloc1d(&x,size)) {
        free2d(&a,size);
        return -2;
    }

    if (alloc1d(&y,size)) {
        free2d(&a,size);
        free1d(&x,size);
        return -3;
    }

    /* initialize data */

    tick(&init);
    gettimeofday(&t, NULL);
    seed = (unsigned int)t.tv_sec;

    for (int i=0; i<size; i++) {
            x[i] = (double)i;
            y[i] = 0.;
    }

    float *p = &(a[0][0]);
    for (int i=0; i<size*size; i++) {
        p[i] = i;
    }
    inittime = tock(&init);


    /* do multiplication */

    tick(&calc);
    if (transpose) {
        #pragma omp parallel default(none) shared(x,y,a,size)
        #pragma omp for
        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                y[i] += a[i][j]*x[j];
            }
        }
    } else {
        #pragma omp parallel default(none) shared(x,y,a,size)
        #pragma omp for
        for (int j=0; j<size; j++) {
            for (int i=0; i<size; i++) {
                y[i] += a[i][j]*x[j];
            }
        }
    }
    calctime = tock(&calc);

    /* Now output files */
    tick(&io);
    if (binoutput) {
        out = fopen("Mat-vec.dat","wb");
        fwrite(&size, sizeof(int),   1,         out);
        fwrite(x,     sizeof(float), size,      out);
        fwrite(y,     sizeof(float), size,      out);
        fwrite(&(a[0][0]),     sizeof(float), size*size, out);
        fclose(out);
    } else {
        out = fopen("Mat-vec.dat","w");
        fprintf(out,"%d\n",size);

        for (int i=0; i<size; i++) 
            fprintf(out,"%f ", x[i]);

        fprintf(out,"\n");

        for (int i=0; i<size; i++) 
            fprintf(out,"%f ", y[i]);

        fprintf(out,"\n");

        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                fprintf(out,"%f ", a[i][j]);
            }
            fprintf(out,"\n");
        }
        fclose(out);
    }
    iotime = tock(&io);

    printf("Timing summary:\n\tInit: %8.5f sec\n\tCalc: %8.5f sec\n\tI/O : %8.5f sec\n",
            inittime, calctime, iotime);

    free2d(&a, size);
    free1d(&x, size);
    free1d(&y, size);
    return 0;
}

int alloc2d(float ***a, int n) {
    float *data = (float *)malloc(n*n*sizeof(float));
    if (data == NULL) return -1;

    *a = (float **)malloc(n*sizeof(float *));
    if (*a == NULL) {free(data); return -1;};

    for (int i=0; i<n; i++) 
        (*a)[i] = &(data[i*n]);
    
    return 0;
}


int free2d(float ***a, int n) {
    free (&((*a)[0][0]));
    free(*a);
    
    return 0;
}


int alloc1d(float **a, int n) {
    *a = (float *)malloc(n*sizeof(float));
    if (*a == NULL) return -1;

    return 0;
}

void tick(struct timeval *t) {
    gettimeofday(t, NULL);
}

/* returns time in microsecs from now to time described by t */
double tock(struct timeval *t) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (double)(now.tv_sec - t->tv_sec) + ((double)(now.tv_usec - t->tv_usec)/1000000.);

}

int free1d(float **a, int n) {
    free(*a);
    
    return 0;
}

int get_options(int argc, char **argv, int *nsize, int *dobin, int *dotrans) {

  int binoutput = *dobin;
  int transpose = *dotrans;

  const struct option long_options[] = {
    {"binary",   no_argument, &binoutput, 1},
    {"ascii",    no_argument, &binoutput, 0},
    {"transpose", no_argument, &transpose, 1},
    {"matsize", required_argument, NULL, 'n'},
    {"help",    no_argument, NULL, 'h'},
    {0, 0, 0, 0}};

  char c;
  int option_index;
  int tempint;
  double tempflt;

  while (1) {
    c = getopt_long(argc, argv, "n:h", long_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0: if (long_options[option_index].flag != 0)
        break;

      case 'n': tempint = atoi(optarg);
          if (tempint < 1 || tempint > 10000) {
            fprintf(stderr,"%s: Cannot use matrix size %s;\n  Using %d\n", argv[0], optarg, *nsize);
          } else {
            *nsize = tempint;
          }
          break;

      case 'h':
          puts("Options: ");
          puts("    --matsize=N   (-n N): Set matrix size.");
          printf("    --ascii             : Use ascii output%s.\n", (*dobin ? "" : "(default)"));
          printf("    --binary            : Use binary  output%s.\n", (*dobin ? "(default)": ""));
          puts("    --transpose         : Use opposite order accessing matrix.");
          puts("");
          return +1;

         break;
    }
  }

  *dobin = binoutput;
  *dotrans = transpose;
  return 0;

}
