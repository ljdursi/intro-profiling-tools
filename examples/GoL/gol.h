#define LIVE '@'
#define DEAD '.'

#define STARVE 2
#define CROWD  3
#define BIRTH  3

#define I_INNERONLY 1
#define I_OUTERONLY 2
#define I_ALL       3

void iterate(rundata_t *r, int where);
void guardcellexchange(rundata_t *d);
void guardcellexchange_nonblocking(rundata_t *d, MPI_Request reqs[16]);
void guardcellexchange_nonblocking_end(MPI_Request reqs[16], MPI_Status statuses[16]);
