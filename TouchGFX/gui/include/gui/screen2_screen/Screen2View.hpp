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

    // Code tự viết: TouchGFX gọi mỗi tick để đọc queue joystick và nút xác nhận.
    virtual void handleTickEvent() override;

private:
    // Code tự viết: Chọn, tô sáng và xác nhận chế độ một/hai người chơi.
    void setSelectedMode(int mode);
    void updateSelectionVisual();
    void confirmSelection();

    int selectedMode;  // Chế độ đang được tô sáng: 1 hoặc 2 người.
    int inputCooldown; // Chống một lần giữ joystick làm đổi lựa chọn liên tục.
};

#endif // SCREEN2VIEW_HPP
