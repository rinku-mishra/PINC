/**
 * @file		pusher.c
 * @author		Sigvald Marholm <sigvaldm@fys.uio.no>
 * @copyright	University of Oslo, Norway
 * @brief		Particle pusher
 * @date		02.12.15
 *
 * Particle pusher schemes
 */

#include "pinc.h"
#include "pusher.h"
#include <math.h>

/******************************************************************************
 * DECLARING LOCAL FUNCTIONS
 *****************************************************************************/

static inline void puInterp3D1(double *result, const double *pos, const double *val, const long int *sizeProd);

/******************************************************************************
 * DEFINING GLOBAL FUNCTIONS
 *****************************************************************************/

void puMove(Population *pop){

	int nSpecies = pop->nSpecies;
	int nDims = pop->nDims;
	double *pos = pop->pos;
	double *vel = pop->vel;

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*nDims;
		long int pStop = pop->iStop[s]*nDims;

		for(long int p=pStart;p<pStop;p++){
			pos[p] += vel[p];
		}
	}
}

void puBndPeriodic(Population *pop, const Grid *grid){

	int nSpecies = pop->nSpecies;
	int nDims = pop->nDims;
	double *pos = pop->pos;
	int *size = grid->size;

	for(int s=0;s<nSpecies;s++){

		long int iStart = pop->iStart[s];
		long int iStop = pop->iStop[s];

		for(long int i=iStart;i<iStop;i++){
			for(int d=0;d<nDims;d++){
				if(pos[i*nDims+d]>size[d+1])	pos[i*nDims+d] -= size[d+1];
				if(pos[i*nDims+d]<0)			pos[i*nDims+d] += size[d+1];
			}
		}
	}
}
// void puBndPeriodicDD();
// void puBndOpen();
// void puBndOpenDD();

// void puAcc3D0();
void puAcc3D1(Population *pop, Grid *E){

	int nSpecies = pop->nSpecies;
	int nDims = 3; // pop->nDims; // hard-coding allows compiler to replace by value
	double *pos = pop->pos;
	double *vel = pop->vel;

	long int *sizeProd = E->sizeProd;
	double *val = E->val;

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*nDims;
		long int pStop = pop->iStop[s]*nDims;

		for(long int p=pStart;p<pStop;p+=nDims){
			double dv[3];
			puInterp3D1(dv,&pos[p],val,sizeProd);
			for(int d=0;d<nDims;d++) vel[p+d] += dv[d];
		}

		// Specie-specific re-normalization
		gMul(E,pop->renormE[s]);
	}
}
// void puAcc3D2();
// void puAccND0();
// void puAccND1();
// void puAccND2();
//
// void puAccBoris3D0();
// void puAccBoris3D1();
// void puAccBoris3D2();
// void puAccBorisND0();
// void puAccBorisND1();
// void puAccBorisND2();
//
// inline void puDistr3D0();
void puDistr3D1(const Population *pop, Grid *rho){

	gZero(rho);
	double *val = rho->val;
	long int *sizeProd = rho->sizeProd;

	int nSpecies = pop->nSpecies;

	for(int s=0;s<nSpecies;s++){

		long int iStart = pop->iStart[s];
		long int iStop = pop->iStop[s];

		for(int i=iStart;i<iStop;i++){

			double *pos = &pop->pos[3*i];

			// Integer parts of position
			int j = (int) pos[0];
			int k = (int) pos[1];
			int l = (int) pos[2];

			// Decimal (cell-referenced) parts of position and their complement
			double x = pos[0]-j;
			double y = pos[1]-k;
			double z = pos[2]-l;
			double xcomp = 1-x;
			double ycomp = 1-y;
			double zcomp = 1-z;

			// Index of neighbouring nodes
			long int p 		= j + k*sizeProd[2] + l*sizeProd[3];
			long int pj 	= p + 1; //sizeProd[1];
			long int pk 	= p + sizeProd[2];
			long int pjk 	= pk + 1; //sizeProd[1];
			long int pl 	= p + sizeProd[3];
			long int pjl 	= pl + 1; //sizeProd[1];
			long int pkl 	= pl + sizeProd[2];
			long int pjkl 	= pkl + 1; //sizeProd[1];

			val[p] 		+= xcomp*ycomp*zcomp;
			val[pj]		+= x    *ycomp*zcomp;
			val[pk]		+= xcomp*y    *zcomp;
			val[pjk]	+= x    *y    *zcomp;
			val[pl]     += xcomp*ycomp*z    ;
			val[pjl]	+= x    *ycomp*z    ;
			val[pkl]	+= xcomp*y    *z    ;
			val[pjkl]	+= x    *y    *z    ;

		}

		gMul(rho,pop->renormRho[s]);

	}

}

// DEPRECATED (ID-technique doesn't work)
void puBndIdMigrants3D(Population *pop, MpiInfo *mpiInfo){

	int nSpecies = pop->nSpecies;
	double *pos = pop->pos;
	double *thresholds = mpiInfo->thresholds;
	int neighborhoodCenter = mpiInfo->neighborhoodCenter;
	long int *nEmigrants = mpiInfo->nEmigrants;
	int nNeighbors = mpiInfo->nNeighbors;

	// By using the dummy to hold data we won't lose track of the beginning of
	// the arrays by, say, migrants[neigh]++.
	long int **migrants = mpiInfo->migrantsDummy;
	for(int neigh=0;neigh<nNeighbors;neigh++){
		migrants[neigh] = mpiInfo->migrants[neigh];
	}
	alSetAll(nEmigrants,nSpecies*nNeighbors,0);

	double lx = thresholds[0];
	double ly = thresholds[1];
	double lz = thresholds[2];
	double ux = thresholds[3];
	double uy = thresholds[4];
	double uz = thresholds[5];

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*3;
		long int pStop = pop->iStop[s]*3;

		for(long int p=pStart;p<pStop;p+=3){
			double x = pos[p];
			double y = pos[p+1];
			double z = pos[p+2];
			int nx = - (x<lx) + (x>=ux);
			int ny = - (y<ly) + (y>=uy);
			int nz = - (z<lz) + (z>=uz);
			int n = 13 + nx + 3*ny + 9*nz;

			if(n!=neighborhoodCenter){
				*(migrants[n]++) = p;
				nEmigrants[n*nSpecies+s]++;
			}
		}
	}

}

// DEPRECATED (ID-technique doesn't work)
void puBndIdMigrantsND(Population *pop, MpiInfo *mpiInfo){

	int nSpecies = pop->nSpecies;
	int nDims = pop->nDims;
	double *pos = pop->pos;
	double *thresholds = mpiInfo->thresholds;
	int neighborhoodCenter = mpiInfo->neighborhoodCenter;
	long int *nEmigrants = mpiInfo->nEmigrants;
	int nNeighbors = mpiInfo->nNeighbors;

	// By using the dummy to hold data we won't lose track of the beginning of
	// the arrays by, say, migrants[neigh]++.
	long int **migrants = mpiInfo->migrantsDummy;
	for(int neigh=0;neigh<nNeighbors;neigh++){
		migrants[neigh] = mpiInfo->migrants[neigh];
	}
	alSetAll(nEmigrants,nSpecies*nNeighbors,0);

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*nDims;
		long int pStop = pop->iStop[s]*nDims;

		for(long int p=pStart;p<pStop;p+=nDims){
			int n = 0;
			for(int d=nDims-1;d>=0;d--){
				n *= 3;
				n += 1 - (pos[p+d]<thresholds[d]) + (pos[p+d]>=thresholds[nDims+d]);
				// A particle at position x will use j=(int)x and j+1 for
				// interpolation. When x is integer and equal to a threshold, it
				// should migrate if on the upper threshold since it may run out
				// of ghost nodes otherwise (unless the user has specified more
				// ghost layers than necessary)
			}
			if(n!=neighborhoodCenter){
				*(migrants[n]++) = p;
				nEmigrants[n*nSpecies+s]++;
			}
		}
	}
}

// Works
// TODO: Add fault-handling in case of too small "emigrants" buffer
void puExtractEmigrants3D(Population *pop, MpiInfo *mpiInfo){

	int nSpecies = pop->nSpecies;
	double *pos = pop->pos;
	double *vel = pop->vel;
	double *thresholds = mpiInfo->thresholds;
	int neighborhoodCenter = mpiInfo->neighborhoodCenter;
	long int *nEmigrants = mpiInfo->nEmigrants;
	int nNeighbors = mpiInfo->nNeighbors;

	// By using the dummy to hold data we won't lose track of the beginning of
	// the arrays when incrementing the pointer
	double **emigrants = mpiInfo->emigrantsDummy;
	for(int ne=0;ne<nNeighbors;ne++){
		emigrants[ne] = mpiInfo->emigrants[ne];
	}
	alSetAll(nEmigrants,nSpecies*nNeighbors,0);

	double lx = thresholds[0];
	double ly = thresholds[1];
	double lz = thresholds[2];
	double ux = thresholds[3];
	double uy = thresholds[4];
	double uz = thresholds[5];

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*3;
		long int pStop = pop->iStop[s]*3;

		for(long int p=pStart;p<pStop;p+=3){
			double x = pos[p];
			double y = pos[p+1];
			double z = pos[p+2];
			int nx = - (x<lx) + (x>=ux);
			int ny = - (y<ly) + (y>=uy);
			int nz = - (z<lz) + (z>=uz);
			int ne = 13 + nx + 3*ny + 9*nz;

			if(ne!=neighborhoodCenter){
				*(emigrants[ne]++) = x;
				*(emigrants[ne]++) = y;
				*(emigrants[ne]++) = z;
				*(emigrants[ne]++) = vel[p];
				*(emigrants[ne]++) = vel[p+1];
				*(emigrants[ne]++) = vel[p+2];
				nEmigrants[ne*nSpecies+s]++;

				pos[p]   = pos[pStop-3];
				pos[p+1] = pos[pStop-2];
				pos[p+2] = pos[pStop-1];
				vel[p]   = vel[pStop-3];
				vel[p+1] = vel[pStop-2];
				vel[p+2] = vel[pStop-1];
				pStop -= 3;
				p -= 3;
				pop->iStop[s]--;

			}
		}
	}

}

// Works
// TODO: Add fault-handling in case of too small "emigrants" buffer
void puExtractEmigrantsND(Population *pop, MpiInfo *mpiInfo){

	int nSpecies = pop->nSpecies;
	int nDims = pop->nDims;
	double *pos = pop->pos;
	double *vel = pop->vel;
	double *thresholds = mpiInfo->thresholds;
	int neighborhoodCenter = mpiInfo->neighborhoodCenter;
	long int *nEmigrants = mpiInfo->nEmigrants;
	int nNeighbors = mpiInfo->nNeighbors;

	// By using the dummy to hold data we won't lose track of the beginning of
	// the arrays by, say, migrants[neigh]++.
	double **emigrants = mpiInfo->emigrantsDummy;
	for(int ne=0;ne<nNeighbors;ne++){
		emigrants[ne] = mpiInfo->emigrants[ne];
	}
	alSetAll(nEmigrants,nSpecies*nNeighbors,0);

	for(int s=0;s<nSpecies;s++){

		long int pStart = pop->iStart[s]*nDims;
		long int pStop = pop->iStop[s]*nDims;

		for(long int p=pStart;p<pStop;p+=nDims){
			int ne = 0;
			for(int d=nDims-1;d>=0;d--){
				ne *= 3;
				ne += 1 - (pos[p+d]<thresholds[d]) + (pos[p+d]>=thresholds[nDims+d]);
				// A particle at position x will use j=(int)x and j+1 for
				// interpolation. When x is integer and equal to a threshold, it
				// should migrate if on the upper threshold since it may run out
				// of ghost nodes otherwise (unless the user has specified more
				// ghost layers than necessary)
			}
			if(ne!=neighborhoodCenter){
				for(int d=0;d<nDims;d++) *(emigrants[ne]++) = pos[p+d];
				for(int d=0;d<nDims;d++) *(emigrants[ne]++) = vel[p+d];
				nEmigrants[ne*nSpecies+s]++;

				for(int d=0;d<nDims;d++) pos[p+d] = pos[pStop-nDims+d];
				for(int d=0;d<nDims;d++) vel[p+d] = vel[pStop-nDims+d];
				pStop -= nDims;
				p -= nDims;
				pop->iStop[s]--;
			}
		}
	}
}

// Works
// TODO: Add fault-handling in case of too small Population struct
inline void exchangeNMigrants(MpiInfo *mpiInfo){

	int nSpecies = mpiInfo->nSpecies;
	int nNeighbors = mpiInfo->nNeighbors;
	MPI_Request *send = mpiInfo->send;
	MPI_Request *recv = mpiInfo->recv;

	// Send number of emigrants and receive number of immigrants.
	// Order of reception is not necessarily as determined by the for-loop since
	// we're using non-blocking send/receive.
	for(int ne=0;ne<nNeighbors;ne++){
		if(ne!=mpiInfo->neighborhoodCenter){
			int rank = puNeighborToRank(mpiInfo,ne);
			int reciprocal = puNeighborToReciprocal(ne,mpiInfo->nDims);
			long int *nEmigrants  = &mpiInfo->nEmigrants[nSpecies*ne];
			long int *nImmigrants = &mpiInfo->nImmigrants[nSpecies*ne];
			MPI_Isend(nEmigrants ,nSpecies,MPI_LONG,rank,reciprocal,MPI_COMM_WORLD,&send[ne]);
			MPI_Irecv(nImmigrants,nSpecies,MPI_LONG,rank,ne        ,MPI_COMM_WORLD,&recv[ne]);
		}
	}

	MPI_Waitall(nNeighbors,send,MPI_STATUS_IGNORE);
	MPI_Waitall(nNeighbors,recv,MPI_STATUS_IGNORE);

}

// Works
inline void shiftImmigrants(MpiInfo *mpiInfo, Grid *grid, int ne){

	double *immigrants = mpiInfo->immigrants;
	int nSpecies = mpiInfo->nSpecies;
	long int nImmigrantsTotal = alSum(&mpiInfo->nImmigrants[ne*nSpecies],nSpecies);
	int nDims = mpiInfo->nDims;

	for(int d=0;d<nDims;d++){
		int n = ne%3-1;
		ne /=3;

		double shift = n*grid->trueSize[d+1];
		for(int i=0;i<nImmigrantsTotal;i++){
			immigrants[d+2*nDims*i] += shift;
		}

	}

}

// Works
inline void importParticles(Population *pop, double *particles, long int *nParticles, int nSpecies){

	int nDims = pop->nDims;
	long int *iStop = pop->iStop;

	for(int s=0;s<nSpecies;s++){

		double *pos = &pop->pos[nDims*iStop[s]];
		double *vel = &pop->vel[nDims*iStop[s]];

		for(int i=0;i<nParticles[s];i++){
			for(int d=0;d<nDims;d++) *(pos++) = *(particles++);
			for(int d=0;d<nDims;d++) *(vel++) = *(particles++);
		}

		iStop[s] += nParticles[s];
	}

}

// Works
inline void exchangeMigrants(Population *pop, MpiInfo *mpiInfo, Grid *grid){

	int nSpecies = mpiInfo->nSpecies;
	int nNeighbors = mpiInfo->nNeighbors;
	long int nImmigrantsAlloc = mpiInfo->nImmigrantsAlloc;
	int nDims = mpiInfo->nDims;
	double **emigrants = mpiInfo->emigrants;
	double *immigrants = mpiInfo->immigrants;
	long int *nImmigrants = mpiInfo->nImmigrants;
	MPI_Request *send = mpiInfo->send;

	for(int ne=0;ne<nNeighbors;ne++){
		if(ne!=mpiInfo->neighborhoodCenter){
			int rank = puNeighborToRank(mpiInfo,ne);
			int reciprocal = puNeighborToReciprocal(ne,nDims);
			long int *nEmigrants  = &mpiInfo->nEmigrants[nSpecies*ne];
			long int length = alSum(nEmigrants,nSpecies)*2*nDims;
			MPI_Isend(emigrants[ne],length,MPI_DOUBLE,rank,reciprocal,MPI_COMM_WORLD,&send[ne]);
		}
	}

	// Since "immigrants" is reused for every receive operation MPI_Irecv cannot
	// be used. However, in order to receive and process whichever comes first
	// MPI_ANY_SOURCE is used.
	for(int a=0;a<nNeighbors-1;a++){

		MPI_Status status;
		MPI_Recv(immigrants,nImmigrantsAlloc,MPI_DOUBLE,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
		int ne = status.MPI_TAG;	// Which neighbor it is from equals the tag

		// adPrint(mpiInfo->immigrants,6);
		shiftImmigrants(mpiInfo,grid,ne);
		// adPrint(mpiInfo->immigrants,6);
		importParticles(pop,immigrants,&nImmigrants[ne*nSpecies],nSpecies);

	}

	MPI_Waitall(nNeighbors,send,MPI_STATUS_IGNORE);

}

// Works
void puMigrate(Population *pop, MpiInfo *mpiInfo, Grid *grid){

	exchangeNMigrants(mpiInfo);
	exchangeMigrants(pop,mpiInfo,grid);

}

void puReflect(){



}

// inline void puDistr3D2();
// inline void puDistrND0();
// inline void puDistrND1();
// inline void puDistrND2();

/******************************************************************************
 * DEFINING LOCAL FUNCTIONS
 *****************************************************************************/

// static inline double puInterp3D0();
static inline void puInterp3D1(double *result, const double *pos, const double *val, const long int *sizeProd){

	// Integer parts of position
	int j = (int) pos[0];
	int k = (int) pos[1];
	int l = (int) pos[2];

	// Decimal (cell-referenced) parts of position and their complement
	double x = pos[0]-j;
	double y = pos[1]-k;
	double z = pos[2]-l;
	double xcomp = 1-x;
	double ycomp = 1-y;
	double zcomp = 1-z;

	// Index of neighbouring nodes
	long int p 		= j*3 + k*sizeProd[2] + l*sizeProd[3];
	long int pj 	= p + 3; //sizeProd[1];
	long int pk 	= p + sizeProd[2];
	long int pjk 	= pk + 3; //sizeProd[1];
	long int pl 	= p + sizeProd[3];
	long int pjl 	= pl + 3; //sizeProd[1];
	long int pkl 	= pl + sizeProd[2];
	long int pjkl 	= pkl + 3; //sizeProd[1];

	// Linear interpolation
	for(int v=0;v<3;v++)
		result[v] =	zcomp*(	 ycomp*(xcomp*val[p   +v]+x*val[pj  +v])
								+y    *(xcomp*val[pk  +v]+x*val[pjk +v]) )
						+z    *( ycomp*(xcomp*val[pl  +v]+x*val[pjl +v])
								+y    *(xcomp*val[pkl +v]+x*val[pjkl+v]) );

}

// static inline double puInterp3D2();
// static inline double puInterpND0();
// static inline double puInterpND1();
// static inline double puInterpND2();

int puNeighborToReciprocal(int neighbor, int nDims){

	int reciprocal = 0;

	for(int d=0;d<nDims;d++){
		reciprocal += (2-(neighbor % 3))*pow(3,d);
		neighbor /= 3;
	}

	return reciprocal;

}

int puNeighborToRank(MpiInfo *mpiInfo, int neighbor){

	int rank = 0;
	int *subdomain = mpiInfo->subdomain;
	int *nSubdomains = mpiInfo->nSubdomains;
	int *nSubdomainsProd = mpiInfo->nSubdomainsProd;

	for(int d=0;d<mpiInfo->nDims;d++){
		int n = (neighbor % 3) - 1;
		neighbor /= 3;

		// The additiona nSubdomains[d] in paranthesis to ensure positive modulo
		n = (subdomain[d]+n+nSubdomains[d])%nSubdomains[d];

		rank += n*nSubdomainsProd[d];
	}

	return rank;
}

int puRankToNeighbor(MpiInfo *mpiInfo, int rank){

	int neighbor = 0;
	int *subdomain = mpiInfo->subdomain;
	int *nSubdomains = mpiInfo->nSubdomains;

	for(int d=0;d<mpiInfo->nDims;d++){
		int n = (rank % nSubdomains[d]);

		// Additional nSubdomains[d] in paranthesis to ensure positive modulo
		n = (n-subdomain[d]+1+nSubdomains[d])%nSubdomains[d];
		rank /= nSubdomains[d];

		neighbor += n*pow(3,d);
	}

	return neighbor;

}