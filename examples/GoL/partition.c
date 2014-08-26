#include <mpi.h>
#include <math.h>
#include "rundata.h"

void nearsquare(int nprocs, int *npx, int *npy) {
    int sq = ceil(sqrt((double)nprocs));
    int n,m;

    if (sq*sq == nprocs) {
        *npx = *npy = sq;
    } else {
        for (n = sq+1; n>=1; n--) {
            if (nprocs % n == 0) {
                m = nprocs/n;
                *npx = n; *npy = m;
                if (m<n) {*npx = m; *npy = n;}
                break;
            } 
        }
    }
    return;
}

void calcpartition(rundata_t *rundata) {
   
    int gnr = rundata -> globalnr; 
    int gnc = rundata -> globalnc; 

    rundata -> localnr = ceil((1.0*gnr)/(1.0*rundata->npr));
    rundata -> localnc = ceil((1.0*gnc)/(1.0*rundata->npc));
   
    rundata -> startr = rundata->myr * rundata->localnr;
    rundata -> startc = rundata->myc * rundata->localnc;

    if (rundata->myr == (rundata->npr - 1))
        rundata -> localnr = gnr - rundata->startr;

    if (rundata->myc == (rundata->npc - 1))
        rundata -> localnc = gnc - rundata->startc;

}
