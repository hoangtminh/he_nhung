#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{

}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::joystickUpdated(bool left, bool right, bool up, bool down, bool button)
{
    view.handleJoystick(left, right, up, down, button);
}

int Screen1Presenter::getNumPlayers() const
{
    return model->getNumPlayers();
}
