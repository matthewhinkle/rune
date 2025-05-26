#if defined(__cplusplus)
#error "This file must be compiled as C, not C++"
#endif // no cpp assert

#define R_DEBUG 1

#include <stdarg.h>

#include "std.h"
#include "str.h"

#include <stdio.h>

#define T int
#include "coll.h"

#include <string.h>
#undef T

#define RUNE_CLI

#ifdef RUNE_CLI
int main() {
    char * upper_str = str("HELLO, WORLD! from matt");
    printf("lower_str = %s\n", upper_str);
    str_lower(upper_str);
    printf("lower_str = %s\n", upper_str);
    str_free(upper_str);

    printf("what happens here = %s\n", str_lower(str("HELLO, world!")));

    char * s = str("0, 1, 2, 3, 4, 5, 6, 7, 8, 9");
    char ** strings = str_split(s, ", ");

    char * next = strings[0];
    size_t i = 0;
    while (next != nullptr) {
        printf("next = %s\n", next);
        str_free(next);
        i++;
        next = strings[i];
    }
    free(strings);

    char * result = str_join(", ", "1", "2", "3", "4", "5", "6", "7", "8", "9");
    printf("result = %s\n", result);
    str_free(result);

    const char * strs[] = {"hello", "world", "this", "is", "a", "test", nullptr};

    char * result2 = str_join(" !:! ", strs);
    printf("result2 = %s\n", result2);
    str_free(result2);

    char * cat = str_cat("foo", "bar", "baz");
    printf("cat = %s\n", cat);
    str_free(cat);

    LIST(int) lst = list(int, 1, 2, 3, 4, 5);

    list_insert(&lst, 1, 17);
    for (int i_list = 0; i_list < lst.size; i_list++) {
        printf("lst[%d] = %d\n", i_list, list_get(&lst, i_list));
    }
    list_free(&lst);

    return 0;
}
#endif

#ifdef RUNE_GUI
#if 0
list_int lst = list(int, 1, 2, 3, 4, 5);

for (int i = 0; i < lst.size; i++) {
    printf("lst[%d] = %d\n", i, list_get(&lst, i));
}
#endif

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