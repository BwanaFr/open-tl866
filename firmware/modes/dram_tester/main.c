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

static void display_address_error(uint32_t startAddr, uint32_t endAddr){
    if(startAddr == endAddr){
        printf("Error at address 0x%lx\n", startAddr);
    }else{
        printf("Error at address 0x%lx-0x%lx (%lu bytes)\n", startAddr, endAddr, (endAddr+1-startAddr));
    }
}

static bool test_41256_refresh()
{
    const uint16_t rows = (1<<9);
    const uint32_t mem_size = (1ul<<18);
    const uint16_t refreshTime = 150;   //150us, like in IBM PC. Well in fact we are slower (~190us)
    const uint32_t retentionTime = 5;   //Retention time in seconds
    uint32_t address = 0;
    uint16_t refreshedRow = 0;
    uint32_t failedStartAddress = 0;
    bool testFailed = false;
    bool prevFailed = false;
    printf("Refresh test - Writing 1\n");

    uint16_t timeout = _XTAL_FREQ / 4 / 1000000
	                              * refreshTime;


    timer_start(refreshTime);
    for(address=0;address < mem_size; address++){
        dram_41_256_early_write(address, 1);
        if(timer_expired()){
            //Time to refresh
            dram_41_256_64_ras_only_refresh(++refreshedRow);
            timer_start(refreshTime);
            if(refreshedRow>=rows){
                refreshedRow = 0;
            }
        }
    }

    printf("Refresh test - Only refresh\n");
    for(uint32_t i=0;i<((retentionTime*1000*1000)/refreshTime);){
        if(timer_expired()){
            //Time to refresh
            dram_41_256_64_ras_only_refresh(++refreshedRow);
            timer_start(refreshTime);
            if(refreshedRow>=rows){
                refreshedRow = 0;
            }
            ++i;
        }
    }

    printf("Refresh test - Checking 1\n");
    for(address=0;address < mem_size; address++){
        if(dram_41_256_read(address) != 1){
            if(!prevFailed){
                failedStartAddress = address;
            }
            prevFailed = true;
            testFailed = true;
        }else{
            if(prevFailed){
                display_address_error(failedStartAddress, (address-1));
            }
            prevFailed = false;
        }
        if(timer_expired()){
            dram_41_256_64_ras_only_refresh(++refreshedRow);
            timer_start(refreshTime);
            if(refreshedRow>=rows){
                refreshedRow = 0;
            }
        }
    }
    if(prevFailed){
        display_address_error(failedStartAddress, (address-1));
    }
    return testFailed;
}


static bool test_41256_value(bool value)
{
    uint32_t address = 0;
    uint32_t failedStartAddress = 0;
    bool testFailed = false;
    bool prevFailed = false;
    const uint32_t mem_size = (1ul<<18);
    printf("Checking %us\n", value);

    for(address=0;address < mem_size; address++){
        dram_41_256_early_write(address, value);
        if(dram_41_256_read(address) != value){
            if(!prevFailed){
                failedStartAddress = address;
            }
            prevFailed = true;
            testFailed = true;
        }else{
            if(prevFailed){
                display_address_error(failedStartAddress, (address-1));
            }
            prevFailed = false;
        }
    }
    return testFailed;
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
    testFailed = test_41256_value(0);
    testFailed |= test_41256_value(1);
    testFailed |= test_41256_refresh();
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
