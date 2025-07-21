#include <exec/exec.h>
#include <exec/bpf.h>
#include <exec/hw.h>
#include <config/config.h>
#include "simbricks/pcie/proto.h"
#include <bits/stdint-uintn.h>
#include <sched.h>
#include <sims/sim.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

 
// #define DEBUG_PRINTS(...) safe_printf(__VA_ARGS__)
#define DEBUG_PRINTS(...)

#if CONFIG_MEM_LPN
#define ENABLE_MEMORY 
#endif

#define QEMU_CLOCK_VIRTUAL 0

// #define YIELD sched_yield()
#define YIELD

static SimbricksPciState *simbricks_all = NULL;

static int num_simbricks = 0;

static volatile uint64_t global_cur_ts = 0;

static pthread_mutex_t global_time_lock = PTHREAD_MUTEX_INITIALIZER;

uint64_t qemu_clock_get_ps(SimbricksPciState* simbricks) {
    return simbricks->cur_ts;
}

void qemu_thread_create(QemuThread *thread, const char *name, 
                        void *(*start_routine)(void *), void *arg, int default_joinable) {
    pthread_create(thread, NULL, start_routine, arg);
}

void qemu_cond_init(QemuCond *cond) {
    pthread_cond_init(cond, NULL);
}

void qemu_thread_join(QemuThread *thread) {
    pthread_join(*thread, NULL);
}

void qemu_cond_destroy(QemuCond *cond) {
    pthread_cond_destroy(cond);
}

void qemu_cond_broadcast(QemuCond *cond) {
    pthread_cond_broadcast(cond);
}

static void simbricks_comm_h2d_force_update(SimbricksPciState *simbricks, int64_t ts);

static void advance_wait_until_(SimbricksPciState* simbricks, uint64_t ts) {
    pthread_mutex_lock(&simbricks->advance_lock);
    simbricks->advance_till_ts = ts;
    // DEBUG_PRINTS("advance_wait_until_ start: cur %lu, next %lu\n", simbricks->cur_ts ,simbricks->advance_till_ts);
    pthread_cond_broadcast(&simbricks->advance_cond);
    pthread_cond_wait(&simbricks->advance_cond, &simbricks->advance_lock);
    pthread_mutex_unlock(&simbricks->advance_lock);
    // DEBUG_PRINTS("advance_wait_until_ end: cur %lu, next %lu\n", simbricks->cur_ts ,simbricks->advance_till_ts);
}

static void broadcast_all(uint64_t ts){
    SimbricksPciState* cur_simbricks = simbricks_all;
    while(cur_simbricks != NULL){
        pthread_mutex_lock(&cur_simbricks->advance_lock);
        cur_simbricks->advance_till_ts = ts;
        // if(simbricks->cur_ts > simbricks->advance_till_ts):
        //  this thread is still blocked
        pthread_cond_broadcast(&cur_simbricks->advance_cond);
        pthread_mutex_unlock(&cur_simbricks->advance_lock);
        cur_simbricks = cur_simbricks->next_simbricks;
    }
}

static void waitfor_all(){
    SimbricksPciState* cur_simbricks = simbricks_all;
    while(cur_simbricks != NULL){
        while(1){
            pthread_mutex_lock(&cur_simbricks->advance_lock);
            if(cur_simbricks->blocked == 1){
                pthread_mutex_unlock(&cur_simbricks->advance_lock);
                break;
            }
            pthread_mutex_unlock(&cur_simbricks->advance_lock);
            YIELD;
        }
        cur_simbricks = cur_simbricks->next_simbricks;
    }
}

static uint64_t mem_active_time = 0;
static void advance_memory_simulator(uint64_t ts){
    while(mem_active(&mem_active_time)){
        if(mem_active_time > ts) {
            return;
        }
        mem_advance_until_time(mem_active_time);
    }
}

static void advance_wait_until_all(uint64_t ts) {
    SimbricksPciState* cur_simbricks;
    if(global_cur_ts > ts){
        return;
    }
    // ts is the future timestamp we want to advance to;
    // wake up all simbricks polls
    // safe_printf("try broadcast all %ld\n", ts);
    broadcast_all(ts);
    // safe_printf("broadcasted all %ld\n", ts);

    // what if after broadcast, the poller is blocked again?
    // if(simbricks->cur_ts > ts): the thread is still blocked
    // skip that thread

#define ADANCE_PERIOD 20*1000
    
    while(1){

        pthread_mutex_lock(&global_time_lock);
        global_cur_ts += ADANCE_PERIOD;
        if(global_cur_ts > ts){
            global_cur_ts = ts;
        }
        pthread_mutex_unlock(&global_time_lock);

        cur_simbricks = simbricks_all;

        // indeed simulations are running in parallel
        while(cur_simbricks != NULL){
            // safe_printf("advance_wait_until_all step 1: simbricks:%p ,cur %ld, global %ld\n", cur_simbricks, cur_simbricks->cur_ts, global_cur_ts);
            // pthread_mutex_lock(&cur_simbricks->advance_lock);
            while(cur_simbricks->blocked == 0 && global_cur_ts >= cur_simbricks->cur_ts){
                // pthread_mutex_unlock(&cur_simbricks->advance_lock);
                YIELD;
                // pthread_mutex_lock(&cur_simbricks->advance_lock);
            }
            // pthread_mutex_unlock(&cur_simbricks->advance_lock);
            cur_simbricks = cur_simbricks->next_simbricks;

        }

        // safe_printf("advance_wait_until_all step 3: simbricks:%p ,cur %ld, global %ld\n", simbricks_all, simbricks_all->cur_ts, global_cur_ts);

        // advance mem simulator
#ifdef ENABLE_MEMORY
        advance_memory_simulator(global_cur_ts);
#endif
        if(global_cur_ts >= ts){
            // at this point, all simbricks are at least at ts
            // all simbricks polls will stop
            break;
        }
    }

    // safe_printf("advance_wait_until_all done %ld\n", ts);
    // wait for all simbricks polls to stop
    waitfor_all();
    // safe_printf("waitfor_all done %ld\n", ts);
}

// PCI DMA read/write functions
void pci_dma_read(SimbricksPciState* simbricks, uint64_t addr, void *buffer, size_t len) {
    // Simulated DMA read
    // // DEBUG_PRINTS("Simulated DMA read from addr: %p, size %ld \n", (void*)addr, len);
    ssize_t nread = pread(simbricks->pid_fd, buffer, len, simbricks->read_addr_base + addr);
    // for(int i = 0; i < nread; i++) {
    //     DEBUG_PRINTS("%02x", ((unsigned char*)buffer)[i]);
    // }
    // DEBUG_PRINTS("\n");
}

void pci_dma_write(SimbricksPciState* simbricks, uint64_t addr, const void *buffer, size_t len) {
    // Simulated DMA write
    DEBUG_PRINTS("Simulated DMA write to addr: %lu, size %d\n", addr, len);
    // For correct results, perform this write to the actual memory
    // pwrite(simbricks->pid_fd, buffer, len, simbricks->write_addr_base + addr);
}
 
#define SIMBRICKS_CLOCK QEMU_CLOCK_VIRTUAL
 
#define TYPE_PCI_SCX_DEVICE "scx-pci"
#define SIMBRICKS_PCI(obj) (SimbricksPciState*)(obj)
 

void* vts_mem_addr = NULL;

static inline uint64_t ts_to_proto(SimbricksPciState *simbricks,
                                   int64_t scx_ts)
{
    return scx_ts;
    // return (scx_ts + simbricks->ts_base) * 1000;
}
 
extern uint64_t read_vts(void);

static inline uint64_t get_scx_ts() {
    return read_vts();
}
 
static inline volatile union SimbricksProtoPcieH2D *simbricks_comm_h2d_alloc(
        SimbricksPciState *simbricks,
        uint64_t ts)
{   
    pthread_mutex_lock(&simbricks->msg_sent_lk);
    volatile union SimbricksProtoPcieH2D *msg;
    while (!(msg = SimbricksPcieIfH2DOutAlloc(&simbricks->pcieif,
                                              ts_to_proto(simbricks, ts))));
    pthread_mutex_unlock(&simbricks->msg_sent_lk);
 
    // whenever we send a message, we need to reschedule our sync timer
    return msg;
}
 
static void simbricks_comm_d2h_dma_read_complete(
        SimbricksPciState *simbricks,
        int64_t ts,
        uint64_t req_id, 
        uint64_t data, 
        uint64_t len)
{
    volatile union SimbricksProtoPcieH2D *h2d;
    volatile struct SimbricksProtoPcieH2DReadcomp *rc;
 
    /* allocate completion */
    h2d = simbricks_comm_h2d_alloc(simbricks, ts);
    rc = &h2d->readcomp;
 
    assert(len <=
        SimbricksPcieIfH2DOutMsgLen(&simbricks->pcieif) - sizeof (*rc));
 
    memcpy((void*)rc->data, (void*)data, len);
 
    /* return completion */
    rc->req_id = req_id;
    SimbricksPcieIfH2DOutSend(&simbricks->pcieif, h2d,
        SIMBRICKS_PROTO_PCIE_H2D_MSG_READCOMP);

    DEBUG_PRINTS("simbricks_comm_d2h_dma_read_complete: ts=%ld req_id=%lu data=%lu len=%lu\n",
       ts, req_id, data, len);
}

static void simbricks_comm_d2h_dma_read(
    SimbricksPciState *simbricks,
    int64_t ts,
    volatile struct SimbricksProtoPcieD2HRead *read)
{
    DEBUG_PRINTS("simbricks_comm_d2h_dma_read: ts=%ld req_id=%lu offset=%lu len=%u\n",
       ts, read->req_id, read->offset, read->len);
    void* mem_ptr = malloc(read->len);

#ifdef ENABLE_MEMORY
    mem_put(simbricks->dev_id, ts, read->req_id, simbricks->read_addr_base + read->offset, read->len, (uint64_t)mem_ptr, simbricks->pid_fd, 0, (uint64_t)simbricks);
#else
    pci_dma_read(simbricks, read->offset, mem_ptr, read->len);
    simbricks_comm_d2h_dma_read_complete(simbricks, ts, read->req_id, (uint64_t)mem_ptr, read->len);
#endif
}


static void simbricks_comm_d2h_dma_write_complete(SimbricksPciState *simbricks, int64_t ts, uint64_t req_id){
    volatile union SimbricksProtoPcieH2D *h2d;
    volatile struct SimbricksProtoPcieH2DWritecomp *wc;
    h2d = simbricks_comm_h2d_alloc(simbricks, ts);
    wc = &h2d->writecomp;
    /* return completion */
    wc->req_id = req_id;
    SimbricksPcieIfH2DOutSend(&simbricks->pcieif, h2d,
        SIMBRICKS_PROTO_PCIE_H2D_MSG_WRITECOMP);
}


static void simbricks_comm_d2h_dma_write(
    SimbricksPciState *simbricks,
    int64_t ts,
    volatile struct SimbricksProtoPcieD2HWrite *write)
{
    DEBUG_PRINTS("simbricks_comm_d2h_dma_write: ts=%ld req_id=%lu offset=%lu len=%u\n",
       ts, write->req_id, write->offset, write->len);
#ifdef ENABLE_MEMORY
    mem_put(simbricks->dev_id, ts, write->req_id, simbricks->write_addr_base + write->offset, write->len, (uint64_t)write->data, simbricks->pid_fd, 1, (uint64_t)simbricks);
#else
    simbricks_comm_d2h_dma_write_complete(simbricks, ts, write->req_id); 
#endif
}

 
static void simbricks_comm_d2h_interrupt(SimbricksPciState *simbricks,
        volatile struct SimbricksProtoPcieD2HInterrupt *interrupt)
{
    if(interrupt->inttype == SIMBRICKS_PROTO_PCIE_INT_MSIX && interrupt->vector == 0){
        // interrupt that says the accelerator is done
        DEBUG_PRINTS("simbricks_comm_d2h_interrupt: int_num=%d int_type=%d\n", int_num, int_type);
        simbricks->active = 0; 
        // fastforward to advance_till_ts 
        simbricks_comm_h2d_force_update(simbricks, simbricks->advance_till_ts);
        return;
    }
    DEBUG_PRINTS("unknown and unregsitered interrupt type %d, num %d\n", int_type, int_num);
    return;

}
 
static void simbricks_comm_d2h_rcomp(SimbricksPciState *simbricks,
                                     uint64_t cur_ts,
                                     uint64_t req_id,
                                     const void *data)
{
    SimbricksPciRequest *req = simbricks->reqs + req_id;
 
    assert(req_id <= simbricks->reqs_len);
 
    if (!req->processing) {
        DEBUG_PRINTS("Error: simbricks_comm_d2h_rcomp: no request currently processing");
        exit(1);
    }
 
    /* copy read value from message */
    req->value = 0;
    memcpy(&req->value, data, req->size);
 
    req->processing = false;
 
    qemu_cond_broadcast(&req->cond);
}
 
/* process and complete message */
static void simbricks_comm_d2h_process(
        SimbricksPciState *simbricks,
        int64_t ts,
        volatile union SimbricksProtoPcieD2H *msg)
{
    uint8_t type;
 
    type = SimbricksPcieIfD2HInType(&simbricks->pcieif, msg);
#ifdef ENABLE_DEBUG_PRINTS
    warn_report("simbricks_comm_d2h_process: ts=%ld type=%u\n", ts, type);
#endif  

    switch (type) {
        case SIMBRICKS_PROTO_MSG_TYPE_SYNC:
            /* nop */
            break;
 
        case SIMBRICKS_PROTO_PCIE_D2H_MSG_READ:
            DEBUG_PRINTS("simbricks_comm_d2h_process: read\n");
            simbricks_comm_d2h_dma_read(simbricks, ts, &msg->read);
            break;
 
        case SIMBRICKS_PROTO_PCIE_D2H_MSG_WRITE:
            DEBUG_PRINTS("simbricks_comm_d2h_process: write\n");
            simbricks_comm_d2h_dma_write(simbricks, ts, &msg->write);
            break;
 
        case SIMBRICKS_PROTO_PCIE_D2H_MSG_INTERRUPT:
            DEBUG_PRINTS("simbricks_comm_d2h_process: interrupt\n");
            simbricks_comm_d2h_interrupt(simbricks, &msg->interrupt);
            break;
 
        case SIMBRICKS_PROTO_PCIE_D2H_MSG_READCOMP:
            DEBUG_PRINTS("simbricks_comm_d2h_process: readcomp\n");
            simbricks_comm_d2h_rcomp(simbricks, ts, msg->readcomp.req_id,
                    (void *) msg->readcomp.data);
            break;
 
        case SIMBRICKS_PROTO_PCIE_D2H_MSG_WRITECOMP:
            /* we treat writes as posted, so nothing we need to do here */
            break;
 
        default:
            // DEBUG_PRINTS("ERROR: simbricks_comm_poll_d2h: unhandled type");
            exit(1);
    }
 
    SimbricksPcieIfD2HInDone(&simbricks->pcieif, msg);
}

static bool poll(SimbricksPciState* simbricks, uint64_t proto_ts, uint64_t* next_ts){
    
    volatile union SimbricksProtoPcieD2H *msg =
        SimbricksPcieIfD2HInPoll(&simbricks->pcieif, proto_ts);
    *next_ts = SimbricksPcieIfD2HInTimestamp(&simbricks->pcieif);
    if (!msg){
        return false;
    }

    simbricks_comm_d2h_process(simbricks, proto_ts, msg);
    return true;

}

// static uint64_t last_sync_ts = 0;
static void *simbricks_poll_thread(void *opaque)
{
    SimbricksPciState *simbricks = opaque;
    uint64_t next_ts;

#ifdef ENABLE_MEMORY
    uint64_t ref_ptr;
    uint64_t buffer_addr;
    uint64_t buffer_len;
    uint64_t req_id;

    uint64_t mem_token_ts = 0;
#endif
    while (!simbricks->stopping) {

        do{
            if(simbricks->mem_side_channel){
                // poll until side_channel has no more messages
                while(mem_side_channel_poll(simbricks->mem_side_channel));
            }
            
        }
        while(poll(simbricks, simbricks->cur_ts, &next_ts));

        // START - send out all messages at current proto_ts
        // advance mem simulator

#ifdef ENABLE_MEMORY
        if(num_simbricks == 1){
            advance_memory_simulator(simbricks->cur_ts);
        }

        while(mem_get(simbricks->dev_id, &ref_ptr, &req_id, &buffer_addr, &buffer_len, &mem_token_ts)){
            DEBUG_PRINTS("mem_get: %p; mem_time %ld, now %ld\n", simbricks, mem_token_ts, proto_ts);
            // note mem_get deques the resp already.
            assert((void*)ref_ptr == (void*)simbricks);
            if(buffer_addr == 0){
                //write complete
                simbricks_comm_d2h_dma_write_complete(simbricks, mem_token_ts, req_id);
            }else{
                // read complete
                simbricks_comm_d2h_dma_read_complete(simbricks, mem_token_ts, req_id, buffer_addr, buffer_len);
            }
        }
        
#endif

        if(simbricks->cur_ts - simbricks->pcieif.base.out_timestamp >= simbricks->sync_period*1000){
            volatile union SimbricksProtoPcieH2D *msg = simbricks_comm_h2d_alloc(simbricks, simbricks->cur_ts);
            SimbricksPcieIfH2DOutSend(&simbricks->pcieif, msg, SIMBRICKS_PROTO_MSG_TYPE_SYNC);
        }

        // END - send out all messages at current proto_ts

        if( next_ts <= simbricks->cur_ts){
            // DEBUG_PRINTS("simbricks_poll_thread: next_ts %lu < cur %lu; last sync: %lu\n", next_ts, proto_ts, simbricks->pcieif.base.out_timestamp);
            // there is no message, just sleep a whie and continue
            // usleep(10);
        }

        if(next_ts > simbricks->cur_ts){
            pthread_mutex_lock(&simbricks->advance_lock);
            simbricks->cur_ts = next_ts+1000;
            // simbricks->cur_ts = next_ts;
            while(simbricks->cur_ts > simbricks->advance_till_ts){
                simbricks->blocked = 1;
                // wake up whoever is trying to advance me;
                pthread_cond_broadcast(&simbricks->advance_cond);
                // waits for someone to advance me
                // DEBUG_PRINTS("simbricks_poll_thread: waiting for advance %lu, cur %lu\n", next_ts, simbricks->advance_till_ts);
                pthread_cond_wait(&simbricks->advance_cond, &simbricks->advance_lock);
            }
            simbricks->blocked = 0;
            pthread_mutex_unlock(&simbricks->advance_lock);

            // I will move on to simbricks->cur_ts
            // MUST: simbricsk->cur_ts <= simbricks->advance_till_ts
            // MUST: eventually global_cur_ts >= simbricks->advance_till_ts
            // IMPLY: global_cur_ts >= simbricks->cur_ts
            // so the following while loop will not be infinite
            // pthread_mutex_lock(&global_time_lock);
            while(global_cur_ts < simbricks->cur_ts){
                // pthread_mutex_unlock(&global_time_lock);
                // global_cur_ts is behind me
                YIELD;
                // pthread_mutex_lock(&global_time_lock);
            }
            // pthread_mutex_unlock(&global_time_lock);
        }
    }
 
    return NULL;
}
 
/******************************************************************************/
/* MMIO interface */

/* submit a bar read or write to the worker thread and wait for it to
 * complete */

static void simbricks_mmio_rw(SimbricksPciState *simbricks,
                              uint8_t bar,
                              hwaddr addr,
                              unsigned size,
                              uint64_t *val,
                              bool is_write)
{
    CPUState cpu_ = { .cpu_index = 0, .stopped = 0 };
    CPUState *cpu = &cpu_;

    SimbricksPciRequest *req;
    volatile union SimbricksProtoPcieH2D *msg;
    volatile struct SimbricksProtoPcieH2DRead *read;
    volatile struct SimbricksProtoPcieH2DWrite *write;
    int64_t cur_ts;

    assert(simbricks->reqs_len > cpu->cpu_index);
    req = simbricks->reqs + cpu->cpu_index;
    // assert(req->cpu == cpu);

    cur_ts = qemu_clock_get_ps(simbricks);

    /* allocate host-to-device queue entry */
    msg = simbricks_comm_h2d_alloc(simbricks, cur_ts);

    /* prepare operation */
    if (is_write) {
        write = &msg->write;

        write->req_id = cpu->cpu_index;
        write->offset = addr;
        write->len = size;
        write->bar = bar;


        assert(size <=
            SimbricksPcieIfH2DOutMsgLen(&simbricks->pcieif) - sizeof (*write));
        /* FIXME: this probably only works for LE */
        memcpy((void *) write->data, val, size);


        SimbricksPcieIfH2DOutSend(&simbricks->pcieif, msg,
            SIMBRICKS_PROTO_PCIE_H2D_MSG_WRITE);


        /* we treat writes as posted and don't wait for completion */
        return;
    } else {
        read = &msg->read;

        read->req_id = req - simbricks->reqs;
        read->offset = addr;
        read->len = size;
        read->bar = bar;

        SimbricksPcieIfH2DOutSend(&simbricks->pcieif, msg,
            SIMBRICKS_PROTO_PCIE_H2D_MSG_READ);

        /* start processing request */
        req->processing = true;
        req->requested = true;
        req->size = size;

        req->addr = addr;
        req->bar = bar;
        req->size = size;

        DEBUG_PRINTS("simbricks_mmio_rw: starting wait for read (%lu) addr=%lx size=%x\n", cur_ts, addr, size);

        // release polling thread to process the message
        while(req->processing){
            uint64_t next_ts_try = global_cur_ts+simbricks->sync_period*1000;
            simbricks_advance_rtl(simbricks, next_ts_try/1000, false);
        }

        DEBUG_PRINTS("simbricks_mmio_rw: finished read (%lu) addr=%lx size=%x val=%lx\n", cur_ts, addr, size, req->value);
        
        *val = req->value;
        req->requested = false;
    }
}

uint64_t simbricks_mmio_read(SimbricksPciState* simbricks, uint8_t bar_index, hwaddr addr, unsigned size)
{
    uint64_t ret = 0;
    simbricks_mmio_rw(simbricks, bar_index, addr, size, &ret, false);
    return ret;
}

void simbricks_mmio_write(SimbricksPciState* simbricks, uint8_t bar_index, hwaddr addr, uint64_t val,
                unsigned size)
{
    simbricks_mmio_rw(simbricks, bar_index, addr, size, &val, true);
} 
 
/******************************************************************************/
/* Initialization */
 
static int simbricks_connect(SimbricksPciState *simbricks)
{
    struct SimbricksProtoPcieDevIntro *d_i = &simbricks->dev_intro;
    struct SimbricksProtoPcieHostIntro host_intro;
    struct SimbricksBaseIfParams params;
    size_t len;
    // uint8_t *pci_conf = simbricks->pdev.config;
    uint64_t first_msg_ts = 0;
    volatile union SimbricksProtoPcieD2H *msg;
    struct SimbricksBaseIf *base_if = &simbricks->pcieif.base;

    if (!simbricks->socket_path) {
        // DEBUG_PRINTS("ERROR: socket path not set but required\n");
        return 0;
    }

    SimbricksPcieIfDefaultParams(&params);
    params.link_latency = simbricks->pci_latency * 1000;
    params.sync_interval = simbricks->sync_period * 1000;
    params.blocking_conn = true;
    params.sock_path = simbricks->socket_path;
    // DEBUG_PRINTS("socket path: %s\n", params.sock_path);
    params.sync_mode = (simbricks->sync ? kSimbricksBaseIfSyncRequired :
        kSimbricksBaseIfSyncDisabled);

    if (SimbricksBaseIfInit(base_if, &params)) {
        // DEBUG_PRINTS("ERROR: SimbricksBaseIfInit failed\n");
        return 0;
    }

    if (SimbricksBaseIfConnect(base_if)) {
        // DEBUG_PRINTS("ERROR: SimbricksBaseIfConnect failed\n");
        return 0;
    }

    if (SimbricksBaseIfConnected(base_if)) {
        // DEBUG_PRINTS("ERROR: SimbricksBaseIfConnected indicates unconnected\n");
        return 0;
    }

    /* prepare & send host intro */
    memset(&host_intro, 0, sizeof(host_intro));
    if (SimbricksBaseIfIntroSend(base_if, &host_intro,
                                 sizeof(host_intro))) {
        // DEBUG_PRINTS("ERROR: SimbricksBaseIfIntroSend failed\n");
        return 0;
    }

    /* receive device intro */
    len = sizeof(*d_i);
    if (SimbricksBaseIfIntroRecv(base_if, d_i, &len)) {
        // DEBUG_PRINTS("ERROR: SimbricksBaseIfIntroRecv failed\n");
        return 0;
    }
    if (len != sizeof(*d_i)) {
        // DEBUG_PRINTS("ERROR: rx dev intro: length is not as expected\n");
        return 0;
    }

    // DEBUG_PRINTS("Simbricks device intro recved: device_id: %u \n", d_i->pci_device_id);

    if(simbricks->mem_side_channel != NULL){
        extern void mem_side_channel_realize(PCIDevice *pdev);
        // DEBUG_PRINTS("connecting to mem side channel\n");
        mem_side_channel_realize((PCIDevice *)simbricks->mem_side_channel);
    }

    /* send a first sync */
    if (SimbricksPcieIfH2DOutSync(&simbricks->pcieif, 0)) {
        // DEBUG_PRINTS("ERROR: sending initial sync failed\n");
        return 0;
    }

    /* wait for first message so we know its timestamp */
    do {
        msg = SimbricksPcieIfD2HInPeek(&simbricks->pcieif, 0);
        first_msg_ts = SimbricksPcieIfD2HInTimestamp(&simbricks->pcieif);
    } while (!msg && !first_msg_ts);

    simbricks->reqs_len = 1;
    simbricks->reqs = calloc(simbricks->reqs_len, sizeof(*simbricks->reqs));
    simbricks->reqs[0].cpu = 0;
    qemu_cond_init(&simbricks->reqs[0].cond);
    qemu_cond_init(&simbricks->advance_cond);
    pthread_mutex_init(&simbricks->advance_lock, NULL);
    pthread_mutex_init(&simbricks->msg_sent_lk, NULL);
    
    #define QEMU_THREAD_JOINABLE PTHREAD_CREATE_JOINABLE
    qemu_thread_create(&simbricks->thread, "simbricks-poll",
            simbricks_poll_thread, simbricks, QEMU_THREAD_JOINABLE);

    return 1;
}


 // call this pci_simbricks_realize
void pci_simbricks_realize(PCIDevice *pdev)
{
    SimbricksPciState* simbricks = SIMBRICKS_PCI(pdev);
    if (!simbricks_connect(simbricks)) {
      return;
    }
    simbricks->next_simbricks = simbricks_all;
    simbricks_all = simbricks;
    num_simbricks++;
}
 
void pci_simbricks_uninit(PCIDevice *pdev)
{
    SimbricksPciState *simbricks = SIMBRICKS_PCI(pdev);
    // CPUState *cpu;
 
    simbricks->stopping = true;
    qemu_thread_join(&simbricks->thread);
 
    qemu_cond_destroy(&simbricks->reqs[0].cond);
    free(simbricks->reqs);

    SimbricksBaseIfClose(&simbricks->pcieif.base);
}
 
 
void simbricks_cleanup(void)
{
    SimbricksPciState *sbs;
    for (sbs = simbricks_all; sbs != NULL; sbs = sbs->next_simbricks)
        pci_simbricks_uninit(&sbs->pdev);

}

static void simbricks_comm_h2d_force_update(SimbricksPciState *simbricks, int64_t ts){
    volatile union SimbricksProtoPcieH2D *h2d;
    // the msg can be read by any timestamp of the receiver
    h2d = simbricks_comm_h2d_alloc(simbricks, 0);
    h2d->forceupdate.payload = ts;
    SimbricksPcieIfH2DOutSend(&simbricks->pcieif, h2d,
        SIMBRICKS_PROTO_PCIE_H2D_MSG_FORCESYNC_TIME);
}

void simbricks_advance_rtl_single(SimbricksPciState *simbricks, uint64_t ts_ns, int force)
{   
    uint64_t proto_ts = ts_ns*1000;
    
    if(force){
        DEBUG_PRINTS("force update to ps : %lu\n", proto_ts);
        simbricks_comm_h2d_force_update(simbricks, proto_ts);

    }

    // assert(proto_ts > simbricks->advance_till_ts);
    if(proto_ts < simbricks->cur_ts){
        proto_ts = simbricks->cur_ts;
        safe_printf("warning: simbricks_advance_rtl: %lu, last time %lu\n", proto_ts/1000, simbricks->cur_ts/1000);
    }
    global_cur_ts = proto_ts;
    advance_wait_until_(simbricks, proto_ts);
}


void simbricks_advance_rtl(SimbricksPciState *simbricks, uint64_t ts_ns, int force)
{   

    uint64_t proto_ts = ts_ns*1000;

    SimbricksPciState* cur_simbricks = simbricks_all;
    while(cur_simbricks != NULL){
        // force update time
        if(cur_simbricks->active == 0){
            simbricks_comm_h2d_force_update(cur_simbricks, proto_ts);
        }
        cur_simbricks = cur_simbricks->next_simbricks;
    }

    // all are inactive
    if(global_cur_ts == 0){
        // fast forward all simbricks to the same time
        // SimbricksPciState* cur_simbricks = simbricks_all;
        // while(cur_simbricks != NULL){
        //     // force update time
        //     simbricks_comm_h2d_force_update(cur_simbricks, proto_ts);
        //     cur_simbricks = cur_simbricks->next_simbricks;
        // }
        global_cur_ts = proto_ts;
        safe_printf("fast forward all simbricks to %lu\n", proto_ts/1000);
    }

    if(simbricks == NULL){
        simbricks = simbricks_all;
        if(simbricks == NULL){
            return;
        }
    }

    if(num_simbricks == 1){
        return simbricks_advance_rtl_single(simbricks, ts_ns, force);
    }
    
    if(force){
        DEBUG_PRINTS("force update to ps : %lu\n", proto_ts);
        simbricks_comm_h2d_force_update(simbricks, proto_ts);
    }

    if(proto_ts < simbricks->cur_ts){
        proto_ts = simbricks->cur_ts;
        safe_printf("warning: simbricks_advance_rtl: %lu, last time %lu; forcing update global time stamp\n", proto_ts/1000, simbricks->cur_ts/1000);
        // return;
    }

    // sync all devices
    advance_wait_until_all(proto_ts);
}

void disconnect_all_simulators(){
    SimbricksPciState* cur_simbricks = simbricks_all;
    while(cur_simbricks != NULL){
        SimbricksBaseIfClose(&cur_simbricks->pcieif.base);
        if(cur_simbricks->mem_side_channel){
            SimbricksBaseIfClose(&cur_simbricks->mem_side_channel->memif.base);
        }
        cur_simbricks = cur_simbricks->next_simbricks;
    }
}