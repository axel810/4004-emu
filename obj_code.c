#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t program[] = {
		0xF3,
		0xD6,
		0xF6,
		0x0,

};

int main(int argc, char** argv) {
    char *outf = "prog.obj";
    FILE *f = fopen(outf, "wb");
    if (NULL==f) {
        printf("Cannot write to file %s\n", outf);
    }
    size_t writ = fwrite(program, sizeof(uint16_t), sizeof(program), f);
    printf("Written size_t=%lu to file %s\n", writ, outf);
    fclose(f);
    return 0;
}
