#include <stdarg.h>

#define T int
#include "coll.h"
#undef T

#include "str.h"

#include <stdio.h>

#define RUNE_CLI

#ifdef RUNE_CLI
int main() {
#if 1
    list_int lst = list(int, 1, 2, 3, 4, 5);

    for (int i = 0; i < lst.size; i++) {
        printf("lst[%d] = %d\n", i, list_get(&lst, i));
    }

    str foo = cstr("this");
    printf("foo = %s\n", foo.chars);

    sadd(&foo, cstr(" a cool -str"));
    printf("foo = %s\n", scstr(&foo));

    sadd(&foo, " why \n\t\t\n\t\t { MORE COOL STUFF }");
    printf("foo = %s\n", foo.chars);
    sadd(&foo, " why \n\t\t\t\t ok now    that is just showing off");
    printf("foo = %s\n", foo.chars);

    printf("beg = %c\n", *sfirst_s(&foo));
    printf("end = %c\n", *(slast_s(&foo)));

    sfree(&foo);
    printf("foo = %s\n", foo.chars);
#endif
    return 0;
}
#endif

#ifdef RUNE_GUI

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
    SDL_Window * window = SDL_CreateWindow("Window", 800, 600, SDL_WINDOW_VULKAN);
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