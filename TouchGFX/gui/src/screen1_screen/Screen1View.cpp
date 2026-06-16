#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>

Screen1View::Screen1View()
    : score(0), isGameOver(false), alienShootCooldown(80), bulletCooldown(0), alienDir(1), alienMoveTick(0)
{
    // Initialize alien and bullet arrays
    for (int r = 0; r < ALIEN_ROWS; r++)
    {
        for (int c = 0; c < ALIEN_COLS; c++)
        {
            alienActive[r][c] = false;
        }
    }
    
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bulletActive[i] = false;
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
    int spacingX = 1;
    int spacingY = 5;

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

    // Initialize bullets (using ShipBullet image)
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].setBitmap(touchgfx::Bitmap(BITMAP_SHIPBULLET_ID));
        bullets[i].setVisible(false);
        add(bullets[i]);
        bulletActive[i] = false;
    }

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

    // Add GameOverWidget (centered)
    gameOverWidget.setPosition(40, 125, 160, 50);
    gameOverWidget.setVisibleState(false);
    add(gameOverWidget);
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

    // Ship movement speed: 3 pixels per frame
    int x = ship.getX();
    int y = ship.getY();

    if (left)
    {
        x -= 3;
    }
    if (right)
    {
        x += 3;
    }
    if (up)
    {
        y -= 3;
    }
    if (down)
    {
        y += 3;
    }

    // Boundary constraints: Screen is 240x320
    if (x < 0)
    {
        x = 0;
    }
    if (x > 240 - ship.getWidth())
    {
        x = 240 - ship.getWidth();
    }
    if (y < 0)
    {
        y = 0;
    }
    if (y > 320 - ship.getHeight())
    {
        y = 320 - ship.getHeight();
    }

    // Update ship position if changed
    if (x != ship.getX() || y != ship.getY())
    {
        ship.moveTo(x, y);
        ship.invalidate();
    }

    // Bullet shooting cooldown management
    if (bulletCooldown > 0)
    {
        bulletCooldown--;
    }

    // Fire bullet if button is pressed and cooldown has elapsed
    if (button && bulletCooldown == 0)
    {
        spawnBullet();
        bulletCooldown = 15; // Cooldown limit (~250ms at 60fps)
    }

    // Run periodic game tick update
    tickGame();
}

void Screen1View::spawnBullet()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bulletActive[i])
        {
            // Spawn bullet exactly from the horizontal center of the ship, just above its top edge
            int bx = ship.getX() + (ship.getWidth() - bullets[i].getWidth()) / 2;
            int by = ship.getY() - bullets[i].getHeight();
            
            bullets[i].moveTo(bx, by);
            bullets[i].setVisible(true);
            bullets[i].invalidate();
            bulletActive[i] = true;
            break;
        }
    }
}

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
    gameOverWidget.setVisibleState(true);
}

void Screen1View::restartGame()
{
    isGameOver = false;
    score = 0;
    scoreWidget.setScore(0);
    ship.moveTo(101, 261);
    ship.setVisible(true);
    ship.invalidate();

    // Reset aliens
    int startX = 0;
    int startY = 10;
    int spacingX = 1;
    int spacingY = 5;

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

    // Reset player bullets
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].setVisible(false);
        bullets[i].invalidate();
        bulletActive[i] = false;
    }

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
    alienShootCooldown = 80;
    gameOverWidget.setVisibleState(false);
}

void Screen1View::tickGame()
{
    if (isGameOver)
    {
        return;
    }

    // 1. Move player bullets straight up
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bulletActive[i])
        {
            int by = bullets[i].getY() - 5; // Bullet speed: 5 pixels per frame
            if (by + bullets[i].getHeight() < 0)
            {
                bullets[i].setVisible(false);
                bullets[i].invalidate();
                bulletActive[i] = false;
            }
            else
            {
                bullets[i].moveTo(bullets[i].getX(), by);
                bullets[i].invalidate();
            }
        }
    }

    // 2. Move alien bullets down
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++)
    {
        if (alienBulletActive[i])
        {
            int by = alienBullets[i].getY() + 3; // Alien bullet speed: 3 pixels per frame
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
                if (alienBullets[i].getRect().intersect(ship.getRect()))
                {
                    alienBullets[i].setVisible(false);
                    alienBullets[i].invalidate();
                    alienBulletActive[i] = false;
                    gameOver();
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
        alienShootCooldown = 80; // Shoot every 80 frames (~1.3s)
        spawnAlienBullet();
    }

    // 5. Move alien grid periodically
    alienMoveTick++;
    if (alienMoveTick >= 12)
    {
        alienMoveTick = 0;
        bool hitEdge = false;
        int step = 4;        // Horizontal speed
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
    for (int b = 0; b < MAX_BULLETS; b++)
    {
        if (bulletActive[b])
        {
            touchgfx::Rect bulletRect = bullets[b].getRect();
            for (int r = 0; r < ALIEN_ROWS; r++)
            {
                for (int c = 0; c < ALIEN_COLS; c++)
                {
                    if (alienActive[r][c])
                    {
                        touchgfx::Rect alienRect = alienGrid[r][c].getRect();
                        if (bulletRect.intersect(alienRect))
                        {
                            // Collision: hide and deactivate both the alien and the bullet
                            alienActive[r][c] = false;
                            alienGrid[r][c].setVisible(false);
                            alienGrid[r][c].invalidate();

                            bulletActive[b] = false;
                            bullets[b].setVisible(false);
                            bullets[b].invalidate();

                            // Spawn explosion
                            spawnExplosion(alienGrid[r][c].getX(), alienGrid[r][c].getY());

                            // Increment score
                            score += 10;
                            scoreWidget.setScore(score);
                            break;
                        }
                    }
                }
                if (!bulletActive[b])
                {
                    break;
                }
            }
        }
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
