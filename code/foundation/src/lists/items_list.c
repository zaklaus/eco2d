#include "models/items.h"
#include "world/entity_view.h"
#include "items_list_helpers.h"

static item_desc items[] = {
    { .kind = 0, .max_quantity = 0, },
    ITEM_INGREDIENT(ASSET_COAL, 64, ASSET_FURNACE, ASSET_BELT, 0),
    ITEM_SELF(ASSET_FENCE, 64),
    ITEM_ENERGY(ASSET_WOOD, ASSET_FURNACE, 64, 15.0f),
    ITEM_HOLD(ASSET_TREE, 64),
    ITEM_SELF(ASSET_TEST_TALL, 64),

    // ITEM_BLUEPRINT(ASSET_BLUEPRINT, 1, 4, 4, "]]]]]CF]   ]]]]]"),
    ITEM_BLUEPRINT(ASSET_BLUEPRINT, 1, 4, 4, PROT({ ASSET_WOOD,ASSET_WOOD,ASSET_WOOD,ASSET_WOOD,
                                                    ASSET_WOOD,ASSET_FURNACE,ASSET_CHEST,ASSET_WOOD,
                                                    ASSET_FENCE,ASSET_EMPTY,ASSET_EMPTY,ASSET_WOOD,
                                                    ASSET_WALL,ASSET_EMPTY,ASSET_EMPTY,ASSET_WOOD})),

    ITEM_SELF_DIR(ASSET_BELT, 999),
    ITEM_PROXY(ASSET_BELT_LEFT, ASSET_BELT),
    ITEM_PROXY(ASSET_BELT_RIGHT, ASSET_BELT),
    ITEM_PROXY(ASSET_BELT_UP, ASSET_BELT),
    ITEM_PROXY(ASSET_BELT_DOWN, ASSET_BELT),

    ITEM_ENT(ASSET_CHEST, 32, ASSET_CHEST),
    ITEM_ENT(ASSET_FURNACE, 32, ASSET_FURNACE),
};