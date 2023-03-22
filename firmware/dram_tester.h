/*
    DRAM tester mode for testing vintage DRAM chips:
    - 41256 : 1x256k DRAM

*/

#ifndef DRAM_TESTER_H
#define DRAM_TESTER_H

#include "io.h"
#include <stdint.h>

// High level API
// Tristate all outputs
// Disable all voltage rails
void dram_tester_reset(void);


/* Starts a timer (0) */
void timer_start(uint16_t timeout_milliseconds);

/* Return true if the timer has expired or false if it has not */
bool timer_expired(void);

/* Stop the timer from running */
void timer_stop(void);

#endif
