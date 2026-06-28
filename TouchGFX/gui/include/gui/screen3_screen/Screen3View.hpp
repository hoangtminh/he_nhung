#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/Color.hpp>
#include <stdio.h>

class PixelFont3
{
public:
    static uint16_t getGlyph(char c)
    {
        switch (c)
        {
            case 'A': case 'a': return 0x5BF7;
            case 'B': case 'b': return 0x65B6;
            case 'C': case 'c': return 0x7927;
            case 'D': case 'd': return 0x6B6E;
            case 'E': case 'e': return 0x79E7;
            case 'F': case 'f': return 0x48E7;
            case 'G': case 'g': return 0x7BE7;
            case 'H': case 'h': return 0x55EF;
            case 'I': case 'i': return 0x2492;
            case 'J': case 'j': return 0x7289;
            case 'K': case 'k': return 0x54CD;
            case 'L': case 'l': return 0x7084;
            case 'M': case 'm': return 0x55F5;
            case 'N': case 'n': return 0x55F7;
            case 'O': case 'o': return 0x7B6F;
            case 'P': case 'p': return 0x41EF;
            case 'Q': case 'q': return 0x7B6F;
            case 'R': case 'r': return 0x5DEF;
            case 'S': case 's': return 0x73E7;
            case 'T': case 't': return 0x2497;
            case 'U': case 'u': return 0x756D;
            case 'V': case 'v': return 0x256D;
            case 'W': case 'w': return 0x7B6D;
            case 'X': case 'x': return 0x54CD;
            case 'Y': case 'y': return 0x24AD;
            case 'Z': case 'z': return 0x724F;
            case ':': return 0x0410;
            case ' ': return 0;
            case '0': return 0x7B6F;
            case '1': return 0x74B2;
            case '2': return 0x79CF;
            case '3': return 0x73CF;
            case '4': return 0x13ED;
            case '5': return 0x73E7;
            case '6': return 0x7BE7;
            case '7': return 0x124F;
            case '8': return 0x7BEF;
            case '9': return 0x73EF;
            default: return 0;
        }
    }

    static void drawText(const touchgfx::Widget& widget, const touchgfx::Rect& invalidatedArea, const char* text, int startX, int startY, int pixelSize, touchgfx::colortype color)
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

class ScoreWidget3 : public touchgfx::Widget
{
public:
    ScoreWidget3() : score(0)
    {
        setWidth(100);
        setHeight(12);
    }

    void setScore(int newScore)
    {
        score = newScore;
        invalidate();
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const override
    {
        char buf[32];
        sprintf(buf, "SCORE:%d", score);
        touchgfx::colortype color = touchgfx::Color::getColorFromRGB(255, 255, 255);
        PixelFont3::drawText(*this, invalidatedArea, buf, 0, 0, 2, color);
    }

    virtual touchgfx::Rect getSolidRect() const override
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    int score;
};

class GameOverWidget3 : public touchgfx::Widget
{
public:
    GameOverWidget3() : visible(false)
    {
        setPosition(40, 120, 160, 50);
    }

    void setVisibleState(bool v)
    {
        visible = v;
        invalidate();
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const override
    {
        if (!visible) return;

        // Vẽ nền đen mờ phía sau chữ Game Over.
        touchgfx::Rect bgRect(0, 0, getWidth(), getHeight());
        touchgfx::Rect drawBg = bgRect & invalidatedArea;
        if (!drawBg.isEmpty())
        {
            touchgfx::Rect tempBg = drawBg;
            translateRectToAbsolute(tempBg);
            touchgfx::HAL::lcd().fillRect(tempBg, touchgfx::Color::getColorFromRGB(0, 0, 0), 200);
        }

        // Vẽ dòng GAME OVER màu đỏ.
        touchgfx::colortype redColor = touchgfx::Color::getColorFromRGB(255, 0, 0);
        PixelFont3::drawText(*this, invalidatedArea, "GAME OVER", 44, 10, 2, redColor);

        // Vẽ hướng dẫn bấm nút để chơi lại.
        touchgfx::colortype whiteColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
        PixelFont3::drawText(*this, invalidatedArea, "PRESS BTN TO RESTART", 8, 30, 1, whiteColor);
    }

    virtual touchgfx::Rect getSolidRect() const override
    {
        return touchgfx::Rect(0, 0, 0, 0);
    }

private:
    bool visible;
};

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen() override;
    virtual void tearDownScreen() override;

    virtual void handleTickEvent() override;

    // Input bàn phím chỉ dùng khi chạy TouchGFX simulator.
    virtual void handleKeyEvent(uint8_t c) override;

protected:
    static const int ALIEN_ROWS = 5;
    static const int ALIEN_COLS = 7;
    static const int MAX_ALIEN_BULLETS = 8;
    static const int MAX_EXPLOSIONS = 8;

    struct GridPos { int r; int c; };

    // Trạng thái của đội hình alien.
    touchgfx::Image alienGrid[ALIEN_ROWS][ALIEN_COLS];
    bool alienActive[ALIEN_ROWS][ALIEN_COLS];
    
    // Pool đạn alien.
    touchgfx::Image alienBullets[MAX_ALIEN_BULLETS];
    bool alienBulletActive[MAX_ALIEN_BULLETS];

    // Pool hiệu ứng nổ.
    struct Explosion
    {
        touchgfx::Container container;
        touchgfx::Image image;
        int frame; // -1 là không hoạt động, 0..2 là frame animation đang chạy.
        int tickCounter;
    } explosions[MAX_EXPLOSIONS];

    // Điểm số, máu và trạng thái Game Over.
    ScoreWidget3 scoreWidget;
    GameOverWidget3 gameOverWidget;
    touchgfx::Container healthContainer;
    int score;
    bool isGameOver;
    int alienShootCooldown;
    int playerHealth;

    int bulletCooldown;
    int bullet2Cooldown;
    int alienDir;
    int alienMoveTick;

    touchgfx::Box ship2Bullet;

    // Các hàm xử lý chính của gameplay.
    void spawnBullet();
    void spawnShip2Bullet();
    void spawnAlienBullet();
    void spawnExplosion(int x, int y);
    void gameOver();
    void restartGame();
    void tickGame();
    void decreaseHealth();
    void checkBulletAlienCollision(touchgfx::Widget& bullet);
};

#endif // SCREEN3VIEW_HPP
