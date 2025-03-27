#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <cmath>

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
SDL_Texture* tankTexture = nullptr;    // xe tăng 1
SDL_Texture* tank2Texture = nullptr;   // xe tăng 2
SDL_Texture* bulletTexture = nullptr;  // texture đạn (dan.png)

// Danh sách chướng ngại vật
std::vector<SDL_Rect> obstacles;

// Xe tăng của người chơi
SDL_Rect tank1 = {0, (GRID_COUNT - 1) * CELL_SIZE, CELL_SIZE, CELL_SIZE}; // góc dưới bên trái
SDL_Rect tank2 = {(GRID_COUNT - 1) * CELL_SIZE, 0, CELL_SIZE, CELL_SIZE};  // góc trên bên phải

// Góc quay (đơn vị độ) của các xe tăng
double tank1Angle = 0.0;
double tank2Angle = 0.0;

// Trạng thái sống của các xe
bool player1Alive = true;
bool player2Alive = true;

// Cấu trúc viên đạn
struct Bullet {
    SDL_Rect rect;
    double angle; // hướng bắn theo độ
    int owner;    // 1: của xe 1, 2: của xe 2
};

// Danh sách đạn đang có trên màn hình
std::vector<Bullet> bullets;

// Hàm khởi tạo SDL và cửa sổ
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

// Hàm tạo viên đạn mới
void shootBullet(int owner, const SDL_Rect& tank, double angle) {
    int bulletSize = CELL_SIZE / 3;
    // Khởi tạo đạn xuất phát từ trung tâm xe
    SDL_Rect bulletRect;
    bulletRect.w = bulletSize;
    bulletRect.h = bulletSize;
    bulletRect.x = tank.x + tank.w / 2 - bulletSize / 2;
    bulletRect.y = tank.y + tank.h / 2 - bulletSize / 2;

    Bullet bullet;
    bullet.rect = bulletRect;
    bullet.angle = angle;
    bullet.owner = owner;
    bullets.push_back(bullet);
}

// Xử lý sự kiện bàn phím cho 2 người chơi, bao gồm di chuyển và bắn đạn
void handleInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        // Xe tăng 1 (dùng mũi tên) và bắn đạn (N)
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
        } else if (e.key.keysym.sym == SDLK_n) { // Bắn đạn cho xe tăng 1
            shootBullet(1, tank1, tank1Angle);
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

        // Xe tăng 2 (dùng WASD) và bắn đạn (T)
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
        } else if (e.key.keysym.sym == SDLK_t) { // Bắn đạn cho xe tăng 2
            shootBullet(2, tank2, tank2Angle);
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

// Cập nhật vị trí và kiểm tra va chạm của đạn
void updateBullets() {
    const double bulletSpeed = 10.0;
    for (auto it = bullets.begin(); it != bullets.end(); ) {
        double rad = it->angle * M_PI / 180.0;
        int dx = static_cast<int>(bulletSpeed * sin(rad));
        int dy = static_cast<int>(-bulletSpeed * cos(rad)); // vì trục y tăng xuống dưới
        it->rect.x += dx;
        it->rect.y += dy;
        bool removeBullet = false;
        // Kiểm tra va chạm với rìa màn hình
        if (it->rect.x < 0 || it->rect.x + it->rect.w > SCREEN_WIDTH ||
            it->rect.y < 0 || it->rect.y + it->rect.h > SCREEN_HEIGHT) {
            removeBullet = true;
        } else {
            // Kiểm tra va chạm với chướng ngại vật
            for (auto obs_it = obstacles.begin(); obs_it != obstacles.end(); ) {
                if (checkCollision(it->rect, *obs_it)) {
                    obs_it = obstacles.erase(obs_it);
                    removeBullet = true;
                    break;
                } else {
                    ++obs_it;
                }
            }
            // Kiểm tra va chạm với xe đối phương
            if (!removeBullet) {
                if (it->owner == 1 && player2Alive && checkCollision(it->rect, tank2)) {
                    player2Alive = false;
                    removeBullet = true;
                } else if (it->owner == 2 && player1Alive && checkCollision(it->rect, tank1)) {
                    player1Alive = false;
                    removeBullet = true;
                }
            }
        }
        if (removeBullet) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
}

// Vẽ toàn bộ các thành phần: chướng ngại vật, xe tăng, đạn
void render() {
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
    // Vẽ đạn
    for (const auto &bullet : bullets) {
        if (bulletTexture) {
            SDL_RenderCopyEx(renderer, bulletTexture, nullptr, &bullet.rect, bullet.angle, nullptr, SDL_FLIP_NONE);
        } else {
            // Nếu chưa có texture, vẽ đạn màu đen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &bullet.rect);
        }
    }

    SDL_RenderPresent(renderer);
}

// Giải phóng các tài nguyên SDL
void closeAll() {
    SDL_DestroyTexture(obstacleTexture);
    obstacleTexture = nullptr;
    SDL_DestroyTexture(tankTexture);
    tankTexture = nullptr;
    SDL_DestroyTexture(tank2Texture);
    tank2Texture = nullptr;
    SDL_DestroyTexture(bulletTexture);
    bulletTexture = nullptr;

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
    tankTexture = loadTexture("tank.png");      // xe tăng 1
    tank2Texture = loadTexture("tank2.png");      // xe tăng 2
    bulletTexture = loadTexture("dan.png");       // texture đạn, nếu không tải được sẽ vẽ đạn màu đen

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
        // Cập nhật vị trí đạn và kiểm tra va chạm
        updateBullets();
        render();
        // Nếu một trong hai xe đã bị tiêu diệt, kết thúc game
        if (!player1Alive || !player2Alive) {
            SDL_Delay(2000);
            quit = true;
        }
        SDL_Delay(16); // khoảng 60 FPS
    }

    closeAll();
    return 0;
}

