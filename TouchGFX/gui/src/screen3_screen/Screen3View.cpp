#include <gui/screen3_screen/Screen3View.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>

#ifndef SIMULATOR
#include "main.h"
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Joystick1QueueHandle;
    extern osMessageQueueId_t Joystick2QueueHandle;
}
#endif

namespace
{
// Giữ hai người chơi ở vùng dưới đội hình alien nhưng vẫn cho di chuyển 2 chiều.
static const int PLAYER_MIN_Y = 160;
static const int PLAYER_MOVE_STEP = 10;
static const int PLAYER_SIM_STEP = 3;

template <typename Ship>
void moveShip2D(Ship& ship, bool left, bool right, bool up, bool down, int step)
{
    int x = ship.getX();
    int y = ship.getY();

    if (left) x -= step;
    if (right) x += step;
    if (up) y -= step;
    if (down) y += step;

    // Giới hạn tàu không đi ra ngoài màn hình và không vượt lên vùng alien.
    if (x < 0) x = 0;
    if (x > 240 - ship.getWidth()) x = 240 - ship.getWidth();
    if (y < PLAYER_MIN_Y) y = PLAYER_MIN_Y;
    if (y > 320 - ship.getHeight()) y = 320 - ship.getHeight();

    if ((x != ship.getX()) || (y != ship.getY()))
    {
        ship.moveTo(x, y);
        ship.invalidate();
    }
}
}

Screen3View::Screen3View()
    : score(0), isGameOver(false), alienShootCooldown(45), bulletCooldown(0), bullet2Cooldown(0), alienDir(1), alienMoveTick(0), playerHealth(5)
{
    // Khởi tạo trạng thái ban đầu cho alien, đạn alien và hiệu ứng nổ.
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            alienActive[r][c] = false;
        }
    }

    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBulletActive[i] = false;
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosions[i].frame = -1;
        explosions[i].tickCounter = 0;
    }
}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();

    // Ẩn alien mẫu từ Designer vì game sẽ tự nhân bản thành đội hình nhiều alien.
    alien.setVisible(false);
    alien.invalidate();

    // Lấy bitmap của alien mẫu để gán cho từng alien trong lưới.
    touchgfx::Bitmap alienBmp = alien.getBitmap();

    // Tạo đội hình alien dạng lưới.
    int startX = 0;
    int startY = 10;
    int spacingX = 4;
    int spacingY = 4;

    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            alienGrid[r][c].setBitmap(alienBmp);
            alienGrid[r][c].setXY(startX + c * (alienGrid[r][c].getWidth() + spacingX),
                                  startY + r * (alienGrid[r][c].getHeight() + spacingY));
            alienGrid[r][c].setVisible(true);
            add(alienGrid[r][c]);
            alienActive[r][c] = true;
        }
    }

    // Đạn của người chơi 1 là widget có sẵn trong Designer.
    shipBullet.setVisible(false);
    shipBullet.invalidate();

    // Tạo pool đạn alien để tái sử dụng, tránh cấp phát động khi đang chơi.
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBullets[i].setBitmap(touchgfx::Bitmap(BITMAP_ALIENBULLET_ID));
        alienBullets[i].setVisible(false);
        add(alienBullets[i]);
        alienBulletActive[i] = false;
    }

    // Tạo pool hiệu ứng nổ từ sprite DeadAlien.
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosions[i].container.setPosition(0, 0, 40, 30);
        explosions[i].image.setBitmap(touchgfx::Bitmap(BITMAP_DEADALIEN_ID));
        explosions[i].image.setXY(0, 0);
        explosions[i].container.add(explosions[i].image);
        explosions[i].container.setVisible(false);
        add(explosions[i].container);
        explosions[i].frame = -1;
        explosions[i].tickCounter = 0;
    }

    // Hiển thị điểm ở góc trên bên phải.
    scoreWidget.setPosition(160, 10, 72, 12);
    scoreWidget.setScore(0);
    add(scoreWidget);

    // Đưa thanh máu lên góc trên trái và dùng container để cắt frame hiển thị.
    remove(health);
    healthContainer.setPosition(10, 10, 32, 16);
    healthContainer.add(health);
    add(healthContainer);
    health.setXY(0, 0);
    health.setVisible(true);
    health.invalidate();
    playerHealth = 5;

    // Thêm lớp Game Over ở giữa màn hình, ban đầu ẩn.
    gameOverWidget.setPosition(40, 125, 160, 50);
    gameOverWidget.setVisibleState(false);
    add(gameOverWidget);

    // Đặt hai tàu ở vị trí khác nhau để không chồng lên nhau khi bắt đầu.
    ship1.moveTo(50, 270);
    ship1.setVisible(true);
    ship1.invalidate();

    ship2.moveTo(160, 240);
    ship2.setVisible(true);
    ship2.invalidate();

    // Đạn của người chơi 2 được tạo bằng Box vì Designer chỉ có sẵn shipBullet.
    ship2Bullet.setPosition(160 + (ship2.getWidth() - 3) / 2, 240 - 10, 3, 10);
    ship2Bullet.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    ship2Bullet.setVisible(false);
    add(ship2Bullet);
    ship2Bullet.invalidate();

    isGameOver = false;
}

void Screen3View::tearDownScreen()
{
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::spawnBullet()
{
    if (!shipBullet.isVisible())
    {
        // Mỗi người chơi chỉ có một viên đạn đang bay; bắn lại khi đạn cũ biến mất.
        int bx = ship1.getX() + (ship1.getWidth() - shipBullet.getWidth()) / 2;
        int by = ship1.getY() - shipBullet.getHeight();
        
        shipBullet.moveTo(bx, by);
        shipBullet.setVisible(true);
        shipBullet.invalidate();
    }
}

void Screen3View::spawnShip2Bullet()
{
    if (!ship2Bullet.isVisible())
    {
        // Người chơi 2 dùng viên đạn tạo bằng code, độc lập với shipBullet của người chơi 1.
        int bx = ship2.getX() + (ship2.getWidth() - ship2Bullet.getWidth()) / 2;
        int by = ship2.getY() - ship2Bullet.getHeight();
        
        ship2Bullet.moveTo(bx, by);
        ship2Bullet.setVisible(true);
        ship2Bullet.invalidate();
    }
}

void Screen3View::spawnAlienBullet()
{
    // Chọn ngẫu nhiên một alien còn sống rồi gán vào slot đạn alien đang rảnh.
    int activeCount = 0;
    GridPos activeAliens[ALIEN_ROWS * ALIEN_COLS];
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            if (alienActive[r][c])
            {
                activeAliens[activeCount++] = {r, c};
            }
        }
    }

    if (activeCount > 0)
    {
        // Bộ sinh số giả ngẫu nhiên đơn giản để chọn alien bắn.
        static unsigned int seed = 12345;
        seed = seed * 1103515245 + 12345;
        int idx = (seed / 65536) % activeCount;
        int r = activeAliens[idx].r;
        int c = activeAliens[idx].c;

        // Tạo đạn alien tại vị trí alien được chọn.
        for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
        {
            if (!alienBulletActive[i])
            {
                int bx = alienGrid[r][c].getX() + (alienGrid[r][c].getWidth() - alienBullets[i].getWidth()) / 2;
                int by = alienGrid[r][c].getY() + alienGrid[r][c].getHeight();

                alienBullets[i].moveTo(bx, by);
                alienBullets[i].setVisible(true);
                alienBullets[i].invalidate();
                alienBulletActive[i] = true;
                break;
            }
        }
    }
}

void Screen3View::spawnExplosion(int x, int y)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (explosions[i].frame == -1)
        {
            int ex = x + (39 - 40) / 2;
            int ey = y + (27 - 30) / 2;
            
            explosions[i].container.setPosition(ex, ey, 40, 30);
            explosions[i].image.setXY(0, 0);
            explosions[i].container.setVisible(true);
            explosions[i].container.invalidate();
            explosions[i].frame = 0;
            explosions[i].tickCounter = 0;
            break;
        }
    }
}

void Screen3View::gameOver()
{
    // Dừng game bằng cách ẩn toàn bộ vật thể động, chỉ giữ lớp Game Over để chờ chơi lại.
    isGameOver = true;
    ship1.setVisible(false);
    ship1.invalidate();
    ship2.setVisible(false);
    ship2.invalidate();

    shipBullet.setVisible(false);
    shipBullet.invalidate();
    ship2Bullet.setVisible(false);
    ship2Bullet.invalidate();

    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBullets[i].setVisible(false);
        alienBullets[i].invalidate();
        alienBulletActive[i] = false;
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosions[i].container.setVisible(false);
        explosions[i].container.invalidate();
        explosions[i].frame = -1;
    }

    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            alienActive[r][c] = false;
            alienGrid[r][c].setVisible(false);
            alienGrid[r][c].invalidate();
        }
    }

    gameOverWidget.setVisibleState(true);
}

void Screen3View::restartGame()
{
    // Khôi phục toàn bộ object runtime vì màn chơi được tái sử dụng sau Game Over.
    isGameOver = false;
    score = 0;
    scoreWidget.setScore(0);
    
    ship1.moveTo(50, 270);
    ship1.setVisible(true);
    ship1.invalidate();

    ship2.moveTo(160, 240);
    ship2.setVisible(true);
    ship2.invalidate();

    ship2Bullet.setVisible(false);
    ship2Bullet.invalidate();

    // Đưa toàn bộ alien về vị trí ban đầu.
    int startX = 0;
    int startY = 10;
    int spacingX = 4;
    int spacingY = 4;

    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            alienGrid[r][c].setXY(startX + c * (alienGrid[r][c].getWidth() + spacingX),
                                  startY + r * (alienGrid[r][c].getHeight() + spacingY));
            alienGrid[r][c].setVisible(true);
            alienGrid[r][c].invalidate();
            alienActive[r][c] = true;
        }
    }

    // Tắt đạn của người chơi 1.
    shipBullet.setVisible(false);
    shipBullet.invalidate();

    // Tắt toàn bộ đạn alien.
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBullets[i].setVisible(false);
        alienBullets[i].invalidate();
        alienBulletActive[i] = false;
    }

    // Tắt toàn bộ hiệu ứng nổ.
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosions[i].container.setVisible(false);
        explosions[i].container.invalidate();
        explosions[i].frame = -1;
    }

    bulletCooldown = 0;
    bullet2Cooldown = 0;
    alienDir = 1;
    alienMoveTick = 0;
    alienShootCooldown = 45;
    playerHealth = 5;
    health.setXY(0, 0);
    health.invalidate();
    gameOverWidget.setVisibleState(false);
}

void Screen3View::checkBulletAlienCollision(touchgfx::Widget& bullet)
{
    // Hàm va chạm dùng chung cho đạn của cả hai người chơi.
    touchgfx::Rect bulletRect = bullet.getRect();
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            if (alienActive[r][c])
            {
                touchgfx::Rect alienRect = alienGrid[r][c].getRect();
                touchgfx::Rect alienHitbox(alienRect.x + 1, alienRect.y + 1, alienRect.width - 2, alienRect.height - 2);
                if (bulletRect.intersect(alienHitbox))
                {
                    alienActive[r][c] = false;
                    alienGrid[r][c].setVisible(false);
                    alienGrid[r][c].invalidate();

                    bullet.setVisible(false);
                    bullet.invalidate();

                    spawnExplosion(alienGrid[r][c].getX(), alienGrid[r][c].getY());

                    score += 10;
                    scoreWidget.setScore(score);
                    return;
                }
            }
        }
    }
}

void Screen3View::tickGame()
{
    if (isGameOver)
    {
        return;
    }

    // 1. Cho đạn người chơi bay thẳng lên và tái sử dụng khi ra khỏi màn hình.
    if (shipBullet.isVisible())
    {
        int by = shipBullet.getY() - 5;
        if (by + shipBullet.getHeight() < 0)
        {
            shipBullet.setVisible(false);
            shipBullet.invalidate();
        }
        else
        {
            shipBullet.moveTo(shipBullet.getX(), by);
            shipBullet.invalidate();
        }
    }
    if (ship2Bullet.isVisible())
    {
        int by = ship2Bullet.getY() - 5;
        if (by + ship2Bullet.getHeight() < 0)
        {
            ship2Bullet.setVisible(false);
            ship2Bullet.invalidate();
        }
        else
        {
            ship2Bullet.moveTo(ship2Bullet.getX(), by);
            ship2Bullet.invalidate();
        }
    }

    // 2. Cho đạn alien bay xuống; trúng tàu nào cũng trừ thanh máu chung.
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        if (alienBulletActive[i])
        {
            int by = alienBullets[i].getY() + 2; // Tốc độ đạn alien: 2 pixel mỗi frame.
            if (by > 320)
            {
                alienBullets[i].setVisible(false);
                alienBullets[i].invalidate();
                alienBulletActive[i] = false;
            }
            else
            {
                alienBullets[i].moveTo(alienBullets[i].getX(), by);
                alienBullets[i].invalidate();

                // Kiểm tra va chạm với tàu người chơi 1.
                touchgfx::Rect shipRect = ship1.getRect();
                touchgfx::Rect shipHitbox(shipRect.x + 2, shipRect.y + 2, shipRect.width - 4, shipRect.height - 4);
                if (alienBullets[i].getRect().intersect(shipHitbox))
                {
                    alienBullets[i].setVisible(false);
                    alienBullets[i].invalidate();
                    alienBulletActive[i] = false;
                    decreaseHealth();
                }
                // Kiểm tra va chạm với tàu người chơi 2.
                else
                {
                    touchgfx::Rect ship2Rect = ship2.getRect();
                    touchgfx::Rect ship2Hitbox(ship2Rect.x + 2, ship2Rect.y + 2, ship2Rect.width - 4, ship2Rect.height - 4);
                    if (alienBullets[i].getRect().intersect(ship2Hitbox))
                    {
                        alienBullets[i].setVisible(false);
                        alienBullets[i].invalidate();
                        alienBulletActive[i] = false;
                        decreaseHealth();
                    }
                }
            }
        }
    }

    // 3. Cập nhật animation nổ từ sprite sheet DeadAlien.
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (explosions[i].frame != -1)
        {
            explosions[i].tickCounter++;
            if (explosions[i].tickCounter >= 6)
            {
                explosions[i].tickCounter = 0;
                explosions[i].frame++;
                if (explosions[i].frame > 2)
                {
                    explosions[i].container.setVisible(false);
                    explosions[i].container.invalidate();
                    explosions[i].frame = -1;
                }
                else
                {
                    explosions[i].image.setXY(0, -explosions[i].frame * 30);
                    explosions[i].image.invalidate();
                }
            }
        }
    }

    // 4. Định kỳ cho một alien còn sống bắn đạn.
    alienShootCooldown--;
    if (alienShootCooldown <= 0)
    {
        alienShootCooldown = 45;
        spawnAlienBullet();
    }

    // 5. Di chuyển đội hình alien ngang; chạm mép thì đảo chiều và hạ xuống.
    alienMoveTick++;
    if (alienMoveTick >= 60)
    {
        alienMoveTick = 0;
        bool hitEdge = false;
        int step = 10;
        int shiftDown = 10;

        for (int r = 0; r < ALIEN_ROWS; r++)
        {
            for (int c = 0; c < ALIEN_COLS; c++)
            {
                if (alienActive[r][c])
                {
                    int nextX = alienGrid[r][c].getX() + alienDir * step;
                    if (alienDir == 1 && (nextX + alienGrid[r][c].getWidth() > 240))
                    {
                        hitEdge = true;
                        break;
                    }
                    if (alienDir == -1 && (nextX < 0))
                    {
                        hitEdge = true;
                        break;
                    }
                }
            }
            if (hitEdge)
            {
                break;
            }
        }

        if (hitEdge)
        {
            alienDir = -alienDir;
            for (int r = 0; r < ALIEN_ROWS; r++)
            {
                for (int c = 0; c < ALIEN_COLS; c++)
                {
                    if (alienActive[r][c])
                    {
                        alienGrid[r][c].moveTo(alienGrid[r][c].getX(), alienGrid[r][c].getY() + shiftDown);
                        alienGrid[r][c].invalidate();
                    }
                }
            }
        }
        else
        {
            for (int r = 0; r < ALIEN_ROWS; r++)
            {
                for (int c = 0; c < ALIEN_COLS; c++)
                {
                    if (alienActive[r][c])
                    {
                        alienGrid[r][c].moveTo(alienGrid[r][c].getX() + alienDir * step, alienGrid[r][c].getY());
                        alienGrid[r][c].invalidate();
                    }
                }
            }
        }
    }

    // 6. Đạn người chơi tiêu diệt alien và tăng điểm chung.
    if (shipBullet.isVisible())
    {
        checkBulletAlienCollision(shipBullet);
    }
    if (ship2Bullet.isVisible())
    {
        checkBulletAlienCollision(ship2Bullet);
    }

    // 7. Alien chạm vào bất kỳ tàu nào thì mất máu và alien đó biến mất.
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            if (alienActive[r][c])
            {
                touchgfx::Rect alienRect = alienGrid[r][c].getRect();
                if (alienRect.intersect(ship1.getRect()))
                {
                    alienActive[r][c] = false;
                    alienGrid[r][c].setVisible(false);
                    alienGrid[r][c].invalidate();

                    spawnExplosion(alienRect.x, alienRect.y);
                    decreaseHealth();
                }
                else if (alienRect.intersect(ship2.getRect()))
                {
                    alienActive[r][c] = false;
                    alienGrid[r][c].setVisible(false);
                    alienGrid[r][c].invalidate();

                    spawnExplosion(alienRect.x, alienRect.y);
                    decreaseHealth();
                }
            }
        }
    }
}

void Screen3View::decreaseHealth()
{
    if (isGameOver)
    {
        return;
    }

    playerHealth--;
    if (playerHealth < 0)
    {
        playerHealth = 0;
    }

    // Dịch bitmap thanh máu bên trong container bị cắt để hiện đúng số mạng còn lại.
    health.setXY(0, -(5 - playerHealth) * 16);
    health.invalidate();

    if (playerHealth == 0)
    {
        gameOver();
    }
}

void Screen3View::handleKeyEvent(uint8_t c)
{
    // Trên simulator: người chơi 1 dùng WASD/Space, người chơi 2 dùng IJKL/F.
    bool p1Left = (c == 'a' || c == 'A');
    bool p1Right = (c == 'd' || c == 'D');
    bool p1Up = (c == 'w' || c == 'W');
    bool p1Down = (c == 's' || c == 'S');
    bool p1Button = (c == ' ');

    bool p2Left = (c == 'j' || c == 'J');
    bool p2Right = (c == 'l' || c == 'L');
    bool p2Up = (c == 'i' || c == 'I');
    bool p2Down = (c == 'k' || c == 'K');
    bool p2Button = (c == 'f' || c == 'F');

    if (isGameOver)
    {
        if (p1Button || p2Button)
        {
            restartGame();
        }
        return;
    }

    moveShip2D(ship1, p1Left, p1Right, p1Up, p1Down, PLAYER_SIM_STEP);
    moveShip2D(ship2, p2Left, p2Right, p2Up, p2Down, PLAYER_SIM_STEP);

    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }
    if (bullet2Cooldown > 0)
    {
        bullet2Cooldown--;
    }

    if (p1Button && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15;
    }

    if (p2Button && bullet2Cooldown == 0)
    {
        spawnShip2Bullet();
        bullet2Cooldown = 15;
    }

    tickGame();
}

void Screen3View::handleTickEvent()
{
#ifndef SIMULATOR
    uint8_t cmd;
    bool p1Left = false;
    bool p1Right = false;
    bool p1Up = false;
    bool p1Down = false;
    bool p2Left = false;
    bool p2Right = false;
    bool p2Up = false;
    bool p2Down = false;
    
    // Queue joystick 1 chỉ điều khiển tàu người chơi 1.
    while ((Joystick1QueueHandle != NULL) && (osMessageQueueGet(Joystick1QueueHandle, &cmd, NULL, 0) == osOK))
    {
        if (cmd == 'L') p1Left = true;
        else if (cmd == 'R') p1Right = true;
        else if (cmd == 'U') p1Up = true;
        else if (cmd == 'D') p1Down = true;
    }

    // Queue joystick 2 chỉ điều khiển tàu người chơi 2.
    while ((Joystick2QueueHandle != NULL) && (osMessageQueueGet(Joystick2QueueHandle, &cmd, NULL, 0) == osOK))
    {
        if (cmd == 'L') p2Left = true;
        else if (cmd == 'R') p2Right = true;
        else if (cmd == 'U') p2Up = true;
        else if (cmd == 'D') p2Down = true;
    }
    
    // Nút bấm ngoài là active-low vì GPIO đang dùng điện trở kéo lên nội bộ.
    bool button1 = (HAL_GPIO_ReadPin(P1_BUTTON_GPIO_Port, P1_BUTTON_Pin) == GPIO_PIN_RESET);
    bool button2 = (HAL_GPIO_ReadPin(P2_BUTTON_GPIO_Port, P2_BUTTON_Pin) == GPIO_PIN_RESET);
    
    if (isGameOver)
    {
        if (button1 || button2)
        {
            restartGame();
        }
        return;
    }
    
    moveShip2D(ship1, p1Left, p1Right, p1Up, p1Down, PLAYER_MOVE_STEP);
    moveShip2D(ship2, p2Left, p2Right, p2Up, p2Down, PLAYER_MOVE_STEP);
    
    // Giảm thời gian chờ giữa hai lần bắn.
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }
    if (bullet2Cooldown > 0)
    {
        bullet2Cooldown--;
    }
    
    // Bắn đạn cho người chơi 1.
    if (button1 && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15;
    }
    
    // Bắn đạn cho người chơi 2.
    if (button2 && bullet2Cooldown == 0)
    {
        spawnShip2Bullet();
        bullet2Cooldown = 15;
    }
    
    // Cập nhật vật lý game sau khi xử lý input.
    tickGame();
#endif
}
