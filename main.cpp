#include <stdlib.h>
#include <stdio.h>

// Windows has no gettimeofday() (or sys/time.h for that matter).
#ifdef _MSC_VER
    #include "wintime.h"
#else
    #include <sys/time.h>
#endif

#include "types.h"
#include "GBC.h"
#include "disassembler.h"

void read_file(char *filename, u8 **src, size_t *size) {
    FILE *fp;
 
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to load file (\"%s\").\n", filename);
        #ifdef _MSC_VER
            __debugbreak();
        #endif
        exit(1);
    }

    // get the file size
    fseek(fp, 0L, SEEK_END);
    long allocsize = ftell(fp);
    rewind(fp);

    *src = (u8 *)malloc(allocsize * sizeof(u8));
    if (*src == NULL) {
        printf("Error while allocating memory for reading file.");
        fclose(fp);
        return;
    }
    *size = fread(*src, sizeof(u8), allocsize, fp);
    fclose(fp);
}

int main(void) {
    //char filename[] = "pkmn_blue.gb"; int rom_type = 0;
    char filename[] = "pkmn_silver.gbc"; int rom_type = 1;
    u8 *file;
    size_t filesize;
    read_file(filename, &file, &filesize);
    
    GBC emu(file, rom_type);
    emu.print_header_info();

    disassemble_bootblock(emu);
    
    printf("==========================\n");
    printf("=== Starting execution ===\n");
    printf("==========================\n\n");


    int ret = 0;
    int instr = 0;
    int seconds_to_emulate, cycles_to_emulate;
    seconds_to_emulate = 100;
    cycles_to_emulate = 4194304 * seconds_to_emulate;
    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);
    
    while (!ret && emu.cycles < cycles_to_emulate) {
        if (emu.pc > 0x6f && (emu.pc < 0x1f80 || emu.pc > 0x1f86) && (emu.pc < 0x36e2 || emu.pc > 0x36e7)) {
            //emu.print_regs();
            disassemble(emu);
        }
        ret = emu.do_instruction();
        instr++;
    }

    gettimeofday(&endtime, NULL);

    if (ret)
        disassemble(emu);

    emu.print_regs();

    int t_usec = endtime.tv_usec - starttime.tv_usec;
    int t_sec = endtime.tv_sec - starttime.tv_sec;
    double exectime = t_sec + (t_usec / 1000000.);

    printf("\nEmulated %f sec (%d instr) in %f sec WCT, %f%%\n", instr / 4194304., instr, exectime, seconds_to_emulate / exectime * 100);

    free(file);

    #ifdef WIN32
        while (1);
    #endif
    
    return 0;
}
