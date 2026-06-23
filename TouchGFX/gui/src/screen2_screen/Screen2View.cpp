#include <gui/screen2_screen/Screen2View.hpp>

#ifndef SIMULATOR
#include "main.h"
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Queue1Handle;
}
#endif

Screen2View::Screen2View()
    : selectedMode(0), blinkCounter(0)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}
void Screen2View::startGame()
{
    // Bạn có thể để trống tạm thời, hoặc thêm logic bắt đầu game vào đây.
    // Việc có cặp ngoặc nhọn {} là bắt buộc để trình biên dịch biết hàm này đã được định nghĩa.
}
void Screen2View::handleTickEvent()
{
    bool selectionButtonPressed = false;

#ifndef SIMULATOR
    uint8_t cmd;
    // Đọc tất cả các lệnh từ hàng đợi joystick để chuyển đổi lựa chọn
    while (osMessageQueueGet(Queue1Handle, &cmd, NULL, 0) == osOK)
    {
        if (cmd == 'U' || cmd == 'L')
        {
            selectedMode = 0; // Chọn 1 Player
        }
        else if (cmd == 'D' || cmd == 'R')
        {
            selectedMode = 1; // Chọn 2 Players
        }
    }

    // Đọc trạng thái chân PA0 (Nút nhấn người dùng - Active High)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
        selectionButtonPressed = true;
    }
#endif

    // Hiệu ứng nhấp nháy cho button đang được chọn
    blinkCounter++;
    bool isVisible = (blinkCounter % 30) < 15;

    if (selectedMode == 0)
    {
        onePLayer.setVisible(isVisible);
        twoPlayer.setVisible(true);
    }
    else
    {
        onePLayer.setVisible(true);
        twoPlayer.setVisible(isVisible);
    }
    onePLayer.invalidate();
    twoPlayer.invalidate();

    // Chuyển màn hình khi xác nhận bằng nút PA0
    if (selectionButtonPressed)
    {
        if (selectedMode == 0)
        {
            application().gotoScreen1ScreenNoTransition();
        }
        else
        {
            application().gotoScreen3ScreenNoTransition();
        }
    }
}
