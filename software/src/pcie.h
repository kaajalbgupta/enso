/*
 * Copyright (c) 2022, Carnegie Mellon University
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *      * Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SOFTWARE_SRC_PCIE_H_
#define SOFTWARE_SRC_PCIE_H_

#include <endian.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <norman/internals.h>

#include "syscall_api/intel_fpga_pcie_api.hpp"

namespace norman {

#define RULE_ID_LINE_LEN 64  // bytes
#define RULE_ID_SIZE 2       // bytes
#define NB_RULES_IN_LINE (RULE_ID_LINE_LEN / RULE_ID_SIZE)

#define MAX_PKT_SIZE 24  // in flits, if changed, must also change hardware

// 4 bytes, 1 dword
#define HEAD_OFFSET 4
#define C2F_TAIL_OFFSET 16

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

typedef struct {
  uint32_t* buf;
  uint64_t buf_phys_addr;
  queue_regs_t* regs;
  uint32_t* buf_head_ptr;
  uint32_t rx_head;
  uint32_t rx_tail;
  uint64_t phys_buf_offset;  // Use to convert between phys and virt address.
} pkt_queue_t;

typedef struct {
  dsc_queue_t* dsc_queue;
  intel_fpga_pcie_dev* dev;
  pkt_queue_t pkt_queue;
  int queue_id;
} socket_internal;

int dma_init(socket_internal* socket_entry, unsigned socket_id,
             unsigned nb_queues);

int get_next_batch_from_queue(socket_internal* socket_entry, void** buf,
                              size_t len);

int get_next_batch(dsc_queue_t* dsc_queue, socket_internal* socket_entries,
                   int* pkt_queue_id, void** buf, size_t len);

/*
 * Free the next `len` bytes in the packet buffer associated with the
 * `socket_entry` socket. If `len` is greater than the number of allocated bytes
 * in the buffer, the behavior is undefined.
 */
void advance_ring_buffer(socket_internal* socket_entry, size_t len);

/**
 * send_to_queue() - Send data through a given queue.
 * @dsc_queue: Descriptor queue to send data through.
 * @phys_addr: Physical memory address of the data to be sent.
 * @len: Length, in bytes, of the data.
 *
 * This function returns as soon as a transmission requests has been enqueued to
 * the TX dsc queue. That means that it is not safe to modify or deallocate the
 * buffer pointed by `phys_addr` right after it returns. Instead, the caller
 * must use `get_unreported_completions` to figure out when the transmission is
 * complete.
 *
 * This function currently blocks if there is not enough space in the descriptor
 * queue.
 *
 * Return: Return 0 if transfer was successful.
 */
int send_to_queue(dsc_queue_t* dsc_queue, void* phys_addr, size_t len);

/**
 * get_unreported_completions() - Return the number of transmission requests
 * that were completed since the last call to this function.
 * @dsc_queue: Descriptor queue to get completions from.
 *
 * Since transmissions are always completed in order, one can figure out which
 * transmissions were completed by keeping track of all the calls to
 * `send_to_queue`. There can be only up to `MAX_PENDING_TX_REQUESTS` requests
 * completed between two calls to `send_to_queue`. However, if `send` is called
 * multiple times, without calling `get_unreported_completions` the number of
 * completed requests can surpass `MAX_PENDING_TX_REQUESTS`.
 *
 * Return: Return the number of transmission requests that were completed since
 *         the last call to this function.
 */
uint32_t get_unreported_completions(dsc_queue_t* dsc_queue);

/**
 * update_tx_head() - Update the tx head and the number of TX completions.
 * @dsc_queue: Descriptor queue to be updated.
 */
void update_tx_head(dsc_queue_t* dsc_queue);

int dma_finish(socket_internal* socket_entry);

uint32_t get_pkt_queue_id_from_socket(socket_internal* socket_entry);

void print_queue_regs(queue_regs_t* pb);

void print_slot(uint32_t* rp_addr, uint32_t start, uint32_t range);

void print_fpga_reg(intel_fpga_pcie_dev* dev, unsigned nb_regs);

void print_ip(uint32_t ip);

void print_pkt_ips(uint8_t* pkt);

void print_pkt_header(uint8_t* pkt);

void print_buf(void* buf, const uint32_t nb_cache_lines);

void print_stats(socket_internal* socket_entry, bool print_global);

}  // namespace norman

#endif  // SOFTWARE_SRC_PCIE_H_
