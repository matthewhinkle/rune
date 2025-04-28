#include <stdarg.h>

#define T int
#include "coll.h"
#undef T

#include "str.h"

#include <stdio.h>

int main() {
    printf("Hello World!\n");

    const str foo = str_const("woah, some data and stuff.");

    printf("\"%s\"\n", foo.chars);
    printf("len = %d\n", (int)foo.size);

    return 0;

    // list_int ints = list(int);
    list_int ints = list(int, 0, 1, 2, 3);

    // for (int i = 0; i < 10; i++) {
    //     list_add(&ints, i);
    // }

    const int size = (int)(ints.size + 10) * 10;
    for (int i = 0; i < size; i++) {
        list_insert(&ints, 0, 100 + i);
    }

    for (int i = 0; i < ints.size; i++) {
        printf("list_get(lst, %d) = %d\n", i, list_get(&ints, i));
    }

    list_free(&ints);

    return 0;
}

#if 0

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <stdio.h>
#include <vulkan/vulkan.h>

int main(void) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Vulkan_LoadLibrary(NULL)) {
        printf("SDL Vulkan support is available.\n");
    } else {
        printf("SDL Vulkan support is not available: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create an SDL window with Vulkan support
    SDL_Window *window = SDL_CreateWindow("Window", 800, 600, SDL_WINDOW_VULKAN);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a Vulkan instance
    VkInstance instance;
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan + SDL3 Example",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("Vulkan and SDL3 initialized successfully!\n");

    // Event loop to keep the window open
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                printf("EXITING!\n");
                running = 0;
            }
        }
    }

    // Clean up
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#endif

#if 0
#include <stdio.h>

#include <SDL3/SDL.h>

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    printf("Hello, World!\n");
    return 0;
}
#endif