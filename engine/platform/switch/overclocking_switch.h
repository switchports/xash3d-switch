#ifndef OC_SWITCH_H
#define OC_SWITCH_H

typedef struct overclockingspeed_s
{
	const char* title;
    int clock;
    int id;
} overclockingspeed_t;

static overclockingspeed_t SWITCH_CPU_OVERCLOCKING_SPEEDS[] =
{
    { "1020 MHz (Stock)", 1020000000, 0 },
    { "1224 MHz", 1224000000, 1 },
    { "1530 MHz", 1530000000, 2 },
    { "1734 MHz", 1734000000, 3 },
    { "1912 MHz", 1912000000, 4 }
};

#define SWITCH_CPU_STOCK_CLOCK 1020000000

void Switch_OC_Init( void );

void Switch_OC_Update( void );

void Switch_OC_Shutdown( void );

#endif