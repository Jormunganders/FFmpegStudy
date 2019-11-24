//
// Created by yunrui on 2019/11/22.
//

#ifndef FFMPEG_SDL_USE_H
#define FFMPEG_SDL_USE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdexcept>
#include <string>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int realScreenWidth;
int realScreenHeight;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;

// 练习
int use_sdl() {
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    SDL_Window *window = nullptr;
    window = SDL_CreateWindow("HelloWorld", 100, 100, 640, 640,
                              SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    /*auto screenSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(screenSurface, NULL,
                 SDL_MapRGB(screenSurface->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);
    SDL_Delay(30000);
    SDL_DestroyWindow(window);
    SDL_Quit();*/

    SDL_Renderer *renderer = nullptr;
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // SDL_RENDERER_ACCELERATED： 硬件加速
    // SDL_RENDERER_PRESENTVSYNC：以显示器的刷新率来更新画面
    if (renderer == nullptr) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    SDL_Surface *bmp = nullptr;
    bmp = SDL_LoadBMP("../res/hello.bmp");
    if (bmp == nullptr) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    SDL_Texture *texture = nullptr;
    texture = SDL_CreateTextureFromSurface(renderer, bmp);
    SDL_FreeSurface(bmp);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    // 更新屏幕画面

    /**
     * 必须要给系统一个运行事件循环的机会
     */
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                quit = true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                quit = true;
            }
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

SDL_Texture *LoadBMP(string file) {
    SDL_Surface *loadedImage = nullptr;
    SDL_Texture *texture = nullptr;

    loadedImage = SDL_LoadBMP(file.c_str());
    if (loadedImage != nullptr) {
        texture = SDL_CreateTextureFromSurface(renderer, loadedImage);
        SDL_FreeSurface(loadedImage);
    } else {
        cout << SDL_GetError() << endl;
    }
    return texture;
}

SDL_Texture *LoadImage(string file) {
    SDL_Texture *texture = nullptr;
    texture = IMG_LoadTexture(renderer, file.c_str());
    if (texture == nullptr) {
        throw runtime_error("Filed to load image: " + file
                            + IMG_GetError());
    }
    return texture;
}

void ApplySurface(int x, int y, SDL_Texture *texture,
                  SDL_Renderer *renderer) {
    SDL_Rect pos;   // 指定 Surface 绘制的位置
    pos.x = x;
    pos.y = y;
    SDL_QueryTexture(texture, NULL, NULL, &pos.w, &pos.h);
    SDL_RenderCopy(renderer, texture, NULL, &pos);
}

void waitEvent() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                quit = true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                quit = true;
            }
        }
    }
}

// 练习
int show() {
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    window = SDL_CreateWindow("HelloWorld",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN |
                                                           SDL_WINDOW_ALLOW_HIGHDPI);   // 支持高分屏幕
    if (window == nullptr) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    // 高分屏适配之后获取真实的宽高
    SDL_GL_GetDrawableSize(window, &realScreenWidth, &realScreenHeight);

    cout << "W: " << realScreenWidth << " H: " << realScreenHeight << endl;

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        cout << SDL_GetError() << endl;
        return -1;
    }

    SDL_Texture *background = nullptr, *image = nullptr;
    try {
        background = LoadImage("../res/background.bmp");
        image = LoadImage("../res/image.png");
    } catch (const runtime_error &e) {
        cout << e.what() << endl;
        return -1;
    }
    if (background == nullptr || image == nullptr) {
        return -1;
    }

    SDL_RenderClear(renderer);

    int bW, bH;
    SDL_QueryTexture(background, NULL, NULL, &bW, &bH);

    for (auto i = 0; i < realScreenWidth; i += bW) {
        for (auto j = 0; j < realScreenHeight; j += bH) {
            ApplySurface(i, j, background, renderer);
        }
    }

    int iW, iH;
    SDL_QueryTexture(image, NULL, NULL, &iW, &iH);
    int x = realScreenWidth / 2 - iW / 2;
    int y = realScreenHeight / 2 - iH / 2;
    ApplySurface(x, y, image, renderer);

    SDL_RenderPresent(renderer);

    waitEvent();

    // End
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
// 第四节

#endif //FFMPEG_SDL_USE_H

