// DRAM tester

#include "../../system.h"
#include "../../comlib.h"
#include "../../mode.h"
#include "../../stock_compat.h"

#include "dram_tester.h"

int main_debug = 0;

static inline void print_help(void)
{
    com_println("open-tl866 (dram_tester)");
    com_println("0      41256 (1x256k) test");
    com_println("b      reset to bootloader");
}

static void prompt_msg(const char *msg)
{
    com_println(msg);
    com_readline();
}

static void prompt_enter(void)
{
    prompt_msg("Press enter to continue");
}

/**
    Test of 41256 (1x256k) DRAM
    DIP chip pinout:
    CHIP    ZIF     FUNC            DIR (TL866 side)
    PIN     PIN
    1       1       A8              OUT
    2       2       D (Data in)     OUT
    3       3       /W              OUT
    4       4       /RAS            OUT
    5       5       A0              OUT
    6       6       A2              OUT
    7       7       A1              OUT
    8       8       VDD             VDD (5V)
    9       33      A7              OUT
    10      34      A5              OUT
    11      35      A4              OUT
    12      36      A3              OUT
    13      37      A6              OUT
    14      38      Q (Data out)    IN
    15      39      /CAS            OUT
    16      40      VSS             VSS (GND)

**/
static void test_41256(void)
{
    bool testFailed = false;
    com_println("41256 DRAM test");
    com_println("Make sure to insert proper chip");
    prompt_enter();
    //Chip is inserted, setup ZIF and initialize chip
    dram_41_256_64_setup();
    uint32_t address = 0;
    const uint32_t mem_size = (1ul<<18);
    printf("Checking 0s\n");
    for(address=0;address < mem_size; address++){
        dram_41_256_early_write(address, 0);
        if(dram_41_256_read(address) != 0){
            printf("Error at address 0x%x (expecting 0)\n", address);
            testFailed = true;
        }
    }

    printf("Checking 1s\n");
    for(address=0;address < mem_size; address++){
        dram_41_256_early_write(address, 1);
        if(dram_41_256_read(address) != 1){
            printf("Error at address 0x%x (expecting 1)\n", address);
            testFailed = true;
        }
    }

    dram_tester_reset();
    if(testFailed){
        printf("Error detected!\n");
    }else{
        printf("Test succeded!\n");
    }
    prompt_enter();
}


static inline void eval_command(unsigned char *cmd)
{
    unsigned char *cmd_t = strtok(cmd, " ");

    if (cmd_t == NULL) {
        return;
    }

    dram_tester_reset();
    switch (cmd_t[0]) {
    case '0':
        test_41256();
        break;

    case '?':
    case 'h':
        print_help();
        break;

    case 'b':
        stock_reset_to_bootloader();
        break;

    default:
        printf("ERROR: unknown command 0x%02X (%c)\r\n", cmd_t[0], cmd_t[0]);
        break;
    }
    dram_tester_reset();
}

void mode_main(void)
{
    dram_tester_reset();

    while (1) {
        eval_command(com_cmd_prompt());
    }
}

void interrupt high_priority isr()
{
    usb_service();
}
