#include "common.h"

#if XASH_INPUT == INPUT_SWITCH
#include "input.h"
#include "joyinput.h"
#include "touch.h"
#include "client.h"
#include "in_switch.h"
#include <switch.h>

typedef struct buttonmapping_s
{
	HidControllerKeys btn;
	int key;
} buttonmapping_t;

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

uint64_t btn_state;
uint64_t old_btn_state;
qboolean touch_down;

float x, y, dx, dy;

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

void Switch_IN_HandleTouch( void )
{
	touchPosition touch;

	u32 touch_count = hidTouchCount();

	if(touch_count > 0) {
		hidTouchRead(&touch, 0);
		x = touch.px / scr_width->value;
		y = touch.py / scr_height->value;
		dx = touch.dx / scr_width->value;
		dy = touch.dy / scr_height->value;

		if (!touch_down) {
			IN_TouchEvent( event_down, 0, x, y, dx, dy );
			touch_down = true;
		} else {
			IN_TouchEvent( event_motion, 0, x, y, dx, dy );
		}
	} else {
		if (touch_down) {
			IN_TouchEvent( event_up, 0, x, y, dx, dy );
			touch_down = false;
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

	Joy_AxisMotionEvent( 0, 2, pos_right.dx );
	Joy_AxisMotionEvent( 0, 3, -pos_right.dy );

	Switch_IN_HandleTouch();
}

#endif