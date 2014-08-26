#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include "rundata.h"
#include "array.h"
#include "gol.h"

int nval(char c) { 
   return c==LIVE? 1 : 0; 
   // return (c == LIVE || (c>='0' && c<='8')) ? 1: 0; 
}

int allneighs(char **p, int r, int c) {
    int n= nval(p[r-1][c-1])+nval(p[r-1][c])+nval(p[r-1][c+1])+
           nval(p[r  ][c-1])+                nval(p[r  ][c+1])+
           nval(p[r+1][c-1])+nval(p[r+1][c])+nval(p[r+1][c+1]);
    return n;
}

void updatecell(char **p, unsigned char **n, int r, int c) {

    if (p[r][c] == LIVE) {
        p[r][c] = ( n[r][c] >= STARVE && n[r][c] <= CROWD ) ? LIVE : DEAD;
    } else {
        p[r][c] = ( n[r][c] == BIRTH ) ? LIVE : DEAD;
    } 

    //if (p[r][c] == LIVE) p[r][c]='0'+n[r][c]; else p[r][c]='a'+n[r][c];
    return;
}

void guardcellexchange(rundata_t *d) {
    int ncg, nrg, nc, nr, ng;
    ng = d->ng;
    nr = d->localnr;
    nc = d->localnc;
    nrg = nr+2*ng;
    ncg = nc+2*ng;
    int ierr;
    MPI_Status status;

    const int rtag=1, ltag=2, utag=3, dtag=4, urtag=5, ultag=6, drtag=7, dltag=8;

    /* make MPI types for GC exchange if we haven't already */

    if (! (d -> lrgctype) ) {
            /* up-down exchanges; exchange nc cells in ng rows */
            ierr = MPI_Type_contiguous(nc*ng, MPI_CHAR, &(d->udgctype));
            MPI_Type_commit(&(d->udgctype));

            /* left-right exchanges; exchange nr cells in ng columns */
            ierr = MPI_Type_vector(nr, ng, ncg, MPI_CHAR, &(d->lrgctype));
            MPI_Type_commit(&(d->lrgctype));
    
            /* corner guardcell exchanges; exchange ng cells in ng rows */
            ierr = MPI_Type_vector(ng, ng, nrg, MPI_CHAR, &(d->cornergctype));
            ierr = MPI_Type_commit(&(d->cornergctype));
    
    }

    /* do right, then left, then up, then down */
    ierr = MPI_Sendrecv(&(d->data[ng][nc]), 1, d->lrgctype, d->rneigh, rtag,
                        &(d->data[ng][0]),  1, d->lrgctype, d->lneigh, rtag, 
                        d->comm_cart, &status);
            
    ierr = MPI_Sendrecv(&(d->data[ng][ng]),    1, d->lrgctype, d->lneigh, ltag,
                        &(d->data[ng][nc+ng]), 1, d->lrgctype, d->rneigh, ltag,
                        d->comm_cart, &status);
            
    /* down is increasing row number */
    ierr = MPI_Sendrecv(&(d->data[ng][ng]), 1, d->udgctype, d->uneigh, utag,
                        &(d->data[nr+ng][ng]),  1, d->udgctype, d->dneigh, utag,
                        d->comm_cart, &status);

    ierr = MPI_Sendrecv(&(d->data[nr][ng]),1, d->udgctype, d->dneigh, dtag,
                        &(d->data[0][ng]), 1, d->udgctype, d->uneigh, dtag,
                        d->comm_cart, &status);
            
    /* Now corners - upper right, lower left, upper left, lower right */
    ierr = MPI_Sendrecv(&(d->data[ng][nc]), 1, d->cornergctype, d->urneigh, urtag,
                        &(d->data[nr+ng][0]),  1, d->cornergctype, d->dlneigh, urtag, 
                        d->comm_cart, &status);
            
    ierr = MPI_Sendrecv(&(d->data[nr][ng]),    1, d->cornergctype, d->dlneigh, dltag,
                        &(d->data[0][nc+ng]), 1, d->cornergctype, d->urneigh, dltag,
                        d->comm_cart, &status);
            
    ierr = MPI_Sendrecv(&(d->data[ng][ng]), 1, d->cornergctype, d->ulneigh, ultag,
                        &(d->data[nr+ng][nc+ng]),  1, d->cornergctype, d->drneigh, ultag,
                        d->comm_cart, &status);

    ierr = MPI_Sendrecv(&(d->data[nr][nc]),1, d->cornergctype, d->drneigh, drtag,
                        &(d->data[0][0]), 1, d->cornergctype, d->ulneigh, drtag,
                        d->comm_cart, &status);
}


void guardcellexchange_nonblocking(rundata_t *d, MPI_Request reqs[16]) {
    int ncg, nrg, nc, nr, ng;
    ng = d->ng;
    nr = d->localnr;
    nc = d->localnc;
    nrg = nr+2*ng;
    ncg = nc+2*ng;
    int ierr;

    const int rtag=1, ltag=2, utag=3, dtag=4, urtag=5, ultag=6, drtag=7, dltag=8;

    /* make MPI types for GC exchange if we haven't already */

    if (! (d -> lrgctype) ) {
            /* up-down exchanges; exchange nc cells in ng rows */
            ierr = MPI_Type_contiguous(nc*ng, MPI_CHAR, &(d->udgctype));
            MPI_Type_commit(&(d->udgctype));

            /* left-right exchanges; exchange nrg cells in ng columns */
            ierr = MPI_Type_vector(nr, ng, ncg, MPI_CHAR, &(d->lrgctype));
            MPI_Type_commit(&(d->lrgctype));
    
            /* corner guardcell exchanges; exchange ng cells in ng rows */
            ierr = MPI_Type_vector(ng, ng, nrg, MPI_CHAR, &(d->cornergctype));
            ierr = MPI_Type_commit(&(d->cornergctype));
    }

    /* do right, then left, then up, then down */
    ierr = MPI_Isend(&(d->data[ng][nc]), 1, d->lrgctype, d->rneigh, rtag, d->comm_cart, &reqs[0]);
    ierr = MPI_Irecv(&(d->data[ng][0]),  1, d->lrgctype, d->lneigh, rtag, d->comm_cart, &reqs[1]);
            
    ierr = MPI_Isend(&(d->data[ng][ng]),    1, d->lrgctype, d->lneigh, ltag, d->comm_cart, &reqs[2]);
    ierr = MPI_Irecv(&(d->data[ng][nc+ng]), 1, d->lrgctype, d->rneigh, ltag, d->comm_cart, &reqs[3]);
            
    /* down is increasing row number */
    ierr = MPI_Isend(&(d->data[ng][ng]),    1, d->udgctype, d->uneigh, utag, d->comm_cart, &reqs[4]);
    ierr = MPI_Irecv(&(d->data[nr+ng][ng]), 1, d->udgctype, d->dneigh, utag, d->comm_cart, &reqs[5]);

    ierr = MPI_Isend(&(d->data[nr][ng]),1, d->udgctype, d->dneigh, dtag, d->comm_cart, &reqs[6]);
    ierr = MPI_Irecv(&(d->data[0][ng]), 1, d->udgctype, d->uneigh, dtag, d->comm_cart, &reqs[7]);
            
    /* Now corners - upper right, lower left, upper left, lower right */
    ierr = MPI_Isend(&(d->data[ng][nc]), 1, d->cornergctype, d->urneigh, urtag, d->comm_cart, &reqs[8]);
    ierr = MPI_Irecv(&(d->data[nr+ng][0]),  1, d->cornergctype, d->dlneigh, urtag, d->comm_cart, &reqs[9]);
            
    ierr = MPI_Isend(&(d->data[nr][ng]),    1, d->cornergctype, d->dlneigh, dltag, d->comm_cart, &reqs[10]);
    ierr = MPI_Irecv(&(d->data[0][nc+ng]), 1, d->cornergctype, d->urneigh, dltag, d->comm_cart, &reqs[11]);
            
    ierr = MPI_Isend(&(d->data[ng][ng]), 1, d->cornergctype, d->ulneigh, ultag, d->comm_cart, &reqs[12]);
    ierr = MPI_Irecv(&(d->data[nr+ng][nc+ng]),  1, d->cornergctype, d->drneigh, ultag, d->comm_cart, &reqs[13]);

    ierr = MPI_Isend(&(d->data[nr][nc]),1, d->cornergctype, d->drneigh, drtag, d->comm_cart, &reqs[14]);
    ierr = MPI_Irecv(&(d->data[0][0]), 1, d->cornergctype, d->ulneigh, drtag, d->comm_cart, &reqs[15]);
    
    return;
}

void guardcellexchange_nonblocking_end(MPI_Request reqs[16], MPI_Status statuses[16]) {
    MPI_Waitall(16, reqs, statuses);
    return;
}

void iterate(rundata_t *d, int where) {
    int nr, nc, ng, nrg, ncg;
    nr = d->localnr;
    nc = d->localnc;
    ng = d->ng;
    nrg = nr + 2*ng;
    ncg = nc + 2*ng;

    int startr = ng;     /* where to start/end for neighbour count */
    int endr  = nrg-ng;
    int startc = ng;
    int endc  = ncg-ng;
    int ustartr = ng;     /* where to start/end for updating cells */
    int uendr  = nrg-ng;
    int ustartc = ng;
    int uendc  = ncg-ng;

    if (where == I_INNERONLY) {
        startr = 2*ng;
        endr   = nr;
        startc = 2*ng;
        endc   = nc;

        /* Here, we can't update even all of the cells we're counting yet, 
         * else it will mess up the outer zone's neighbour counts.   */
        ustartr = 3*ng; 
        uendr  = nr-ng;
        ustartc = 3*ng;
        uendc  = nc-ng;
    }

    if ((where == I_INNERONLY) || (where == I_ALL))  {
        /* count neighbours */
        for (int row=startr; row<endr; row++) {
            for (int col=startc; col<endc; col++) {
                d->neighs[row][col] = allneighs(d->data,row,col);
            }
        }

        /* update values  */
        for (int row=ustartr; row<uendr; row++) {
            for (int col=ustartc; col<uendc; col++) {
                updatecell(d->data, d->neighs, row, col);
            }
        }
    } else if (where == I_OUTERONLY ) {
        /* have to do these in separate bits... first, count neighbours*/

        /* top */
        startr = ng; endr = 2*ng;
        startc = ng; endc = ncg-ng;
        for (int row=startr; row<endr; row++) {
            for (int col=startc; col<endc; col++) {
                d->neighs[row][col] = allneighs(d->data,row,col);
            }
        }

        /* bottom */
        startr = nr; endr = nrg-ng;
        startc = ng; endc = ncg-ng;
        for (int row=startr; row<endr; row++) {
            for (int col=startc; col<endc; col++) {
                d->neighs[row][col] = allneighs(d->data,row,col);
            }
        }

        /* left + right.  Note that we've already done the corners. */
        startr = 2*ng; endr=nr;
        for (int row=startr; row<endr; row++) {
            /* left */
            for (int col=ng; col<2*ng; col++) {
                d->neighs[row][col] = allneighs(d->data,row,col);
            }
            /* right */
            for (int col=nc; col<ncg-ng; col++) {
                d->neighs[row][col] = allneighs(d->data,row,col);
            }
        }

        /* Now, update values.  We update some INNERONLY cells, too, 
         * because they couldn't be updated until we were finished our
         * neighbour counts for the OUTERONLY*/

        /* top */
        startr = ng; endr = 3*ng;
        startc = ng; endc = ncg-ng;
        for (int row=startr; row<endr; row++) {
            for (int col=startc; col<endc; col++) {
                updatecell(d->data, d->neighs, row, col);
            }
        }

        /* bottom */
        startr = nr-ng; endr = nrg-ng;
        startc = ng;    endc = ncg-ng;
        for (int row=startr; row<endr; row++) {
            for (int col=startc; col<endc; col++) {
                updatecell(d->data, d->neighs, row, col);
            }
        }

        /* left + right.  Note that we've already done the corners. */
        startr = 3*ng; endr=nr-ng;
        for (int row=startr; row<endr; row++) {
            /* left */
            for (int col=ng; col<3*ng; col++) {
                updatecell(d->data, d->neighs, row, col);
            }
            /* right */
            for (int col=nc-ng; col<nc+ng; col++) {
                updatecell(d->data, d->neighs, row, col);
            }
        }
    } else {
        fprintf(stderr,"iterate: invalid region chosen.\n");
    }
    return;    
}

