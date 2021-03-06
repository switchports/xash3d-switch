#include "common.h"

#if XASH_INPUT == INPUT_SWITCH
#include "input.h"
#include "joyinput.h"
#include "touch.h"
#include "client.h"
#include "in_switch.h"
#include <switch.h>

convar_t *switch_console;

typedef struct buttonmapping_s
{
	HidControllerKeys btn;
	int key;
} buttonmapping_t;

typedef struct touchfinger_s
{
	float x, y, dx, dy;
	qboolean down;
} touchfinger_t;

static buttonmapping_t btn_map[15] =
{
	{ KEY_MINUS, '~' },
	{ KEY_PLUS, K_ESCAPE },
	{ KEY_DUP, K_UPARROW },
	{ KEY_DRIGHT, K_MWHEELUP },
	{ KEY_DDOWN, K_DOWNARROW },
	{ KEY_DLEFT, K_MWHEELDOWN },
	{ KEY_ZL, K_MOUSE2 },
	{ KEY_ZR, K_MOUSE1 },
	{ KEY_L, K_CTRL },
	{ KEY_R, 'r' },
	{ KEY_LSTICK, K_SHIFT },
	{ KEY_A, K_ENTER },
	{ KEY_B, K_SPACE },
	{ KEY_X, 'f' },
	{ KEY_Y, 'e' },
};



static uint64_t btn_state;
static uint64_t old_btn_state;
static touchfinger_t touch_finger[10];

#define SWITCH_JOYSTICK_DEADZONE 1024

//https://github.com/fgsfdsfgs/vitaXash3D/blob/master/engine/platform/vita/in_vita.c
static void RescaleAnalog( int *x, int *y, int dead )
{
	//radial and scaled deadzone
	//http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html
	float analogX = (float)*x;
	float analogY = (float)*y;
	float deadZone = (float)dead;
	float maximum = (float)JOYSTICK_MAX;
	float magnitude = sqrtf( analogX * analogX + analogY * analogY );
	if( magnitude >= deadZone )
	{
		float scalingFactor = maximum / magnitude * ( magnitude - deadZone ) / ( maximum - deadZone );
		*x = (int)( analogX * scalingFactor );
		*y = (int)( analogY * scalingFactor );
	}
	else
	{
		*x = 0;
		*y = 0;
	}
}

void Switch_IN_Init( void )
{
	switch_console = Cvar_Get( "switch_console", "0", FCVAR_ARCHIVE, "enable switch console override" );
}

qboolean Switch_IN_ConsoleEnabled( void )
{
	return switch_console->value > 0;
}

void Switch_IN_HandleTouch( void )
{
	u32 touch_count = hidTouchCount();

	const size_t finger_count = sizeof(touch_finger) / sizeof(touch_finger[1]);

	qboolean touched_down_now[finger_count];

	if( touch_count > 0 ) {
		touchPosition touch;

		for( int i = 0; i < touch_count; i ++ ) {
			hidTouchRead(&touch, i);
			if(touch.id >= finger_count)
				continue;

			touchfinger_t *finger = &touch_finger[touch.id];

			finger->x = touch.px / scr_width->value;
			finger->y = touch.py / scr_height->value;
			finger->dx = touch.dx / scr_width->value;
			finger->dy = touch.dy / scr_height->value;

			touched_down_now[touch.id] = true;

			if (!touch_finger[touch.id].down) {
				IN_TouchEvent( event_down, touch.id, finger->x, finger->y, finger->dx, finger->dy );
				finger->down = true;
			} else {
				IN_TouchEvent( event_motion, touch.id, finger->x, finger->y, finger->dx, finger->dy );
			}
		}

		for( int i = 0; i < finger_count; i ++ ) {
			if(touch_finger[i].down && !touched_down_now[i]) {
				touchfinger_t *finger = &touch_finger[i];
				finger->down = false;
				IN_TouchEvent( event_up, i, finger->x, finger->y, finger->dx, finger->dy );
			}
		}
	} else {
		for( int i = 0; i < finger_count; i ++ ) {
			if(touch_finger[i].down) {
				touchfinger_t *finger = &touch_finger[i];
				finger->down = false;
				IN_TouchEvent( event_up, i, finger->x, finger->y, finger->dx, finger->dy );
			}
		}
	}
}

void Switch_IN_Frame( void )
{
    hidScanInput();

    btn_state = hidKeysHeld(CONTROLLER_P1_AUTO);

    for ( int i = 0; i < 15; ++i ) {
        if ((btn_state & btn_map[i].btn) != (old_btn_state & btn_map[i].btn)) {
            Key_Event( btn_map[i].key, !!( btn_state & btn_map[i].btn ) );
        }
    }

    old_btn_state = btn_state;

	JoystickPosition pos_left, pos_right;

	//Read the joysticks' position
	hidJoystickRead(&pos_left, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
	hidJoystickRead(&pos_right, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);

	if (abs( pos_left.dx ) < SWITCH_JOYSTICK_DEADZONE)
		pos_left.dx = 0;
	if (abs( pos_left.dy ) < SWITCH_JOYSTICK_DEADZONE)
		pos_left.dy = 0;

	Joy_AxisMotionEvent( 0, 0, pos_left.dx );
	Joy_AxisMotionEvent( 0, 1, -pos_left.dy );

	RescaleAnalog(&pos_right.dx, &pos_right.dy, SWITCH_JOYSTICK_DEADZONE);

	Joy_AxisMotionEvent( 0, 2, -pos_right.dx );
	Joy_AxisMotionEvent( 0, 3, pos_right.dy );

	Switch_IN_HandleTouch();
}

#endif