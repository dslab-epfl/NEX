#pragma once
#include <exec/exec.h>
#include <exec/hw_rw.h>
#include <pthread.h>
#include <simbricks/pcie/if.h>
#include <simbricks/mem/if.h>

typedef pthread_cond_t QemuCond;

typedef pthread_t QemuThread;

typedef struct CPUState{
    int cpu_index;
    int stopped;
}CPUState;

// Simulate PCI Device and MemoryRegion (normally provided by QEMU)
typedef struct PCIDevice {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint32_t class_id;
    uint8_t interrupt_pin;
    uint64_t config[256]; // simplified PCI config space
} PCIDevice;

typedef struct MemoryRegion {
    void *memory;       // Pointer to the actual memory region
    size_t size;        // Size of the memory region
} MemoryRegion;

typedef struct SimbricksPciBarInfo {
    struct SimbricksPciState *simbricks;
    uint8_t index;
    bool is_io;
    bool is_dummy;
} SimbricksPciBarInfo;

// Thread and synchronization handling using pthread
typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile bool stopping;
} SimbricksPciThread;

typedef struct SimbricksPciRequest {
    CPUState *cpu;          /* CPU associated with this request */
    QemuCond cond;
    uint64_t addr;
    uint8_t bar;
    uint64_t value;         /* value read/to be written */
    unsigned size;
    bool processing;
    bool requested;
} SimbricksPciRequest;


typedef struct SimbricksMemState {
    PCIDevice pdev;
    QemuThread thread;
    QemuThread thread_sync;
    QemuCond advance_cond;
    pthread_mutex_t advance_lock;
    volatile bool stopping;

    /* config parameters */
    char *socket_path;      /* path to ux socket to connect to */

    struct SimbricksMemIf memif;
    struct SimbricksProtoMemMemIntro mem_intro; 

    // added for nex
    int dma_fd;
    int pid;
    uint64_t dma_base_addr;
    uint64_t read_addr_base; 
    uint64_t write_addr_base;
    struct SimbricksMemState *next_simbricks;
} SimbricksMemState;


typedef struct SimbricksPciState {
    PCIDevice pdev;
    pthread_mutex_t msg_sent_lk;
    QemuThread thread;
    QemuThread thread_sync;
    QemuCond advance_cond;
    pthread_mutex_t advance_lock;
    volatile bool stopping;

    /* config parameters */
    char *socket_path;      /* path to ux socket to connect to */
    uint64_t pci_latency;
    uint64_t sync_period;


    MemoryRegion mmio_bars[6];
    SimbricksPciBarInfo bar_info[6];

    struct SimbricksPcieIf pcieif;  // Simbricks PCI interface
    struct SimbricksProtoPcieDevIntro dev_intro; // Simbricks Device introduction

    bool sync_ts_bumped;

    size_t reqs_len;
    SimbricksPciRequest *reqs;

    bool sync;
    int sync_mode;
    int64_t ts_base;
    

    // added for zurvan
    int dev_id;
    int dma_fd;
    int pid;
    int open_fd_pid;
    uint64_t dma_base_addr;
    uint64_t read_addr_base; 
    uint64_t write_addr_base;
    volatile uint64_t cur_ts;
    volatile uint64_t advance_till_ts;
    volatile uint32_t blocked;
    int active;
    int issued_task_cnt;
    struct SimbricksPciState *next_simbricks;
    struct SimbricksMemState *mem_side_channel;
} SimbricksPciState;


#define hwaddr uint64_t
void disconnect_all_simulators();
void pci_simbricks_realize(PCIDevice *simbricks);
void pci_simbricks_uninit(PCIDevice *pdev);
void simbricks_cleanup(void);
uint64_t simbricks_mmio_read(SimbricksPciState* simbricks, uint8_t bar_index, hwaddr addr, unsigned size);
void simbricks_mmio_write(SimbricksPciState* simbricks, uint8_t bar_index, hwaddr addr, uint64_t val, unsigned size);
void simbricks_advance_rtl(SimbricksPciState *simbricks, uint64_t ts, int force);
#define warn_report(...) printf(__VA_ARGS__)

void mem_side_channel_realize(PCIDevice *pdev);
void mem_side_channel_uninit(PCIDevice *pdev);
void mem_side_channel_cleanup(void);
bool mem_side_channel_poll(SimbricksMemState*);