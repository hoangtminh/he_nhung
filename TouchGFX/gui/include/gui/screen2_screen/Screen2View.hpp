#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void startGame();
    virtual void handleTickEvent() override;
    
    // Override TouchGFX Designer virtual functions
private:
    uint8_t selectedMode; // 0 for 1 Player, 1 for 2 Players
    uint32_t blinkCounter;
};

#endif // SCREEN2VIEW_HPP
