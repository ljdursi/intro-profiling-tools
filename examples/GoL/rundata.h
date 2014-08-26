#ifndef _RUNDATA_H

#define MAXFILENAME 128
#define RD_UNDEFINED   -1
typedef struct {
    int globalnr, globalnc;
    int localnr, localnc;
    int startc, startr;
    int niters;
    int outevery;
    int ng;
    int npc, npr, nprocs, rank;
    int myc, myr;
    int overlap;
    MPI_Comm comm_cart;
    MPI_Datatype filetype, memtype;
    MPI_Datatype lrgctype, udgctype;
    MPI_Datatype cornergctype;
    MPI_Datatype edgetype;
    int lneigh, rneigh;
    int uneigh, dneigh;
    int ulneigh, urneigh;
    int dlneigh, drneigh;
    char filename[MAXFILENAME+1];
    char infilename[MAXFILENAME+1];
    char **data;
    unsigned char **neighs;
} rundata_t;

void init_rundata(rundata_t *rundata);
void finalize_rundata(rundata_t *rundata);
#define _RUNDATA_H 1
#endif
