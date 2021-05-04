#define ZPL_IMPL
#include "zpl.h"

#include "system.h"
#include "world/world.h"
#include "network.h"
#include "utils/options.h"
#include "signal_handling.h"

#include "flecs/flecs.h"
#include "flecs/flecs_dash.h"
#include "flecs/flecs_systems_civetweb.h"
#include "flecs/flecs_os_api_stdcpp.h"

#define DEFAULT_WORLD_SEED 302097
#define DEFAULT_BLOCK_SIZE 64 /* amount of units within a block (single axis) */
#define DEFAULT_CHUNK_SIZE 3 /* amount of blocks within a chunk (single axis) */
#define DEFAULT_WORLD_SIZE 8 /* amount of chunks within a world (single axis) */

#define IF(call) do { \
    if (call != 0) { \
        zpl_printf("[ERROR] A call to a function %s failed\n", #call); \
        return 1; \
    } \
} while (0)

zpl_global zpl_b32 is_running = true;

int main(int argc, char** argv) {
    zpl_opts opts={0};
    zpl_opts_init(&opts, zpl_heap(), argv[0]);

    zpl_opts_add(&opts, "?", "help", "the HELP section", ZPL_OPTS_FLAG);
    zpl_opts_add(&opts, "p", "preview-map", "draw world preview", ZPL_OPTS_FLAG);
    zpl_opts_add(&opts, "s", "seed", "world seed", ZPL_OPTS_INT);
    zpl_opts_add(&opts, "r", "random-seed", "generate random world seed", ZPL_OPTS_FLAG);
    zpl_opts_add(&opts, "bs", "block-size", "amount of units within a block (single axis)", ZPL_OPTS_INT);
    zpl_opts_add(&opts, "cs", "chunk-size", "amount of blocks within a chunk (single axis)", ZPL_OPTS_INT);
    zpl_opts_add(&opts, "ws", "world-size", "amount of chunks within a world (single axis)", ZPL_OPTS_INT);

    uint32_t ok = zpl_opts_compile(&opts, argc, argv);

    if (!ok) {
        zpl_opts_print_errors(&opts);
        zpl_opts_print_help(&opts);
        return -1;
    }

    int32_t seed = zpl_opts_integer(&opts, "seed", DEFAULT_WORLD_SEED);
    uint16_t block_size = zpl_opts_integer(&opts, "block-size", DEFAULT_BLOCK_SIZE);
    uint16_t chunk_size = zpl_opts_integer(&opts, "chunk-size", DEFAULT_CHUNK_SIZE);
    uint16_t world_size = zpl_opts_integer(&opts, "world-size", DEFAULT_WORLD_SIZE);

    if (zpl_opts_has_arg(&opts, "random-seed")) {
        zpl_random rnd={0};
        zpl_random_init(&rnd);
        seed = zpl_random_gen_u32(&rnd);
        zpl_printf("Seed: %u\n", seed);
    }

    if (zpl_opts_has_arg(&opts, "preview-map")) {
        generate_minimap(seed, block_size, chunk_size, world_size);
        return 0;
    }
    
    game_init(1, seed, block_size, chunk_size, world_size);

    while (is_running) {
        game_update();
    }
    
    game_shutdown();

    return 0;
}

void platform_shutdown(void) {
    is_running = false;
}
