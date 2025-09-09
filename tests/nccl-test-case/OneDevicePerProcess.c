#include <sched.h>
#include <stdio.h>
#include "cuda_runtime.h"
#include "nccl.h"
#include "mpi.h"
#include <threads.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>


#define MPICHECK(cmd) do {                          \
  int e = cmd;                                      \
  if( e != MPI_SUCCESS ) {                          \
    printf("Failed: MPI error %s:%d '%d'\n",        \
        __FILE__,__LINE__, e);   \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)


#define CUDACHECK(cmd) do {                         \
  cudaError_t e = cmd;                              \
  if( e != cudaSuccess ) {                          \
    printf("Failed: Cuda error %s:%d '%s'\n",             \
        __FILE__,__LINE__,cudaGetErrorString(e));   \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)


#define NCCLCHECK(cmd) do {                         \
  ncclResult_t r = cmd;                             \
  if (r!= ncclSuccess) {                            \
    printf("Failed, NCCL error %s:%d '%s'\n",             \
        __FILE__,__LINE__,ncclGetErrorString(r));   \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)


static uint64_t getHash(const char* string) {
  // Based on DJB2a, result = result * 33 ^ char
  uint64_t result = 5381;
  for (int c = 0; string[c] != '\0'; c++){
    result = ((result << 5) + result) ^ string[c];
  }
  return result;
}

/* Generate a hash of the unique identifying string for this host
 * that will be unique for both bare-metal and container instances
 * Equivalent of a hash of;
 *
 * $(hostname)$(cat /proc/sys/kernel/random/boot_id)
 *
 */
#define HOSTID_FILE "/proc/sys/kernel/random/boot_id"
static uint64_t getHostHash(const char* hostname) {
  char hostHash[1024];

  // Fall back is the hostname if something fails
  (void) strncpy(hostHash, hostname, sizeof(hostHash));
  int offset = strlen(hostHash);

  FILE *file = fopen(HOSTID_FILE, "r");
  if (file != NULL) {
    char *p;
    if (fscanf(file, "%ms", &p) == 1) {
        strncpy(hostHash+offset, p, sizeof(hostHash)-offset-1);
        free(p);
    }
  }
  fclose(file);

  // Make sure the string is terminated
  hostHash[sizeof(hostHash)-1]='\0';

  return getHash(hostHash);
}

static void getHostName(char* hostname, int maxlen) {
  gethostname(hostname, maxlen);
  for (int i=0; i< maxlen; i++) {
    if (hostname[i] == '.') {
        hostname[i] = '\0';
        return;
    }
  }
}


int main(int argc, char* argv[])
{
  int size = 4; // 512K elements--> 16MB of data


  int myRank, nRanks, localRank = 0;


  //initializing MPI
  MPICHECK(MPI_Init(&argc, &argv));
  MPICHECK(MPI_Comm_rank(MPI_COMM_WORLD, &myRank));
  MPICHECK(MPI_Comm_size(MPI_COMM_WORLD, &nRanks));


  //calculating localRank based on hostname which is used in selecting a GPU
  uint64_t hostHashs[nRanks];
  char hostname[1024];
  getHostName(hostname, 1024);
  hostHashs[myRank] = getHostHash(hostname) + myRank;
  MPICHECK(MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, hostHashs, sizeof(uint64_t), MPI_BYTE, MPI_COMM_WORLD));
  for (int p=0; p<nRanks; p++) {
     if (p == myRank) break;
     if (hostHashs[p] == hostHashs[myRank]) localRank++;
  }


  ncclUniqueId id;
  ncclComm_t comm;


  //get NCCL unique ID at rank 0 and broadcast it to all others
  if (myRank == 0) ncclGetUniqueId(&id);
  MPICHECK(MPI_Bcast((void *)&id, sizeof(id), MPI_BYTE, 0, MPI_COMM_WORLD));


  CUDACHECK(cudaSetDevice(localRank));

  //initializing NCCL
  printf("[MPI Rank %d] Initializing NCCL with localRank %d, nRanks %d, size %d \n", myRank, localRank, nRanks, size);
  NCCLCHECK(ncclCommInitRank(&comm, nRanks, id, myRank));

  cudaStream_t s;
  CUDACHECK(cudaStreamCreate(&s));
  cudaStream_t s2;
  CUDACHECK(cudaStreamCreate(&s2));
  
  //communicating using NCCL
  for (int i = 0; i < 1; i++){
    printf("launching iteration %d of allreaduce\n", i);
    float *sendbuff, *recvbuff;
   
    //picking a GPU based on localRank, allocate device buffers
    CUDACHECK(cudaMalloc(&sendbuff, size * sizeof(float)));
    CUDACHECK(cudaMalloc(&recvbuff, size * sizeof(float)));
    // NCCLCHECK(ncclAllReduce((const void*)sendbuff, (void*)recvbuff, size, ncclFloat, ncclSum, \
           comm, s));

    // int recvPeer = (myRank-1+nRanks) % nRanks;
    // int sendPeer = (myRank+1) % nRanks;

    // NCCLCHECK(ncclGroupStart());
    // NCCLCHECK(ncclSend(sendbuff, size, ncclFloat, sendPeer, comm, s));
    // NCCLCHECK(ncclRecv(recvbuff, size, ncclFloat, recvPeer, comm, s));
    // NCCLCHECK(ncclGroupEnd());

    NCCLCHECK(ncclGroupStart());
    if(myRank == 0){
      //send 
      NCCLCHECK(ncclSend((void*)sendbuff, size, ncclFloat, 1, comm, s));
      NCCLCHECK(ncclRecv((void*)recvbuff, size, ncclFloat, 1, comm, s));
    }else{
      NCCLCHECK(ncclSend((void*)sendbuff, size, ncclFloat, 0, comm, s));
      NCCLCHECK(ncclRecv((void*)recvbuff, size, ncclFloat, 0, comm, s));
    }
    int ret = ncclGroupEnd();
    ncclResult_t state ;
    if (ret == ncclInProgress) {
        do {
          ncclCommGetAsyncError(comm, &state);
        } while (state == ncclInProgress);
      }
    }

  cudaDeviceSynchronize();
  printf("I am checking if compiled what nccl \n");
  // volatile int cnt = 0;
  // while(cnt<1000000){
  //   sched_yield();
  //   cnt++;
  // }

  //free device buffers
  // CUDACHECK(cudaFree(sendbuff));
  // CUDACHECK(cudaFree(recvbuff));


  //finalizing NCCL
  ncclCommDestroy(comm);

  //finalizing MPI
  MPICHECK(MPI_Finalize());


  printf("[MPI Rank %d] Success \n", myRank);
  return 0;
}

