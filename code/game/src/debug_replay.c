#include "debug_replay.h"
#include "camera.h"
#include "entity.h"

#include "cwpack/cwpack.h"

typedef enum {
    RPKIND_KEY,
    
    // NOTE(zaklaus): Special actions
    RPKIND_SPAWN_CAR,
} replay_kind;

typedef struct {
    replay_kind kind;
    pkt_send_keystate pkt;
    uint64_t delay;
} replay_record;

static uint8_t is_recording = false;
static replay_record *records = NULL;
static uint64_t last_record_time = 0.0f;

static uint8_t is_playing = false;
static int record_pos = 0;
static uint64_t playback_time = 0;
static ecs_entity_t mime = 0;
static ecs_entity_t plr = 0;
static ecs_entity_t *temp_actors = NULL;

#define REPLAY_MAGIC 0x421DC97E
#define REPLAY_VERSION 2

static char replay_filename[1024] = {0};
static char replaybuf[sizeof(replay_record)*UINT16_MAX + 32];

void debug_replay_store(void) {
    assert(replay_filename[0]);
    assert(records);
    
    cw_pack_context pc = {0};
    cw_pack_context_init(&pc, replaybuf, sizeof(replaybuf), 0);
    cw_pack_unsigned(&pc, REPLAY_MAGIC);
    cw_pack_unsigned(&pc, REPLAY_VERSION);
    cw_pack_array_size(&pc, zpl_array_count(records));
    
    for (int i = 0; i < zpl_array_count(records); i++) {
        cw_pack_bin(&pc, &records[i], sizeof(replay_record));
    }
    
    zpl_file f = {0};
    zpl_file_create(&f, replay_filename);
    zpl_file_write(&f, replaybuf, pc.current - pc.start);
    zpl_file_close(&f);
}

void debug_replay_load(void) {
    assert(replay_filename[0]);
    
    zpl_file f = {0};
    assert(zpl_file_open(&f, replay_filename) == ZPL_FILE_ERROR_NONE);
    size_t file_size = zpl_file_size(&f);
    zpl_file_read(&f, replaybuf, file_size);
    zpl_file_close(&f);
    
    cw_unpack_context uc = {0};
    cw_unpack_context_init(&uc, replaybuf, file_size, 0);
    
    cw_unpack_next(&uc);
    assert(uc.item.type == CWP_ITEM_POSITIVE_INTEGER && uc.item.as.u64 == REPLAY_MAGIC);
    
    cw_unpack_next(&uc);
    assert(uc.item.type == CWP_ITEM_POSITIVE_INTEGER && uc.item.as.u64 == REPLAY_VERSION);
    
    cw_unpack_next(&uc);
    assert(uc.item.type == CWP_ITEM_ARRAY);
    size_t items = uc.item.as.array.size;
    
    zpl_array_init_reserve(records, zpl_heap(), sizeof(replay_record)*items);
    
    for (size_t i = 0; i < items; i++) {
        cw_unpack_next(&uc);
        assert(uc.item.type == CWP_ITEM_BIN);
        
        replay_record rec = {0};
        zpl_memcopy(&rec, uc.item.as.bin.start, sizeof(replay_record));
        zpl_array_append(records, rec);
    }
}

void debug_replay_start(void) {
    is_recording = true;
    
    if (records) zpl_array_free(records);
    zpl_array_init_reserve(records, zpl_heap(), UINT16_MAX);
    
    last_record_time = zpl_time_rel_ms();
}

void debug_replay_clear(void) {
    if (!records || is_playing || is_recording) return;
    zpl_array_free(records);
    records = NULL;
    record_pos = 0;
}

void debug_replay_cleanup_ents(void) {
    if (!mime) return;
    
    entity_despawn(mime);
    mime = 0;
    
    is_playing = false;
    camera_set_follow(plr);
    
    for (int i = 0; i < zpl_array_count(temp_actors); i++) {
        entity_despawn(temp_actors[i]);
    }
    
    zpl_array_free(temp_actors);
}

void debug_replay_stop(void) {
    is_recording = false;
    is_playing = false;
    record_pos = 0;
    debug_replay_cleanup_ents();
}

void debug_replay_run(void) {
    if (mime) return;
    is_playing = true;
    record_pos = 0;
    playback_time = zpl_time_rel_ms();
    zpl_array_init(temp_actors, zpl_heap());
    
    plr = camera_get().ent_id;
    Position const *p1 = ecs_get(world_ecs(), plr, Position);
    
    mime = entity_spawn(EKIND_MACRO_BOT);
    Position *pos = ecs_get_mut(world_ecs(), mime, Position, NULL);
    *pos = *p1;
    
    ecs_set(world_ecs(), mime, Input, {0});
    
    camera_set_follow(mime);
}

void debug_replay_update(void) {
    if (!is_playing) return;
    if (playback_time >= zpl_time_rel_ms()) return;
    
    replay_record *r = &records[record_pos];
    playback_time = zpl_time_rel() + r->delay;
    
    switch (r->kind) {
        case RPKIND_KEY: {
            Input *i = ecs_get_mut(world_ecs(), mime, Input, NULL);
            i->x = r->pkt.x;
            i->y = r->pkt.y;
            i->use = r->pkt.use;
            i->sprint = r->pkt.sprint;
        }break;
        case RPKIND_SPAWN_CAR: {
            ecs_entity_t e = vehicle_spawn();
            
            Position const *origin = ecs_get(world_ecs(), mime, Position);
            Position *dest = ecs_get_mut(world_ecs(), e, Position, NULL);
            *dest = *origin;
            
            zpl_array_append(temp_actors, e);
        }break;
        default: {
            ZPL_PANIC("unreachable");
        }break;
    }
    
    record_pos += 1;
    
    // NOTE(zaklaus): remove our dummy art exhibist
    if (mime && record_pos == zpl_array_count(records)) {
        debug_replay_cleanup_ents();
    }
}

void debug_replay_record_keystate(pkt_send_keystate state) {
    if (!is_recording) return;
    float record_time = zpl_time_rel_ms();
    
    replay_record rec = {
        .kind = RPKIND_KEY,
        .pkt = state,
        .delay = (record_time - last_record_time),
    };
    
    zpl_array_append(records, rec);
    last_record_time = zpl_time_rel_ms();
}

void debug_replay_special_action(replay_kind kind) {
    assert(kind != RPKIND_KEY);
    if (!is_recording || is_playing) return;
    float record_time = zpl_time_rel_ms();
    
    replay_record rec = {
        .kind = kind,
        .delay = (record_time - last_record_time),
    };
    
    zpl_array_append(records, rec);
    last_record_time = zpl_time_rel_ms();
}