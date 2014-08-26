#ifndef _ARRAY_H
#include <stdio.h>

char **array2d(int nx, int ny);
void freearray2d(void **p);
void printarray2d(rundata_t *d, int hdr, int array, int gc, FILE *out);
void readarray2d(char *fname, char ***d, int *nx, int *ny);
void preadarray2d_mpiio(char *fname, rundata_t *rundata);
void pwritearray2d_mpiio(char *fname, rundata_t *rundata);
void chk(const char *routine, int ierr);

#define SHOWHEADER   1
#define NOSHOWHEADER 0
#define SHOWGUARD    1
#define NOSHOWGUARD  0
#define DATA   1
#define NEIGHS 2

#define _ARRAY_H 1
#endif
