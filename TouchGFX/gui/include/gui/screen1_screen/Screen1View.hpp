#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

// Code tự viết: Các widget và HAL cần cho HUD, ảnh game và chữ pixel tự vẽ.
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>
#include <stdio.h>

// Code tự viết: Font 3x5 tối giản để tự vẽ điểm số và thông báo game over.
class PixelFont
{
public:
    static uint16_t getGlyph(char c)
    {
        switch (c)
        {
        case 'A':
        case 'a':
            return 0x5BF7;
        case 'B':
        case 'b':
            return 0x65B6;
        case 'C':
        case 'c':
            return 0x7927;
        case 'D':
        case 'd':
            return 0x6B6E;
        case 'E':
        case 'e':
            return 0x79E7;
        case 'F':
        case 'f':
            return 0x48E7;
        case 'G':
        case 'g':
            return 0x7BE7;
        case 'H':
        case 'h':
            return 0x55EF;
        case 'I':
        case 'i':
            return 0x2492;
        case 'J':
        case 'j':
            return 0x7289;
        case 'K':
        case 'k':
            return 0x54CD;
        case 'L':
        case 'l':
            return 0x7084;
        case 'M':
        case 'm':
            return 0x55F5;
        case 'N':
        case 'n':
            return 0x55F7;
        case 'O':
        case 'o':
            return 0x7B6F;
        case 'P':
        case 'p':
            return 0x41EF;
        case 'Q':
        case 'q':
            return 0x7B6F;
        case 'R':
        case 'r':
            return 0x5DEF;
        case 'S':
        case 's':
            return 0x73E7;
        case 'T':
        case 't':
            return 0x2497;
        case 'U':
        case 'u':
            return 0x756D;
        case 'V':
        case 'v':
            return 0x256D;
        case 'W':
        case 'w':
            return 0x7B6D;
        case 'X':
        case 'x':
            return 0x54CD;
        case 'Y':
        case 'y':
            return 0x24AD;
        case 'Z':
        case 'z':
            return 0x724F;
        case ':':
            return 0x0410;
        case ' ':
            return 0;
        case '0':
            return 0x7B6F;
        case '1':
            return 0x74B2;
        case '2':
            return 0x79CF;
        case '3':
            return 0x73CF;
        case '4':
            return 0x13ED;
        case '5':
            return 0x73E7;
        case '6':
            return 0x7BE7;
        case '7':
            return 0x124F;
        case '8':
            return 0x7BEF;
        case '9':
            return 0x73EF;
        default:
            return 0;
        }
    }

    static void drawText(const touchgfx::Widget &widget, const touchgfx::Rect &invalidatedArea, const char *text, int startX, int startY, int pixelSize, touchgfx::colortype color)
    {
        int charW = 3;
        int charSpacing = 1 * pixelSize;
        int charTotalWidth = charW * pixelSize + charSpacing;

        for (int i = 0; text[i] != '\0'; i++)
        {
            char c = text[i];
            uint16_t glyph = getGlyph(c);
            if (glyph != 0 || c == ' ')
            {
                int cx = startX + i * charTotalWidth;
                int cy = startY;

                for (int row = 0; row < 5; row++)
                {
                    int rowBits = (glyph >> (row * 3)) & 0x7;
                    for (int col = 0; col < 3; col++)
                    {
                        if (rowBits & (1 << (2 - col)))
                        {
                            touchgfx::Rect pixelRect(cx + col * pixelSize, cy + row * pixelSize, pixelSize, pixelSize);
                            touchgfx::Rect drawRect = pixelRect & invalidatedArea;
                            if (!drawRect.isEmpty())
                            {
                                touchgfx::Rect tempRect = drawRect;
                                widget.translateRectToAbsolute(tempRect);
                                touchgfx::HAL::lcd().fillRect(tempRect, color, 255);
                            }
                        }
                    }
                }
            }
        }
    }
};

// Code tự viết: Widget HUD hiển thị và cập nhật điểm số trong lúc chơi.
class ScoreWidget : public touchgfx::Widget
{
public:
    ScoreWidget() : score(0)
    {
        setWidth(100);
        setHeight(12);
    }

    void setScore(int newScore)
    {
        score = newScore;
        invalidate();
    }

    virtual void draw(const touchgfx::Rect &invalidatedArea) const override
    {
        char buf[32];
        sprintf(buf, "SCORE:%d", score);
        touchgfx::colortype color = touchgfx::Color::getColorFromRGB(255, 255, 255);
        PixelFont::drawText(*this, invalidatedArea, buf, 0, 0, 2, color);
    }

    virtual touchgfx::Rect getSolidRect() const override
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    int score;
};

// Code tự viết: Widget phủ lên màn hình khi thua và hướng dẫn chơi lại.
class GameOverWidget : public touchgfx::Widget
{
public:
    GameOverWidget() : visible(false)
    {
        setPosition(40, 120, 160, 50);
    }

    void setVisibleState(bool v)
    {
        visible = v;
        invalidate();
    }

    virtual void draw(const touchgfx::Rect &invalidatedArea) const override
    {
        if (!visible)
            return;

        // Draw semi-transparent black background
        touchgfx::Rect bgRect(0, 0, getWidth(), getHeight());
        touchgfx::Rect drawBg = bgRect & invalidatedArea;
        if (!drawBg.isEmpty())
        {
            touchgfx::Rect tempBg = drawBg;
            translateRectToAbsolute(tempBg);
            touchgfx::HAL::lcd().fillRect(tempBg, touchgfx::Color::getColorFromRGB(0, 0, 0), 200);
        }

        // Draw GAME OVER in Red
        touchgfx::colortype redColor = touchgfx::Color::getColorFromRGB(255, 0, 0);
        PixelFont::drawText(*this, invalidatedArea, "GAME OVER", 44, 10, 2, redColor);

        // Draw PRESS BTN TO RESTART in White
        touchgfx::colortype whiteColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
        PixelFont::drawText(*this, invalidatedArea, "PRESS BTN TO RESTART", 8, 30, 1, whiteColor);
    }

    virtual touchgfx::Rect getSolidRect() const override
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    bool visible;
};

// Code tự viết: Widget phủ lên màn hình khi thắng và hiển thị điểm số.
class VictoryWidget : public touchgfx::Widget
{
public:
    VictoryWidget() : visible(false), score(0)
    {
        setPosition(40, 115, 160, 70); // Kích thước đủ lớn để hiển thị 3 dòng chữ
    }

    void setVisibleState(bool v, int s = 0)
    {
        visible = v;
        score = s;
        invalidate();
    }

    virtual void draw(const touchgfx::Rect &invalidatedArea) const override
    {
        if (!visible)
            return;

        // Vẽ nền đen mờ phía sau chữ Victory
        touchgfx::Rect bgRect(0, 0, getWidth(), getHeight());
        touchgfx::Rect drawBg = bgRect & invalidatedArea;
        if (!drawBg.isEmpty())
        {
            touchgfx::Rect tempBg = drawBg;
            translateRectToAbsolute(tempBg);
            touchgfx::HAL::lcd().fillRect(tempBg, touchgfx::Color::getColorFromRGB(0, 0, 0), 200);
        }

        // Vẽ dòng VICTORY màu xanh lá
        touchgfx::colortype greenColor = touchgfx::Color::getColorFromRGB(0, 255, 0);
        PixelFont::drawText(*this, invalidatedArea, "VICTORY", 52, 10, 2, greenColor);

        // Vẽ điểm số của người chơi
        char scoreBuf[32];
        sprintf(scoreBuf, "SCORE:%d", score);
        touchgfx::colortype whiteColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
        PixelFont::drawText(*this, invalidatedArea, scoreBuf, 44, 32, 1, whiteColor);

        // Vẽ hướng dẫn bấm nút để chơi lại
        PixelFont::drawText(*this, invalidatedArea, "PRESS BTN TO RESTART", 8, 50, 1, whiteColor);
    }

    virtual touchgfx::Rect getSolidRect() const override
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    bool visible;
    int score;
};

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // Code tự viết: TouchGFX gọi mỗi tick để đọc input và cập nhật game một người.
    virtual void handleTickEvent() override;

    // Code tự viết: Hỗ trợ điều khiển bằng joystick thật và bàn phím simulator.
    void handleJoystick(bool left, bool right, bool up, bool down, bool button);
    virtual void handleKeyEvent(uint8_t c);

protected:
    // Code tự viết: Kích thước các pool đối tượng dùng lại, tránh cấp phát động khi chơi.
    static const int ALIEN_ROWS = 5;
    static const int ALIEN_COLS = 7;
    static const int MAX_ALIEN_BULLETS = 8;
    static const int MAX_EXPLOSIONS = 8;

    struct GridPos
    {
        int r;
        int c;
    };

    // Code tự viết: Trạng thái đội hình alien.
    touchgfx::Image alienGrid[ALIEN_ROWS][ALIEN_COLS];
    bool alienActive[ALIEN_ROWS][ALIEN_COLS];

    // Code tự viết: Pool đạn alien.
    touchgfx::Image alienBullets[MAX_ALIEN_BULLETS];
    bool alienBulletActive[MAX_ALIEN_BULLETS];

    // Code tự viết: Pool hiệu ứng nổ nhiều frame.
    struct Explosion
    {
        touchgfx::Container container;
        touchgfx::Image image;
        int frame; // -1 if inactive, 0..2 if active
        int tickCounter;
    } explosions[MAX_EXPLOSIONS];

    // Code tự viết: Điểm, máu, cooldown và trạng thái kết thúc game.
    ScoreWidget scoreWidget;
    GameOverWidget gameOverWidget;
    VictoryWidget victoryWidget;
    touchgfx::Container healthContainer;
    int score;
    bool isGameOver;
    int alienShootCooldown;
    int playerHealth;

    int bulletCooldown;
    int alienDir;
    int alienMoveTick;

    // Code tự viết: Các hàm xử lý gameplay một người.
    void spawnBullet();
    void spawnAlienBullet();
    void spawnExplosion(int x, int y);
    void gameOver();
    void victory();
    void restartGame();
    void tickGame();
    void decreaseHealth();
    void checkBulletAlienCollision(touchgfx::Widget &bullet);
};

#endif // SCREEN1VIEW_HPP
