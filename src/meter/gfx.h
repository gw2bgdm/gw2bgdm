#include "core/types.h"

// UI sizes.
#define GFX_SMALL 0
#define GFX_NORMAL 1
#define GFX_LARGE 2
#define GFX_LARGER 3
#define GFX_MAX 4

// A 2D UI coordinate.
typedef struct GFXCoord
{
	i32 x;
	i32 y;
} GFXCoord;

// Graphics for a target type.
typedef struct GFXTarget
{
	GFXCoord hp;
	GFXCoord bb;
	i32 dst;
	i32 dps;
} GFXTarget;

// Graphics values for an interface size.
typedef struct GFX
{
	GFXTarget normal;
	GFXTarget gadget;
	GFXTarget attack;
	i32 hp;
	GFXCoord utils;
} GFX;

// Interface values.
static GFX g_gfx[GFX_MAX] =
{
	// Small interface.
	{
		{
			{ -159, 79 },
			{ -39, 99 },
	65,
	145
		},
		{
			{ -122, 100 },
			{ -122, 112 },
	91,
	100
		},
		{
			{ -151, 100 },
			{ -135, 118 },
	152,
	160
		}, 85,
		
		{ 49, 12 }
	},

	// Normal interface.
	{
		{
			{ -176, 88 },
			{ -43, 110 },
	72,
	163
		},
		{
			{ -136, 111 },
			{ -136, 126 },
	101,
	109
		},
		{
			{ -167, 111 },
			{ -150, 134 },
	170,
	178
		}, 94,
		
		{ 53, 12 }
	},

	// Large interface.
	{
		{
			{ -196, 99 },
			{ -47, 123 },
	80,
	184
		},
		{
			{ -150, 124 },
			{ -150, 140 },
	113,
	122
		},
		{
			{ -186, 124 },
			{ -166, 144 },
	189,
	198
		}, 105,
		
		{ 59, 13 }
	},

	// Larger interface.
	{
		{
			{ -216, 109 },
			{ -52, 135 },
	89,
	204
		},
		{
			{ -166, 136 },
			{ -166, 154 },
	124,
	133
		},
		{
			{ -204, 136 },
			{ -182, 163 },
	208,
	216
		}, 122,
		
		{ 64, 14 }
	},
};
