#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Color.hpp>

#ifndef SIMULATOR
#include "main.h"
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Joystick1QueueHandle;
    extern osMessageQueueId_t Joystick2QueueHandle;
}
#endif

Screen2View::Screen2View()
    : selectedMode(0), inputCooldown(0)
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();
    setSelectedMode(2);
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::setSelectedMode(int mode)
{
    if (mode != 1 && mode != 2)
    {
        return;
    }

    if (selectedMode != mode)
    {
        selectedMode = mode;
        updateSelectionVisual();
    }
}

void Screen2View::updateSelectionVisual()
{
    // Màu vàng thể hiện chế độ đang được chọn bằng joystick, màu trắng là chế độ còn lại.
    const touchgfx::colortype selectedColor = touchgfx::Color::getColorFromRGB(255, 230, 0);
    const touchgfx::colortype normalColor = touchgfx::Color::getColorFromRGB(255, 255, 255);

    onePLayer.setLabelColor(selectedMode == 1 ? selectedColor : normalColor);
    onePLayer.setLabelColorPressed(selectedMode == 1 ? selectedColor : normalColor);
    twoPlayer.setLabelColor(selectedMode == 2 ? selectedColor : normalColor);
    twoPlayer.setLabelColorPressed(selectedMode == 2 ? selectedColor : normalColor);

    onePLayer.invalidate();
    twoPlayer.invalidate();
}

void Screen2View::confirmSelection()
{
    // Hai nút ngoài đều dùng để xác nhận chế độ hiện tại khi cảm ứng không còn hoạt động.
    if (selectedMode == 1)
    {
        application().gotoScreen1ScreenNoTransition();
    }
    else
    {
        application().gotoScreen3ScreenNoTransition();
    }
}

void Screen2View::handleTickEvent()
{
#ifndef SIMULATOR
    uint8_t cmd;
    bool selectOnePlayer = false;
    bool selectTwoPlayer = false;

    if (inputCooldown > 0)
    {
        inputCooldown--;
    }

    // Mỗi joystick có queue riêng nhưng cùng dùng mã L/R/U/D để chọn chế độ.
    osMessageQueueId_t queues[] = {Joystick1QueueHandle, Joystick2QueueHandle};
    for (int i = 0; i < 2; i++)
    {
        if (queues[i] == NULL)
        {
            continue;
        }

        while (osMessageQueueGet(queues[i], &cmd, NULL, 0) == osOK)
        {
            if (cmd == 'L' || cmd == 'U')
            {
                selectOnePlayer = true;
            }
            else if (cmd == 'R' || cmd == 'D')
            {
                selectTwoPlayer = true;
            }
        }
    }

    // Cooldown giúp giữ cần joystick lâu không làm lựa chọn nhảy liên tục từng frame.
    if (inputCooldown == 0)
    {
        if (selectOnePlayer)
        {
            setSelectedMode(1);
            inputCooldown = 10;
        }
        else if (selectTwoPlayer)
        {
            setSelectedMode(2);
            inputCooldown = 10;
        }
    }

    // PG3 và PG2 đều active-low vì đã cấu hình pull-up nội bộ ở main.c.
    const bool p1ButtonPressed = (HAL_GPIO_ReadPin(P1_BUTTON_GPIO_Port, P1_BUTTON_Pin) == GPIO_PIN_RESET);
    const bool p2ButtonPressed = (HAL_GPIO_ReadPin(P2_BUTTON_GPIO_Port, P2_BUTTON_Pin) == GPIO_PIN_RESET);

    if (p1ButtonPressed || p2ButtonPressed)
    {
        confirmSelection();
    }
#endif
}
