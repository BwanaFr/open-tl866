#include <xc.h>

#include "system.h"
#include "comlib.h"
#include "dram_tester.h"

#include <stdio.h>
#include <string.h>

#define OSCILLATOR_HZ _XTAL_FREQ
//Low level access, directly manipulate ports
//Port C: 
//RC5 : A8
//RC4 : Data in
//RC3 : /WE
//RC2 : /RAS
//RC6 : A1

//Port J:
//RJ7 : A0
//RJ6 : A2
//RJ2 : A7
//RJ3 : A5

//Port B:
//RB2 : A4
//RB3 : A3
//RB4 : A6
//RB5 : Dout
//RB6 : /CAS
#define LL_ACCESS 1

#ifdef LL_ACCESS
    #define CAS PORTBbits.RB6
    #define RAS PORTCbits.RC2
    #define WE  PORTCbits.RC3
    #define DIN PORTCbits.RC4
    #define DOUT PORTBbits.RB5
#endif 

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

/* Configure ZIF socket for 41256 and 4164 DRAM */
void dram_41_256_64_setup(void)
{
    //Disable pullup/pulldown
    pupd(1, 0);
    //Sets direction
    zif_bits_t dir = {  0b10000000,     // Pin 1 to 8 : A8, Din, /WRITE, /RAS, A0, A2, A1, VCC
                        0b11111111,     // Not used (ZIF 9-16)
                        0b11111111,     // Not used (ZIF 17-25)
                        0b11111111,     // Not used (ZIF 25-32)
                        0b10100000};    // Pin 33 to 40: A7, A5, A4, A3, A6, Dout, /CAS, Vss
    dir_write(dir);
    //Sets VDD
    zif_bits_t vdd = {  0b10000000, // Pin 1 to 8 : A8, Din, /WRITE, /RAS, A0, A2, A1, VCC
                    0b00000000,     // Not used (ZIF 9-16)
                    0b00000000,     // Not used (ZIF 17-25)
                    0b00000000,     // Not used (ZIF 25-32)
                    0b00000000};    // Pin 33 to 40: A7, A5, A4, A3, A6, Dout, /CAS, Vss
    set_vdd(vdd);
    //Sets GND
    zif_bits_t gnd = {  0b00000000, // Pin 1 to 8 : A8, Din, /WRITE, /RAS, A0, A2, A1, VCC
                    0b00000000,     // Not used (ZIF 9-16)
                    0b00000000,     // Not used (ZIF 17-25)
                    0b00000000,     // Not used (ZIF 25-32)
                    0b10000000};    // Pin 33 to 40: A7, A5, A4, A3, A6, Dout, /CAS, Vss
    set_gnd(gnd);
    
    // Set voltages
    vdd_val(VDD_51); // 5.0 v - 5.2 v
    vdd_en();
    vpp_dis();
    
    //Begin initialization of chip
    __delay_us(300);
    dram_41_256_64_cas(true);
    dram_41_256_64_ras(true);
    //Cycle /RAS eight times
    for(int i=0;i<8;++i){
        __delay_us(150);
        dram_41_256_64_ras(false);
        __delay_us(150);
        dram_41_256_64_ras(true);
    }

    //Configuration/initialization done    
}

void dram_41_256_64_address(uint16_t address)
{
#ifndef LL_ACCESS
    zif_bits_t val;
    zif_read(val);
    val[0] &= 0b00001110;   //Mask A8, A0, A2, A1 (and VCC) pins
    val[4] &= 0b01000000;   //Mask A7, A5, A3, A6 (and Vss) pins
    //Sets address bits
    //A8
    val[0] |= ((address >> 8) & 0x1);
    //A0 (bit 4 on ZIF)
    val[0] |= (((address >> 0) & 0x1)) << 4;
    //A2 (bit 5 on ZIF)
    val[0] |= (((address >> 2) & 0x1)) << 5;
    //A1 (bit 6 on ZIF)
    val[0] |= (((address >> 1) & 0x1)) << 6;
    //A7 (bit 0 on ZIF)
    val[4] |= (((address >> 7) & 0x1));
    //A5 (bit 1 on ZIF)
    val[4] |= (((address >> 5) & 0x1)) << 1;
    //A4 (bit 2 on ZIF)
    val[4] |= (((address >> 4) & 0x1)) << 2;
    //A3 (bit 3 on ZIF)
    val[4] |= (((address >> 3) & 0x1)) << 3;
    //A6 (bit 4 on ZIF)
    val[4] |= (((address >> 6) & 0x1)) << 4;

    //Write to ZIF
    zif_write(val);
#else
    //Low level access, directly manipulate ports
    //Port C: 
    //RC5 : A8
    //RC4 : Data in
    //RC3 : /WE
    //RC2 : /RAS
    //RC6 : A1

    //Port J:
    //RJ7 : A0
    //RJ6 : A2
    //RJ2 : A7
    //RJ3 : A5
    
    //Port B:
    //RB2 : A4
    //RB3 : A3
    //RB4 : A6
    //RB5 : Dout
    //RB6 : /CAS

    unsigned char portC = PORTC & 0b10011111;   //Mask RC6, RC5
    unsigned char portJ = PORTJ & 0b00110011;   //Mask RJ2, RJ3, RJ6, RJ7
    unsigned char portB = PORTB & 0b11100011;   //Mask RB2, RB3, RB4

    //A8
    portC |= ((address >> 8) & 0x1) << 5;
    //A1
    portC |= ((address >> 1) & 0x1) << 6;

    //A0
    portJ |= ((address >> 0) & 0x1) << 7;
    //A2
    portJ |= ((address >> 2) & 0x1) << 6;
    //A7
    portJ |= ((address >> 7) & 0x1) << 2;
    //A5
    portJ |= ((address >> 5) & 0x1) << 3;

    //A4
    portB |= ((address >> 4) & 0x1) << 2;
    //A3
    portB |= ((address >> 3) & 0x1) << 3;
    //A6
    portB |= ((address >> 6) & 0x1) << 4;

    //Write back to ports
    PORTC = portC;
    PORTJ = portJ;
    PORTB = portB;

#endif
}

void dram_41_256_64_cas(bool level)
{
#ifndef LL_ACCESS
    zif_bits_t val;
    zif_read(val);
    if(level){
        //Sets bit
        val[4] |= 0b01000000;
    }else{
        //Reset bit
        val[4] &= 0b10111111;
    }
    zif_write(val);
#else
    CAS = level;
#endif
}

void dram_41_256_64_ras(bool level)
{
#ifndef LL_ACCESS
    zif_bits_t val;
    zif_read(val);
    if(level){
        //Sets bit
        val[0] |= 0b00001000;
    }else{
        //Reset bit
        val[0] &= 0b11110111;
    }
    zif_write(val);
#else
    RAS = level;
#endif
}

void dram_41_256_64_we(bool level)
{
#ifndef LL_ACCESS
    zif_bits_t val;
    zif_read(val);
    if(level){
        //Sets bit
        val[0] |= 0b00000100;
    }else{
        //Reset bit
        val[0] &= 0b11111011;
    }
    zif_write(val);
#else
    WE = level;
#endif
}

void dram_41_256_64_out(bool dout)
{
#ifndef LL_ACCESS
    zif_bits_t val;
    zif_read(val);
    if(dout){
        //Sets bit
        val[0] |= 0b00000010;
    }else{
        //Reset bit
        val[0] &= 0b11111101;
    }
    zif_write(val);
#else
    DIN = dout;
#endif
}

bool dram_41_256_64_in(void)
{
#ifndef LL_ACCESS    
    zif_bits_t val;
    zif_read(val);
    return ((val[4] >> 5) & 0x1);   //Dout at ZIF 38
#else
    return DOUT;
#endif
}

void dram_41_256_64_early_write(uint32_t address, bool value, uint16_t addrLen)
{
    uint16_t row = (address >> addrLen) & ((1<<addrLen)-1);
    uint16_t col = address & ((1<<addrLen)-1);
    dram_41_256_64_address(row);
    dram_41_256_64_ras(0);
    dram_41_256_64_we(0);
    dram_41_256_64_address(col);
    dram_41_256_64_out(value);
    dram_41_256_64_cas(0);
    dram_41_256_64_cas(1);
    dram_41_256_64_ras(1);    
}

bool dram_41_256_64_read(uint32_t address, uint16_t addrLen)
{
    bool ret = false;
    uint16_t row = (address >> addrLen) & ((1<<addrLen)-1);
    uint16_t col = address & ((1<<addrLen)-1);
    dram_41_256_64_address(row);
    dram_41_256_64_ras(0);
    dram_41_256_64_we(1);
    dram_41_256_64_address(col);
    dram_41_256_64_cas(0);
    ret = dram_41_256_64_in();
    dram_41_256_64_cas(1);
    dram_41_256_64_ras(1);
    return ret;
}

void dram_41_256_64_ras_only_refresh(uint16_t row)
{
    dram_41_256_64_address(row);
    dram_41_256_64_ras(0);
    dram_41_256_64_ras(1);
}

void timer_start(uint16_t timeout_microseconds)
{
	uint16_t timeout = OSCILLATOR_HZ / 4 / 1000000
	                              * timeout_microseconds;

	/* Timer sets TMR0IF at overflow, so TMR0 needs to be set to the
	 * number of ticks _before_ overflow. */
	timeout = 65535 - timeout + 1;
    INTCONbits.TMR0IE = 0;
	INTCONbits.TMR0IF = 0; /* Turn off the interrupt flag */
	T0CONbits.TMR0ON = 0;  /* 0 = timer off */
	T0CONbits.T08BIT = 0;  /* 0 = 16-bit timer */
	T0CONbits.T0CS = 0;    /* clock select: 0 = Fosc/4 */
	T0CONbits.PSA = 1;     /* 1 = don't use prescaler */
	T0CONbits.T0PS= 0;     /* 0 = 1:2 prescaler */
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