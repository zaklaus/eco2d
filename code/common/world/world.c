#include "zpl.h"
#include "librg.h"
#include "modules/general.h"
#include "modules/net.h"
#include "world/world.h"
#include "entity_view.h"

#include "packets/pkt_send_librg_update.h"

typedef struct {
    uint8_t *data;
    uint32_t seed;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t block_size;
    uint16_t chunk_size;
    uint16_t world_size;
    uint64_t tracker_update;
    ecs_world_t *ecs;
    librg_world *tracker;
    world_pkt_reader_proc *reader_proc;
    world_pkt_writer_proc *writer_proc;
} world_data;


static world_data world = {0};

#define WORLD_TRACKER_UPDATE_MS 100

int32_t world_gen();

entity_view world_build_entity_view(int64_t e) {
    ECS_IMPORT(world_ecs(), General);
    entity_view view = {0};
    
    Position *pos = ecs_get(world_ecs(), e, Position);
    view.x = pos->x;
    view.y = pos->y;
    
    return view;
}

int32_t tracker_write_create(librg_world *w, librg_event *e) {
    int64_t owner_id = librg_event_owner_get(w, e);
    int64_t entity_id = librg_event_entity_get(w, e);
    size_t actual_length = librg_event_size_get(w, e);
    char *buffer = librg_event_buffer_get(w, e);
    
    entity_view_pack_struct(buffer, actual_length, world_build_entity_view(entity_id));
    
    return 0;
}

int32_t tracker_write_remove(librg_world *w, librg_event *e) {
    return 0;
}

int32_t tracker_write_update(librg_world *w, librg_event *e) {
    int64_t entity_id = librg_event_entity_get(w, e);
    size_t actual_length = librg_event_size_get(w, e);
    char *buffer = librg_event_buffer_get(w, e);
    
    entity_view_pack_struct(buffer, actual_length, world_build_entity_view(entity_id));
    
    return 0;
}

void world_setup_pkt_handlers(world_pkt_reader_proc *reader_proc, world_pkt_writer_proc *writer_proc) {
    world.reader_proc = reader_proc;
    world.writer_proc = writer_proc;
}

int32_t world_init(int32_t seed, uint16_t block_size, uint16_t chunk_size, uint16_t world_size) {
    if (world.data) {
        return 0;
    }
    
    world.seed = seed;
    world.chunk_size = chunk_size;
    world.world_size = world_size;
    
    world.width = chunk_size * world_size;
    world.height = chunk_size * world_size;
    world.block_size = block_size;
    world.size = world.width * world.height;
    
    if (world.tracker == NULL) {
        world.tracker = librg_world_create();
    }
    
    if (world.tracker == NULL) {
        zpl_printf("[ERROR] An error occurred while trying to create a server world.\n");
        return WORLD_ERROR_TRACKER_FAILED;
    }
    
    /* config our world grid */
    librg_config_chunksize_set(world.tracker, block_size * chunk_size, block_size * chunk_size, 1);
    librg_config_chunkamount_set(world.tracker, world_size, world_size, 1);
    librg_config_chunkoffset_set(world.tracker, LIBRG_OFFSET_MID, LIBRG_OFFSET_MID, 0);
    
    librg_event_set(world.tracker, LIBRG_WRITE_CREATE, tracker_write_create);
    librg_event_set(world.tracker, LIBRG_WRITE_REMOVE, tracker_write_remove);
    librg_event_set(world.tracker, LIBRG_WRITE_UPDATE, tracker_write_update);
    
    world.data = zpl_malloc(sizeof(uint8_t)*world.size);
    world.tracker_update = 0;
    
    if (!world.data) {
        return WORLD_ERROR_OUTOFMEM;
    }
    
    world.ecs = ecs_init();
    ecs_set_entity_range(world.ecs, 0, UINT32_MAX);
    
    ECS_IMPORT(world.ecs, General);

    for (int i = 0; i < world_size * world_size; ++i) {
        ecs_entity_t e = ecs_new(world.ecs, 0);
        ecs_set(world.ecs, e, Chunk, {
            .x = i % world_size,
            .y = i / world_size,
        });

        librg_entity_track(world.tracker, e);
        librg_entity_chunk_set(world.tracker, e, i);
    }

    zpl_printf("[INFO] Created a new server world\n");
    
    return world_gen();
}

int32_t world_destroy(void) {
    librg_world_destroy(world.tracker);
    ecs_fini(world.ecs);
    zpl_mfree(world.data);
    zpl_memset(&world, 0, sizeof(world));
    zpl_printf("[INFO] World was destroyed.\n");
    return WORLD_ERROR_NONE;
}

static void world_tracker_update(void) {
    if (world.tracker_update > zpl_time_rel_ms()) return;
        world.tracker_update = zpl_time_rel_ms() + WORLD_TRACKER_UPDATE_MS;
    
    ECS_IMPORT(world.ecs, Net);
    ecs_query_t *query = ecs_query_new(world.ecs, "Net.ClientInfo");
    
    ecs_iter_t it = ecs_query_iter(query);
    char buffer[8096] = {0};
    
    while (ecs_query_next(&it)) {
        ClientInfo *p = ecs_column(&it, ClientInfo, 1);
        
        for (int i = 0; i < it.count; i++) {
            size_t datalen = 8096;
            int32_t result = librg_world_write(world_tracker(), p[i].peer, buffer, &datalen, NULL);
            
            if (result > 0) {
                zpl_printf("[info] buffer size was not enough, please increase it by at least: %d\n", result);
            } else if (result < 0) {
                zpl_printf("[error] an error happened writing the world %d\n", result);
            }
            
            pkt_world_write(MSG_ID_LIBRG_UPDATE, pkt_send_librg_update_encode(buffer, datalen), 1, p[i].view_id, p[i].peer);
        }
    }
}

int32_t world_update() {
    ecs_progress(world.ecs, 0);

    world_tracker_update();
    return 0;
}

int32_t world_read(void* data, uint32_t datalen, void *udata) {
    if (world.reader_proc) {
        return world.reader_proc(data, datalen, udata);
    }
    return -1;
}

int32_t world_write(pkt_header *pkt, void *udata) {
    if (world.writer_proc) {
        return world.writer_proc(pkt, udata);
    }
    return -1;
}

uint32_t world_buf(uint8_t const **ptr, uint32_t *width) {
    ZPL_ASSERT_NOT_NULL(world.data);
    ZPL_ASSERT_NOT_NULL(ptr);
    *ptr = world.data;
    if (width) *width = world.width;
    return world.size;
}

ecs_world_t * world_ecs() {
    return world.ecs;
}

librg_world * world_tracker() {
    return world.tracker;
}

uint16_t world_block_size(void) {
    return world.block_size;
}

uint16_t world_chunk_size(void) {
    return world.chunk_size;
}

uint16_t world_world_size(void) {
    return world.world_size;
}

#include "world_gen.c"