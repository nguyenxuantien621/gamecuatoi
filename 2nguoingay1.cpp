#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>

// Kích thước cửa sổ và lưới
const int SCREEN_WIDTH = 720;
const int SCREEN_HEIGHT = 720;
const int GRID_COUNT = 16;
const int CELL_SIZE = SCREEN_WIDTH / GRID_COUNT; // 720/16 = 45

// Số hàng, cột chướng ngại vật: 8x8
const int OBSTACLE_ROWS = 8;
const int OBSTACLE_COLS = 8;

// SDL global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* obstacleTexture = nullptr;
SDL_Texture* tankTexture = nullptr;   // xe 1
SDL_Texture* tank2Texture = nullptr;  // xe 2

// Danh sách chướng ngại vật
// Mỗi chướng ngại vật là 1 ô vuông (45x45) sử dụng ảnh obstacle.png
std::vector<SDL_Rect> obstacles;

// Xe tăng của người chơi
SDL_Rect tank1 = {0, (GRID_COUNT - 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE}; // góc dưới bên trái
SDL_Rect tank2 = {(GRID_COUNT - 1) * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE};  // góc trên bên phải

// Góc quay (đơn vị độ) của các xe tăng
double tank1Angle = 0.0;
double tank2Angle = 0.0;

// Trạng thái sống của các xe (có thể mở rộng sau)
bool player1Alive = true;
bool player2Alive = true;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL không khởi tạo được! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "SDL_image không khởi tạo được! IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Battle City 2 Người Chơi", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << "Không thể tạo cửa sổ! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cout << "Không thể tạo renderer! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (!loadedSurface) {
        std::cout << "Không tải được ảnh " << path << "! IMG_Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (!newTexture) {
        std::cout << "Không tạo được texture từ " << path << "! SDL_Error: " << SDL_GetError() << std::endl;
    }
    SDL_FreeSurface(loadedSurface);
    return newTexture;
}

// Hàm kiểm tra va chạm giữa 2 hình chữ nhật
bool checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x &&
            a.y < b.y + b.h && a.y + a.h > b.y);
}

// Tạo chướng ngại vật xen kẽ trên bản đồ
// Chướng ngại vật được đặt tại các ô có chỉ số (2*i+1, 2*j+1) với i, j từ 0 đến 7
void setupObstacles() {
    obstacles.clear();
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

// Xử lý sự kiện bàn phím cho 2 người chơi
void handleInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        // Xe tăng 1 (dùng mũi tên)
        int dx = 0, dy = 0;
        bool moved = false;
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
        }
        if (moved) {
            SDL_Rect newPos = { tank1.x + dx, tank1.y + dy, tank1.w, tank1.h };
            // Kiểm tra vượt biên
            if (newPos.x >= 0 && newPos.x + newPos.w <= SCREEN_WIDTH &&
                newPos.y >= 0 && newPos.y + newPos.h <= SCREEN_HEIGHT) {
                // Kiểm tra va chạm với chướng ngại vật
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

        // Xe tăng 2 (dùng WASD: W-up, S-down, A-left, D-right)
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

void render() {
    // Xóa màn hình với màu đen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ chướng ngại vật
    for (const auto& obs : obstacles) {
        SDL_RenderCopy(renderer, obstacleTexture, nullptr, &obs);
    }

    // Vẽ xe tăng 1
    if (player1Alive && tankTexture) {
        SDL_RenderCopyEx(renderer, tankTexture, nullptr, &tank1, tank1Angle, nullptr, SDL_FLIP_NONE);
    }
    // Vẽ xe tăng 2
    if (player2Alive && tank2Texture) {
        SDL_RenderCopyEx(renderer, tank2Texture, nullptr, &tank2, tank2Angle, nullptr, SDL_FLIP_NONE);
    }

    SDL_RenderPresent(renderer);
}

void closeAll() {
    SDL_DestroyTexture(obstacleTexture);
    obstacleTexture = nullptr;
    SDL_DestroyTexture(tankTexture);
    tankTexture = nullptr;
    SDL_DestroyTexture(tank2Texture);
    tank2Texture = nullptr;

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

    // Tải texture
    obstacleTexture = loadTexture("obstacle.png");
    tankTexture = loadTexture("tank.png");    // xe tăng 1
    tank2Texture = loadTexture("tank2.png");    // xe tăng 2

    if (!obstacleTexture || !tankTexture || !tank2Texture) {
        std::cout << "Không tải được một hoặc nhiều texture!" << std::endl;
        closeAll();
        return -1;
    }

    setupObstacles();

    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            handleInput(e);
        }
        render();
        SDL_Delay(16); // khoảng 60 FPS
    }

    closeAll();
    return 0;
}
