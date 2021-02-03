#ifndef _KERN_SOCKET_INTERNAL_NORMAN_
#define _KERN_SOCKET_INTERNAL_NORMAN_

/* TODO change these values */
#define MAX_KERN_SOCKS 7
uint64_t num_kern_socks = 0;

DEFINE_SPINLOCK(kern_sock_lock);

/* WARNING: mirrors struct in fd/pcie.h */
typedef struct pcie_block {
    uint32_t dsc_buf_tail;
    uint32_t dsc_buf_head;
    uint32_t dsc_buf_mem_low;
    uint32_t dsc_buf_mem_high;
    uint32_t pkt_buf_tail;
    uint32_t pkt_buf_head;
    uint32_t pkt_buf_mem_low;
    uint32_t pkt_buf_mem_high;
    uint32_t c2f_tail;
    uint32_t c2f_head;
    uint32_t c2f_kmem_low;
    uint32_t c2f_kmem_high;
    uint32_t padding[4];
} pcie_block_t;

/* WARNING: mirrors struct in fd/pcie.h */
typedef struct {
    uint64_t signal;         // 1 = busy, 0 = avail
    uint64_t pkt_queue_id;   // for retrieving pkt buf
    uint64_t tail;           // for pkt_buf
    uint64_t pad[5];         // padding
} pcie_pkt_desc_t;

/* kernel socket structure for bookkeeping */
typedef struct {
    /* socket in use? */
    uint32_t active;

    /* packet descriptors */
    pcie_pkt_desc_t *dsc_buf;
    uint32_t dsc_buf_head;

    /* packet contents */
    uint32_t *pkt_buf;
    uint32_t pkt_buf_head;

    /* app information*/
    int app_id;
    struct task_struct *app_thr; // assume app is single-thr
    pcie_block_t *uio_data_bar2;

    /* socket identifier */
    uint64_t sock_id;
} kern_sock_norman_t;

kern_sock_norman_t kern_socks[MAX_KERN_SOCKS];

int kern_create_socket(void);
int kern_verify_socket(void);

#endif
