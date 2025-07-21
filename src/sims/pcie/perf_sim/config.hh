
//make sure READMPS <= Credit*UNIT
//make sure MPS <= Credit*UNIT
#define MPS 256
#define READMPS 256
#define RCTLP_SIZE MPS
#define ProcessTLPCost 7
#define UNIT 16
#define DelayCycle 200 // there is a switch in between, so 2*200 = 400 is the one way latency
#define CREDIT MPS/UNIT
#define MEM_N_PARALLEL 30
#define REQ_BUF_CAP 16
