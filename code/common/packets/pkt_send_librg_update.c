#include "packet_utils.h"
#include "packets/pkt_send_librg_update.h"
#include "world/world.h"
#include "game.h"

size_t pkt_send_librg_update_encode(void *data, int32_t data_length) {
    cw_pack_context pc = {0};
    pkt_pack_msg(&pc, 1);
    cw_pack_bin(&pc, data, data_length);
    return pkt_pack_msg_size(&pc);
}

int32_t pkt_send_librg_update_handler(pkt_header *header) {
    cw_unpack_context uc = {0};
    pkt_unpack_msg(&uc, header, 1);
    cw_unpack_next(&uc);

    if (uc.item.type != CWP_ITEM_BIN)
        return -1;
    
    world_view *view = game_world_view_get(header->view_id);
    
    int32_t state = librg_world_read(view->tracker, header->view_id, uc.item.as.bin.start, uc.item.as.bin.length, NULL);
    if (state < 0) zpl_printf("[ERROR] world read error: %d\n", state);
    
    return state;
}