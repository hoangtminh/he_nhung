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
    virtual void handleTickEvent() override;
    
private:
    // Các hàm hỗ trợ chọn chế độ bằng joystick và nút bấm ngoài.
    void setSelectedMode(int mode);
    void updateSelectionVisual();
    void confirmSelection();

    int selectedMode;
    int inputCooldown;
};

#endif // SCREEN2VIEW_HPP
