#include <SDL.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int GRID_SIZE = 12;
const int CELL_SIZE = SCREEN_WIDTH / GRID_SIZE;
const int TANK_SIZE = CELL_SIZE;
const int OBSTACLE_ROWS = 6;
const int OBSTACLE_COLS = 6;
const int ENEMY_COUNT = 5;
const int MOVE_DELAY = 500; // Độ trễ di chuyển xe địch (ms)

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* tankTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
SDL_Texture* obstacleTexture = nullptr;
SDL_Rect tank = {0, (GRID_SIZE - 1) * CELL_SIZE, TANK_SIZE, TANK_SIZE};
SDL_Rect obstacles[OBSTACLE_ROWS][OBSTACLE_COLS];
SDL_Rect enemies[ENEMY_COUNT] = {
    {CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {3 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {5 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {7 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {7 * CELL_SIZE, 2 * CELL_SIZE, TANK_SIZE, TANK_SIZE}
};
double tankAngle = 0.0;
double enemyAngles[ENEMY_COUNT] = {0};
Uint32 lastMoveTime = 0;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (!IMG_Init(IMG_INIT_PNG)) return false;

    window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    return renderer != nullptr;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface) {
        newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

void close() {
    SDL_DestroyTexture(tankTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(obstacleTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void setupObstacles() {
    for (int i = 0; i < OBSTACLE_ROWS; i++) {
        for (int j = 0; j < OBSTACLE_COLS; j++) {
            obstacles[i][j].x = (1 + j * 2) * CELL_SIZE;
            obstacles[i][j].y = (1 + i * 2) * CELL_SIZE;
            obstacles[i][j].w = CELL_SIZE;
            obstacles[i][j].h = CELL_SIZE;
        }
    }
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x &&
            a.y < b.y + b.h && a.y + a.h > b.y);
}

void handleInput(SDL_Event& event) {
    int dx = 0, dy = 0;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP: dy = -CELL_SIZE; tankAngle = 0.0; break;
            case SDLK_DOWN: dy = CELL_SIZE; tankAngle = 180.0; break;
            case SDLK_LEFT: dx = -CELL_SIZE; tankAngle = 270.0; break;
            case SDLK_RIGHT: dx = CELL_SIZE; tankAngle = 90.0; break;
        }

        SDL_Rect newTankPos = {tank.x + dx, tank.y + dy, TANK_SIZE, TANK_SIZE};

        if (newTankPos.x >= 0 && newTankPos.x + TANK_SIZE <= SCREEN_WIDTH &&
            newTankPos.y >= 0 && newTankPos.y + TANK_SIZE <= SCREEN_HEIGHT) {

            bool collision = false;
            for (int i = 0; i < OBSTACLE_ROWS; i++) {
                for (int j = 0; j < OBSTACLE_COLS; j++) {
                    if (checkCollision(newTankPos, obstacles[i][j])) {
                        collision = true;
                        break;
                    }
                }
                if (collision) break;
            }

            if (!collision) {
                tank.x += dx;
                tank.y += dy;
            }
        }
    }
}

void moveEnemies() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastMoveTime < MOVE_DELAY) return;
    lastMoveTime = currentTime;

    int directions[4][2] = {{0, -CELL_SIZE}, {0, CELL_SIZE}, {-CELL_SIZE, 0}, {CELL_SIZE, 0}};
    double angles[4] = {0.0, 180.0, 270.0, 90.0};

    for (int i = 0; i < ENEMY_COUNT; i++) {
        int randomDir = rand() % 4;
        int dx = directions[randomDir][0];
        int dy = directions[randomDir][1];
        SDL_Rect newEnemyPos = {enemies[i].x + dx, enemies[i].y + dy, TANK_SIZE, TANK_SIZE};

        if (newEnemyPos.x >= 0 && newEnemyPos.x + TANK_SIZE <= SCREEN_WIDTH &&
            newEnemyPos.y >= 0 && newEnemyPos.y + TANK_SIZE <= SCREEN_HEIGHT) {

            bool collision = false;
            for (int j = 0; j < OBSTACLE_ROWS; j++) {
                for (int k = 0; k < OBSTACLE_COLS; k++) {
                    if (checkCollision(newEnemyPos, obstacles[j][k])) {
                        collision = true;
                        break;
                    }
                }
                if (collision) break;
            }
            if (!collision) {
                enemies[i] = newEnemyPos;
                enemyAngles[i] = angles[randomDir];
            }
        }
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (tankTexture)
        SDL_RenderCopyEx(renderer, tankTexture, nullptr, &tank, tankAngle, nullptr, SDL_FLIP_NONE);

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemyTexture)
            SDL_RenderCopyEx(renderer, enemyTexture, nullptr, &enemies[i], enemyAngles[i], nullptr, SDL_FLIP_NONE);
    }

    for (int i = 0; i < OBSTACLE_ROWS; i++) {
        for (int j = 0; j < OBSTACLE_COLS; j++) {
            if (obstacleTexture)
                SDL_RenderCopy(renderer, obstacleTexture, nullptr, &obstacles[i][j]);
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (!init()) return -1;
    srand(time(nullptr));

    tankTexture = loadTexture("tank.png");
    enemyTexture = loadTexture("tank2.png");
    obstacleTexture = loadTexture("obstacle.png");
    setupObstacles();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            handleInput(event);
        }
        moveEnemies();
        render();
    }

    close();
    return 0;
}
