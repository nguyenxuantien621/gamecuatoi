#include <SDL.h>
#include <SDL_image.h>

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;
const int GRID_SIZE = 12;
const int CELL_SIZE = SCREEN_WIDTH / GRID_SIZE;
const int TANK_SIZE = CELL_SIZE;
const int OBSTACLE_ROWS = 6;
const int OBSTACLE_COLS = 6;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* tankTexture = nullptr;
SDL_Texture* obstacleTexture = nullptr;
SDL_Rect tank = {0, (GRID_SIZE - 1) * CELL_SIZE, TANK_SIZE, TANK_SIZE}; // Xe tăng ở góc trái dưới cùng
SDL_Rect obstacles[OBSTACLE_ROWS][OBSTACLE_COLS];

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

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw tank
    if (tankTexture)
        SDL_RenderCopy(renderer, tankTexture, nullptr, &tank);

    // Draw obstacles
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

    tankTexture = loadTexture("tank.png");
    obstacleTexture = loadTexture("obstacle.png");
    setupObstacles();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        render();
    }

    close();
    return 0;
}
