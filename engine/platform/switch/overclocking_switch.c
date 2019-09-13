#include "common.h"
#include "overclocking_switch.h"
#include <switch.h>

convar_t *switch_overclock;

void Switch_OC_GetClockRate( u32 *out )
{
    if(hosversionBefore(8, 0, 0))
    {
        pcvGetClockRate(PcvModule_CpuBus, out);
    }
    else
    {
        ClkrstSession session = {0};
        clkrstOpenSession(&session, PcvModuleId_CpuBus, 3);
        clkrstGetClockRate(&session, out);
        clkrstCloseSession(&session);
    }
}

void Switch_OC_SetClockRate( u32 rate )
{
    if(hosversionBefore(8, 0, 0))
    {
        pcvSetClockRate(PcvModule_CpuBus, rate);
    }
    else
    {
        ClkrstSession session = {0};
        clkrstOpenSession(&session, PcvModuleId_CpuBus, 3);
        clkrstSetClockRate(&session, rate);
        clkrstCloseSession(&session);
    }
}

void Switch_OC_Init( void )
{
    pcvInitialize();
    switch_overclock = Cvar_Get( "switch_overclock", "0", FCVAR_ARCHIVE, "switch overclock" );
    Cmd_AddCommand( "switch_overclock_update", (xcommand_t)Switch_OC_Update, "Apply the current overclock");
}

void Switch_OC_Update( void )
{
    overclockingspeed_t overclock = SWITCH_CPU_OVERCLOCKING_SPEEDS[0];

    qboolean found;

    int overclock_id = switch_overclock->value;

    if(overclock_id == 0) {
        u32 clock_rate = 0;

        Switch_OC_GetClockRate(&clock_rate);

        if(clock_rate == SWITCH_CPU_STOCK_CLOCK)
            return;
    }

    const size_t cpu_overclock_count = sizeof(SWITCH_CPU_OVERCLOCKING_SPEEDS) / sizeof(SWITCH_CPU_OVERCLOCKING_SPEEDS[1]);

    for (int i = 0; i < cpu_overclock_count; i++)
    {
        if((int)SWITCH_CPU_OVERCLOCKING_SPEEDS[i].id == overclock_id) {
            overclock = SWITCH_CPU_OVERCLOCKING_SPEEDS[i];
            found = true;
            break;
        }
    }

    if(!found)
        return;

    Msg("Switch: Switching clock speed to %d\n", overclock.clock);

    Switch_OC_SetClockRate(overclock.clock);
}

void Switch_OC_Shutdown( void )
{
    Switch_OC_SetClockRate(SWITCH_CPU_STOCK_CLOCK);
    pcvExit();
}