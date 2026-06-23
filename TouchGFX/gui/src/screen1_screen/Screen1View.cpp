#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>

#ifndef SIMULATOR
#include "main.h"
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Joystick1QueueHandle;
}
#endif

namespace
{
// Giữ tàu ở nửa dưới màn hình để không đi lẫn vào vùng đội hình alien.
static const int PLAYER_MIN_Y = 160;
}

Screen1View::Screen1View()
    : score(0), isGameOver(false), alienShootCooldown(45), bulletCooldown(0), alienDir(1), alienMoveTick(0), playerHealth(5)
{
    // Initialize alien and bullet arrays
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

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    // Hide the original designer alien widget
    alien.setVisible(false);
    alien.invalidate();

    // Get the original alien bitmap
    touchgfx::Bitmap alienBmp = alien.getBitmap();

    // Initialize replicated alien grid
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

    // Initialize shipBullet
    shipBullet.setVisible(false);
    shipBullet.invalidate();

    // Initialize alien bullets (using AlienBullet image)
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBullets[i].setBitmap(touchgfx::Bitmap(BITMAP_ALIENBULLET_ID));
        alienBullets[i].setVisible(false);
        add(alienBullets[i]);
        alienBulletActive[i] = false;
    }

    // Initialize explosions (DeadAlien animation pool)
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

    // Add ScoreWidget (upper right corner)
    scoreWidget.setPosition(160, 10, 72, 12);
    scoreWidget.setScore(0);
    add(scoreWidget);

    // Reposition health widget to top left using healthContainer clipping
    remove(health);
    healthContainer.setPosition(10, 10, 32, 16);
    healthContainer.add(health);
    add(healthContainer);
    health.setXY(0, 0);
    health.setVisible(true);
    health.invalidate();
    playerHealth = 5;

    // Add GameOverWidget (centered)
    gameOverWidget.setPosition(40, 125, 160, 50);
    gameOverWidget.setVisibleState(false);
    add(gameOverWidget);

    // 1-player mode (Line 1: Y=270)
    ship.moveTo(101, 270);
    ship.setVisible(true);
    ship.invalidate();

    isGameOver = false;
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleJoystick(bool left, bool right, bool up, bool down, bool button)
{
    if (isGameOver)
    {
        if (button)
        {
            restartGame();
        }
        return;
    }

    // Joystick điều khiển tàu theo cả hai trục; giới hạn để tàu không ra ngoài màn hình.
    int x1 = ship.getX();
    int y1 = ship.getY();
    if (left)
    {
        x1 -= 3;
    }
    if (right)
    {
        x1 += 3;
    }
    if (up)
    {
        y1 -= 3;
    }
    if (down)
    {
        y1 += 3;
    }
    if (x1 < 0)
    {
        x1 = 0;
    }
    if (x1 > 240 - ship.getWidth())
    {
        x1 = 240 - ship.getWidth();
    }
    if (y1 < PLAYER_MIN_Y)
    {
        y1 = PLAYER_MIN_Y;
    }
    if (y1 > 320 - ship.getHeight())
    {
        y1 = 320 - ship.getHeight();
    }

    if ((x1 != ship.getX()) || (y1 != ship.getY()))
    {
        ship.moveTo(x1, y1);
        ship.invalidate();
    }

    // Bullet shooting cooldown management
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }

    // Fire Ship 1 bullet (button is outside button / Spacebar in simulator)
    if (button && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15;
    }

    // Run periodic game tick update
    tickGame();
}

void Screen1View::spawnBullet()
{
    if (!shipBullet.isVisible())
    {
        int bx = ship.getX() + (ship.getWidth() - shipBullet.getWidth()) / 2;
        int by = ship.getY() - shipBullet.getHeight();
        
        shipBullet.moveTo(bx, by);
        shipBullet.setVisible(true);
        shipBullet.invalidate();
    }
}

// spawnShip2Bullet removed

void Screen1View::spawnAlienBullet()
{
    // Find all active aliens
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
        // Simple pseudo-random index selection
        static unsigned int seed = 12345;
        seed = seed * 1103515245 + 12345;
        int idx = (seed / 65536) % activeCount;
        int r = activeAliens[idx].r;
        int c = activeAliens[idx].c;

        // Spawn bullet
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

void Screen1View::spawnExplosion(int x, int y)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (explosions[i].frame == -1)
        {
            // Center the 40x30 explosion container over the 39x27 alien
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

void Screen1View::gameOver()
{
    isGameOver = true;
    ship.setVisible(false);
    ship.invalidate();



    // Reset and clean screen to prevent leftover pixels from bullets, aliens, and explosions
    shipBullet.setVisible(false);
    shipBullet.invalidate();

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

void Screen1View::restartGame()
{
    isGameOver = false;
    score = 0;
    scoreWidget.setScore(0);
    
    ship.moveTo(101, 270);
    ship.setVisible(true);
    ship.invalidate();

    // Reset aliens
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

    // Reset player bullet
    shipBullet.setVisible(false);
    shipBullet.invalidate();

    // Reset alien bullets
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        alienBullets[i].setVisible(false);
        alienBullets[i].invalidate();
        alienBulletActive[i] = false;
    }

    // Reset explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosions[i].container.setVisible(false);
        explosions[i].container.invalidate();
        explosions[i].frame = -1;
    }

    bulletCooldown = 0;
    alienDir = 1;
    alienMoveTick = 0;
    alienShootCooldown = 45;
    playerHealth = 5;
    health.setXY(0, 0);
    health.invalidate();
    gameOverWidget.setVisibleState(false);
}

void Screen1View::checkBulletAlienCollision(touchgfx::Widget& bullet)
{
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

void Screen1View::tickGame()
{
    if (isGameOver)
    {
        return;
    }

    // 1. Move player bullets straight up
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
    // 2. Move alien bullets down
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        if (alienBulletActive[i])
        {
            int by = alienBullets[i].getY() + 2; // Alien bullet speed: 2 pixels per frame
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

                // Check collision with player ship
                touchgfx::Rect shipRect = ship.getRect();
                touchgfx::Rect shipHitbox(shipRect.x + 2, shipRect.y + 2, shipRect.width - 4, shipRect.height - 4);
                if (alienBullets[i].getRect().intersect(shipHitbox))
                {
                    alienBullets[i].setVisible(false);
                    alienBullets[i].invalidate();
                    alienBulletActive[i] = false;
                    decreaseHealth();
                }
            }
        }
    }

    // 3. Update explosions (DeadAlien animation)
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (explosions[i].frame != -1)
        {
            explosions[i].tickCounter++;
            if (explosions[i].tickCounter >= 6) // 6 ticks per frame (~100ms)
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

    // 4. Periodically trigger alien shooting
    alienShootCooldown--;
    if (alienShootCooldown <= 0)
    {
        alienShootCooldown = 45; // Shoot every 45 frames (increased from 80)
        spawnAlienBullet();
    }

    // 5. Move alien grid periodically (every 1 second = 60 frames)
    alienMoveTick++;
    if (alienMoveTick >= 60)
    {
        alienMoveTick = 0;
        bool hitEdge = false;
        int step = 10;       // Horizontal speed (1 đoạn nhỏ)
        int shiftDown = 10;  // Shift down step

        // Check if moving would hit the left/right screen edges
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

        // Perform grid movement: reverse & drop down, or slide horizontally
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

    // 6. Collision detection: Active player bullets hitting active aliens
    if (shipBullet.isVisible())
    {
        checkBulletAlienCollision(shipBullet);
    }
    // 7. Collision detection: Ship colliding with active aliens
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            if (alienActive[r][c])
            {
                touchgfx::Rect alienRect = alienGrid[r][c].getRect();
                if (alienRect.intersect(ship.getRect()))
                {
                    // Alien touches ship
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

void Screen1View::decreaseHealth()
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

    // Update health widget visual frame by sliding it inside the container
    health.setXY(0, -(5 - playerHealth) * 16);
    health.invalidate();

    if (playerHealth == 0)
    {
        gameOver();
    }
}

void Screen1View::handleKeyEvent(uint8_t c)
{
    // Support playing in TouchGFX simulator using WASD / Spacebar
    bool left = (c == 'a' || c == 'A');
    bool right = (c == 'd' || c == 'D');
    bool up = (c == 'w' || c == 'W');
    bool down = (c == 's' || c == 'S');
    bool button = (c == ' ');

    if (left || right || up || down || button)
    {
        handleJoystick(left, right, up, down, button);
    }
}

void Screen1View::handleTickEvent()
{
#ifndef SIMULATOR
    uint8_t cmd;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    
    // Màn 1 người chỉ đọc queue của joystick 1.
    while ((Joystick1QueueHandle != NULL) && (osMessageQueueGet(Joystick1QueueHandle, &cmd, NULL, 0) == osOK))
    {
        if (cmd == 'L') left = true;
        else if (cmd == 'R') right = true;
        else if (cmd == 'U') up = true;
        else if (cmd == 'D') down = true;
    }
    
    bool button1 = (HAL_GPIO_ReadPin(P1_BUTTON_GPIO_Port, P1_BUTTON_Pin) == GPIO_PIN_RESET);
    
    if (isGameOver)
    {
        if (button1)
        {
            restartGame();
        }
        return;
    }
    
    // Người chơi 1 di chuyển theo cả X/Y bằng joystick 1.
    int x1 = ship.getX();
    int y1 = ship.getY();
    if (left) x1 -= 10;
    if (right) x1 += 10;
    if (up) y1 -= 10;
    if (down) y1 += 10;
    if (x1 < 0) x1 = 0;
    if (x1 > 240 - ship.getWidth()) x1 = 240 - ship.getWidth();
    if (y1 < PLAYER_MIN_Y) y1 = PLAYER_MIN_Y;
    if (y1 > 320 - ship.getHeight()) y1 = 320 - ship.getHeight();
    
    if ((x1 != ship.getX()) || (y1 != ship.getY()))
    {
        ship.moveTo(x1, y1);
        ship.invalidate();
    }
    
    // Bullet shooting cooldown management
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }
    
    // Fire Ship 1 bullet
    if (button1 && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15;
    }
    
    // Call game physics update
    tickGame();
#endif
}
