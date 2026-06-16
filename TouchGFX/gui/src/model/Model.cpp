#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#ifndef SIMULATOR
#include "main.h"
extern "C" {
    extern JoystickState g_joystick;
}
#endif

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool button = false;

#ifndef SIMULATOR
    left = g_joystick.left;
    right = g_joystick.right;
    up = g_joystick.up;
    down = g_joystick.down;
    button = g_joystick.button;
#endif

    if (modelListener != 0)
    {
        modelListener->joystickUpdated(left, right, up, down, button);
    }
}
