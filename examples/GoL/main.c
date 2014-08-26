#include <stdio.h>
#include <mpi.h>
#include "rundata.h"
#include "array.h"
#include "gol.h"
#include "options.h"
#include "partition.h"

int main(int argc, char **argv) {
    int ierr;
    rundata_t rundata;
    int crank;
    int dims[2];
    int periods[2] = {1,1};
    char outfilename[128];
    int outnum = 0;
    MPI_Status statuses[16];
    MPI_Request reqs[16];

    init_rundata(&rundata);

    /* get our MPI positions in the global domain */
    ierr = MPI_Init(&argc, &argv);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &rundata.nprocs);
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rundata.rank);

    nearsquare( rundata.nprocs, &dims[0], &dims[1]);
    rundata.npr = dims[0];
    rundata.npc = dims[1];

    sprintf(rundata.filename,"gol_");
    sprintf(rundata.infilename,"in.txt");

    /* now get options */
    ierr =  get_options(argc, argv, &rundata);
    if (ierr) {
        ierr = MPI_Finalize();
        return ierr;
    }

    dims[0] = rundata.npr;
    dims[1] = rundata.npc;

    /* Now that we have the final answer for the processor grid,
     * make the communicators */

    ierr = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, 
                           &rundata.comm_cart);
    ierr = MPI_Comm_rank(rundata.comm_cart, &crank);
    ierr = MPI_Cart_coords(rundata.comm_cart, crank, 2, dims);
    rundata.myr = dims[0];
    rundata.myc = dims[1];

    ierr = MPI_Cart_shift(rundata.comm_cart, 1, 1, &rundata.lneigh, 
                          &rundata.rneigh);
    /* "down" is increasing row number -- eg, the way we print things out */
    ierr = MPI_Cart_shift(rundata.comm_cart, 0, 1, &rundata.uneigh, 
                          &rundata.dneigh);

    /* now we need to get corner neighbours, too...*/
    dims[0] = (rundata.myr + 1) % rundata.npr;
    dims[1] = (rundata.myc - 1 + rundata.npc) % rundata.npc;
    ierr = MPI_Cart_rank(rundata.comm_cart, dims, &rundata.dlneigh);

    dims[0] = (rundata.myr + 1) % rundata.npr;
    dims[1] = (rundata.myc + 1) % rundata.npc;
    ierr = MPI_Cart_rank(rundata.comm_cart, dims, &rundata.drneigh);

    dims[0] = (rundata.myr - 1 + rundata.npr) % rundata.npr;
    dims[1] = (rundata.myc + 1) % rundata.npc;
    ierr = MPI_Cart_rank(rundata.comm_cart, dims, &rundata.urneigh);

    dims[0] = (rundata.myr - 1 + rundata.npr) % rundata.npr;
    dims[1] = (rundata.myc - 1 + rundata.npc) % rundata.npc;
    ierr = MPI_Cart_rank(rundata.comm_cart, dims, &rundata.ulneigh);
    preadarray2d_mpiio(rundata.infilename, &rundata);

    if ((rundata.npr > rundata.globalnr/2) || (rundata.npc > rundata.globalnc/2)) {
        fprintf(stderr,"Error: need at least 2 rows & columns per processor.\n");
        fprintf(stderr,"(Rows, columns) = (%d,%d)\n", rundata.globalnr, rundata.globalnc);
        fprintf(stderr,"(Procs along Rows, Procs along Cols) = (%d,%d)\n", rundata.npr, rundata.npc);
        MPI_Finalize();
        return -1;
    }

    outnum = 0;

    for (int i=0;i<rundata.niters;i++) {

        if (i % rundata.outevery == 0) {
            sprintf(outfilename,"%s%03d.txt",rundata.filename,outnum);
            pwritearray2d_mpiio(outfilename,&rundata);
            outnum++;
        }
        if (rundata.overlap) {
            guardcellexchange_nonblocking(&rundata, reqs);
            iterate(&rundata, I_INNERONLY);
            guardcellexchange_nonblocking_end(reqs, statuses);
            iterate(&rundata, I_OUTERONLY);
        } else {
            guardcellexchange(&rundata);
            iterate(&rundata, I_ALL);
        }

    }

    sprintf(outfilename,"%s%03d.txt",rundata.filename,outnum);
    pwritearray2d_mpiio(outfilename,&rundata);
    finalize_rundata(&rundata);

    ierr = MPI_Finalize();
    return 0;
} 
