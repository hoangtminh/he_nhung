#include <gui/screen2_screen/Screen2View.hpp>

#ifndef SIMULATOR
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Queue1Handle;
}
#endif

Screen2View::Screen2View()
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
#ifndef SIMULATOR
    uint8_t cmd;
    if (osMessageQueueGet(Queue1Handle, &cmd, NULL, 0) == osOK)
    {
        if (cmd == 'R')
        {
        }
    }
#endif
}
