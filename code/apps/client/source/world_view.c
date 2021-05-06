#include "world_view.h"
#include "librg.h"

int32_t tracker_read_create(librg_world *w, librg_event *e) {
    int64_t owner_id = librg_event_owner_get(w, e);
    int64_t entity_id = librg_event_entity_get(w, e);
    zpl_printf("[INFO] An entity %d was created for owner: %d\n", (int)entity_id, (int)owner_id);
    world_view *view = (world_view*)librg_world_userdata_get(w);
    
    // TODO(zaklaus): receive real data
    entity_view_update_or_create(&view->entities, entity_id, (entity_view){0});
    return 0;
}

int32_t tracker_read_remove(librg_world *w, librg_event *e) {
    int64_t owner_id = librg_event_owner_get(w, e);
    int64_t entity_id = librg_event_entity_get(w, e);
    zpl_printf("[INFO] An entity %d was removed for owner: %d\n", (int)entity_id, (int)owner_id);
    world_view *view = (world_view*)librg_world_userdata_get(w);

    entity_view_destroy(&view->entities, entity_id);
    return 0;
}

int32_t tracker_read_update(librg_world *w, librg_event *e) {
    int64_t entity_id = librg_event_entity_get(w, e);
    size_t actual_length = librg_event_size_get(w, e);
    char *buffer = librg_event_buffer_get(w, e);
    
    // TODO(zaklaus): update real data
    return 0;
}

world_view world_view_create(void) {
    world_view view = {0};
    view.tracker = librg_world_create();
    entity_view_init(&view.entities);
    return view;
}

void world_view_init(world_view *view, uint64_t ent_id, uint16_t block_size, uint16_t chunk_size, uint16_t world_size) {
    view->owner_id = ent_id;
    librg_config_chunksize_set(view->tracker, block_size * chunk_size, block_size * chunk_size, 1);
    librg_config_chunkamount_set(view->tracker, world_size, world_size, 1);
    librg_config_chunkoffset_set(view->tracker, LIBRG_OFFSET_MID, LIBRG_OFFSET_MID, 0);
    
    librg_event_set(view->tracker, LIBRG_READ_CREATE, tracker_read_create);
    librg_event_set(view->tracker, LIBRG_READ_REMOVE, tracker_read_remove);
    librg_event_set(view->tracker, LIBRG_READ_UPDATE, tracker_read_update);
    librg_world_userdata_set(view->tracker, view);
}

void world_view_destroy(world_view *view) {
    librg_world_destroy(view->tracker);
    entity_view_free(&view->entities);
}