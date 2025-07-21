#include <exec/bpf.h>
#include <exec/ptrace.h>
#include "simbricks/base/proto.h"
#include "simbricks/mem/proto.h"
#include "simbricks/mem/if.h"
#include "simbricks/pcie/proto.h"
#include <bits/stdint-uintn.h>
#include <exec/hw.h>
#include <sims/sim.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

static SimbricksMemState *simbricks_all = NULL;

#define SIMBRICKS_PCI(obj) (SimbricksMemState*)(obj)
 
extern uint64_t read_vts(void);

// PCI DMA read/write functions
static void pci_dma_read(SimbricksMemState* simbricks, uint64_t addr, void *buffer, size_t len) {

    // safe_printf("Simulated DMA read from addr: %p, size %d; base addr %p\n", addr, len, simbricks->read_addr_base);
    // safe_printf("Simulated DMA read from addr: %p, size %d\n", addr, len);
    // assert(addr >= simbricks->read_addr_base);
    // safe_printf("DMA offset into base %p\n", addr - simbricks->read_addr_base);
    size_t size = pread(simbricks->pid_fd, buffer, len, simbricks->read_addr_base + addr);
    // assert(size == len);
    // memcpy(buffer, (void*)simbricks->dma_base_addr + addr - simbricks->read_addr_base, len);
    
    // for(int i = 0; i < len; i++) {
        // safe_printf("%d", ((unsigned char*)buffer)[i]);
    // }
    // safe_printf("\n");
}

static void pci_dma_write(SimbricksMemState* simbricks, uint64_t addr, const void *buffer, size_t len) {
    // Simulated DMA write
    // safe_printf("Simulated DMA write to addr: %lu, size %d\n", addr, len);
    // For correct results, perform this write to the actual memory
    // pwrite(simbricks->pid_fd, buffer, len, simbricks->write_addr_base + addr);
}

volatile union SimbricksProtoMemM2H *outAlloc(SimbricksMemState* simbricks) {
    volatile union SimbricksProtoBaseMsg *msg;
    do {
        msg = SimbricksBaseIfOutAlloc((struct SimbricksBaseIf*)&simbricks->memif, 0);
    } while (!msg);
    return (volatile union SimbricksProtoMemM2H *)msg;
}

void outSend(SimbricksMemState* simbricks, volatile union SimbricksProtoMemM2H *msg, uint8_t ty) {
    SimbricksBaseIfOutSend((struct SimbricksBaseIf*)&simbricks->memif, (volatile union SimbricksProtoBaseMsg*) msg, ty);
}

/* process and complete message */
static void mem_side_channel_comm_h2m_process(
    SimbricksMemState *simbricks,
    volatile union SimbricksProtoMemH2M *msg)
{
    uint8_t type;
    type = SimbricksMemIfH2MInType(&simbricks->memif, msg);
    switch (type) {
        case SIMBRICKS_PROTO_MEM_H2M_MSG_READ: {
            volatile struct SimbricksProtoMemH2MRead *read_msg = &msg->read;
            volatile union SimbricksProtoMemM2H *out_msg = outAlloc(simbricks);
            // volatile struct SimbricksProtoMemM2HReadcomp *read_comp = &out_msg->readcomp;
            uint16_t req_len = read_msg->len;
            // if (sizeof(*read_comp) + req_len > outMaxSize()) {
            // }
            volatile struct SimbricksProtoMemM2HReadcomp *rc;

            safe_printf("DEBUG, side channel mem read %d\n", req_len);

            rc = &out_msg->readcomp;
            rc->req_id = read_msg->req_id;

            pci_dma_read(simbricks, read_msg->addr, (void*)rc->data, req_len);
    
            outSend(simbricks, out_msg, SIMBRICKS_PROTO_MEM_M2H_MSG_READCOMP);
        }    
        break;

        case SIMBRICKS_PROTO_MEM_H2M_MSG_WRITE: 
        case SIMBRICKS_PROTO_MEM_H2M_MSG_WRITE_POSTED:{
            volatile struct SimbricksProtoMemH2MWrite *write_msg = &msg->write;
            uint16_t req_len = write_msg->len;
            // if (sizeof(*write_msg) + req_len > adapter.outMaxSize()) {
            // }
            pci_dma_write(simbricks, write_msg->addr, (void*)write_msg->data, req_len);
            if (type == SIMBRICKS_PROTO_MEM_H2M_MSG_WRITE) {
                volatile union SimbricksProtoMemM2H *out_msg = outAlloc(simbricks);
                volatile struct SimbricksProtoMemM2HWritecomp *write_comp =
                    &out_msg->writecomp;
                write_comp->req_id = write_msg->req_id;
                outSend(simbricks, out_msg, SIMBRICKS_PROTO_MEM_M2H_MSG_WRITECOMP);
            }
        }
        break;
        default:
            safe_printf("ERROR: simbricks_comm_poll_h2m: unhandled type");
            exit(1);
    }

    SimbricksMemIfH2MInDone(&simbricks->memif, msg);
}

bool mem_side_channel_poll(SimbricksMemState* simbricks){
    volatile union SimbricksProtoMemH2M *msg =
        SimbricksMemIfH2MInPoll(&simbricks->memif, 0);
    if (!msg)
        return false;

    mem_side_channel_comm_h2m_process(simbricks, msg);
    return true;
}
 
static int mem_side_channel_connect(SimbricksMemState *simbricks)
{
    safe_printf("mem_side_channel_connect\n");
    struct SimbricksProtoMemMemIntro *m_i = &simbricks->mem_intro;
    struct SimbricksProtoMemHostIntro host_intro;
    struct SimbricksBaseIfParams params;
    // volatile union SimbricksProtoMemD2H *msg;
    struct SimbricksBaseIf *base_if = &simbricks->memif.base;
    size_t len;

    if (!simbricks->socket_path) {
        safe_printf("ERROR: socket path not set but required\n");
        return 0;
    }

    SimbricksMemIfDefaultParams(&params);
    params.blocking_conn = true;
    params.sock_path = simbricks->socket_path;
    params.sync_mode = kSimbricksBaseIfSyncDisabled;

    if (SimbricksBaseIfInit(base_if, &params)) {
        safe_printf("ERROR: SimbricksBaseIfInit failed\n");
        return 0;
    }

    if (SimbricksBaseIfConnect(base_if)) {
        safe_printf("ERROR: SimbricksBaseIfConnect failed\n");
        return 0;
    }

    if (SimbricksBaseIfConnected(base_if)) {
        safe_printf("ERROR: SimbricksBaseIfConnected indicates unconnected\n");
        return 0;
    }

    /* prepare & send host intro */
    memset(&host_intro, 0, sizeof(host_intro));
    if (SimbricksBaseIfIntroSend(base_if, &host_intro,
                                sizeof(host_intro))) {
        safe_printf("ERROR: SimbricksBaseIfIntroSend failed\n");
        return 0;
    }

    /* receive device intro */
    len = sizeof(*m_i);
    if (SimbricksBaseIfIntroRecv(base_if, m_i, &len)) {
        safe_printf("ERROR: SimbricksBaseIfIntroRecv failed\n");
        return 0;
    }
    if (len != sizeof(*m_i)) {
        safe_printf("ERROR: rx dev intro: length is not as expected\n");
        return 0;
    }
  
    safe_printf("Simbricks mem intro recved \n");
  
    return 1;
}


 // call this pci_simbricks_realize
void mem_side_channel_realize(PCIDevice *pdev)
{
    SimbricksMemState* simbricks = SIMBRICKS_PCI(pdev);
    if (!mem_side_channel_connect(simbricks)) {
      return;
    }
    simbricks->next_simbricks = simbricks_all;
    simbricks_all = simbricks;
}
 
void mem_side_channel_uninit(PCIDevice *pdev)
{
    SimbricksMemState *simbricks = SIMBRICKS_PCI(pdev);
    // CPUState *cpu;
 
    simbricks->stopping = true;

    SimbricksBaseIfClose(&simbricks->memif.base);
}
 
 
void mem_side_channel_cleanup(void)
{
    SimbricksMemState *sbs;
    for (sbs = simbricks_all; sbs != NULL; sbs = sbs->next_simbricks)
        mem_side_channel_uninit(&sbs->pdev);

}
