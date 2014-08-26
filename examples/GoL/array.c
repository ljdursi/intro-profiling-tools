#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "rundata.h"
#include "array.h"
#include "partition.h"
#include "gol.h"

void chk(const char *routine, int ierr) {
    char str1[MPI_MAX_ERROR_STRING+1];
    char str2[MPI_MAX_ERROR_STRING+1];
    int len, class;
    if (ierr != MPI_SUCCESS) {
        MPI_Error_string(ierr, str1, &len);
        MPI_Error_class(ierr, &class);
        MPI_Error_string(class, str2, &len);
        fprintf(stderr,"%s: MPI Error #%d-%d: %s;\n\t%s\n", routine, class, ierr, str2, str1);
    }
    return;
}

char **array2d(int nr, int nc) {
    int i;
    char *data = (char *)malloc(nr*nc*sizeof(char));
    char **p = (char **)malloc(nr*sizeof(char *));

    if (data == NULL) return NULL;
    if (p == NULL) {
        free(data);
        return NULL;
    }

    for (i=0; i<nr; i++) {
        p[i] = &(data[nc*i]);
    }
    
    return p;
}

void freearray2d(void **p) {
    free(p[0]);
    free(p);
    return;
}



void printarray2d(rundata_t *d, int hdr, int array, int gc, FILE *out) {
    int r,c;
    int nr, nc;
    int startr, startc;

    if (gc != NOSHOWGUARD) {
        nr = (d->localnr)+2*(d->ng);
        nc = (d->localnc)+2*(d->ng);
        startr = 0;
        startc = 0;
    } else {
        nr = (d->localnr);
        nc = (d->localnc);
        startr = d->ng;
        startc = d->ng;
    }

    if (hdr != NOSHOWHEADER) {
        fprintf(out,"%3d\n",nr);
        fprintf(out,"%3d\n",nr);
    }
    if (array == NEIGHS) {
        for (r=startr;r<startr+nr;r++) {
            for (c=startc;c<startc+nc;c++) {
                fprintf(out,"%3d ",(int)(d->neighs[r][c]));
            }
            fputs("\n",out);
        } 
    } else {
        for (r=startr;r<startr+nr;r++) {
            for (c=startc;c<startc+nc;c++) {
                fprintf(out,"%c",d->data[r][c]);
            }
            fputs("\n",out);
        } 
    }

    return;
}


void readarray2d(char *fname, char ***d, int *nr, int *nc) {
    FILE *in;
    const int maxline=128;
    char line[maxline+2];
    int i;
    
    in = fopen(fname,"r");
    if (!in) {
        fprintf(stderr,"readarray2d: Could not open file.\n");
        return;
    }  

    i = fscanf(in,"%3d\n",nr);
    if (i != 1) {
       fprintf(stderr,"Could not read nrows\n");
       fclose(in);
       return;
    }
    i = fscanf(in,"%3d\n",nc);
    if (i != 1) {
       fprintf(stderr,"Could not read ncols\n");
       fclose(in);
       return;
    }

    if (*nr < 0 || *nr > maxline || *nc < 0 || *nc > maxline) {
        fprintf(stderr,"readarray2d: invalid nr, nc (%d,%d)\n", *nr, *nc);
        fclose(in);
        return;
    }

    *d = array2d(*nr, *nc);
    memset(&((*d)[0][0]),DEAD,(*nr)*(*nc)*sizeof(char));
    for (int row=0;row<*nr;row++) {
        if (feof(in)) {
              fprintf(stderr,"readarray2d: File too short.\n");
              fclose(in);
              return;
        }
        fgets(line,maxline,in);
        if (strlen(line) < *nc) {
          fprintf(stderr,"readarray2d: Line too short.\n");
          fclose(in);
          return;
        }
        for (int col=0;col<*nc;col++) {
            (*d)[row][col] = (line[col] == LIVE ? LIVE : DEAD);
        }
    } 
    fclose(in);
    return;
}

void preadarray2d_mpiio(char *fname, rundata_t *rundata) {
    const int HDRSIZE=8;
    char line[HDRSIZE+2];
    int i;
    int ierr;
    MPI_File in;
    MPI_Status status;
    MPI_Comm comm = rundata->comm_cart;
    int nrg, ncg, nr, nc, gnr, gnc, ng;
    int sizes[2], subsizes[2], starts[2];

    ierr=MPI_File_open(comm, fname, MPI_MODE_RDONLY, MPI_INFO_NULL,  &in);
    if (ierr != MPI_SUCCESS) {
        fprintf(stderr,"preadarray2d_mpiio: Could not open file %s.\n",fname);
        return;
    }  
    chk("preadarray2d: opening", ierr);

    ierr= MPI_File_read_all(in, line, HDRSIZE, MPI_CHAR, &status);
    line[8]='\0';
    chk("preadarray2d_mpiio", ierr);
    
    i = sscanf(line,"%3d\n%3d\n",&rundata->globalnr,&rundata->globalnc);
    if (i != 2) {
       fprintf(stderr,"Could not read nrows, ncols from file %s\n",fname);
       fprintf(stderr,"line = <%s>\n",line);
       fprintf(stderr,"Got: i = %d, nx = %d, ny = %d\n", i, rundata->globalnr, rundata->globalnc);
       MPI_File_close(&in);
       return;
    }

    /* divy up the domain */
    calcpartition(rundata);

    /* and allocate the arrays */
    nr = rundata->localnr;  nc = rundata->localnc;
    gnr= rundata->globalnr; gnc= rundata->globalnc;
    ng = rundata->ng;
    nrg= nr+2*ng;           ncg= nc+2*ng;

    rundata->data = array2d(nrg, ncg);
    rundata->neighs = (unsigned char **)array2d(nrg, ncg);

    memset(&(rundata->neighs[0][0]),   0, (nrg*ncg)*sizeof(unsigned char));
    memset(&(rundata->data[0][0]),  DEAD, (nrg*ncg)*sizeof(unsigned char));

    /* Now we want to set up some data types describing the layout *
     * of memory both in the file (choosing only our data),        * 
     * and in the array (skipping guardcells).                     */

    /* you can do this with hvectors, but subarray is so much easier...*/

    /* In the file, first.  Rows have a carriage return at end. */
    sizes[0]    = gnr;   sizes[1]    = gnc+1;
    subsizes[0] = nr;    subsizes[1] = nc;
    starts[0]   = rundata->startr;
    starts[1]   = rundata->startc;

    MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, 
                            MPI_CHAR, &rundata->filetype);
    chk("preadarray2d_mpiio", ierr);
    MPI_Type_commit(&rundata->filetype);
    chk("preadarray2d_mpiio", ierr);

    /* then with local data */
    sizes[0]    = nrg; sizes[1]    = ncg;
    subsizes[0] = nr;  subsizes[1] = nc;
    starts[0]   = ng;  starts[1]   = ng;

    MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, 
                            MPI_CHAR, &rundata->memtype);
    chk("preadarray2d_mpiio", ierr);
    MPI_Type_commit(&rundata->memtype);
    chk("preadarray2d_mpiio", ierr);

    ierr =  MPI_File_set_view(in, HDRSIZE, MPI_CHAR, rundata->filetype, 
                              "native", MPI_INFO_NULL);
    chk("preadarray2d_mpiio", ierr);

    /* Now that that's done, read the file */
    ierr =  MPI_File_read_all(in, &(rundata->data[0][0]), 1, rundata->memtype, 
                          &status);
    chk("preadarray2d_mpiio", ierr);
    
    /* .. and close it */ 
    MPI_File_close(&in);

    /* clean up the data */
    for (int row=0; row<nrg; row++)
        for (int col=0; col<ncg; col++)
            if (rundata->data[row][col] != LIVE) rundata->data[row][col] = DEAD;
    return;
}

void pwritearray2d_mpiio(char *fname, rundata_t *rundata) {
    const int HDRSIZE=8;
    char line[HDRSIZE+2];
    int ierr;
    MPI_File out;
    MPI_Status status;
    MPI_Comm comm = rundata->comm_cart;
    static char *crs;

    if (!(rundata -> edgetype)) {
        /* something to put the carriage returns in the file */
        int gnc, ng, gnr;
        ng = rundata->ng;
        gnr = rundata->globalnr; 
        gnc = rundata->globalnc;

        int sizes[2] = {gnr, gnc+1};
        int subsizes[2] = {gnr, 1};
        int starts[2] = {0, gnc};

        MPI_Type_create_subarray(2, sizes, subsizes, starts, MPI_ORDER_C, MPI_CHAR, &(rundata->edgetype));
        MPI_Type_commit(&(rundata->edgetype));
        crs = (char *)malloc((gnr)*sizeof(char));
        for (int i=0;i<gnr;i++) crs[i]='\n';

    }

    ierr=MPI_File_open(comm, fname, MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL,  &out);
    if (ierr != MPI_SUCCESS) {
        fprintf(stderr,"pwritearray2d_mpiio: Could not open file %s.\n",fname);
        return;
    }  
    chk("pwritearray2d: opening", ierr);

    /* proc (0,0) writes the header */
    if ((rundata->myr == 0) && (rundata->myc == 0)) {
        sprintf(line,"%3d\n%3d\n",rundata->globalnr, rundata->globalnc);
        ierr= MPI_File_write(out, line, HDRSIZE, MPI_CHAR, &status);
        chk("pwritearray2d_mpiio", ierr);

    } 

    ierr =  MPI_File_set_view(out, HDRSIZE, MPI_CHAR, rundata->filetype, 
                              "native", MPI_INFO_NULL);
    chk("pwritearray2d_mpiio", ierr);

    /* Now that that's done, write our data to the file */
    ierr =  MPI_File_write_all(out, &(rundata->data[0][0]), 1, rundata->memtype, 
                          &status);
    chk("pwritearray2d_mpiio", ierr);
    
    /* go back to beginning */
    ierr = MPI_File_seek(out, 0, MPI_SEEK_SET);
    chk("pwrite: seek", ierr);
    ierr =  MPI_File_set_view(out, HDRSIZE, MPI_CHAR, rundata->edgetype, 
                              "native", MPI_INFO_NULL);
    chk("pwrite: seek+setview", ierr);

    /* Finally, proc (0,0) fills in the \n's */
    if ((rundata->myr == 0) && (rundata->myc == 0)) {
        ierr = MPI_File_write(out, crs, rundata->globalnr, MPI_CHAR, MPI_STATUS_IGNORE);
        chk("pwrite: seek+setview+write", ierr);
    } 
        
    MPI_File_close(&out);
    /* .. and close it */ 

    return;
}

