#ifndef _4004_DBG_H_
#define _4004_DBG_H_

void print_cpu_state(uint16_t *stack, bool carry_flg, bool test_flg, uint8_t *regs);
void print_mem(int bank, int chip, uint8_t *mem);
void print_ram_outputs(uint8_t *ports, int port);
void print_rom_outputs(int port);

#endif /* 4004_DBG_H_ */
