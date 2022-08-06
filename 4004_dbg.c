#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "4004_dbg.h"

char *to_binary(int num)
{
	return 0;
}

void print_cpu_state(uint16_t *stack, bool carry_flg, bool test_flg, uint8_t *regs)
{
	printf("*******************************************\n");
	printf(" level 1 %3d           R0 R1 %X %X   R8 R9 %X %X\n", stack[0], regs[0], regs[1], regs[8], regs[9]);
	printf(" level 2 %3d	       R2 R3 %X %X   RA RB %X %X\n", stack[1], regs[2], regs[3], regs[10], regs[11]);
	printf(" level 3 %3d           R4 R5 %X %X   RC RD %X %X\n", stack[2], regs[4], regs[5], regs[12], regs[13]);
	printf("         			   R6 R7 %X %X   RE RF %X %X\n", regs[6], regs[7], regs[14], regs[15]);
	printf("\n");
	printf(" accumulator: %X  carry: %d  test: %d\n", regs[16], carry_flg, test_flg);
	printf("\n");
}

void print_mem(int bank,  int chip, uint8_t *mem)
{
	for(int i = bank; i < bank + 1; i++)//0x8
	{
		for(int j = chip; j < chip + 1; j++)//0x4
		{
			for(int n = 0; n < 0x4; n++)
			{
				for(int m = 0; m < 0x10; m++)
					printf("%X ", mem[i * 256 + j * 64 + n * 16 + m]);
				printf("\n");
			}
			printf("\n");
		}
		printf("\n");
	}
}

void print_status()
{

}

void print_ram_outputs(uint8_t *ports, int port)
{
	printf("0x%X\n",ports[port]);
}

void print_rom_outputs(int port)
{
	printf("0x%X\n",port);
}


void print_mem_nozero(uint8_t *mem)
{

}
