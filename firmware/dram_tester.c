// #define DEBUG(x) x
#define DEBUG(x)                                                               \
    do {                                                                       \
    } while (0)

#include <xc.h>


#include "system.h"
#include "comlib.h"
#include "dram_tester.h"

#include <stdio.h>
#include <string.h>

#define OSCILLATOR_HZ _XTAL_FREQ

// Direction
zif_bits_t ezzif_zbd = {0};
// Output
zif_bits_t ezzif_zbo = {0};
zif_bits_t ezzif_zb_vpp = {0};
zif_bits_t ezzif_zb_vdd = {0};
zif_bits_t ezzif_zb_gnd = {0};
zif_bits_t ezzif_zb_read = {0};

const_zif_bits_t ezzif_zero = {0, 0, 0, 0, 0};

static int has_error = 0;

void dram_tester_reset(void)
{
    has_error = 0;

    // Disable pullup/pulldown
    pupd(1, 0);

    // tristate all pins
    memset(ezzif_zbd, 0xFF, sizeof(ezzif_zbd));
    dir_write(ezzif_zbd);

    memset(ezzif_zbo, 0, sizeof(ezzif_zbo));
    zif_write(ezzif_zbo);

    vpp_dis();
    vdd_dis();
    
    memset(ezzif_zb_gnd, 0, sizeof(ezzif_zb_gnd));
    set_gnd(ezzif_zb_gnd);
    
    // Make sure?
    for (unsigned i = 0; i < 8; ++i) {
        write_latch(i, 0x00);
    }
}


void timer_start(uint16_t timeout_microseconds)
{
	uint16_t timeout = OSCILLATOR_HZ / 4 /
	                             256 / timeout_microseconds;

	/* Timer sets TMR0IF at overflow, so TMR0 needs to be set to the
	 * number of ticks _before_ overflow. */
	timeout = 65535 - timeout + 1;

	INTCONbits.TMR0IF = 0; /* Turn off the interrupt flag */
	T0CONbits.TMR0ON = 0;  /* 0 = timer off */
	T0CONbits.T08BIT = 0;  /* 0 = 16-bit timer */
	T0CONbits.T0CS = 0;    /* clock select: 0 = Fosc/4 */
	T0CONbits.PSA = 0;     /* 0 = use prescaler */
	T0CONbits.T0PS= 7;     /* 7 = 1:256 prescaler */
	TMR0H = timeout >> 8 & 0xff;
	TMR0L = timeout & 0xff;
	T0CONbits.TMR0ON = 1;  /* timer on */
}

bool timer_expired(void)
{
	bool ret = (INTCONbits.TMR0IF != 0);
    if(ret) {
        timer_stop();
    }
    return ret;
}

void timer_stop(void)
{
	T0CONbits.TMR0ON = 0;
}