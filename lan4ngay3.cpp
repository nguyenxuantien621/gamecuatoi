#include <SDL.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>
#include <vector>

// Kích thước màn hình và bản đồ
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;
const int GRID_SIZE = 12;
const int CELL_SIZE = SCREEN_WIDTH / GRID_SIZE;
const int TANK_SIZE = CELL_SIZE;
const int OBSTACLE_ROWS = 6;
const int OBSTACLE_COLS = 6;
const int ENEMY_COUNT = 5;
const int MOVE_DELAY = 500; // Độ trễ di chuyển xe địch (ms)

// Các hằng số cho đạn
const int BULLET_SIZE_SMALL = TANK_SIZE / 5; // 1/5 của ô vuông
const int BULLET_SIZE_LARGE = TANK_SIZE / 3; // 1/3 của ô vuông
const int BULLET_SPEED_SMALL = 10; // tốc độ đạn nhỏ
const int BULLET_SPEED_LARGE = 7;  // tốc độ đạn lớn

// Cấu trúc lưu thông tin đạn
struct Bullet {
    SDL_Rect rect;
    int dx, dy;    // Vận tốc theo x, y
    bool large;    // true: đạn 3x3, false: đạn 1x1
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* tankTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
SDL_Texture* obstacleTexture = nullptr;

// Xe tăng của người chơi, xuất hiện ở góc trái dưới cùng
SDL_Rect tank = {0, (GRID_SIZE - 1) * CELL_SIZE, TANK_SIZE, TANK_SIZE};
bool playerAlive = true;

// Chướng ngại vật: 6x6
SDL_Rect obstacles[OBSTACLE_ROWS][OBSTACLE_COLS];

// Xe địch: 5 xe tại vị trí cố định ban đầu
SDL_Rect enemies[ENEMY_COUNT] = {
    {CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {3 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {5 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {7 * CELL_SIZE, CELL_SIZE, TANK_SIZE, TANK_SIZE},
    {7 * CELL_SIZE, 2 * CELL_SIZE, TANK_SIZE, TANK_SIZE}
};
bool enemyAlive[ENEMY_COUNT] = { true, true, true, true, true };

double tankAngle = 0.0;          // Góc quay của xe tăng người chơi
double enemyAngles[ENEMY_COUNT] = {0}; // Góc quay của các xe địch
Uint32 lastMoveTime = 0;         // Biến theo dõi thời gian di chuyển địch

// Danh sách đạn đang tồn tại
std::vector<Bullet> bullets;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;

    window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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

// Sắp xếp chướng ngại vật theo lưới: vị trí (1+2*j, 1+2*i)
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

// Hàm bắn đạn; nếu large==true thì bắn đạn 3x3, ngược lại bắn đạn 1x1
void shootBullet(bool large) {
    int bulletSize = large ? BULLET_SIZE_LARGE : BULLET_SIZE_SMALL;
    int speed = large ? BULLET_SPEED_LARGE : BULLET_SPEED_SMALL;
    int dx = 0, dy = 0;
    // Xác định hướng bắn dựa theo góc tankAngle (giá trị giả định: 0, 90, 180, 270)
    if (tankAngle == 0.0)      { dy = -speed; }
    else if (tankAngle == 180.0){ dy = speed; }
    else if (tankAngle == 90.0) { dx = speed; }
    else if (tankAngle == 270.0){ dx = -speed; }

    // Đạn được tạo ra từ tâm xe tăng
    SDL_Rect bulletRect;
    bulletRect.w = bulletSize;
    bulletRect.h = bulletSize;
    bulletRect.x = tank.x + TANK_SIZE / 2 - bulletSize / 2;
    bulletRect.y = tank.y + TANK_SIZE / 2 - bulletSize / 2;

    Bullet bullet;
    bullet.rect = bulletRect;
    bullet.dx = dx;
    bullet.dy = dy;
    bullet.large = large;
    bullets.push_back(bullet);
}

// Xử lý bàn phím: di chuyển và bắn đạn
void handleInput(SDL_Event& event) {
    int dx = 0, dy = 0;
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:    dy = -CELL_SIZE; tankAngle = 0.0; break;
            case SDLK_DOWN:  dy = CELL_SIZE;  tankAngle = 180.0; break;
            case SDLK_LEFT:  dx = -CELL_SIZE; tankAngle = 270.0; break;
            case SDLK_RIGHT: dx = CELL_SIZE;  tankAngle = 90.0; break;
            case SDLK_n:     // bắn đạn 1x1 (nhỏ, màu trắng)
                shootBullet(false);
                break;
            case SDLK_m:     // bắn đạn 3x3 (lớn, màu đỏ)
                shootBullet(true);
                break;
        }

        // Nếu có di chuyển (không bắn đạn) thì kiểm tra va chạm và cập nhật vị trí tank
        if ((dx != 0 || dy != 0) && playerAlive) {
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
}

// Di chuyển xe địch ngẫu nhiên (chỉ di chuyển nếu xe vẫn sống)
void moveEnemies() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastMoveTime < MOVE_DELAY) return;
    lastMoveTime = currentTime;

    int directions[4][2] = {{0, -CELL_SIZE}, {0, CELL_SIZE}, {-CELL_SIZE, 0}, {CELL_SIZE, 0}};
    double angles[4] = {0.0, 180.0, 270.0, 90.0};

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (!enemyAlive[i]) continue; // Nếu xe địch đã chết thì không di chuyển

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

// Cập nhật vị trí của các viên đạn và xử lý va chạm
void updateBullets() {
    for (int i = bullets.size() - 1; i >= 0; i--) {
        Bullet &bullet = bullets[i];
        // Cập nhật vị trí đạn
        bullet.rect.x += bullet.dx;
        bullet.rect.y += bullet.dy;

        bool removeBullet = false;
        bool triggerExplosion = false;

        // Nếu đạn ra khỏi màn hình, kích hoạt nổ
        if (bullet.rect.x < 0 || bullet.rect.x + bullet.rect.w > SCREEN_WIDTH ||
            bullet.rect.y < 0 || bullet.rect.y + bullet.rect.h > SCREEN_HEIGHT) {
            if (bullet.large)
                triggerExplosion = true;
            removeBullet = true;
        }

        // Kiểm tra va chạm với chướng ngại vật
        for (int r = 0; r < OBSTACLE_ROWS && !triggerExplosion; r++) {
            for (int c = 0; c < OBSTACLE_COLS && !triggerExplosion; c++) {
                if (obstacles[r][c].w > 0 && checkCollision(bullet.rect, obstacles[r][c])) {
                    if (bullet.large)
                        triggerExplosion = true;
                    else
                        obstacles[r][c].w = obstacles[r][c].h = 0;
                    removeBullet = true;
                    break;
                }
            }
        }

        // Kiểm tra va chạm với xe địch
        for (int e = 0; e < ENEMY_COUNT && !triggerExplosion; e++) {
            if (enemyAlive[e] && checkCollision(bullet.rect, enemies[e])) {
                if (bullet.large)
                    triggerExplosion = true;
                else {
                    enemyAlive[e] = false;
                    enemies[e].w = enemies[e].h = 0;
                }
                removeBullet = true;
                break;
            }
        }

        if (triggerExplosion && bullet.large) {
            // Tính tâm của viên đạn làm tâm vùng nổ
            int centerX = bullet.rect.x + bullet.rect.w / 2;
            int centerY = bullet.rect.y + bullet.rect.h / 2;
            SDL_Rect explosion;
            explosion.w = 3 * CELL_SIZE;
            explosion.h = 3 * CELL_SIZE;
            explosion.x = centerX - explosion.w / 2;
            explosion.y = centerY - explosion.h / 2;

            // Tiêu diệt chướng ngại vật trong vùng nổ
            for (int r = 0; r < OBSTACLE_ROWS; r++) {
                for (int c = 0; c < OBSTACLE_COLS; c++) {
                    if (obstacles[r][c].w > 0 && checkCollision(explosion, obstacles[r][c])) {
                        obstacles[r][c].w = obstacles[r][c].h = 0;
                    }
                }
            }
            // Tiêu diệt xe địch trong vùng nổ
            for (int e = 0; e < ENEMY_COUNT; e++) {
                if (enemyAlive[e] && checkCollision(explosion, enemies[e])) {
                    enemyAlive[e] = false;
                    enemies[e].w = enemies[e].h = 0;
                }
            }
            // Kiểm tra tự sát: nếu xe tăng nằm trong vùng nổ
            if (checkCollision(tank, explosion)) {
                playerAlive = false;
                tank.w = tank.h = 0;
            }
        }

        if (removeBullet) {
            bullets.erase(bullets.begin() + i);
        }
    }
}

// Hàm render: vẽ xe tăng, xe địch, chướng ngại vật và đạn
void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ xe tăng (nếu còn sống)
    if (playerAlive && tankTexture)
        SDL_RenderCopyEx(renderer, tankTexture, nullptr, &tank, tankAngle, nullptr, SDL_FLIP_NONE);

    // Vẽ xe địch
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemyAlive[i] && enemyTexture)
            SDL_RenderCopyEx(renderer, enemyTexture, nullptr, &enemies[i], enemyAngles[i], nullptr, SDL_FLIP_NONE);
    }

    // Vẽ chướng ngại vật
    for (int i = 0; i < OBSTACLE_ROWS; i++) {
        for (int j = 0; j < OBSTACLE_COLS; j++) {
            if (obstacleTexture && obstacles[i][j].w > 0)
                SDL_RenderCopy(renderer, obstacleTexture, nullptr, &obstacles[i][j]);
        }
    }

    // Vẽ đạn
    for (const auto& bullet : bullets) {
        if (bullet.large)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Đạn lớn màu đỏ
        else
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // Đạn nhỏ màu trắng
        SDL_RenderFillRect(renderer, &bullet.rect);
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
            if (event.type == SDL_QUIT)
                running = false;
            handleInput(event);
        }
        moveEnemies();
        updateBullets();
        render();

        // Kiểm tra điều kiện kết thúc: nếu xe của người chơi chết hoặc tất cả xe địch chết
        bool allEnemiesDead = true;
        for (int i = 0; i < ENEMY_COUNT; i++) {
            if (enemyAlive[i]) {
                allEnemiesDead = false;
                break;
            }
        }
        if (!playerAlive || allEnemiesDead)
            running = false;

        SDL_Delay(16); // ~60 FPS
    }

    close();
    return 0;
}

