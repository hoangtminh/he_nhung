#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v)
    : view(v)
{

}

void Screen3Presenter::activate()
{

}

void Screen3Presenter::deactivate()
{

}

void Screen3Presenter::joystickUpdated(bool left, bool right, bool up, bool down, bool button)
{
    view.handleJoystick(left, right, up, down, button);
}

int Screen3Presenter::getNumPlayers() const
{
    return model->getNumPlayers();
}
