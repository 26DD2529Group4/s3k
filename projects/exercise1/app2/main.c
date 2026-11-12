#include "s3k.h"

#include <stdio.h>

// Main server loop for IPC (Inter-Process Communication)
// Receives a server endpoint as argument
int main(s3k_word_t server)
{
	printf("app2> starting...\n");
	s3k_msg_t msg = {};
	while (1) {  
		printf("app2> listening for messages...\n");
		int err = s3k_ipc_replyrecv(server, &msg);
		if (err < 0) {
			printf("app2> IPC call failed with error: %d\n", err);
			continue;
		}
		s3k_word_t code_addr = (s3k_word_t)msg.data[0];
		if (code_addr == 0) {
			printf("app2> received message NULL\n");
			msg.data[0] = 0xdeadbeef;
			continue;
		}
		printf("app2> overwriting at address: 0x%lx\n", code_addr);
		for (int i = 0; i < 1024; i++) {
			*((s3k_word_t *)code_addr + i) = 0x1337;
			printf("app2> overwritten %d blocks of code\n", i+1);
		}
	}
}
