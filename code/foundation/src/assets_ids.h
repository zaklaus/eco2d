#pragma once

#define ASSET_INVALID 0xFF

#define _ASSETS\
	X(ASSET_EMPTY)\
	X(ASSET_BLANK)\
	X(ASSET_BLOCK_FRAME)\
	X(ASSET_BUILDMODE_HIGHLIGHT)\
	X(ASSET_PLAYER)\
	X(ASSET_THING)\
	X(ASSET_CREATURE)\
	X(ASSET_CREATURE_FOOD)\
	X(ASSET_CHEST)\
	X(ASSET_SPLITTER)\
	X(ASSET_ASSEMBLER)\
	X(ASSET_FURNACE)\
	X(ASSET_CRAFTBENCH)\
	X(ASSET_BLUEPRINT_BEGIN)\
	X(ASSET_BLUEPRINT)\
	X(ASSET_BLUEPRINT_DEMO_HOUSE)\
	X(ASSET_BLUEPRINT_END)\
	X(ASSET_FENCE)\
	X(ASSET_DEV)\
	X(ASSET_GROUND)\
	X(ASSET_DIRT)\
	X(ASSET_WATER)\
	X(ASSET_LAVA)\
	X(ASSET_WALL)\
	X(ASSET_HILL)\
	X(ASSET_HILL_SNOW)\
	X(ASSET_HOLE)\
	X(ASSET_WOOD)\
	X(ASSET_TREE)\
	X(ASSET_COAL)\
	X(ASSET_IRON_ORE)\
	X(ASSET_IRON_INGOT)\
	X(ASSET_IRON_PLATES)\
	X(ASSET_SCREWS)\
	X(ASSET_LOG)\
	X(ASSET_PLANK)\
	X(ASSET_TEST_TALL)\
	X(ASSET_BELT)\
	X(ASSET_BELT_LEFT)\
	X(ASSET_BELT_RIGHT)\
	X(ASSET_BELT_UP)\
	X(ASSET_BELT_DOWN)

typedef enum {
#define X(idx) idx,
    _ASSETS
#undef X
    MAX_INTERNAL_ASSETS,
    NEXT_FREE_ASSET = MAX_INTERNAL_ASSETS,
    MAX_ASSETS = 255
} asset_id;

extern const char *asset_names[];
