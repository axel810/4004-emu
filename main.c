#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

//TODO change all the 'i' in functions to 'byte'
//set START_MEM to 0 after reset

#define NOPS (46)
#define RAM 5120
//#define ROM 32
#define ROM 32768
#define FOUR_BIT_MAX 15

#define OPC(i) ((i)>>4)
#define SR(i) ((i)&0xF)
#define EXT(i) ((i) & 0xF)
#define TWOS_COM(i) (((~(i)) + 0x1) & 0xF)

bool running = true;
bool crry = false;
bool test = false;

typedef void (*op_ex_f)(uint8_t, ...);
typedef void (*ext_ex_f)();
typedef void (*io_ex_f)();

enum regs { R0 = 0, R1, R2, R3, R4, R5, R6, R7, R8, R9, RA, RB, RC, RE, RF, ACC, RCNT };

int sp = 0;
int bank[] = {0, 256, 512, 768, 1024, 1280, 1536, 1792 };//2048
/*int chip[] = {0, 128, 256, 384};
int mem_reg[] = {0, 32, 64, 96};*/
int chip[] = {0, 64, 128, 192}; //255
int mem_reg[] = {0, 16, 32, 48}; //64
int cur_bank = 0;
int cur_char = 0;
int cur_status = 0;
int cur_ram_iport = 0;
int cur_rom_ioport = 0;
uint16_t rpc;
uint16_t stack[3];
uint16_t MEM_START = 0x0;
uint8_t mem[ROM] = {0};
uint8_t ram[RAM] = {0};
uint8_t reg[RCNT] = {0};
uint8_t pars[] = { 0, 2, 4, 6, 8, 10, 12, 14};
uint8_t status_regs[512];
uint8_t ram_iport[32];
uint8_t rom_ioport[16];


uint16_t mr(uint8_t address) { return mem[address]; }
void mw(uint8_t address, uint16_t val) { mem[address] = val;   }

/*
4 bit opcode + 4 bit reg parametr instructions;
*/
void nop(uint8_t i, ...) { running = false; } //i is unused

void jin(uint8_t i, ...)
{
	uint8_t buf = (((i) >> 1) & 0x7);

	for(int bit = 4, j = 0x1; bit < 8; j <<= 1, bit++)
	{
		if(reg[pars[buf]] & j)
			rpc |= (1 << bit);
		else
			rpc &= ~(1 << bit);
	}

	for(int bit =  0, j = 0x1; bit < 4; j <<= 1, bit++)
	{
		if(reg[pars[buf] + 1] & j)
			rpc |= (1 << bit);
		else
			rpc &= ~(1 << bit);
	}
}

void fin(uint8_t i, ...)
{
	if(i & 0x1)
		jin(i);
	else
	{
		uint8_t buf = (((i) >> 1) & 0x7);
		uint16_t addr = ((rpc - 1) & 0xF00);
		//(((addr) >> 4) & 0xF) = reg[0];
		//((addr) & 0xF) = reg[1];
		reg[pars[buf]] = (((mr(addr)) >> 4) & 0xF);
		reg[pars[buf + 1]] = ((mr(addr)) & 0xF);
	}
}

void jms(uint8_t msb, uint8_t lsb)
{
	if(sp > 3)
		sp = 0;

	stack[sp++] = rpc;
	rpc = (((msb) & 0xF) << 8);

	for(int i = 1, j = 0; i < 0x100; i <<= 1, j++)
	{
		if(lsb & i)
			rpc |= (1 << j);
	}
}

void inc(uint8_t i, ...)
{
	if(reg[SR(i)] == FOUR_BIT_MAX)
		reg[SR(i)] = 0x0;
	else
		reg[SR(i)]++;
}

void isz(uint8_t i, ...) {}
void add(uint8_t i, ...)
{
	if((reg[ACC] += reg[SR(i)] + crry) > FOUR_BIT_MAX || crry)
	{
		 reg[ACC] &= ~(1 << 4);
		 crry = true;
	}
}

void sub(uint8_t i, ...)
{
	if((reg[ACC] += TWOS_COM(reg[SR(i)])) > FOUR_BIT_MAX)
		crry = true;
	else
		crry = false;

	reg[ACC] &= ~(1 << 4);
}

void ld(uint8_t i, ...) { reg[ACC] = reg[SR(i)]; }

void xch(uint8_t i, ...)
{
	uint8_t temp = reg[SR(i)];
	reg[SR(i)] = reg[ACC];
	reg[ACC] = temp;
}

void bbl(uint8_t i, ...)
{
	reg[ACC] = ((i) & 0xF);
	rpc = stack[sp - 1];
}

void ldm(uint8_t i, ...) { reg[ACC] = ((i) & 0xF); }

//child instuctions of ext (stands for extended) instructions
void clb() { reg[ACC] = 0; crry = false; }

void clc() { crry = false;  }

void iac()
{
	if(reg[ACC] == FOUR_BIT_MAX)
	{
		reg[ACC] = 0;
		crry = true;
	}
	else
		reg[ACC]++;
}

void cmc() { crry ^= 0x1; }

void cma() { reg[ACC] = ~reg[ACC]; }

void ral()
{
	bool temp;
	temp = crry;

	if(!crry)
		crry |= ((reg[ACC] >> 3) & 0x1);
	reg[ACC] <<= 1;
	reg[ACC] &= ~(1 << 4);
	reg[ACC] |=  temp;
}

void rar()
{
	bool temp;
	temp = crry;

	if(!crry)
		crry |= ((reg[ACC]) & 0x1);

	reg[ACC] >>= 1;
	reg[ACC] |= (temp << 3);
}

void tcc()
{
	if(crry)
		reg[ACC] = 0x1;
	else
		reg[ACC] = 0x0;

	crry = false;
}

void dac()
{
	if((reg[ACC] += TWOS_COM(1)) > FOUR_BIT_MAX)
		crry = true;
	else
		crry = false;

	reg[ACC] &= ~(1 << 4);
}

void tcs()
{
	if(!crry)
		reg[ACC] = 0x9;
	else
		reg[ACC] = 0xA;
}

void stc() { crry = true; }

void daa()
{
	if(reg[ACC] > 9 || crry)
	{
		reg[ACC] += 6;

		if(reg[ACC] > 15)
		{
			crry = true;
			reg[ACC] &= ~(1 << 4);
		}
	}
}

void kbp()
{
	int count = 0;

	uint8_t buf = 0;

	if(!reg[ACC])
		return;

	for(int i = 1, j = 1; i < 0x1F; i <<= 1, j++)
	{
		if(i & reg[ACC])
		{
			buf = j;
			count++;
		}
		if(count > 1)
		{
			reg[ACC] = 0xF;
			return;
		}
	}
	reg[ACC] = buf;
}

void dcl()
{
	cur_bank = bank[((reg[ACC]) & 0x7)];
	//cur_bank = ((reg[ACC]) & 0x7) * 256;
}

void src(uint8_t i, ...)
{
	uint8_t buf;
	uint8_t addr;
	addr = (((i) >> 1) & 0x7);
	buf = ((reg[pars[addr]]) << 4);
	buf |= reg[pars[addr] + 1];
	cur_char = cur_bank + chip[(((buf) >> 6) & 0x3)] + mem_reg[(((buf) >> 4) & 0x3)] + ((buf) & 0xF);
	cur_status = cur_bank + ((((buf) >> 6) & 0x3) * 4) + ((((buf) >> 4) & 0x3) * 4);
	cur_ram_iport = (((buf) >> 6) & 0x3);
	cur_rom_ioport = (((buf) >> 4) & 0xF);
}

//two bytes instructions
void fim(uint8_t msb, uint8_t lsb, ...)
{
	if(msb & 0x1)
		src(msb);
	else
	{
		uint8_t i;
		i = (((msb) >> 1) & 0x7);
		reg[pars[i]] = (((lsb) >> 4) & 0xF);
		reg[pars[i] + 1] = ((lsb) & 0xF);
	}
}

void jun(uint8_t msb, uint8_t lsb, ...)
{
	rpc = (((msb) & 0xF) << 8);
	for(int i = 1, j = 0; i < 0x100; i <<= 1, j++)//0x100
	{
		if(lsb & i)
			rpc |= (1 << j);
	}
}

void jcn(uint8_t msb, uint8_t lsb)
{
	for(int cnt = 0; cnt < 3; cnt++)
	{
		if (msb & (1 << cnt))
		{
			if(msb & 0x8)
			{
					if((cnt == 0 && test) || (cnt == 1 && !crry) || (cnt == 2 && reg[ACC]))
						goto new_addr;
			}
			else
			{
					if((cnt == 0 && !test) || (cnt == 1 && crry) || (cnt == 2 && !reg[ACC]))
						goto new_addr;
			}
		}
	}

	return;

new_addr:
	for(int i = 0x1, bit = 0; i < 0x100; i<<=1, bit++)
	{
		if(lsb & i)
			rpc |= (1 << bit);
		else
			rpc &= ~(1 << bit);
	}
}

void rdm() { reg[ACC] = ram[cur_char]; }

void rd0() { reg[ACC] = status_regs[cur_status]; }

void rd1() { reg[ACC] = status_regs[cur_status + 1]; }

void rd2() { reg[ACC] = status_regs[cur_status + 2]; }

void rd3() { reg[ACC] = status_regs[cur_status + 3]; }

void rdr() { reg[ACC] = rom_ioport[cur_rom_ioport]; }

void wrm() { ram[cur_char] = reg[ACC]; }

void wr0() { status_regs[cur_status] = reg[ACC]; }

void wr1() { status_regs[cur_status + 1] = reg[ACC]; }

void wr2() { status_regs[cur_status + 2] = reg[ACC]; }

void wr3() { status_regs[cur_status + 3] = reg[ACC]; }

void wmp() { ram_iport[cur_ram_iport] = reg[ACC]; }

void wrr() { rom_ioport[cur_rom_ioport] = reg[ACC]; }

void adm()
{
	if((reg[ACC] += ram[cur_char] + crry) > FOUR_BIT_MAX || crry)
	{
		 reg[ACC] &= ~(1 << 4);
		 crry = true;
	}
}

void sbm()
{
	if((reg[ACC] += TWOS_COM(ram[cur_char])) > FOUR_BIT_MAX)
		crry = true;
	else
		crry = false;

	reg[ACC] &= ~(1 << 4);
}

void wpm()
{

}

io_ex_f io_ex[] = { wrm, wmp, wrr, wpm, wr0, wr1, wr2, wr3, sbm, rdm, rdr };
void io(uint8_t i, ...) { io_ex[((i) & 0xF)](); }

ext_ex_f ext_ex[14] = {clb, clc, iac, cmc, cma, ral, rar, tcc, dac, tcs, stc, daa, kbp, dcl };
void ext(uint8_t i, ...)  { ext_ex[EXT(i)](); }
/*
uint8_t ops[NOPS] = {   0x0, 0x1, 0x2, 0x3, 0x3, 0x4, 0x5, 0x6, 0x7 0x8, 0x9, 0xA, 0xB, 0xC, 0xD}
op_ex_f op_ex[NOPS] = { nop, jcn, fim, fin, jin, jun, jms, inc, isz, add, sub, ld, xch, bbl, ldm };
*/

uint8_t ops[NOPS]    = 	 {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xF, 0xE };
op_ex_f op_ex[NOPS] = {nop, jcn, fim, jin, jun, jms, inc, add, sub, ld, xch, bbl, ldm, ext, io };

int find_op(int i)
{
	i >>= 4;
	for(int j = 0; j < NOPS; j++)
	{
		if(ops[j] == i)
			return j;
	}
	printf("messed up!\n");
	return -1;
}

void start(uint8_t offset)
{
	int next_byte = 0;
    rpc = MEM_START + offset;
    while(running)
    {
        uint8_t cur_byte = mr(rpc++);
        if((OPC(cur_byte) == 0x1) || ((OPC(cur_byte) == 0x2) && !(cur_byte & 0x1)) || (OPC(cur_byte) == 0x4) ||
        		(OPC(cur_byte) == 0x5) || (OPC(cur_byte) == 0x7))
        {
        	next_byte = mr(rpc++);
        	op_ex[find_op(cur_byte)](cur_byte, next_byte);
        }
        else
        {
        	op_ex[find_op(cur_byte)](cur_byte);
        }
    }
}

void ld_img(char *fname, uint16_t offset)
{
    FILE *in = fopen(fname, "rb");
    if (NULL==in)
    {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);    
    }
    uint8_t *p = mem + MEM_START + offset;
    fread(p, sizeof(uint16_t), (UINT16_MAX-MEM_START), in);
    fclose(in);
}

int main(int argc, char **argv)
{
	ld_img(argv[1], 0x0);
    start(0x0);  // START PROGRAM
    return 0;
}
