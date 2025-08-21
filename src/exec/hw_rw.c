#include <exec/exec.h>
#include <exec/bpf.h>
#include <exec/hw.h>
#include <exec/hw_rw.h>
#include <exec/ptrace.h>
#include <exec/decode.h>
#include <stdint.h>
#include <sys/mman.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <drivers/driver_common.h>
#include <sims/sim.h>
#include <capstone/x86.h>

#define WRITE_REG_TYPE 0
#define READ_REG_TYPE 1
#define START_TYPE 2
#define FINISH_TYPE 3

#define MAX_NUM_DEVICES_EACH 10

extern int create_smem(const char *shm_name, int size, int init);
uintptr_t init_accelerator_dma_region(const char *shm_name, int size, int init, int* fd){
  int shm_fd = create_smem(shm_name, size, init);
  *fd = shm_fd;
  // no protection
  void* mem_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (mem_base == MAP_FAILED) {
      perror("Failed to map MMIO region");
      exit(EXIT_FAILURE);
  }
  memset(mem_base, 0, size);
  return (uintptr_t)mem_base;
}

int open_pid_mem(pid_t pid){
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    int fd = open(mem_path, O_RDWR);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    return fd;
}

int close_pid_mem(int fd){
    if (close(fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

ssize_t read_process_memory(int fd, uintptr_t address, void *buffer, size_t size) {
   //log out to a file 
    safe_printf("zurvan read_process_memory %p %zu\n", (void*)address, size);
    ssize_t nread = pread(fd, buffer, size, address);
    for(int i = 0; i < nread; i++) {
        safe_printf("%02x", ((unsigned char*)buffer)[i]);
    }
    safe_printf("\n");
    return nread;
}

ssize_t write_process_memory(int fd, uintptr_t address, void *buffer, size_t size){
    safe_printf("zurvan write_process_memory %p %zu\n", (void*)address, size);
    ssize_t nwrite = pwrite(fd, buffer, size, address);
    return nwrite;
}

static int bpf_sched_ctrl_fd = -1;
void bpf_sched_update_state(uint64_t value){
  // Update the state of the current thread
  // safe_printf("Updating state to %lu\n", value);
  if(bpf_sched_ctrl_fd == -1){
    bpf_sched_ctrl_fd = get_bpf_map("bpf_sched_ctrl");
  }
  int index = 0;
  put_bpf_map(bpf_sched_ctrl_fd, &index, &value, BPF_MAP_UPDATE);
}

void bpf_sched_update_state_per_pid(uint32_t ctrl_pid, uint32_t ctrl_msg){
  struct pstate state;
  put_bpf_map(sim_proc_state_fd, &ctrl_pid, &state, BPF_MAP_LOOKUP);
  state.ctrl_msg = ctrl_msg;
  put_bpf_map(sim_proc_state_fd, &ctrl_pid, &state, BPF_MAP_UPDATE);
}


void add_trace_event(int accel, uint64_t event, uint64_t ts) {
} 


void trigger_application_interrupt(int pid){
  // triggered
  if(pid != -1){
    safe_printf("Triggering application interrupt for pid %d\n", pid);
    kill(pid, SIGUSR1);
    safe_printf("Triggering application interrupt for pid return %d\n", pid);
  }
}

void set_mmio_to_user(){
  // switched to share memory, so not needed
  return;
}

SchedRegs sched_reg = {0};

#if CONFIG_NUM_JPEG > 0
static JpegRegs* jpeg_regs_arr[CONFIG_NUM_JPEG];
static SimbricksPciState* jpeg_array[CONFIG_NUM_JPEG];
#else
static JpegRegs* jpeg_regs_arr[1];
static SimbricksPciState* jpeg_array[1];
#endif

#if CONFIG_NUM_VTA > 0
static VTARegs* vta_regs_arr[CONFIG_NUM_VTA];
static SimbricksPciState* vta_array[CONFIG_NUM_VTA];
#else
static VTARegs* vta_regs_arr[1];
static SimbricksPciState* vta_array[1];
#endif

#if CONFIG_NUM_PROTOACC > 0
static ProtoAccRegs* protoacc_regs_arr[CONFIG_NUM_PROTOACC];
static SimbricksPciState* protoacc_array[CONFIG_NUM_PROTOACC];
#else
static ProtoAccRegs* protoacc_regs_arr[1];
static SimbricksPciState* protoacc_array[1];
#endif

static int dev_id_counter = 0;

void redirect_output(char* simulator_name, int device_num);

#define RTL_TYPE 0
#define DSIM_TYPE 1
void fork_simulator(char* name, int type, int device_num, int link_latency, int frequency){
  pid_t pid = fork();
  if(pid == 0){
    char path[200];
    char sock_path[200];
    char mem_path[200];
    char mem_side_channel_sock_path[200];
    char mem_side_channel_mem_path[200];
    char frequency_str[20];
    sprintf(frequency_str, "%d", frequency);
    char link_latency_str[20];
    sprintf(link_latency_str, "%d", link_latency);
    if(type == RTL_TYPE){
      sprintf(path, "%s/simulators/rtl/%s", CONFIG_PROJECT_PATH, name);
      // ./${sim}_simbricks /tmp/${sim}_sock /tmp/${sim}_shm 0 $latency $latency 2000
      sprintf(sock_path, "/tmp/%s_rtl_sock.%d", name, device_num);
      if (remove(sock_path) == -1) {
        safe_printf("remove failed %s %d\n", sock_path, errno);
      }
      sprintf(mem_path, "/tmp/%s_rtl_mem.%d", name, device_num);
      char* args[] = {path, sock_path, mem_path, "0", link_latency_str, link_latency_str, frequency_str, NULL};
      prctl(PR_SET_PDEATHSIG, SIGTERM);
      redirect_output(name, device_num);
      execvp(args[0], args);
      perror("execvp");

    }else if(type == DSIM_TYPE){
      sprintf(path, "%s/simulators/dsim/%s", CONFIG_PROJECT_PATH, name);
      sprintf(sock_path, "/tmp/%s_dsim_sock.%d", name, device_num);
      if (remove(sock_path) == -1) {
        safe_printf("remove failed %s %d\n", sock_path, errno);
      }
      sprintf(mem_path, "/tmp/%s_dsim_mem.%d", name, device_num);\
      sprintf(mem_side_channel_sock_path, "/tmp/%s_dsim_msc_sock.%d", name, device_num);
      if (remove(mem_side_channel_sock_path) == -1) {
        safe_printf("remove failed %s %d\n", mem_side_channel_sock_path, errno);
      }
      sprintf(mem_side_channel_mem_path, "/tmp/%s_dsim_msc_mem.%d", name, device_num);
      char* args[] = {path, mem_side_channel_sock_path, mem_side_channel_mem_path,sock_path, mem_path, "0", link_latency_str, link_latency_str, NULL};
      prctl(PR_SET_PDEATHSIG, SIGTERM);
      redirect_output(name, device_num);
      execvp(args[0], args);
      perror("execvp");
    }
  }
}

void wait_for_simulator_launch(char* name, int dev, int type){
  char sock_path[200];
  if(type == RTL_TYPE){
    sprintf(sock_path, "/tmp/%s_rtl_sock.%d", name, dev);
  }else{
    sprintf(sock_path, "/tmp/%s_dsim_sock.%d", name, dev);
  }
  while (access(sock_path, F_OK) == -1) {
    if (errno != ENOENT) {  // Something else went wrong
        perror("access");
        exit(EXIT_FAILURE);
    }
    usleep(10000); 
  }
}

void hw_init(){

  #define DMA_SIZE 0x20000000

  for (int i = 0; i < CONFIG_NUM_JPEG; i++) {
    safe_printf("forking JPEG %d\n", i);
    int sim_type = CONFIG_JPEG_RTL ? RTL_TYPE : DSIM_TYPE;
    fork_simulator("jpeg", sim_type, i, CONFIG_JPEG_LINK_DELAY, 2000);
  }
  for (int i = 0; i < CONFIG_NUM_VTA; i++) {
    safe_printf("forking VTA %d\n", i);
    int sim_type = CONFIG_VTA_RTL ? RTL_TYPE : DSIM_TYPE;
    fork_simulator("vta", sim_type, i, CONFIG_VTA_LINK_DELAY, 2000);
  }
  for (int i = 0; i < CONFIG_NUM_PROTOACC; i++) {
    safe_printf("forking ProtoAcc %d\n", i);
    int sim_type = CONFIG_PROTOACC_RTL ? RTL_TYPE : DSIM_TYPE;
    fork_simulator("protoacc", sim_type, i, CONFIG_PROTOACC_LINK_DELAY, 2000);
  }

  #define GET_REWR_ADDR(name, type, dev) \

  // sleep(1);
  #define NEW_RTL_ADPATER(name, path, link_delay) \
  name = (SimbricksPciState*) malloc(sizeof(SimbricksPciState)); \
  name->socket_path = path; \
  name->pci_latency = link_delay; \
  name->sync_period = link_delay; \
  name->sync = true; \
  name->advance_till_ts = 0; \
  name->dma_fd = -1; \
  name->pid = -1; \
  name->dev_id = dev_id_counter++; \
  name->active = 0; \
  name->issued_task_cnt = 0; \
  pci_simbricks_realize((PCIDevice*) name);

  #define NEW_DSIM_ADPATER(name, path, mem_path, link_delay) \
  name = (SimbricksPciState*) malloc(sizeof(SimbricksPciState)); \
  name->socket_path = path; \
  name->pci_latency = link_delay; \
  name->sync_period = link_delay; \
  name->sync = true; \
  name->advance_till_ts = 0; \
  name->dma_fd = -1; \
  name->pid = -1; \
  name->dev_id = dev_id_counter++; \
  name->active = 0; \
  name->issued_task_cnt = 0; \
  name->mem_side_channel = (SimbricksMemState*) malloc(sizeof(SimbricksMemState)); \
  name->mem_side_channel->socket_path = mem_path; \
  name->mem_side_channel->dma_fd = -1; \
  pci_simbricks_realize((PCIDevice*) name);

  for (int i = 0; i < CONFIG_NUM_JPEG; i++) {
    int sim_type = CONFIG_JPEG_RTL ? RTL_TYPE : DSIM_TYPE;
    wait_for_simulator_launch("jpeg", i, sim_type);
    safe_printf("connect JPEG %d\n", i);
    char shm_dma_region[200] = {0};
    sprintf(shm_dma_region, "jpeg_nex_dma_%d", i);
    int dma_fd;
    uintptr_t dma_base = init_accelerator_dma_region(shm_dma_region, DMA_SIZE, 1, &dma_fd);
    if(sim_type == RTL_TYPE){
      char* jpeg_sock_path = malloc(200);
      sprintf(jpeg_sock_path, "/tmp/jpeg_rtl_sock.%d", i);
      NEW_RTL_ADPATER(jpeg_array[i], jpeg_sock_path, CONFIG_JPEG_LINK_DELAY);
      jpeg_array[i]->dma_fd = dma_fd;
      jpeg_array[i]->dma_base_addr = dma_base;
    }else{
      char* jpeg_sock_path = malloc(200);
      sprintf(jpeg_sock_path, "/tmp/jpeg_dsim_sock.%d", i);
      char* jpeg_mem_msc_path = malloc(200);
      sprintf(jpeg_mem_msc_path, "/tmp/jpeg_dsim_msc_sock.%d", i);
      NEW_DSIM_ADPATER(jpeg_array[i], jpeg_sock_path, jpeg_mem_msc_path, CONFIG_JPEG_LINK_DELAY);
      jpeg_array[i]->dma_fd = dma_fd;
      jpeg_array[i]->dma_base_addr = dma_base;
      jpeg_array[i]->mem_side_channel->dma_fd = dma_fd;
      jpeg_array[i]->mem_side_channel->dma_base_addr = dma_base;
    }
  }

  for (int i = 0; i < CONFIG_NUM_VTA; i++) {
    int sim_type = CONFIG_VTA_RTL ? RTL_TYPE : DSIM_TYPE;
    wait_for_simulator_launch("vta", i, sim_type);
    safe_printf("connect VTA %d\n", i);

    char shm_dma_region[200] = {0};
    sprintf(shm_dma_region, "vta_nex_dma_%d", i);
    int dma_fd;
    uintptr_t dma_base = init_accelerator_dma_region(shm_dma_region, DMA_SIZE, 1, &dma_fd);

    if(sim_type == RTL_TYPE){
      char* vta_sock_path = malloc(200);
      sprintf(vta_sock_path, "/tmp/vta_rtl_sock.%d", i);
      NEW_RTL_ADPATER(vta_array[i], vta_sock_path, CONFIG_VTA_LINK_DELAY);
      vta_array[i]->dma_fd = dma_fd;
      vta_array[i]->dma_base_addr = dma_base;
    }else{
      char* vta_sock_path = malloc(200);
      sprintf(vta_sock_path, "/tmp/vta_dsim_sock.%d", i);
      char* vta_mem_path = malloc(200);
      sprintf(vta_mem_path, "/tmp/vta_dsim_msc_sock.%d", i);
      NEW_DSIM_ADPATER(vta_array[i], vta_sock_path, vta_mem_path, CONFIG_VTA_LINK_DELAY);
      vta_array[i]->dma_fd = dma_fd;
      vta_array[i]->dma_base_addr = dma_base;
      vta_array[i]->mem_side_channel->dma_fd = dma_fd;
      vta_array[i]->mem_side_channel->dma_base_addr = dma_base;
    }
  }

  for(int i = 0; i < CONFIG_NUM_PROTOACC; i++){
    int sim_type = CONFIG_PROTOACC_RTL ? RTL_TYPE : DSIM_TYPE;
    wait_for_simulator_launch("protoacc", i, sim_type);
    safe_printf("connect ProtoAcc %d\n", i);
    char shm_dma_region[200] = {0};
    sprintf(shm_dma_region, "protoacc_nex_dma_%d", i);
    int dma_fd;
    uintptr_t dma_base = init_accelerator_dma_region(shm_dma_region, DMA_SIZE, 1, &dma_fd);

    if(sim_type == RTL_TYPE){
      char* protoacc_sock_path = malloc(200);
      sprintf(protoacc_sock_path, "/tmp/protoacc_rtl_sock.%d", i);
      NEW_RTL_ADPATER(protoacc_array[i], protoacc_sock_path, CONFIG_PROTOACC_LINK_DELAY);
      protoacc_array[i]->dma_fd = dma_fd;
      protoacc_array[i]->dma_base_addr = dma_base;
    }else{
      char* protoacc_sock_path = malloc(200);
      sprintf(protoacc_sock_path, "/tmp/protoacc_dsim_sock.%d", i);
      char* protoacc_mem_path = malloc(200);
      sprintf(protoacc_mem_path, "/tmp/protoacc_dsim_msc_sock.%d", i);
      NEW_DSIM_ADPATER(protoacc_array[i], protoacc_sock_path, protoacc_mem_path, CONFIG_PROTOACC_LINK_DELAY);
      protoacc_array[i]->dma_fd = dma_fd;
      protoacc_array[i]->dma_base_addr = dma_base;
      protoacc_array[i]->mem_side_channel->dma_fd = dma_fd;
      protoacc_array[i]->mem_side_channel->dma_base_addr = dma_base;
    }
    safe_printf("ProtoAcc %d, sim ptr %p \n", i, protoacc_array[i]);
  }
  
#if CONFIG_PCIE_LPN 
    pcie_lpn_init();
#endif

  mem_lpn_init();

  set_mmio_to_user();

  for(int i = 0; i < CONFIG_NUM_JPEG; i++){
    jpeg_regs_arr[i] = (JpegRegs*) malloc(sizeof(JpegRegs));
    memset(jpeg_regs_arr[i], 0, sizeof(JpegRegs));
  }

  for(int i = 0; i < CONFIG_NUM_VTA; i++){
    vta_regs_arr[i] = (VTARegs*) malloc(sizeof(VTARegs));
    memset(vta_regs_arr[i], 0, sizeof(VTARegs));
  }

  for(int i = 0; i < CONFIG_NUM_PROTOACC; i++){
    protoacc_regs_arr[i] = (ProtoAccRegs*) malloc(sizeof(ProtoAccRegs));
    memset(protoacc_regs_arr[i], 0, sizeof(ProtoAccRegs));
  }
}

void hw_deinit(){
  disconnect_all_simulators();

  pid_t setup_child = fork();
  if(setup_child == 0){
      char command[100];
      sprintf(command, "rm /tmp/*sock*");
      char *args[] = {"/bin/sh", "-c", command, NULL};
      execvp(args[0], args);
      perror("execvp failed");
  }
  waitpid(setup_child, NULL, 0);
}

void* eager_sync_accelerator_manager(void* args){
  struct custom_event evnt;
  evnt.type = 0;
  evnt.data = 0;
  struct custom_event evnt_from_kernel;
  while(eager_sync_stop!=1){
    if(put_bpf_map(to_nex_runtime_event_q_fd, NULL, &evnt_from_kernel, BPF_MAP_LOOKUP_DELETE)==0){
      safe_printf("kernel event, vts: %lu event vts: %lu\n", read_vts(), evnt_from_kernel.ts);
      simbricks_advance_rtl(NULL, evnt_from_kernel.ts, false);  
      evnt.ts = evnt_from_kernel.ts;
      put_bpf_map(from_nex_runtime_event_q_fd, NULL, &evnt, BPF_MAP_UPDATE);
      // trigger_application_interrupt(jpeg_array[0]->pid);
    }
    sched_yield();
  }
  return NULL;
}

typedef struct reg_updates{
  int rd_wr;
  uint32_t type;
  uint32_t dev;
  uint64_t addr;
  uint64_t value;
  uint64_t size;
  struct reg_updates* next;
  int has_next;
} reg_updates_t;

reg_updates_t reg_update_head = {0};

#define MMIO_ADDR_OF_X(type, dev, offset) ((uint64_t*)(mmio_base + 4096 * (type*MAX_NUM_DEVICES_EACH + dev) + offset))
static int detect_change_each_reg(reg_updates_t* record_update_parent, uint64_t last_value, int type, int dev, uint64_t offset, int size){
  uint64_t new_value;
  if(size == 8){
    new_value = *((uint64_t*)MMIO_ADDR_OF_X(type, dev, offset));
  }else{
    // size = 4
    new_value = *((uint32_t*)MMIO_ADDR_OF_X(type, dev, offset));
  }
  if(new_value != last_value){
    record_update_parent->has_next = 1;
    reg_updates_t* record_update = record_update_parent->next;
    if(record_update == NULL){
      record_update_parent->next = (reg_updates_t*)malloc(sizeof(reg_updates_t));
      memset(record_update_parent->next, 0, sizeof(reg_updates_t));
      record_update = record_update_parent->next;
      if (record_update == NULL) {
        perror("malloc failed");
        return -1; // Handle allocation failure
      }
    }
    record_update->type = type;
    record_update->dev = dev;
    record_update->addr = offset;
    record_update->value = new_value;
    record_update->size = size;
    record_update->rd_wr = WRITE_REG_TYPE;
    if(size == 8 && new_value == 0xFFFFFFFFFFFFFFFF){
      record_update->rd_wr = READ_REG_TYPE;
    }
    if(size == 4 && new_value == 0xFFFFFFFF){
      record_update->rd_wr = READ_REG_TYPE;
    }
    return 1;
  }else{
    // didn't change
    record_update_parent->has_next = 0;
    return 0;
  }
}

void detect_mmio_region_changes(){
  reg_updates_t* current = &reg_update_head;
  current->has_next = 0;

  {
    SchedRegs *regs = &sched_reg;
    if(detect_change_each_reg(current, regs->ctrl, BPF_SCHED, 0, 0, 8)){
      current = current->next;
      regs->ctrl = current->value;
    }
  }

  for (int i = 0; i < CONFIG_NUM_JPEG; i++) {
    JpegRegs* regs = jpeg_regs_arr[i];
    if(detect_change_each_reg(current, (uint64_t)regs->ctrl, JPEG_DECODER, i, 0, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->ctrl = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->isBusy, JPEG_DECODER, i, 4, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->isBusy = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->src, JPEG_DECODER, i, 8, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->src = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->dst, JPEG_DECODER, i, 16, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->dst = current->value;
      }
    }
  }

  for(int i = 0; i < CONFIG_NUM_VTA; i++){
    VTARegs* regs = vta_regs_arr[i];
    if(detect_change_each_reg(current, (uint64_t)regs->_0x00, VTA, i, 0, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x00 = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->_0x04, VTA, i, 4, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x04 = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->_0x08, VTA, i, 8, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x08 = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->_0x0c, VTA, i, 12, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x0c = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->_0x10, VTA, i, 16, 4)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x10 = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->_0x24, VTA, i, 36, 8)){
      // not to be updated to the accelerator
      // for self reference of NEX
      current->has_next = 0;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->_0x24 = current->value;
      }
    }
  }

  for(int i = 0; i < CONFIG_NUM_PROTOACC; i++){
    ProtoAccRegs* regs = protoacc_regs_arr[i];
    if(detect_change_each_reg(current, (uint64_t)regs->ctrl, PROTOACC, i, 0, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->ctrl = current->value;      
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->completed_msg, PROTOACC, i, 8, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->completed_msg = current->value;  
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->stringobj_output_addr, PROTOACC, i, 16, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->stringobj_output_addr = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->string_ptr_output_addr, PROTOACC, i, 24, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->string_ptr_output_addr = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->hasbits_offset, PROTOACC, i, 32, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->hasbits_offset = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->min_max_fieldno, PROTOACC, i, 40, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->min_max_fieldno = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->descriptor_table_ptr, PROTOACC, i, 48, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->descriptor_table_ptr = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->src_base_addr, PROTOACC, i, 56, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->src_base_addr = current->value;
      }
    }
    if(detect_change_each_reg(current, (uint64_t)regs->mem_base, PROTOACC, i, 64, 8)){
      current = current->next;
      if(current->rd_wr == WRITE_REG_TYPE){
        regs->mem_base = current->value;
      }
    }
  }
}

void handle_update(reg_updates_t* current, uint64_t virtual_ts, int pid){
  switch(current->type){
    case BPF_SCHED:{
      SchedRegs *regs = &sched_reg;
      uint32_t ctrl_pid = (regs->ctrl >> 32) & 0xFFFFFFFF;
      uint32_t ctrl_msg = regs->ctrl & 0xFFFFFFFF;
      if(ctrl_pid == 0){
        // not specified, default to the current one faulting, so needs to lock the ctrl->lock to protect access
        ctrl_pid = pid;
        assert(ctrl_msg == 0x1000 || ctrl_msg == 0x2000);
        // only no pid specific messages can be used without pid
      }else{
        safe_printf("ctrl pid %d, current pid %d\n", ctrl_pid, pid);
        // allow it to differ
        // assert(pid == ctrl_pid);
        if(pid != ctrl_pid){
           // not my tick 
           regs->ctrl = regs->ctrl + 1; // to make it different
           return;
        }
      }
      bpf_sched_update_state_per_pid(ctrl_pid, ctrl_msg);
      // bpf_sched_update_state(current->value);

      regs->ctrl = 0;
      *(uint64_t*)MMIO_ADDR_OF_X(BPF_SCHED, 0, 0) = regs->ctrl;

      break;
    }
    case JPEG_DECODER:{
      assert(current->dev < CONFIG_NUM_JPEG);
      JpegRegs *regs = jpeg_regs_arr[current->dev];
      SimbricksPciState* jpeg_dev = jpeg_array[current->dev];
      if (regs->ctrl & CTRL_REG_START_BIT) {
        safe_printf("Starting JPEG decoding %d, ctrl %p\n", current->dev, regs->ctrl);
        // hack for interrupts
        jpeg_dev->pid = pid;
        jpeg_dev->read_addr_base = regs->src;
        jpeg_dev->write_addr_base = regs->dst;
        if(jpeg_dev->mem_side_channel != NULL){
          jpeg_dev->mem_side_channel->read_addr_base = regs->src;
          jpeg_dev->mem_side_channel->write_addr_base = regs->dst;
        }
        simbricks_advance_rtl(jpeg_dev, virtual_ts, true);
        simbricks_mmio_write(jpeg_dev, 0, 0x8, 0, 4);
        simbricks_mmio_write(jpeg_dev, 0, 0xc, 0, 4);
        simbricks_mmio_write(jpeg_dev, 0, 0x0, regs->ctrl, 4);
        jpeg_dev->active = 1;
        
        // always do both copies
        regs->ctrl &= ~CTRL_REG_START_BIT;
        *(uint32_t*)MMIO_ADDR_OF_X(JPEG_DECODER, current->dev, 0) = regs->ctrl;
        safe_printf("Starting JPEG decoding done %d, ctrl %p\n", current->dev, regs->ctrl);
      }
      break;
    }
    case VTA:{
      assert(current->dev < CONFIG_NUM_VTA);
      VTARegs *regs = vta_regs_arr[current->dev];
      SimbricksPciState* vta_dev = vta_array[current->dev];

      if ((regs->_0x00 & 0x1) == 0x1) {
        vta_dev->read_addr_base = 0; //regs->_0x24;
        vta_dev->write_addr_base = 0; //regs->_0x24;
        if(vta_dev->mem_side_channel != NULL){
          vta_dev->mem_side_channel->read_addr_base = 0; //regs->_0x24;
          vta_dev->mem_side_channel->write_addr_base = 0; //regs->_0x24;
        }
        simbricks_advance_rtl(vta_dev, virtual_ts, true);
  
        safe_printf("vta_dev instruct count %u; id %d\n", regs->_0x08, current->dev);
        simbricks_mmio_write(vta_dev, 0, 0x08, regs->_0x08, 4);
        simbricks_mmio_write(vta_dev, 0, 0x10, regs->_0x10, 4);
        simbricks_mmio_write(vta_dev, 0, 0x0c, regs->_0x0c, 4);
        simbricks_mmio_write(vta_dev, 0, 0x00, regs->_0x00, 4);
        vta_dev->active = 1;
        // force advance VTA timestamp
        // revert started states
        regs->_0x00 &= ~0x1;
        *(uint32_t*)MMIO_ADDR_OF_X(VTA, current->dev, 0) = regs->_0x00;
      }

      break;
    }
    case PROTOACC:{
      assert(current->dev < CONFIG_NUM_PROTOACC);
      ProtoAccRegs *regs = protoacc_regs_arr[current->dev];
      SimbricksPciState* protoacc_dev = protoacc_array[current->dev];
      if (((regs->ctrl & 0xFF) >> 7) == 1){
          protoacc_dev->read_addr_base = 0 - regs->mem_base;
          protoacc_dev->write_addr_base = 0 - regs->mem_base;
          if(protoacc_dev->mem_side_channel != NULL){
            protoacc_dev->mem_side_channel->read_addr_base = 0 - regs->mem_base;
            protoacc_dev->mem_side_channel->write_addr_base = 0 - regs->mem_base;
          }
          simbricks_advance_rtl(protoacc_dev, virtual_ts, false);
          simbricks_mmio_write(protoacc_dev, 0, 0x28, regs->stringobj_output_addr & 0xffffffff, 4);
          simbricks_mmio_write(protoacc_dev, 0, 0x2c, (regs->stringobj_output_addr >> 32)& 0xffffffff, 4);
          simbricks_mmio_write(protoacc_dev, 0, 0x30, regs->string_ptr_output_addr & 0xffffffff, 4);
          simbricks_mmio_write(protoacc_dev, 0, 0x34, (regs->string_ptr_output_addr >> 32) & 0xffffffff, 4);
          simbricks_mmio_write(protoacc_dev, 0, 0x0, regs->ctrl, 4);
          safe_printf("force updating protoacc timestamp, ctrl value %p\n", regs->ctrl);
          regs->ctrl = 0;
          *(uint64_t*)MMIO_ADDR_OF_X(PROTOACC, current->dev, 0) = regs->ctrl;
      }
      
      if ((regs->ctrl & 0x1) == 1) {
        uint32_t max_fieldno = regs->min_max_fieldno & 0xFFFFFFFF;
        uint32_t min_fieldno = (regs->min_max_fieldno >> 32) & 0xFFFFFFFF;
        // to offset the src_base_addr
        simbricks_advance_rtl(protoacc_dev, virtual_ts, false);
        simbricks_mmio_write(protoacc_dev, 0, 0x8, min_fieldno, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0xc, max_fieldno, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x10, regs->descriptor_table_ptr & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x14, (regs->descriptor_table_ptr >> 32)& 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x18, regs->src_base_addr & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x1c, (regs->src_base_addr >> 32) & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x20, regs->hasbits_offset & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x24, (regs->hasbits_offset >> 32) & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x0, regs->ctrl, 4);
        protoacc_dev->active = 1;
        protoacc_dev->issued_task_cnt++;

        // if(protoacc_dev->issued_task_cnt%100 == 0){
          safe_printf("protoacc_dev issued task count %d; id %d, virtual_ts_us %lu\n", protoacc_dev->issued_task_cnt, current->dev, virtual_ts/1000);
        // }
  
        regs->ctrl &= ~0x1;
        *(uint64_t*)MMIO_ADDR_OF_X(PROTOACC, current->dev, 0) = regs->ctrl;
      }
    
      break;
    }
  }
}

void handle_fetch(reg_updates_t* current, uint64_t virtual_ts, int pid){
  uintptr_t reg_addr_reg = 0;
  switch(current->type){
    case BPF_SCHED:{
      break;
    }
    case JPEG_DECODER:{
      assert(current->dev < CONFIG_NUM_JPEG);
      JpegRegs *regs = jpeg_regs_arr[current->dev];
      SimbricksPciState* jpeg_dev = jpeg_array[current->dev];
      uint64_t is_busy = simbricks_mmio_read(jpeg_dev, 0, 0x4, 4);
      if(is_busy == 0){
        regs->ctrl |= CTRL_REG_DONE_BIT;
        jpeg_dev->active = 0;
      }
      current->value = regs->ctrl;
      // safe_printf("advance jpeg accel %ld done; dev %d; is busy %d, ctrl %p\n", virtual_ts, current->dev, is_busy, regs->ctrl);
      reg_addr_reg = (uintptr_t)jpeg_regs_arr[current->dev];
      break;
    }
    case VTA:{
      assert(current->dev < CONFIG_NUM_VTA);
      VTARegs *regs = vta_regs_arr[current->dev];
      SimbricksPciState* vta_dev = vta_array[current->dev];
      safe_printf("reading vta status virtual_ts %lu \n", virtual_ts);
      uint64_t finished = (simbricks_mmio_read(vta_dev, 0, 0x00, 4) & 0x2) == 0x2;

      if(finished){
        regs->_0x00 |= 0x2;
        vta_dev->active = 0;
      }
      current->value = regs->_0x00;
      reg_addr_reg = (uintptr_t)vta_regs_arr[current->dev];
      break;
    }
    case PROTOACC:{
      assert(current->dev < CONFIG_NUM_PROTOACC);
      ProtoAccRegs *regs = protoacc_regs_arr[current->dev];
      SimbricksPciState* protoacc_dev = protoacc_array[current->dev];
      safe_printf("reading protoacc completed_msg %ld, virtual_ts %lu \n", regs->completed_msg, virtual_ts);
      simbricks_advance_rtl(protoacc_dev, virtual_ts, false);
      // hard code
      current->value = simbricks_mmio_read(protoacc_dev, 0, 0x04, 4);
      regs->completed_msg = current->value;
      if(protoacc_dev->issued_task_cnt == regs->completed_msg){
        protoacc_dev->active = 0;
        safe_printf("protoacc_dev completed %d; id %d, virtual_ts_us %lu\n", regs->completed_msg, current->dev, virtual_ts/1000);
      }
      reg_addr_reg = (uintptr_t)protoacc_regs_arr[current->dev];
      break;
    }
  }

  uintptr_t mmio_addr_reg = (uintptr_t)MMIO_ADDR_OF_X(current->type, current->dev, current->addr);
  if(current->size == 8){
    *((uint64_t*)mmio_addr_reg) = current->value;
    *((uint64_t*)(reg_addr_reg + current->addr)) = current->value;
  }else if(current->size == 4){
    *((uint32_t*)mmio_addr_reg) = current->value;
    *((uint32_t*)(reg_addr_reg + current->addr)) = current->value;
  }else if(current->size == 2){
    *((uint16_t*)mmio_addr_reg) = current->value;
    *((uint16_t*)(reg_addr_reg + current->addr)) = current->value;
  }else if(current->size == 1){
    *((uint8_t*)mmio_addr_reg) = current->value;
    *((uint8_t*)(reg_addr_reg + current->addr)) = current->value;
  }
}

// let's use FFFFF..FFF of the update to indicate a read
void* lazy_sync_accelerator_manager(pid_t pid){
  uint64_t virtual_ts = read_vts();
  detect_mmio_region_changes();
  // advance all simulators
  reg_updates_t* current = &reg_update_head;
  while(current->has_next){
    assert(current->next != NULL);
    current = current->next;
    if(current->rd_wr == WRITE_REG_TYPE){
      // advance in each handler
      handle_update(current, virtual_ts, pid);
    }else{
      simbricks_advance_rtl(NULL, virtual_ts, false);
      handle_fetch(current, virtual_ts, pid);
    }
  }
  return NULL;
}

static decoded_insn_info insn_info;
void hw_sigill_handler(int sig, siginfo_t *si, void *ucontext, pid_t pid) {
  lazy_sync_accelerator_manager(pid);
  ucontext_t *ctx = (ucontext_t *)ucontext;
  ctx->uc_mcontext.gregs[REG_RIP] += 2;

}



void handle_write(ucontext_t *ctx, decoded_insn_info* insn_info, uint64_t virtual_ts, pid_t pid) {
  DEBUG_MMIO("Handling mmio write\n");
  // uintptr_t addr = insn_info->mem_addr;
  // how about one process use multiple the same accelerator?
  // maybe require to set device first
  int type_of_accel = (insn_info->mem_addr - (uintptr_t)mmio_base) / 4096 / MAX_NUM_DEVICES_EACH;
  uint64_t offset = (insn_info->mem_addr - (uintptr_t)mmio_base) % 4096;
  reg_map_entry_t* reg_entry = CAPSTONE_TO_MCONTEXT(insn_info->reg);
  uint64_t write_value = 0;
  if (insn_info->imm_valid != 1){
    assert(MCSIZE(reg_entry) == insn_info->mem_size);
    write_value = ctx->uc_mcontext.gregs[MCREG(reg_entry)];
  }else{
    write_value = insn_info->imm;
  }
  DEBUG_MMIO("Type of accel %d, Write value: %lx\n", type_of_accel, write_value);
  
  uintptr_t addr = 0;
  int dev = ((insn_info->mem_addr - (uintptr_t)mmio_base) / 4096 - ((type_of_accel)*MAX_NUM_DEVICES_EACH));
  switch (type_of_accel) {
    case BPF_SCHED:
      addr = insn_info->mem_addr;
      break;
    case JPEG_DECODER:
      addr = (uintptr_t)jpeg_regs_arr[dev] + offset;
      break;
    case VTA:
      addr = (uintptr_t)vta_regs_arr[dev] + offset;
      break;
    case PROTOACC:
      addr = (uintptr_t)protoacc_regs_arr[dev] + offset;
      break;
   
    default:
      safe_printf("Unknown accelerator type\n");
      exit(EXIT_FAILURE);
  }

  if (insn_info->mem_size == 8) {
    *((long long *)addr) = write_value;
  } else if (insn_info->mem_size == 4) {
    *((int *)addr) = write_value;
  } else if (insn_info->mem_size == 2) {
    *((short *)addr) = write_value;
  } else if (insn_info->mem_size == 1) {
    *((char *)addr) = write_value;
  } else {
    DEBUG_MMIO("ERROR: unsupported size %d\n", MCSIZE(reg_entry));
    assert(0);
  }
  
  switch (type_of_accel) {
  
  case BPF_SCHED:{
      SchedRegs *mmio_regs = (SchedRegs *)(mmio_base);
      SchedRegs *regs = &sched_reg;
      regs->ctrl = mmio_regs->ctrl;
      uint32_t ctrl_pid = (regs->ctrl >> 32) & 0xFFFFFFFF;
      uint32_t ctrl_msg = regs->ctrl & 0xFFFFFFFF;
      if(ctrl_pid == 0){
        // not specified, default to the current one handling
        ctrl_pid = pid;
      }else{
        safe_printf("ctrl pid %d, current pid %d\n", ctrl_pid, pid);
        assert(pid == ctrl_pid);
      }
      safe_printf("bpf_sched_update_state pid: %d, msg: %d \n", ctrl_pid, ctrl_msg);
      bpf_sched_update_state_per_pid(ctrl_pid, ctrl_msg);
      // bpf_sched_update_state(regs->ctrl);
      // regs->ctrl = 0;
    break;
  }
  
  case JPEG_DECODER: {
    assert(dev < CONFIG_NUM_JPEG);

    // Perform write to registers
    JpegRegs *regs = jpeg_regs_arr[dev];
    SimbricksPciState* jpeg_dev = jpeg_array[dev];
    // Start decoding image
    if (regs->ctrl & CTRL_REG_START_BIT) {
      safe_printf("Starting JPEG decoding %d\n", dev);
      if(jpeg_dev->dma_fd == -1){
        jpeg_dev->dma_fd = open_pid_mem(pid);
        jpeg_dev->open_fd_pid = pid;
      }
      jpeg_dev->read_addr_base = regs->src;
      jpeg_dev->write_addr_base = regs->dst;
      if(jpeg_dev->mem_side_channel != NULL){
        jpeg_dev->mem_side_channel->pid = pid;
        jpeg_dev->mem_side_channel->dma_fd = jpeg_dev->dma_fd;
        jpeg_dev->mem_side_channel->read_addr_base = regs->src;
        jpeg_dev->mem_side_channel->write_addr_base = regs->dst;
      }

      simbricks_advance_rtl(jpeg_dev, virtual_ts, true);

      simbricks_mmio_write(jpeg_dev, 0, 0x8, 0, 4);
      simbricks_mmio_write(jpeg_dev, 0, 0xc, 0, 4);
      simbricks_mmio_write(jpeg_dev, 0, 0x0, regs->ctrl, 4);

      safe_printf("Starting JPEG decoding done %d\n", dev);
      regs->ctrl &= ~CTRL_REG_START_BIT;
      add_trace_event(type_of_accel, START_TYPE, virtual_ts);
    }else{
      add_trace_event(type_of_accel, WRITE_REG_TYPE, virtual_ts);
    }
    break;
  }
  case VTA:{
    assert(dev < CONFIG_NUM_VTA);

    VTARegs *regs = vta_regs_arr[dev];
    SimbricksPciState* vta_dev = vta_array[dev];
    DEBUG_MMIO ("VTA write %lx, %d\n", write_value, dev);
    if ((regs->_0x00 & 0x1) == 0x1) {
      add_trace_event(type_of_accel, START_TYPE, virtual_ts);
      if(vta_dev->dma_fd == -1){
        vta_dev->dma_fd = open_pid_mem(pid);
        vta_dev->open_fd_pid = pid;
      }
      vta_dev->read_addr_base = regs->_0x24;
      vta_dev->write_addr_base = regs->_0x24;
      if(vta_dev->mem_side_channel != NULL){
        vta_dev->mem_side_channel->pid = pid;
        vta_dev->mem_side_channel->dma_fd = vta_dev->dma_fd;
        vta_dev->mem_side_channel->read_addr_base = regs->_0x24;
        vta_dev->mem_side_channel->write_addr_base = regs->_0x24;
      }
      simbricks_advance_rtl(vta_dev, virtual_ts, true);

      safe_printf("vta_dev instruct count %u; id %d; pid %d\n", regs->_0x08, dev, pid);
      simbricks_mmio_write(vta_dev, 0, 0x08, regs->_0x08, 4);
      simbricks_mmio_write(vta_dev, 0, 0x10, regs->_0x10, 4);
      simbricks_mmio_write(vta_dev, 0, 0x0c, regs->_0x0c, 4);
      simbricks_mmio_write(vta_dev, 0, 0x00, regs->_0x00, 4);
      // force advance VTA timestamp
      // revert started states
      regs->_0x00 &= ~0x1;
    }else{
      add_trace_event(type_of_accel, WRITE_REG_TYPE, virtual_ts);
    }
    break;
  }

  case PROTOACC:{
    assert(dev < CONFIG_NUM_PROTOACC);
    ProtoAccRegs *regs = protoacc_regs_arr[dev];
    SimbricksPciState* protoacc_dev = protoacc_array[dev];
    if (((regs->ctrl & 0xFF) >> 7) == 1){
        simbricks_advance_rtl(protoacc_dev, virtual_ts, true);

        simbricks_mmio_write(protoacc_dev, 0, 0x28, regs->stringobj_output_addr & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x2c, (regs->stringobj_output_addr >> 32)& 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x30, regs->string_ptr_output_addr & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x34, (regs->string_ptr_output_addr >> 32) & 0xffffffff, 4);
        simbricks_mmio_write(protoacc_dev, 0, 0x0, regs->ctrl, 4);
        safe_printf("force updating protoacc timestamp, ctrl value %lx\n", regs->ctrl);
        regs->ctrl = 0;
    }
    
    if ((regs->ctrl & 0x1) == 1) {
      uint32_t max_fieldno = regs->min_max_fieldno & 0xFFFFFFFF;
      uint32_t min_fieldno = (regs->min_max_fieldno >> 32) & 0xFFFFFFFF;
      if(protoacc_dev->dma_fd == -1){
        protoacc_dev->dma_fd = open_pid_mem(pid);
        protoacc_dev->open_fd_pid = pid;
      }
      protoacc_dev->read_addr_base = 0;
      protoacc_dev->write_addr_base = 0;
      if(protoacc_dev->mem_side_channel != NULL){
        protoacc_dev->mem_side_channel->pid = pid;
        protoacc_dev->mem_side_channel->dma_fd = protoacc_dev->dma_fd;
        protoacc_dev->mem_side_channel->read_addr_base = 0;
        protoacc_dev->mem_side_channel->write_addr_base = 0;
      }
      simbricks_advance_rtl(protoacc_dev, virtual_ts, true);
      
      simbricks_mmio_write(protoacc_dev, 0, 0x8, min_fieldno, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0xc, max_fieldno, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x10, regs->descriptor_table_ptr & 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x14, (regs->descriptor_table_ptr >> 32)& 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x18, regs->src_base_addr & 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x1c, (regs->src_base_addr >> 32) & 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x20, regs->hasbits_offset & 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x24, (regs->hasbits_offset >> 32) & 0xffffffff, 4);
      simbricks_mmio_write(protoacc_dev, 0, 0x0, regs->ctrl, 4);

      regs->ctrl &= ~0x1;
    }

    break;
  }
 
  default:
    return;
    // safe_printf("Unknown accelerator type\n");
    // exit(EXIT_FAILURE);
  }
}

void handle_read(ucontext_t *ctx, decoded_insn_info *insn_info, uint64_t virtual_ts, pid_t pid) {
  DEBUG_MMIO("Handling mmio read\n");
  uintptr_t addr = insn_info->mem_addr;

  int type_of_accel = (insn_info->mem_addr - (uintptr_t)mmio_base) / 4096 / MAX_NUM_DEVICES_EACH;
  // int type_of_accel = (addr - (uintptr_t)mmio_base) / 4096;
  int64_t offset = (addr - (uintptr_t)mmio_base) % 4096;
  int dev = ((insn_info->mem_addr - (uintptr_t)mmio_base) / 4096 - ((type_of_accel)*MAX_NUM_DEVICES_EACH));

  if (type_of_accel == JPEG_DECODER) {
    assert(dev < CONFIG_NUM_JPEG);
    JpegRegs *regs = jpeg_regs_arr[dev];
    addr = (uintptr_t)regs + offset;
    SimbricksPciState* jpeg_dev = jpeg_array[dev];
    // DEBUG_MMIO("advance jpeg accel %ld \n", virtual_ts);
    safe_printf("advance jpeg accel %ld, dev %d \n", virtual_ts, dev);
    simbricks_advance_rtl(jpeg_dev, virtual_ts, false);
    uint64_t is_busy = simbricks_mmio_read(jpeg_dev, 0, 0x4, 4);
    safe_printf("advance jpeg accel %ld done; dev %d \n", virtual_ts, dev);
    if(is_busy == 0){
      regs->ctrl |= CTRL_REG_DONE_BIT;
    }
  }else if (type_of_accel == VTA){
    assert(dev < CONFIG_NUM_VTA);

    VTARegs *regs = vta_regs_arr[dev];
    addr = (uintptr_t)regs + offset;

    SimbricksPciState* vta_dev = vta_array[dev];
    
    simbricks_advance_rtl(vta_dev, virtual_ts, false);
    uint64_t finished = (simbricks_mmio_read(vta_dev, 0, 0x00, 4) & 0x2) == 0x2;
    if(finished){
      regs->_0x00 |= 0x2;
      regs->_0x04 = 10000;
    }

  }else if (type_of_accel == PROTOACC){
    assert(dev < CONFIG_NUM_PROTOACC);

    ProtoAccRegs *regs = protoacc_regs_arr[dev];
    SimbricksPciState* protoacc_dev = protoacc_array[dev];
    addr = (uintptr_t)regs + offset;
   
    simbricks_advance_rtl(protoacc_dev, virtual_ts, false);
    uint64_t completed_msg = simbricks_mmio_read(protoacc_dev, 0, 0x04, 4);
    regs->completed_msg = completed_msg;

  }

  mem_stats();
  // Perform the read or use mask to return the correct value
  reg_map_entry_t *reg_entry = CAPSTONE_TO_MCONTEXT(insn_info->reg);
  if (MCSIZE(reg_entry) == 8) {
    ctx->uc_mcontext.gregs[MCREG(reg_entry)] = *((long long *)addr);
  } else if (MCSIZE(reg_entry) == 4) {
    ctx->uc_mcontext.gregs[MCREG(reg_entry)] = *((int *)addr);
  } else if (MCSIZE(reg_entry) == 2) {
    ctx->uc_mcontext.gregs[MCREG(reg_entry)] = *((short *)addr);
  } else if (MCSIZE(reg_entry) == 1) {
    ctx->uc_mcontext.gregs[MCREG(reg_entry)] = *((char *)addr);
  } else {
    safe_printf("ERROR: unsupported size\n");
    assert(0);
  }
}

void hw_segfault_handler(int sig, siginfo_t *si, void *ucontext, pid_t pid) {

  uint64_t virtual_ts = read_vts();
  size_t mmio_size_protected = 4096;

  ucontext_t *ctx = (ucontext_t *)ucontext;
  uintptr_t addr = (uintptr_t)si->si_addr; // Address that causes the fault
  DEBUG_MMIO( "addr: %p mmio base %p\n", (void *)addr, (void *)mmio_base);
  addr = (addr & 0xFFF) + (uintptr_t)mmio_base; 
  if ((addr >= (uintptr_t)mmio_base && addr < ((uintptr_t)mmio_base + mmio_size_protected))) {
    memset(&insn_info, 0, sizeof(decoded_insn_info));
    decode(ctx, &insn_info, pid);
    DEBUG_MMIO("decode done \n");

    DEBUG_MMIO("mem addr: %p and addr %p \n", (void *)insn_info.mem_addr, (void *)addr);
    insn_info.mem_addr = addr;
    
    assert( insn_info.mem_addr == addr);
    if (insn_info.rw == 'R') {
      DEBUG_MMIO("Read from %p\n", (void *)insn_info.mem_addr);
      handle_read(ctx, &insn_info, virtual_ts, pid);
    } else if (insn_info.rw == 'W') {
      DEBUG_MMIO("Write to %p\n", (void *)insn_info.mem_addr);
      handle_write(ctx, &insn_info, virtual_ts, pid);
    } else {
      DEBUG_MMIO("ERROR: Unknown operation\n");
      assert(0);
    }
  
    // Get instruction size
    uint32_t insn_size = insn_info.size;
    // Increment RIP by instruction size
    ctx->uc_mcontext.gregs[REG_RIP] += insn_size;

    DEBUG_MMIO("Write new RIP %p\n", (void *)ctx->uc_mcontext.gregs[REG_RIP]);
    return;
  }else{
    safe_printf( "addr: %p mmio base %p\n", (void *)addr, (void *)mmio_base);
    // Not a MMIO access
    safe_printf("Not a MMIO access\n");
    assert(0);
  }
}

void hw_segfpe_handler(int sig, siginfo_t *si, void *ucontext, pid_t pid) {

  ucontext_t *ctx = (ucontext_t *)ucontext;
  memset(&insn_info, 0, sizeof(decoded_insn_info));
  decode(ctx, &insn_info, pid);
  uint32_t insn_size = insn_info.size;
  ctx->uc_mcontext.gregs[REG_RIP] += insn_size;
  safe_printf("Write new RIP %p\n", (void *)ctx->uc_mcontext.gregs[REG_RIP]);
  return;
}


void handle_hw_fault(pid_t child, int type) {
    struct user_regs_struct* regs = malloc(sizeof(struct user_regs_struct));
    ptrace(PTRACE_GETREGS, child, NULL, regs);

    // Get siginfo_t from the child
    siginfo_t siginfo;
    if (ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == -1) {
        perror("ptrace(PTRACE_GETSIGINFO)");
        safe_printf("ptrace(PTRACE_GETSIGINFO) failed\n");
        exit(EXIT_FAILURE);
    }

    // Build ucontext_t from the child's state
    ucontext_t* ucontext = malloc(sizeof(ucontext_t));
    memset(ucontext, 0, sizeof(ucontext_t));

    ucontext->uc_mcontext.gregs[REG_RIP] = regs->rip;
    ucontext->uc_mcontext.gregs[REG_EFL] = regs->eflags;
    ucontext->uc_mcontext.gregs[REG_RAX] = regs->rax;
    ucontext->uc_mcontext.gregs[REG_RBX] = regs->rbx;
    ucontext->uc_mcontext.gregs[REG_RCX] = regs->rcx;
    ucontext->uc_mcontext.gregs[REG_RDX] = regs->rdx;
    ucontext->uc_mcontext.gregs[REG_RSI] = regs->rsi;
    ucontext->uc_mcontext.gregs[REG_RDI] = regs->rdi;
    ucontext->uc_mcontext.gregs[REG_RBP] = regs->rbp;
    ucontext->uc_mcontext.gregs[REG_RSP] = regs->rsp;
    ucontext->uc_mcontext.gregs[REG_R8]  = regs->r8;
    ucontext->uc_mcontext.gregs[REG_R9]  = regs->r9;
    ucontext->uc_mcontext.gregs[REG_R10] = regs->r10;
    ucontext->uc_mcontext.gregs[REG_R11] = regs->r11;
    ucontext->uc_mcontext.gregs[REG_R12] = regs->r12;
    ucontext->uc_mcontext.gregs[REG_R13] = regs->r13;
    ucontext->uc_mcontext.gregs[REG_R14] = regs->r14;
    ucontext->uc_mcontext.gregs[REG_R15] = regs->r15;

    // Call the segfault handler in the tracer's context
    if (type == SIGFPE){
      hw_segfpe_handler(SIGFPE, &siginfo, ucontext, child);
    }else if(type == SIGSEGV)
      hw_segfault_handler(SIGSEGV, &siginfo, ucontext, child);
    else
      hw_sigill_handler(SIGILL, &siginfo, ucontext, child);

    // Update the child's registers if necessary
    regs->rip = ucontext->uc_mcontext.gregs[REG_RIP];
    regs->rax = ucontext->uc_mcontext.gregs[REG_RAX];
    regs->rbx = ucontext->uc_mcontext.gregs[REG_RBX];
    regs->rcx = ucontext->uc_mcontext.gregs[REG_RCX];
    regs->rdx = ucontext->uc_mcontext.gregs[REG_RDX];
    regs->rsi = ucontext->uc_mcontext.gregs[REG_RSI];
    regs->rdi = ucontext->uc_mcontext.gregs[REG_RDI];
    regs->rbp = ucontext->uc_mcontext.gregs[REG_RBP];
    regs->r8  = ucontext->uc_mcontext.gregs[REG_R8];
    regs->r9  = ucontext->uc_mcontext.gregs[REG_R9];
    regs->r10 = ucontext->uc_mcontext.gregs[REG_R10];
    regs->r11 = ucontext->uc_mcontext.gregs[REG_R11];
    regs->r12 = ucontext->uc_mcontext.gregs[REG_R12];
    regs->r13 = ucontext->uc_mcontext.gregs[REG_R13];
    regs->r14 = ucontext->uc_mcontext.gregs[REG_R14];
    regs->r15 = ucontext->uc_mcontext.gregs[REG_R15];

    ptrace(PTRACE_SETREGS, child, NULL, regs);
}


void handle_hw_free_resources(pid_t child){
  // return;
  for(int i = 0; i < CONFIG_NUM_JPEG; i++){
    if(jpeg_array[i]->open_fd_pid == child){
      close_pid_mem(jpeg_array[i]->dma_fd);
      jpeg_array[i]->dma_fd = -1;
    }
  }

  for(int i = 0; i < CONFIG_NUM_VTA; i++){
    if(vta_array[i]->open_fd_pid == child){
      close_pid_mem(vta_array[i]->dma_fd);
      vta_array[i]->dma_fd = -1;
    }
  }
  
  for(int i = 0; i < CONFIG_NUM_PROTOACC; i++){
    if(protoacc_array[i]->open_fd_pid == child){
      close_pid_mem(protoacc_array[i]->dma_fd);
      protoacc_array[i]->dma_fd = -1;
    }
  }
}

void redirect_output(char* simulator_name, int device_num){
  // create out if not exits
  char out_dir[200];
  sprintf(out_dir, "%s/out", CONFIG_PROJECT_PATH);
  mkdir(out_dir, 0777);
  
  char stdout_name[200];
  char stderr_name[200];
  char* exp_name = getenv("NEX_EXP_NAME");
  if(exp_name != NULL) {
    char dir_path[200];
    sprintf(dir_path, "%s/out/%s", CONFIG_PROJECT_PATH, exp_name);
    mkdir(dir_path, 0777);
    sprintf(stdout_name, "%s/out/%s/%s_%d.log", CONFIG_PROJECT_PATH, exp_name, simulator_name, device_num);
    sprintf(stderr_name, "%s/out/%s/%s_%d.err", CONFIG_PROJECT_PATH, exp_name, simulator_name, device_num);    

  }else{
    sprintf(stdout_name, "%s/out/%s_%d.log", CONFIG_PROJECT_PATH, simulator_name, device_num);
    sprintf(stderr_name, "%s/out/%s_%d.err", CONFIG_PROJECT_PATH, simulator_name, device_num);    
  }
 

  int fd = open(stdout_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
      perror("open");
      exit(EXIT_FAILURE);
  }

  // Redirect stdout to the file.
  if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2");
      close(fd);
      exit(EXIT_FAILURE);
  }
  close(fd);
  
  fd = open(stderr_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
      perror("open");
      exit(EXIT_FAILURE);
  }
  if(dup2(fd, STDERR_FILENO) < 0){
    perror("dup2");
    close(fd);
    exit(EXIT_FAILURE);
  }
  close(fd);
}