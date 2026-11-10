#include "s3k.h"

#include <stdio.h>


extern char __uart_base[]; // UART base address

void mem_init(s3k_word_t mon_idx, s3k_word_t idx, s3k_word_t slot, s3k_word_t cfree, s3k_word_t perm, s3k_word_t base,
	      s3k_word_t size)
{
	idx = s3k_mon_mem_derive(mon_idx, idx, cfree, perm, base, size);
	if (idx < 0) {
		printf("Failed to derive memory capability %lx\n", base);
		return;
	} 

	s3k_word_t addr = s3k_pmp_napot_encode(base, size);
	int err = s3k_mon_mem_pmp_set(mon_idx, idx, slot, perm, addr);
	if (err < 0) {
		printf("Failed to set PMP for derived memory %lx, err=%d\n", base, err);
		return;
	}

}


static void dump_mon_memory_cap(int idx)
{
	s3k_cap_mem_t cap = {};
	s3k_pmp_slot_t slot = 0;
	s3k_mem_perm_t perm = 0;
	s3k_pmp_addr_t addr = 0;

	int rc = s3k_mon_mem_get(idx, &cap);
	printf("mem[%d] get rc=%d owner=%u cfree=%u csize=%u slot=%u perm=0x%x begin=0x%lx end=0x%lx\n",
	       idx, rc, (unsigned)cap.owner, (unsigned)cap.cfree, (unsigned)cap.csize, (unsigned)cap.slot,
	       (unsigned)cap.rwx, (unsigned long)cap.begin, (unsigned long)cap.end);

	rc = s3k_mon_mem_pmp_get(idx, &slot, &perm, &addr);
	printf("mem[%d] pmp rc=%d slot=%u perm=0x%x addr=0x%lx base=0x%lx size=0x%lx\n", idx, rc,
	       (unsigned)slot, (unsigned)perm, (unsigned long)addr,
	       (unsigned long)s3k_pmp_napot_decode_base(addr), (unsigned long)s3k_pmp_napot_decode_size(addr));
}


void app2_change_cap(void)
{
	int mon_idx = S3K_BOOT_MON_P2_IDX;   // Monitor index
	int ram_idx = S3K_BOOT_MEM_RAM_IDX;   // RAM index

	// revoke cap for code mem

	//prints everything to console
	dump_mon_memory_cap(mon_idx);
	

}



s3k_word_t app2_init(void)
{
	int mon_idx = S3K_BOOT_MON_P2_IDX;   // Monitor index
	int ram_idx = S3K_BOOT_MEM_RAM_IDX;   // RAM index
	int uart_idx = S3K_BOOT_MEM_UART_IDX; // UART index

	// ##### code memory configuration #####
	s3k_word_t b_code_base = APP2_RAM_ORIGIN;
	s3k_word_t b_code_size = APP2_RAM_LENGTH;
	s3k_word_t b_code_perm = S3K_MEM_PERM_RX; // Read/Execute permissions
	s3k_word_t b_code_slot = 1;
	s3k_word_t b_code_fuel = 1;
	
	// first create the manager in order to revoke the child capability later
	s3k_word_t manager_idx = s3k_mem_derive(ram_idx, 2, b_code_perm, b_code_base, b_code_size);
	if (manager_idx < 0) {
		printf("Failed to derive memory capability %lx\n", b_code_base);
		return NULL;
	}
	
	// probably not need since we are not using the capability to access the memory
	// s3k_word_t addr = s3k_pmp_napot_encode(base, size);
	// int err = s3k_mon_mem_pmp_set(mon_idx, idx, slot, perm, addr);
	// if (err < 0) {
	// 	printf("Failed to set PMP for derived memory %lx, err=%d\n", base, err);
	// 	return NULL;
	// }

	// then derive the child capability for the code memory
	mem_init(mon_idx, manager_idx, b_code_slot, b_code_fuel, b_code_perm, b_code_base, b_code_size);

	

	// ##### data memory configuration #####
	s3k_word_t b_stack_base = APP3_RAM_ORIGIN;
	s3k_word_t b_stack_size = 64; // 64 bytes for stack
	s3k_word_t b_stack_perm = S3K_MEM_PERM_RW; // Read/Write permissions
	s3k_word_t b_stack_fuel = 1;
	s3k_word_t b_stack_slot = 2;
	mem_init(mon_idx, ram_idx, b_stack_slot, b_stack_fuel, b_stack_perm, b_stack_base, b_stack_size);

	// ##### UART configuration #####
	s3k_word_t uart_base = (s3k_word_t)__uart_base;
	s3k_word_t uart_size = 0x20;
	s3k_word_t uart_perm = S3K_MEM_PERM_RW; // Read/Write permissions
	s3k_word_t uart_fuel = 1;
	s3k_word_t uart_slot = 3;
	mem_init(mon_idx, uart_idx, uart_slot, uart_fuel, uart_perm, uart_base, uart_size);

	if (s3k_mon_reg_set(mon_idx, S3K_REG_PC, b_code_base) != 0) {
		printf("Failed to set program counter to code start\n");
		return;
	}

	if (s3k_mon_reg_set(mon_idx, S3K_REG_SP, b_stack_base) != 0) {
		printf("Failed to set program stack pointer\n");
		return;
	}
	
	return manager_idx;
}
