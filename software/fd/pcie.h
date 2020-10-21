

#ifndef PCIE_H
#define PCIE_H

#include "api/intel_fpga_pcie_api.hpp"

#define RULE_ID_LINE_LEN 64 // bytes
#define RULE_ID_SIZE 2 // bytes
#define NB_RULES_IN_LINE (RULE_ID_LINE_LEN/RULE_ID_SIZE)

#ifndef HEAD_UPDATE_PERIOD
// Number of packets to wait until we update the header pointer on the FPGA.
// We may override this value when compiling
#define HEAD_UPDATE_PERIOD 64
#endif

#ifndef BUFFER_SIZE
// This should be the max buffer supported by the hardware, we may override this
// value when compiling. It is defined in number of flits (64 bytes)
#define BUFFER_SIZE 65535
#endif
#define C2F_BUFFER_SIZE 512

// (BUFFER_SIZE + 1) should be page aligned (in flits)
#define PAGE_ALIGNED_BUFFER_SIZE ((BUFFER_SIZE / 64 + 1) * 64 - 1)

// In terms of dwords. 1 means the first 16 dwords are global registers
// the 1024 is the extra page for memory-copy
// if changed, should also change the kernel
#define C2F_BUFFER_OFFSET ((PAGE_ALIGNED_BUFFER_SIZE+1)*16 + 1024)

// 4 bytes, 1 dword
#define HEAD_OFFSET 4
#define C2F_TAIL_OFFSET 16

#define MAX_NB_APPS 16 // TODO(sadok) Increase this when we increase the number
                       // of supported apps

#define PDU_ID_OFFSET 0
#define PDU_PORTS_OFFSET 1
#define PDU_DST_IP_OFFSET 2
#define PDU_SRC_IP_OFFSET 3
#define PDU_PROTOCOL_OFFSET 4
#define PDU_SIZE_OFFSET 5
#define PDU_FLIT_OFFSET 6
#define ACTION_OFFSET 7
#define QUEUE_ID_LO_OFFSET 8
#define QUEUE_ID_HI_OFFSET 9

#define ACTION_NO_MATCH 1
#define ACTION_MATCH 2

#define MEMORY_SPACE_PER_APP (1<<12)

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define CONTROL_MSG

#ifdef CONTROL_MSG

#define REGISTERS_PER_APP 8
typedef struct pcie_block {
    uint32_t tail;
    uint32_t head;
    uint32_t kmem_low;
    uint32_t kmem_high;
    uint32_t c2f_tail;
    uint32_t c2f_head;
    uint32_t c2f_kmem_low;
    uint32_t c2f_kmem_high;
    uint32_t padding[8];
} pcie_block_t;

#else

typedef struct pcie_block {
    uint32_t tail;
    uint32_t head;
    uint32_t kmem_low;
    uint32_t kmem_high;
    uint32_t padding[12];
} pcie_block_t;

#endif // CONTROL_MSG

typedef struct block {
    uint32_t pdu_id;
    uint16_t dst_port;
    uint16_t src_port;
    uint32_t dst_ip;
    uint32_t src_ip;
    uint32_t protocol;
    uint32_t pdu_size; // in bytes
    uint32_t pdu_flit;
    uint32_t action;
    uint64_t queue_id;
    uint8_t *pdu_payload; // immediately after pdu_hdr
} block_s;

typedef struct {
    intel_fpga_pcie_dev* dev;
    uint32_t* kdata;
    pcie_block_t* uio_data_bar2;
    uint32_t last_flits;
    uint32_t* tail_ptr;
    uint32_t* head_ptr;
    uint32_t cpu_head;
    uint32_t c2f_cpu_tail;
    uint32_t nb_head_updates;
    int app_id;
} socket_internal;

int dma_init(socket_internal* socket_entry, unsigned socket_id, unsigned nb_queues);
int dma_run(socket_internal* socket_entry, void** buf, size_t len);
void advance_ring_buffer(socket_internal* socket_entry);
int send_control_message(socket_internal* socket_entry, unsigned int nb_rules);
int dma_finish(socket_internal* socket_entry);
void print_pcie_block(pcie_block_t * pb);
void print_slot(uint32_t *rp_addr, uint32_t start, uint32_t range);
void print_fpga_reg(intel_fpga_pcie_dev *dev);
void print_block(block_s *block);
void fill_block(uint32_t *addr, block_s *block);
uint32_t c2f_copy_head(uint32_t c2f_tail, pcie_block_t *global_block, 
         block_s *block, uint32_t *kdata);

#endif // PCIE_H
