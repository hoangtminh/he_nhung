#include <gui/screen3_screen/Screen3View.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>

#ifndef SIMULATOR
#include "main.h"
#include "cmsis_os.h"
extern "C" {
    extern osMessageQueueId_t Queue1Handle;
}
#endif

Screen3View::Screen3View()
    : score(0), isGameOver(false), isVictory(false), alienShootCooldown(45), bulletCooldown(0), bullet2Cooldown(0), alienDir(1), alienMoveTick(0), playerHealth(5)
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

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();

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

    // Initialize shipBullet (for Ship 1)
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
    gameOverWidget.setPosition(40, 125, 160, 70);
    gameOverWidget.setVisibleState(false);
    add(gameOverWidget);

    // Setup ship 1 (Line 1: Y=270, horizontal only)
    ship1.moveTo(50, 270);
    ship1.setVisible(true);
    ship1.invalidate();

    // Setup ship 2 (Line 2: Y=240, horizontal only)
    ship2.moveTo(160, 240);
    ship2.setVisible(true);
    ship2.invalidate();

    // Setup ship2Bullet
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

void Screen3View::handleJoystick(bool left, bool right, bool up, bool down, bool button)
{
    if (isGameOver)
    {
        if (button)
        {
            restartGame();
        }
        return;
    }

    // Ship 1 moves horizontally on Line 1 (Y=270)
    int x1 = ship1.getX();
    if (left)
    {
        x1 -= 3;
    }
    if (right)
    {
        x1 += 3;
    }
    if (x1 < 0)
    {
        x1 = 0;
    }
    if (x1 > 240 - ship1.getWidth())
    {
        x1 = 240 - ship1.getWidth();
    }

    if (x1 != ship1.getX())
    {
        ship1.moveTo(x1, ship1.getY());
        ship1.invalidate();
    }

    // Ship 2 moves horizontally on Line 2 (Y=240) using Up/Down
    int x2 = ship2.getX();
    if (up)
    {
        x2 -= 3; // Up moves Left
    }
    if (down)
    {
        x2 += 3; // Down moves Right
    }
    if (x2 < 0)
    {
        x2 = 0;
    }
    if (x2 > 240 - ship2.getWidth())
    {
        x2 = 240 - ship2.getWidth();
    }

    if (x2 != ship2.getX())
    {
        ship2.moveTo(x2, ship2.getY());
        ship2.invalidate();
    }

    // Bullet shooting cooldown management
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }
    if (bullet2Cooldown > 0)
    {
        bullet2Cooldown--;
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

void Screen3View::spawnBullet()
{
    if (!shipBullet.isVisible())
    {
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
        int bx = ship2.getX() + (ship2.getWidth() - ship2Bullet.getWidth()) / 2;
        int by = ship2.getY() - ship2Bullet.getHeight();
        
        ship2Bullet.moveTo(bx, by);
        ship2Bullet.setVisible(true);
        ship2Bullet.invalidate();
    }
}

void Screen3View::spawnAlienBullet()
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

void Screen3View::victory()
{
    isGameOver = true;
    isVictory = true;
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

    gameOverWidget.setVisibleState(true, true, score);
}

void Screen3View::restartGame()
{
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
    bullet2Cooldown = 0;
    alienDir = 1;
    alienMoveTick = 0;
    alienShootCooldown = 45;
    playerHealth = 5;
    health.setXY(0, 0);
    health.invalidate();
    isVictory = false;
    gameOverWidget.setVisibleState(false);
}

void Screen3View::checkBulletAlienCollision(touchgfx::Widget& bullet)
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

void Screen3View::tickGame()
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

                // Check collision with player ship 1
                touchgfx::Rect shipRect = ship1.getRect();
                touchgfx::Rect shipHitbox(shipRect.x + 2, shipRect.y + 2, shipRect.width - 4, shipRect.height - 4);
                if (alienBullets[i].getRect().intersect(shipHitbox))
                {
                    alienBullets[i].setVisible(false);
                    alienBullets[i].invalidate();
                    alienBulletActive[i] = false;
                    decreaseHealth();
                }
                // Check collision with player ship 2
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

    // 3. Update explosions
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

    // 4. Periodically trigger alien shooting
    alienShootCooldown--;
    if (alienShootCooldown <= 0)
    {
        alienShootCooldown = 45;
        spawnAlienBullet();
    }

    // 5. Move alien grid periodically
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

    // 6. Collision detection
    if (shipBullet.isVisible())
    {
        checkBulletAlienCollision(shipBullet);
    }
    if (ship2Bullet.isVisible())
    {
        checkBulletAlienCollision(ship2Bullet);
    }

    // 7. Collision detection: Ships colliding with active aliens
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

    // 8. Check victory condition (all aliens killed)
    bool anyActive = false;
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            if (alienActive[r][c])
            {
                anyActive = true;
                break;
            }
        }
        if (anyActive) break;
    }
    if (!anyActive && !isGameOver)
    {
        victory();
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

    health.setXY(0, -(5 - playerHealth) * 16);
    health.invalidate();

    if (playerHealth == 0)
    {
        gameOver();
    }
}

void Screen3View::handleKeyEvent(uint8_t c)
{
    // Support playing in TouchGFX simulator using WASD / Spacebar / F
    bool left = (c == 'a' || c == 'A');
    bool right = (c == 'd' || c == 'D');
    bool up = (c == 'w' || c == 'W');
    bool down = (c == 's' || c == 'S');
    bool button = (c == ' ');
    bool button2 = (c == 'f' || c == 'F');

    if (left || right || up || down || button)
    {
        handleJoystick(left, right, up, down, button);
    }
    
    if (button2 && bullet2Cooldown == 0)
    {
        spawnShip2Bullet();
        bullet2Cooldown = 15;
        tickGame();
    }
}

void Screen3View::handleTickEvent()
{
#ifndef SIMULATOR
    uint8_t cmd;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    
    // Drain all commands in the queue
    while (osMessageQueueGet(Queue1Handle, &cmd, NULL, 0) == osOK)
    {
        if (cmd == 'L') left = true;
        else if (cmd == 'R') right = true;
        else if (cmd == 'U') up = true;
        else if (cmd == 'D') down = true;
    }
    
    // Ship 1: outside button PG3 (Active Low - Pull-up)
    bool button1 = (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_3) == GPIO_PIN_RESET);
    // Ship 2: onboard button PA0 (Active High - Pull-down)
    bool button2 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    
    if (isGameOver)
    {
        if (button1 || button2)
        {
            restartGame();
        }
        return;
    }
    
    // Ship 1 horizontal movement on Line 1 (Y=270)
    int x1 = ship1.getX();
    if (left) x1 -= 10;
    if (right) x1 += 10;
    if (x1 < 0) x1 = 0;
    if (x1 > 240 - ship1.getWidth()) x1 = 240 - ship1.getWidth();
    
    if (x1 != ship1.getX())
    {
        ship1.moveTo(x1, ship1.getY());
        ship1.invalidate();
    }
    
    // Ship 2 horizontal movement on Line 2 (Y=240) using Y-axis (Up/Down)
    int x2 = ship2.getX();
    if (up) x2 -= 10;
    if (down) x2 += 10;
    if (x2 < 0) x2 = 0;
    if (x2 > 240 - ship2.getWidth()) x2 = 240 - ship2.getWidth();
    
    if (x2 != ship2.getX())
    {
        ship2.moveTo(x2, ship2.getY());
        ship2.invalidate();
    }
    
    // Bullet shooting cooldown management
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }
    if (bullet2Cooldown > 0)
    {
        bullet2Cooldown--;
    }
    
    // Fire Ship 1 bullet
    if (button1 && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15;
    }
    
    // Fire Ship 2 bullet
    if (button2 && bullet2Cooldown == 0)
    {
        spawnShip2Bullet();
        bullet2Cooldown = 15;
    }
    
    // Call game physics update
    tickGame();
#endif
}
