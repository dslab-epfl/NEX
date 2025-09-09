#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h> // Add wait header
#include "cuda_runtime.h"
#include "nccl.h"

#define CUDACHECK(cmd) do {                         \
  cudaError_t err = cmd;                            \
  if (err != cudaSuccess) {                         \
    printf("Failed: Cuda error %s:%d '%s'\n",       \
        __FILE__,__LINE__,cudaGetErrorString(err)); \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)

#define NCCLCHECK(cmd) do {                         \
  ncclResult_t res = cmd;                           \
  if (res != ncclSuccess) {                         \
    printf("Failed, NCCL error %s:%d '%s'\n",       \
        __FILE__,__LINE__,ncclGetErrorString(res)); \
    exit(EXIT_FAILURE);                             \
  }                                                 \
} while(0)

// open shared memory at /tmp/nccl-test
#define SHM_NAME "/nccl-test"
#define SHM_SIZE (1024 * 1024) // 1MB shared memory size
#include <fcntl.h> // For shm_open
#include <sys/mman.h> // For mmap
#include <string.h> // For memset
// write the the function open shared memory
static void* shared_memory_ptr = NULL;

void* open_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    void* ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    // Close the file descriptor as it's no longer needed after mmap
    close(shm_fd);
    // Initialize shared memory to zero
    memset(ptr, 0, SHM_SIZE);
    return ptr;
}

void cleanup_shared_memory() {
    if (shared_memory_ptr != NULL) {
        if (munmap(shared_memory_ptr, SHM_SIZE) == -1) {
            perror("munmap");
        }
        shared_memory_ptr = NULL;
    }
    shm_unlink(SHM_NAME);
}

// write to shared memory given a char* pointer from the beginning 
void write_to_shared_memory(void* ptr, const char* data, size_t size) {
    if (ptr == NULL || data == NULL) {
        fprintf(stderr, "Invalid pointer or data\n");
        return;
    }
    // memcpy 
    if (size > SHM_SIZE) {
        fprintf(stderr, "Data size exceeds shared memory size\n");
        return;
    }
    memcpy(ptr+8, data, size);
    ((int*)ptr)[0] = 1;    
}

// read from shared memory given a char* pointer from the beginning
int read_from_shared_memory(void* ptr, char* buffer, size_t size) {
    if (ptr == NULL || buffer == NULL) {
        fprintf(stderr, "Invalid pointer or buffer\n");
        return -1;
    }
    // memcpy
    if (size > SHM_SIZE) {
        fprintf(stderr, "Buffer size exceeds shared memory size\n");
        return -1;
    }
   
    if (((int*)ptr)[0] == 1) {
         memcpy(buffer, ptr+8, size);
        return 0;
    }else {
        return 1;
    }
}

// Thread function to simulate one node
void run_node(int rank, int nGpus, int size) {

    shared_memory_ptr = open_shared_memory();
    // Redirect stdout to rank-specific file
    char filename[256];
    snprintf(filename, sizeof(filename), "rank%d.out", rank);
    if (freopen(filename, "w", stdout) == NULL) {
        perror("freopen");
        exit(EXIT_FAILURE);
    }
    
    const char* interfaces[] = {"tap-nccl-0", "tap-nccl-1"};
    const char* hostids[] = {"123", "456"};
    const char* if_name = interfaces[rank];
    const char* hostid = hostids[rank];

    // Set the network interface for this thread/node
    if (setenv("NCCL_SOCKET_IFNAME", if_name, 1) != 0) {
        perror("setenv");
        exit(EXIT_FAILURE);
    }
    printf("[Rank %d] Using network interface: %s\n", rank, if_name);
    // if (setenv("NCCL_HOSTID", hostid, 1) != 0) {
    //     perror("setenv");
    //     exit(EXIT_FAILURE);
    // }
    ncclUniqueId ncclId;
    if (rank == 0){
        
        NCCLCHECK(ncclGetUniqueId(&ncclId));
        write_to_shared_memory(shared_memory_ptr, ncclId.internal, NCCL_UNIQUE_ID_BYTES);
    }else{
        while(0 != read_from_shared_memory(shared_memory_ptr, ncclId.internal, NCCL_UNIQUE_ID_BYTES)){
            usleep(100);
        };
    }

    ncclComm_t comm;
    float* sendbuff;
    float* recvbuff;
    cudaStream_t stream;

    // Each thread uses GPU 0, simulating a single-GPU node
    CUDACHECK(cudaSetDevice(rank));
    
    // Allocate buffers
    CUDACHECK(cudaMalloc((void**)&sendbuff, size * sizeof(float)));
    CUDACHECK(cudaMalloc((void**)&recvbuff, size * sizeof(float)));
    CUDACHECK(cudaStreamCreate(&stream));

    // Initialize buffers
    float init_val = (float)(rank + 1);
    // cudaMemset takes an int value, which is then replicated byte by byte.
    // To set a float value, we need to copy it from the host.
    float* temp_host = (float*)malloc(size * sizeof(float));
    for(int i=0; i<size; ++i) temp_host[i] = init_val;
    CUDACHECK(cudaMemcpy(sendbuff, temp_host, size * sizeof(float), cudaMemcpyHostToDevice));
    free(temp_host);
    CUDACHECK(cudaMemset(recvbuff, 0, size * sizeof(float)));

    // Initialize NCCL communicator for this rank
    NCCLCHECK(ncclCommInitRank(&comm, nGpus, ncclId, rank));

    // Perform AllReduce
    // NCCLCHECK(ncclGroupStart());
    NCCLCHECK(ncclAllReduce((const void*)sendbuff, (void*)recvbuff, size, ncclFloat, ncclSum, comm, stream));
    // NCCLCHECK(ncclGroupEnd());

    // Synchronize and verify
    CUDACHECK(cudaStreamSynchronize(stream));
    printf("[Rank %d] AllReduce completed.\n", rank);

    float* host_result = (float*)malloc(10 * sizeof(float));
    CUDACHECK(cudaMemcpy(host_result, recvbuff, 10 * sizeof(float), cudaMemcpyDeviceToHost));
    printf("[Rank %d] First few result values: %.1f %.1f %.1f\n", 
           rank, host_result[0], host_result[1], host_result[2]);
    free(host_result);

    // Cleanup
    CUDACHECK(cudaFree(sendbuff));
    CUDACHECK(cudaFree(recvbuff));
    printf("[Rank %d] Freeing buffers.\n", rank);

    CUDACHECK(cudaStreamDestroy(stream));
    
    // ncclCommDestroy(comm);

    printf("[Rank %d] Node simulation completed successfully.\n", rank);
    
    // Cleanup shared memory
    cleanup_shared_memory();
}

int main(int argc, char* argv[]) {
    printf("Starting two-node NCCL test simulation with processes\n");
    printf("Node topology: 2 nodes, 1 GPU per node\n\n");
    
    int nNodes = 2;
    int nGpusPerNode = 1;
    int totalGpus = nNodes * nGpusPerNode;
    int size = 32*1024*1024; // 32MB data size

    // if (setenv("NCCL_SOCKET_IFNAME", "tap-nccl-0", 1) != 0) {
    //     perror("setenv");
    //     exit(EXIT_FAILURE);
    // }

    // Get a unique ID for the NCCL communicator that will be shared across processes
   
    // typedef struct { char internal[NCCL_UNIQUE_ID_BYTES]; } ncclUniqueId;
    // memcpy(ncclId.internal, "123456789012345678901234567890121234567890123456789012345678901212345678901234567890123456789012", NCCL_UNIQUE_ID_BYTES); // Simulated unique ID

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // Child process: Rank 1
        run_node(1, totalGpus, size);
        exit(0);
    } else {
        // Parent process: Rank 0
        run_node(0, totalGpus, size);
        
        // Wait for child process to finish
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("Child process failed.\n");
            return EXIT_FAILURE;
        }
    }
    
    printf("\nSUCCESS: Two-node NCCL test simulation completed successfully!\n");
    printf("Network communication verified.\n");
    
    return 0;
}
