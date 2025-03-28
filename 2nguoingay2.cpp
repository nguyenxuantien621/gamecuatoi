#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Window dimensions
const int SCREEN_WIDTH = 720;
const int SCREEN_HEIGHT = 720;
const int GRID_COUNT = 16;
const int CELL_SIZE = SCREEN_WIDTH / GRID_COUNT; // 720/16 = 45

// Number of obstacle rows and columns (8x8 obstacles)
const int OBSTACLE_ROWS = 8;
const int OBSTACLE_COLS = 8;

// Pickup special duration (in ms)
const Uint32 PICKUP_DURATION = 3000;

// Bullet speeds
const double NORMAL_BULLET_SPEED = 10.0;
const double SPECIAL_BULLET_SPEED = 5.0; // slower

// SDL global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* obstacleTexture = nullptr;
SDL_Texture* tankTexture = nullptr;    // tank 1
SDL_Texture* tank2Texture = nullptr;   // tank 2
SDL_Texture* bulletTexture = nullptr;  // bullet texture (dan.png)
SDL_Texture* specialTexture = nullptr; // special pickup texture (tenlua.png)

// Obstacles vector
std::vector<SDL_Rect> obstacles;

// Tanks for players
SDL_Rect tank1 = { 0, (GRID_COUNT - 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE }; // bottom-left
SDL_Rect tank2 = { (GRID_COUNT - 1) * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE };  // top-right

// Tank rotation angles (in degrees)
double tank1Angle = 0.0;
double tank2Angle = 0.0;

// Tank alive status
bool player1Alive = true;
bool player2Alive = true;

// Bullet structure
struct Bullet {
    SDL_Rect rect;
    double angle; // in degrees
    int owner;    // 1: tank1, 2: tank2
    bool special; // false: normal bullet, true: special bullet
};

// Vector of active bullets
std::vector<Bullet> bullets;

// Audio variables
Mix_Chunk* fireSound = nullptr;     // sound for shooting
Mix_Music* backgroundMusic = nullptr; // background music (if any)

// Special Pickup structure (item for special bullet)
struct SpecialPickup {
    SDL_Rect rect;    // displayed area (size equals one grid cell)
    Uint32 spawnTime; // time of spawn
};
std::vector<SpecialPickup> specialPickups;

// Inventory for special bullets (number of items collected)
int specialInventory1 = 0;
int specialInventory2 = 0;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer could not initialize! Mix_Error: " << Mix_GetError() << std::endl;
        return false;
    }
    Mix_Volume(-1, MIX_MAX_VOLUME);

    window = SDL_CreateWindow("Battle City 2 Players", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (!loadedSurface) {
        std::cout << "Unable to load image " << path << "! IMG_Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (!newTexture) {
        std::cout << "Unable to create texture from " << path << "! SDL_Error: " << SDL_GetError() << std::endl;
    }
    SDL_FreeSurface(loadedSurface);
    return newTexture;
}

bool checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x &&
            a.y < b.y + b.h && a.y + a.h > b.y);
}

// Setup obstacles in a fixed grid pattern (64 obstacles)
// Obstacles in the top row (i==0) and bottom row (i==OBSTACLE_ROWS-1) are fixed spawn positions for special pickups.
void setupObstacles() {
    obstacles.clear();
    specialPickups.clear();
    for (int i = 0; i < OBSTACLE_ROWS; i++) {
        for (int j = 0; j < OBSTACLE_COLS; j++) {
            SDL_Rect rect;
            rect.x = (2 * j + 1) * CELL_SIZE;
            rect.y = (2 * i + 1) * CELL_SIZE;
            rect.w = CELL_SIZE;
            rect.h = CELL_SIZE;
            obstacles.push_back(rect);
        }
    }
}

// If the obstacle is in the top row or bottom row, spawn a special pickup at that location.
void trySpawnSpecialPickup(const SDL_Rect& obsRect) {
    int topY = CELL_SIZE; // top row
    int bottomY = (2 * (OBSTACLE_ROWS - 1) + 1) * CELL_SIZE; // bottom row
    if (obsRect.y == topY || obsRect.y == bottomY) {
        SpecialPickup pickup;
        pickup.rect.w = CELL_SIZE;   // size equals one grid cell
        pickup.rect.h = CELL_SIZE;
        pickup.rect.x = obsRect.x;
        pickup.rect.y = obsRect.y;
        pickup.spawnTime = SDL_GetTicks();
        specialPickups.push_back(pickup);
    }
}

// Shoot bullet: if special is false, shoot normal bullet; if true, shoot special bullet.
void shootBullet(int owner, const SDL_Rect& tank, double angle, bool special = false) {
    if (fireSound) {
        int channel = Mix_PlayChannel(-1, fireSound, 0);
        if (channel == -1) {
            std::cout << "Failed to play sound: " << Mix_GetError() << std::endl;
        }
    }
    int bulletSize = (!special) ? CELL_SIZE / 3 : CELL_SIZE / 2; // normal: 1/3 cell; special: 1/2 cell

    SDL_Rect bulletRect;
    bulletRect.w = bulletSize;
    bulletRect.h = bulletSize;
    // Spawn at the center of the tank
    bulletRect.x = tank.x + tank.w / 2 - bulletSize / 2;
    bulletRect.y = tank.y + tank.h / 2 - bulletSize / 2;

    Bullet bullet;
    bullet.rect = bulletRect;
    bullet.angle = angle;
    bullet.owner = owner;
    bullet.special = special;
    bullets.push_back(bullet);
}

// Update pickups: if pickup exists longer than duration, remove; if a tank intersects, add to inventory.
void updatePickups() {
    Uint32 currentTime = SDL_GetTicks();
    for (auto it = specialPickups.begin(); it != specialPickups.end(); ) {
        if (currentTime - it->spawnTime > PICKUP_DURATION) {
            it = specialPickups.erase(it);
        } else {
            if (player1Alive && SDL_HasIntersection(&it->rect, &tank1)) {
                specialInventory1++;
                std::cout << "Player 1 picked up a special bullet! (Total: " << specialInventory1 << ")\n";
                it = specialPickups.erase(it);
                continue;
            }
            if (player2Alive && SDL_HasIntersection(&it->rect, &tank2)) {
                specialInventory2++;
                std::cout << "Player 2 picked up a special bullet! (Total: " << specialInventory2 << ")\n";
                it = specialPickups.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void renderPickups() {
    for (const auto& pickup : specialPickups) {
        if (specialTexture) {
            SDL_RenderCopy(renderer, specialTexture, nullptr, &pickup.rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_RenderFillRect(renderer, &pickup.rect);
        }
    }
}

// Handle input: Player 1 uses arrow keys to move, N to shoot normal, M to shoot special (if available).
// Player 2 uses WASD to move, T to shoot normal, Y to shoot special (if available).
void handleInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        int dx = 0, dy = 0;
        bool moved = false;
        // Tank 1 controls
        if (e.key.keysym.sym == SDLK_UP) {
            dy = -CELL_SIZE;
            tank1Angle = 0.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_DOWN) {
            dy = CELL_SIZE;
            tank1Angle = 180.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_LEFT) {
            dx = -CELL_SIZE;
            tank1Angle = 270.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_RIGHT) {
            dx = CELL_SIZE;
            tank1Angle = 90.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_n) {
            shootBullet(1, tank1, tank1Angle, false);
        } else if (e.key.keysym.sym == SDLK_m) {
            if (specialInventory1 > 0) {
                shootBullet(1, tank1, tank1Angle, true);
                specialInventory1--;
            }
        }
        if (moved) {
            SDL_Rect newPos = { tank1.x + dx, tank1.y + dy, tank1.w, tank1.h };
            if (newPos.x >= 0 && newPos.x + newPos.w <= SCREEN_WIDTH &&
                newPos.y >= 0 && newPos.y + newPos.h <= SCREEN_HEIGHT) {
                bool collision = false;
                for (const auto& obs : obstacles) {
                    if (obs.w > 0 && checkCollision(newPos, obs)) {
                        collision = true;
                        break;
                    }
                }
                if (!collision) {
                    tank1 = newPos;
                }
            }
        }
        // Tank 2 controls
        dx = 0; dy = 0; moved = false;
        if (e.key.keysym.sym == SDLK_w) {
            dy = -CELL_SIZE;
            tank2Angle = 0.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_s) {
            dy = CELL_SIZE;
            tank2Angle = 180.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_a) {
            dx = -CELL_SIZE;
            tank2Angle = 270.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_d) {
            dx = CELL_SIZE;
            tank2Angle = 90.0;
            moved = true;
        } else if (e.key.keysym.sym == SDLK_t) {
            shootBullet(2, tank2, tank2Angle, false);
        } else if (e.key.keysym.sym == SDLK_y) {
            if (specialInventory2 > 0) {
                shootBullet(2, tank2, tank2Angle, true);
                specialInventory2--;
            }
        }
        if (moved) {
            SDL_Rect newPos = { tank2.x + dx, tank2.y + dy, tank2.w, tank2.h };
            if (newPos.x >= 0 && newPos.x + newPos.w <= SCREEN_WIDTH &&
                newPos.y >= 0 && newPos.y + newPos.h <= SCREEN_HEIGHT) {
                bool collision = false;
                for (const auto& obs : obstacles) {
                    if (obs.w > 0 && checkCollision(newPos, obs)) {
                        collision = true;
                        break;
                    }
                }
                if (!collision) {
                    tank2 = newPos;
                }
            }
        }
    }
}

// Update bullets: special bullet now only explodes when it collides with an obstacle, a tank, or goes out-of-bound.
void updateBullets() {
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        double speed = it->special ? SPECIAL_BULLET_SPEED : NORMAL_BULLET_SPEED;
        double rad = it->angle * M_PI / 180.0;
        int dx = static_cast<int>(speed * sin(rad));
        int dy = static_cast<int>(-speed * cos(rad));
        it->rect.x += dx;
        it->rect.y += dy;
        bool removeBullet = false;
        bool collisionOccurred = false;

        // Check boundary collision
        if (it->rect.x < 0 || it->rect.x + it->rect.w > SCREEN_WIDTH ||
            it->rect.y < 0 || it->rect.y + it->rect.h > SCREEN_HEIGHT) {
            collisionOccurred = true;
        } else {
            // Check collision with obstacles
            for (auto obs_it = obstacles.begin(); obs_it != obstacles.end(); ) {
                if (checkCollision(it->rect, *obs_it)) {
                    collisionOccurred = true;
                    // If normal bullet, spawn pickup if obstacle is in top or bottom row
                    int topY = CELL_SIZE;
                    int bottomY = (2 * (OBSTACLE_ROWS - 1) + 1) * CELL_SIZE;
                    if (!it->special && ((*obs_it).y == topY || (*obs_it).y == bottomY)) {
                        trySpawnSpecialPickup(*obs_it);
                    }
                    obs_it = obstacles.erase(obs_it);
                    break;
                } else {
                    ++obs_it;
                }
            }
            // Check collision with enemy tanks
            if (!collisionOccurred) {
                if (!it->special) {
                    if (it->owner == 1 && player2Alive && checkCollision(it->rect, tank2)) {
                        player2Alive = false;
                        collisionOccurred = true;
                    } else if (it->owner == 2 && player1Alive && checkCollision(it->rect, tank1)) {
                        player1Alive = false;
                        collisionOccurred = true;
                    }
                } else {
                    // For special bullet, check collision with enemy tanks
                    if (it->owner == 1 && player2Alive && checkCollision(it->rect, tank2)) {
                        collisionOccurred = true;
                    } else if (it->owner == 2 && player1Alive && checkCollision(it->rect, tank1)) {
                        collisionOccurred = true;
                    }
                }
            }
        }

        // If collision occurred for special bullet, trigger explosion
        if (it->special && collisionOccurred) {
            SDL_Rect explosion;
            int centerX = it->rect.x + it->rect.w / 2;
            int centerY = it->rect.y + it->rect.h / 2;
            explosion.w = 3 * CELL_SIZE;
            explosion.h = 3 * CELL_SIZE;
            explosion.x = centerX - explosion.w / 2;
            explosion.y = centerY - explosion.h / 2;
            // Destroy obstacles in explosion region
            for (auto obs_it = obstacles.begin(); obs_it != obstacles.end(); ) {
                if (checkCollision(explosion, *obs_it)) {
                    obs_it = obstacles.erase(obs_it);
                } else {
                    ++obs_it;
                }
            }
            // Damage tanks in explosion region
            if (player1Alive && SDL_HasIntersection(&explosion, &tank1)) {
                player1Alive = false;
            }
            if (player2Alive && SDL_HasIntersection(&explosion, &tank2)) {
                player2Alive = false;
            }
            removeBullet = true;
        }

        // For normal bullet, remove if collision occurred
        if (!it->special && collisionOccurred) {
            removeBullet = true;
        }

        if (removeBullet) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
    // Update pickups
    updatePickups();
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // Render obstacles
    for (const auto& obs : obstacles) {
        SDL_RenderCopy(renderer, obstacleTexture, nullptr, &obs);
    }
    // Render pickups
    renderPickups();
    // Render tanks
    if (player1Alive && tankTexture) {
        SDL_RenderCopyEx(renderer, tankTexture, nullptr, &tank1, tank1Angle, nullptr, SDL_FLIP_NONE);
    }
    if (player2Alive && tank2Texture) {
        SDL_RenderCopyEx(renderer, tank2Texture, nullptr, &tank2, tank2Angle, nullptr, SDL_FLIP_NONE);
    }
    // Render bullets
    for (const auto &bullet : bullets) {
        if (bulletTexture) {
            SDL_RenderCopyEx(renderer, bulletTexture, nullptr, &bullet.rect, bullet.angle, nullptr, SDL_FLIP_NONE);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &bullet.rect);
        }
    }
    SDL_RenderPresent(renderer);
}

void closeAll() {
    Mix_FreeChunk(fireSound);
    fireSound = nullptr;
    Mix_FreeMusic(backgroundMusic);
    backgroundMusic = nullptr;
    Mix_CloseAudio();

    SDL_DestroyTexture(obstacleTexture);
    obstacleTexture = nullptr;
    SDL_DestroyTexture(tankTexture);
    tankTexture = nullptr;
    SDL_DestroyTexture(tank2Texture);
    tank2Texture = nullptr;
    SDL_DestroyTexture(bulletTexture);
    bulletTexture = nullptr;
    SDL_DestroyTexture(specialTexture);
    specialTexture = nullptr;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = nullptr;
    window = nullptr;

    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!init())
        return -1;

    // Load textures
    obstacleTexture = loadTexture("obstacle.png");
    tankTexture = loadTexture("tank.png");
    tank2Texture = loadTexture("tank2.png");
    bulletTexture = loadTexture("dan.png");
    specialTexture = loadTexture("tenlua.png");

    if (!obstacleTexture || !tankTexture || !tank2Texture) {
        std::cout << "Failed to load one or more textures!" << std::endl;
        closeAll();
        return -1;
    }

    // Load shooting sound
    fireSound = Mix_LoadWAV("pim.wav");
    if (!fireSound) {
        std::cout << "Failed to load shooting sound! Mix_Error: " << Mix_GetError() << std::endl;
    } else {
        std::cout << "Loaded shooting sound successfully!" << std::endl;
    }

    // Initialize obstacles and fixed spawn positions for pickups (top and bottom rows)
    setupObstacles();

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            handleInput(e);
        }
        updateBullets();
        render();
        SDL_Delay(16); // ~60 FPS

        // End game if one tank is destroyed
        if (!player1Alive || !player2Alive) {
            SDL_Delay(2000);
            quit = true;
        }
    }

    closeAll();
    return 0;
}
