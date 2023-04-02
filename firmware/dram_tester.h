/*
    DRAM tester mode for testing vintage DRAM chips:
    - 41256 : 1x256k DRAM
    - 4164 : 1x64k DRAM

*/

#ifndef DRAM_TESTER_H
#define DRAM_TESTER_H

#include "io.h"
#include <stdint.h>

// High level API
// Tristate all outputs
// Disable all voltage rails
void dram_tester_reset(void);

/* Configure ZIF socket for 41256 and 4164 DRAM */
void dram_41_256_64_setup(void);

/* Sets address on bus for 41256 and 4164 devices */
void dram_41_256_64_address(uint16_t address);

/* Set the /CAS output for 41256 and 4164 devices */
void dram_41_256_64_cas(bool level);

/* Set the /RAS output for 41256 and 4164 devices */
void dram_41_256_64_ras(bool level);

/* Set the /WE output for 41256 and 4164 devices */
void dram_41_256_64_we(bool level);

/* Sets the Din pin for 41256 and 4164 devices */
void dram_41_256_64_out(bool dout);

/* Reads the Dout pin for 41256 and 4164 devices */
bool dram_41_256_64_in(void);

/* Performs an early write cycle */
void dram_41_256_64_early_write(uint32_t address, bool value, uint16_t addrLen);

/* Performs an read cycle */
bool dram_41_256_64_read(uint32_t address, uint16_t addrLen);

/* Performs /RAS-only refresh of specified row*/
void dram_41_256_64_ras_only_refresh(uint16_t row);

/* Starts a timer (0) */
void timer_start(uint16_t timeout_microseconds);

/* Return true if the timer has expired or false if it has not */
bool timer_expired(void);

/* Stop the timer from running */
void timer_stop(void);

#endif
