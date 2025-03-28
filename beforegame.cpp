#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <cstdlib>

// Kích thước cửa sổ
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main(int argc, char* argv[]) {
    // Khởi tạo SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL không khởi tạo được: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Khởi tạo SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf không khởi tạo được: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    // Khởi tạo SDL_image (hỗ trợ PNG)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image không khởi tạo được: " << IMG_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Tạo cửa sổ và renderer
    SDL_Window* window = SDL_CreateWindow("Game Setup",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Không tạo được cửa sổ: " << SDL_GetError() << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Không tạo được renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    //===========================
    // Màn hình Start Game
    //===========================
    // Load ảnh nền cho Start Game ("background.png")
    SDL_Surface* bgSurface = IMG_Load("background.png");
    if (!bgSurface) {
        std::cerr << "Không load được ảnh nền: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* bgTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
    SDL_FreeSurface(bgSurface);
    if (!bgTexture) {
        std::cerr << "Không tạo được texture cho ảnh nền: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Load font arial.ttf (kích thước 48)
    TTF_Font* fontStart = TTF_OpenFont("arial.ttf", 48);
    if (!fontStart) {
        std::cerr << "Không load được font: " << TTF_GetError() << std::endl;
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Render text "START GAME"
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(fontStart, "START GAME", white);
    if (!textSurface) {
        std::cerr << "Không render được text: " << TTF_GetError() << std::endl;
        TTF_CloseFont(fontStart);
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textW = textSurface->w;
    int textH = textSurface->h;
    SDL_FreeSurface(textSurface);
    if (!textTexture) {
        std::cerr << "Không tạo được texture chữ: " << SDL_GetError() << std::endl;
        TTF_CloseFont(fontStart);
        SDL_DestroyTexture(bgTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Tính vị trí chữ "START GAME" ở giữa màn hình
    SDL_Rect textRect;
    textRect.w = textW;
    textRect.h = textH;
    textRect.x = (WINDOW_WIDTH - textW) / 2;
    textRect.y = (WINDOW_HEIGHT - textH) / 2;
    // Vùng nền đen cho chữ (padding 10)
    const int padding = 10;
    SDL_Rect blackRect = { textRect.x - padding, textRect.y - padding,
                           textRect.w + 2 * padding, textRect.h + 2 * padding };

    // Vòng lặp sự kiện màn hình Start Game
    bool running = true;
    SDL_Event event;
    while (running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    running = false;
                    break;
                }
            }
        }
        SDL_RenderClear(renderer);
        // Vẽ nền (background)
        SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
        // Vẽ vùng đen cho chữ
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &blackRect);
        // Vẽ chữ "START GAME"
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_RenderPresent(renderer);
    }
    // Dọn dẹp tài nguyên màn hình Start Game
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(fontStart);
    SDL_DestroyTexture(bgTexture);

    // Nếu người dùng nhấn X (QUIT) thì thoát luôn
    if (event.type == SDL_QUIT) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 0;
    }

    //===========================
    // Màn hình chọn chế độ chơi (Mode Selection)
    //===========================
    // Load ảnh hướng dẫn "huongdan.png"
    // Chiều rộng = WINDOW_WIDTH, chiều cao = WINDOW_HEIGHT/2, đặt ở nửa trên cửa sổ.
    SDL_Surface* hdSurface = IMG_Load("huongdan.png");
    if (!hdSurface) {
        std::cerr << "Không load được ảnh hướng dẫn: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* hdTexture = SDL_CreateTextureFromSurface(renderer, hdSurface);
    SDL_FreeSurface(hdSurface);
    if (!hdTexture) {
        std::cerr << "Không tạo được texture cho ảnh hướng dẫn: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Vị trí ảnh hướng dẫn (nửa trên cửa sổ)
    SDL_Rect hdRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT / 2 };

    // Load ảnh mũi tên "muiten.png"
    SDL_Surface* mtSurface = IMG_Load("muiten.png");
    if (!mtSurface) {
        std::cerr << "Không load được ảnh mũi tên: " << IMG_GetError() << std::endl;
        SDL_DestroyTexture(hdTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* mtTexture = SDL_CreateTextureFromSurface(renderer, mtSurface);
    SDL_FreeSurface(mtSurface);
    if (!mtTexture) {
        std::cerr << "Không tạo được texture cho mũi tên: " << SDL_GetError() << std::endl;
        SDL_DestroyTexture(hdTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font arial.ttf cho Mode Selection (kích thước 36)
    TTF_Font* fontMode = TTF_OpenFont("arial.ttf", 36);
    if (!fontMode) {
        std::cerr << "Không load được font: " << TTF_GetError() << std::endl;
        SDL_DestroyTexture(mtTexture);
        SDL_DestroyTexture(hdTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Tạo texture cho chữ "1 PLAYER" và "2 PLAYER"
    SDL_Surface* surf1 = TTF_RenderText_Solid(fontMode, "1 PLAYER", white);
    SDL_Surface* surf2 = TTF_RenderText_Solid(fontMode, "2 PLAYER", white);
    if (!surf1 || !surf2) {
        std::cerr << "Không render được text Mode Selection: " << TTF_GetError() << std::endl;
        if(surf1) SDL_FreeSurface(surf1);
        if(surf2) SDL_FreeSurface(surf2);
        TTF_CloseFont(fontMode);
        SDL_DestroyTexture(mtTexture);
        SDL_DestroyTexture(hdTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* tex1 = SDL_CreateTextureFromSurface(renderer, surf1);
    SDL_Texture* tex2 = SDL_CreateTextureFromSurface(renderer, surf2);
    int w1 = surf1->w, h1 = surf1->h;
    int w2 = surf2->w, h2 = surf2->h;
    SDL_FreeSurface(surf1);
    SDL_FreeSurface(surf2);
    if (!tex1 || !tex2) {
        std::cerr << "Không tạo được texture cho text Mode Selection: " << SDL_GetError() << std::endl;
        if(tex1) SDL_DestroyTexture(tex1);
        if(tex2) SDL_DestroyTexture(tex2);
        TTF_CloseFont(fontMode);
        SDL_DestroyTexture(mtTexture);
        SDL_DestroyTexture(hdTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    // Vị trí của chữ ở phần dưới cửa sổ (nửa dưới)
    SDL_Rect rect1 = { WINDOW_WIDTH / 4 - w1 / 2, (WINDOW_HEIGHT * 3) / 4 - h1 / 2, w1, h1 };
    SDL_Rect rect2 = { (WINDOW_WIDTH * 3) / 4 - w2 / 2, (WINDOW_HEIGHT * 3) / 4 - h2 / 2, w2, h2 };

    // Biến lưu lựa chọn (mặc định là 1 PLAYER)
    int selection = 1;
    running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_LEFT) {
                    selection = 1;
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    selection = 2;
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    // Khi xác nhận, gọi file thực thi tương ứng
                    if (selection == 1) {
                        system("1player.exe"); // Hoặc system("./1player") trên Linux
                    } else {
                        system("2player.exe"); // Hoặc system("./2player") trên Linux
                    }
                    running = false;
                    break;
                }
            }
        }
        SDL_RenderClear(renderer);
        // Vẽ ảnh hướng dẫn (nửa trên)
        SDL_RenderCopy(renderer, hdTexture, NULL, &hdRect);
        // Vẽ các text "1 PLAYER" và "2 PLAYER" (nửa dưới)
        SDL_RenderCopy(renderer, tex1, NULL, &rect1);
        SDL_RenderCopy(renderer, tex2, NULL, &rect2);

        // Tính vị trí để vẽ mũi tên
        // Thu nhỏ mũi tên về kích thước 40x40
        int arrowW = 40, arrowH = 40;
        SDL_Rect mtRect;
        mtRect.w = arrowW;
        mtRect.h = arrowH;
        // Canh mũi tên nằm bên trái của text lựa chọn hiện hành, căn giữa theo chiều cao text
        if (selection == 1) {
            mtRect.x = rect1.x - arrowW - 10;
            mtRect.y = rect1.y + (rect1.h - arrowH) / 2;
        } else {
            mtRect.x = rect2.x - arrowW - 10;
            mtRect.y = rect2.y + (rect2.h - arrowH) / 2;
        }
        SDL_RenderCopy(renderer, mtTexture, NULL, &mtRect);

        SDL_RenderPresent(renderer);
    }

    // In ra chế độ được chọn (debug)
    if (selection == 1) {
        std::cout << "Ban da chon che do: 1 PLAYER" << std::endl;
    } else {
        std::cout << "Ban da chon che do: 2 PLAYER" << std::endl;
    }

    // Dọn dẹp tài nguyên màn hình Mode Selection
    SDL_DestroyTexture(tex1);
    SDL_DestroyTexture(tex2);
    TTF_CloseFont(fontMode);
    SDL_DestroyTexture(mtTexture);
    SDL_DestroyTexture(hdTexture);

    // Dọn dẹp SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
