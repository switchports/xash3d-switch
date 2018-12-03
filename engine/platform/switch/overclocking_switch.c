#include "common.h"
#include "overclocking_switch.h"
#include <switch.h>

convar_t *switch_overclock;

void Switch_OC_Init( void )
{
    switch_overclock = Cvar_Get( "switch_overclock", "0", FCVAR_ARCHIVE, "switch overclock" );
    Cmd_AddCommand( "switch_overclock_update", (xcommand_t)Switch_OC_Update, "Apply the current overclock");
}

void Switch_OC_Update( void )
{
    overclockingspeed_t overclock;

    qboolean found;

    int overclock_id = switch_overclock->value;
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

    pcvSetClockRate(PcvModule_Cpu, overclock.clock);
}

void Switch_OC_Free( void )
{
    pcvSetClockRate(PcvModule_Cpu, SWITCH_CPU_STOCK_CLOCK);
}