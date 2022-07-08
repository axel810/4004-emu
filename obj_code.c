#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t program[] = {
		0xF0, //clb
		0xD0, //ldm 0
		0xFD, //dcl
		0x20,0x00, // fim 0p %0000 0000
		0x21, //src 0p
		0xD7, //ldm 7
		0xE2, //wrr
		0xD0, //ldm 4
		0xEA, //rdr
		0x0, //nop
};

int main(int argc, char** argv) {
    char *outf = "prog.obj";
    FILE *f = fopen(outf, "wb");
    if (NULL==f) {
        fprintf(stderr, "Cannot write to file %s\n", outf);
    }
    size_t writ = fwrite(program, sizeof(uint16_t), sizeof(program), f);
    fprintf(stdout, "Written size_t=%lu to file %s\n", writ, outf);
    fclose(f);
    return 0;
}
