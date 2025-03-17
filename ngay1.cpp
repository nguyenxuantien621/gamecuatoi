
#include <SDL2/SDL.h>
#include <iostream>

// Định nghĩa kích thước cửa sổ
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

class Game {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;

public:
    Game() : window(nullptr), renderer(nullptr), running(false) {}

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        running = true;
        return true;
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // Vẽ map ở đây
        SDL_RenderPresent(renderer);
    }

    void run() {
        SDL_Event event;
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }
            render();
        }
    }

    ~Game() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    Game game;
    if (!game.init()) {
        return -1;
    }
    game.run();
    return 0;
}
