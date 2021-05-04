#include "zpl.h"

#define ENET_IMPLEMENTATION
#include "enet.h"

#define LIBRG_IMPL
#define LIBRG_CUSTOM_ZPL
#include "librg.h"

#include "system.h"
#include "network.h"
#include "packets/packet.h"
#include "world/world.h"

#include "modules/general.h"
#include "modules/controllers.h"
#include "modules/net.h"

#include "assets.h"

#define NETWORK_UPDATE_DELAY 0.100
#define NETWORK_MAX_CLIENTS 32

static ENetHost *server = NULL;
static zpl_timer nettimer = {0};

WORLD_PKT_WRITER(mp_pkt_writer) {
    if (pkt->is_reliable) {
        return network_msg_send(udata, pkt->data, pkt->datalen);
    }
    else {
        return network_msg_send_unreliable(udata, pkt->data, pkt->datalen);
    }
}

int32_t network_init(void) {
    zpl_timer_set(&nettimer, NETWORK_UPDATE_DELAY, -1, network_server_update);
    return enet_initialize() != 0;
}

int32_t network_destroy(void) {
    enet_deinitialize();
    return 0;
}

int32_t network_server_start(const char *host, uint16_t port) {
    zpl_unused(host);

    ENetAddress address = {0};

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost. */
    address.port = port; /* Bind the server to port. */

    /* create a server */
    server = enet_host_create(&address, NETWORK_MAX_CLIENTS, 2, 0, 0);

    if (server == NULL) {
        zpl_printf("[ERROR] An error occurred while trying to create an ENet server host.\n");
        return 1;
    }

    zpl_printf("[INFO] Started an ENet server...\n");
    zpl_timer_start(&nettimer, NETWORK_UPDATE_DELAY);

    return 0;
}

int32_t network_server_stop(void) {
    zpl_printf("[INFO] Shutting down the ENet server...\n");
    zpl_timer_stop(&nettimer);
    enet_host_destroy(server);
    server = NULL;
    return 0;
}

int32_t network_server_tick(void) {
    ENetEvent event = {0};
    while (enet_host_service(server, &event, 1) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                zpl_printf("[INFO] A new user %d connected.\n", event.peer->incomingPeerID);
                uint16_t peer_id = event.peer->incomingPeerID;
                uint64_t ent_id = network_client_create(event.peer);
                // TODO: Make sure ent_id does not get truncated with large entity numbers.
                event.peer->data = (void*)((uint32_t)ent_id);
                
                pkt_01_welcome table = {.block_size = world_block_size(), .chunk_size = world_chunk_size(), .world_size = world_world_size()};
                pkt_world_write(MSG_ID_01_WELCOME, pkt_01_welcome_encode(&table), 1, event.peer);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT:
            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                zpl_printf("[INFO] A user %d disconnected.\n", event.peer->incomingPeerID);
                ecs_entity_t e = (ecs_entity_t)((uint32_t)event.peer->data);
                network_client_destroy(e);
            } break;

            case ENET_EVENT_TYPE_RECEIVE: {
                if (!world_read(event.packet->data, event.packet->dataLength, event.peer)) {
                    zpl_printf("[INFO] User %d sent us a malformed packet.\n", event.peer->incomingPeerID);
                    ecs_entity_t e = (ecs_entity_t)((uint32_t)event.peer->data);
                    network_client_destroy(e);
                }

                // /* handle a newly received event */
                // librg_world_read(
                //     world_tracker(),
                //     event.peer->incomingPeerID,
                //     (char *)event.packet->data,
                //     event.packet->dataLength,
                //     NULL
                // );
 
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
            } break;

            case ENET_EVENT_TYPE_NONE: break;
        }
    }

    zpl_timer_update(&nettimer);

    return 0;
}

void network_server_update(void *data) {
    // /* iterate peers and send them updates */
    // ENetPeer *currentPeer;
    // for (currentPeer = server->peers; currentPeer < &server->peers[server->peerCount]; ++currentPeer) {
    //     if (currentPeer->state != ENET_PEER_STATE_CONNECTED) {
    //         continue;
    //     }

    //     char buffer[1024] = {0};
    //     size_t buffer_length = 1024;

    //     /* serialize peer's the world view to a buffer */
    //     librg_world_write(
    //         world_tracker(),
    //         currentPeer->incomingPeerID,
    //         buffer,
    //         &buffer_length,
    //         NULL
    //     );

    //     /* create packet with actual length, and send it */
    //     ENetPacket *packet = enet_packet_create(buffer, buffer_length, ENET_PACKET_FLAG_RELIABLE);
    //     enet_peer_send(currentPeer, 0, packet);
    // }
}

uint64_t network_client_create(ENetPeer *peer) {
    ECS_IMPORT(world_ecs(), General);
    ECS_IMPORT(world_ecs(), Controllers);
    ECS_IMPORT(world_ecs(), Net);

    ecs_entity_t e = ecs_new(world_ecs(), 0);
    ecs_add(world_ecs(), e, EcsClient);
    ecs_set(world_ecs(), e, ClientInfo, {peer});
    ecs_set(world_ecs(), e, EcsName, {.alloc_value = zpl_bprintf("client_%d", peer->incomingPeerID) });

    librg_entity_track(world_tracker(), e);
    librg_entity_owner_set(world_tracker(), e, (int64_t)peer);
    librg_entity_radius_set(world_tracker(), e, 2); /* 2 chunk radius visibility */
    // librg_entity_chunk_set(world_tracker(), e, 1);

    return (uint64_t)e;
}

void network_client_destroy(uint64_t ent_id) {
    librg_entity_untrack(world_tracker(), ent_id);
    ecs_delete(world_ecs(), ent_id);
}

static int32_t network_msg_send_raw(ENetPeer *peer, void *data, size_t datalen, uint32_t flags) {
    ENetPacket *packet = enet_packet_create(data, datalen, flags);
    return enet_peer_send(peer, 0, packet);
}

int32_t network_msg_send(ENetPeer *peer, void *data, size_t datalen) {
    return network_msg_send_raw(peer, data, datalen, ENET_PACKET_FLAG_RELIABLE);
}

int32_t network_msg_send_unreliable(ENetPeer *peer, void *data, size_t datalen) {
    return network_msg_send_raw(peer, data, datalen, 0);
}
