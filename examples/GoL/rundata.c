#include <mpi.h>
#include "rundata.h"
#include "array.h"

void init_rundata(rundata_t *rundata) {

    rundata -> globalnr = rundata -> globalnc = RD_UNDEFINED; 
    rundata -> localnr = rundata -> localnc   = RD_UNDEFINED; 
    rundata -> npr = rundata -> npc   = RD_UNDEFINED; 
    rundata -> npr = rundata -> npc   = RD_UNDEFINED; 
    rundata -> nprocs = rundata -> rank = RD_UNDEFINED;
    rundata -> comm_cart = MPI_COMM_NULL;
    rundata -> lneigh = rundata -> rneigh = RD_UNDEFINED;
    rundata -> uneigh = rundata -> dneigh = RD_UNDEFINED;
    rundata -> ulneigh = rundata -> dlneigh = RD_UNDEFINED;
    rundata -> urneigh = rundata -> drneigh = RD_UNDEFINED;
    rundata -> filetype = rundata -> memtype = NULL;
    rundata -> lrgctype = rundata -> udgctype = NULL;
    rundata -> cornergctype = NULL;
    rundata -> edgetype = NULL;
    rundata -> filename[0] = '\0';
    rundata -> infilename[0] = '\0';
    rundata -> data = (char **)NULL;
    rundata -> neighs = (unsigned char **)NULL;

    rundata -> ng = 1;
    rundata -> overlap = 1;
    rundata -> niters = 100;
    rundata -> outevery = 5;
    return;
} 

void finalize_rundata(rundata_t *rundata) {

    rundata -> globalnr = rundata -> globalnc = RD_UNDEFINED; 
    rundata -> localnr = rundata -> localnc   = RD_UNDEFINED; 
    rundata -> ng = RD_UNDEFINED;
    rundata -> npr = rundata -> npc   = RD_UNDEFINED; 
    rundata -> npr = rundata -> npc   = RD_UNDEFINED; 
    rundata -> nprocs = rundata -> rank = RD_UNDEFINED;
    rundata -> lneigh = rundata -> rneigh = RD_UNDEFINED;
    rundata -> uneigh = rundata -> dneigh = RD_UNDEFINED;
    rundata -> ulneigh = rundata -> dlneigh = RD_UNDEFINED;
    rundata -> urneigh = rundata -> drneigh = RD_UNDEFINED;
    rundata -> filename[0] = '\0';
    rundata -> infilename[0] = '\0';

    MPI_Type_free(&(rundata->filetype));
    MPI_Type_free(&(rundata->memtype));
    MPI_Type_free(&(rundata->lrgctype));
    MPI_Type_free(&(rundata->udgctype));
    MPI_Type_free(&(rundata->edgetype));
    //MPI_Type_free(&(rundata->cornergctype));
    MPI_Comm_free(&(rundata->comm_cart));
    rundata -> comm_cart = MPI_COMM_NULL;

    freearray2d((void **)(rundata -> data));
    freearray2d((void **)(rundata -> neighs));
    return;
} 
