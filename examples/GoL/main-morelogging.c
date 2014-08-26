#include <stdio.h>
#include <mpi.h>
#include "rundata.h"
#include "array.h"
#include "gol.h"
#include "options.h"
#include "partition.h"
#ifdef _mpilog
#include <mpe.h>
void createevents(int rank, int *inits, int *inite, int *comps, int *compe, int *gcs, int *gce, int *ios, int *ioe);
#endif

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

#ifdef _mpilog
    int start_init, end_init;
    int start_comp, end_comp;
    int start_gc,   end_gc;
    int start_io,   end_io;
    createevents(rundata.rank, &start_init, &end_init,
                               &start_comp, &end_comp,
                               &start_gc,   &end_gc,
                               &start_io,   &end_io);
#endif
    dims[0] = rundata.npr;
    dims[1] = rundata.npc;

    /* Now that we have the final answer for the processor grid,
     * make the communicators */

#ifdef _mpilog
    MPE_Log_event(start_init, 0, "Start Initialization");
#endif
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
#ifdef _mpilog
    MPE_Log_event(end_init, 0, "End Initialization");
    MPE_Log_event(start_io, 0, "Start IO");
#endif
    preadarray2d_mpiio(rundata.infilename, &rundata);
#ifdef _mpilog
    MPE_Log_event(end_io, 0, "End IO");
#endif
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
#ifdef _mpilog
            MPE_Log_event(start_io, 0, "Start IO");
#endif
            sprintf(outfilename,"%s%03d.txt",rundata.filename,outnum);
            pwritearray2d_mpiio(outfilename,&rundata);
            outnum++;
#ifdef _mpilog
            MPE_Log_event(end_io, 0, "End IO");
#endif
        }
        if (rundata.overlap) {
#ifdef _mpilog
            MPE_Log_event(start_gc, 0, "Start GC Fill init");
#endif
            guardcellexchange_nonblocking(&rundata, reqs);
#ifdef _mpilog
            MPE_Log_event(end_gc,     0, "End GC Fill init");
            MPE_Log_event(start_comp, 0, "Start Inner comp");
#endif
            iterate(&rundata, I_INNERONLY);
#ifdef _mpilog
            MPE_Log_event(end_comp,   0, "End Inner comp");
            MPE_Log_event(start_gc,   0, "Start GC Fill final");
#endif 
            guardcellexchange_nonblocking_end(reqs, statuses);
#ifdef _mpilog
            MPE_Log_event(end_gc,     0, "End GC Fill final");
            MPE_Log_event(start_comp, 0, "Start Outer comp");
#endif
            iterate(&rundata, I_OUTERONLY);
#ifdef _mpilog
            MPE_Log_event(end_comp,   0, "End Outer comp");
#endif
        } else {
#ifdef _mpilog
            MPE_Log_event(start_gc, 0, "Start GC Fill");
#endif
            guardcellexchange(&rundata);
#ifdef _mpilog
            MPE_Log_event(end_gc,     0, "End GC Fill");
            MPE_Log_event(start_comp, 0, "Start Computation");
#endif
            iterate(&rundata, I_ALL);
#ifdef _mpilog
            MPE_Log_event(end_comp,   0, "End Computation");
#endif
        }

    }

    sprintf(outfilename,"%s%03d.txt",rundata.filename,outnum);
    pwritearray2d_mpiio(outfilename,&rundata);
    finalize_rundata(&rundata);

    ierr = MPI_Finalize();
    return 0;
} 
#ifdef _mpilog
void createevents(int rank, int *inits, int *inite, int *comps, int *compe, int *gcs, int *gce, int *ios, int *ioe) {

    *inits = MPE_Log_get_event_number(); 
    *inite = MPE_Log_get_event_number(); 

    *comps = MPE_Log_get_event_number(); 
    *compe = MPE_Log_get_event_number(); 

    *gcs   = MPE_Log_get_event_number(); 
    *gce   = MPE_Log_get_event_number(); 

    *ios   = MPE_Log_get_event_number(); 
    *ioe   = MPE_Log_get_event_number(); 

    if (rank == 0) {
        MPE_Describe_state(*inits, *inite, "Intitialization", "blue");
        MPE_Describe_state(*comps, *compe, "Computation", "green");
        MPE_Describe_state(*gcs,   *gce,   "GC Filling", "red");
        MPE_Describe_state(*ios,   *ioe,   "I/O", "orange");
    }

    return;
}
#endif
