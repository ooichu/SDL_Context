/*
 * File: main.c
 * Autor: @ooichu
 * Description: game example for SDL_Context.
 */

#include <SDL_Context.h>
#include <SDL2/SDL_Mixer.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

//
// Constants
//

#define WIDTH 160
#define HEIGHT 144
#define SCALE 4
#define TITLE ("TRAX")

//
// Utility
//

#define swap(T, a, b) \
	do { \
		T __tmp = a; \
		a = b; \
		b = __tmp; \
	} while (0)

#define clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define lerp(a, b, t) ((a) + ((b) - (a)) * t)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

//
// Memory managment
//

static inline void* xmalloc(size_t bytes)
{
	void* ptr = malloc(bytes);
#ifndef NDEBUG
	assert(ptr && "xmalloc: cannot allocate memory!");
	fprintf(stdout, "xmalloc: allocated %lu bytes! Block: 0x%p\n", (unsigned long)bytes, ptr);
#endif
	return ptr;
}

static inline void* xcalloc(size_t n, size_t bytes)
{
	void* ptr = calloc(n, bytes);
#ifndef NDEBUG
	assert(ptr && "xcalloc: cannot allocate memory!");
	fprintf(stdout, "xcalloc: allocated %lu bytes! Block: 0x%p\n", (unsigned long)(n * bytes), ptr);
#endif
	return ptr;
}

static inline void* xrealloc(void* ptr, size_t bytes)
{
	ptr = realloc(ptr, bytes);
#ifndef NDEBUG
	assert(ptr && "xrealloc: cannot reallocate!");
	fprintf(stdout, "xrealloc: reallocated %lu bytes! Block: 0x%p\n", (unsigned long)bytes, ptr);
#endif
	return ptr;
}

static inline void xfree(void* ptr)
{
#ifndef NDEBUG
	if (ptr) fprintf(stdout, "xfree: freed block 0x%p\n", ptr);
#endif
	free(ptr);
}

//
// Globals
//

static struct entity* restrict player_global;
static SDL_ContextBitmap* restrict asset_global;

#define PALLETE_NUM_COLORS
static const uint32_t palette[PALLETE_NUM_COLORS] =
{
	0x000000FF, // BLACK
	0x55415FFF, // VIOLET
	0x646964FF, // GRAY
	0xD77355FF, // ORANGE
	0x508CD7FF, // BLUE
	0x64B964FF, // GREEN
	0xE6C86EFF, // YELLOW
	0xDCF5FFFF  // WHITE
};

#define BLACK (palette[0])
#define VIOLET (palette[1])
#define GRAY (palette[2])
#define ORANGE (palette[3])
#define BLUE (palette[4])
#define GREEN (palette[5])
#define YELLOW (palette[6])
#define WHITE (palette[7])

//
// Maps
//

#define NUM_TILES 7
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
static const unsigned short tiles[NUM_TILES][4] =
{
	{0, 16, 15, 15},
	{16, 16, 15, 15},
	{32, 16, 15, 15},
};

#define MAP_WIDTH 12
#define MAP_HEIGHT 12

static const signed char map1[MAP_WIDTH][MAP_HEIGHT] =
{
	"1111111111",
	"1000000001",
	"1022200001",
	"1000000001",
	"1000222001",
	"1000000001",
	"1000022001",
	"1000000001",
	"1111111111",
};

//
// Vector
//

typedef float vec_type;

struct vec { vec_type x, y; };

#define vec_new(x, y) (struct vec){((vec_type)x), ((vec_type)y)}

static inline struct vec vec_add(struct vec a, struct vec b) { return (struct vec){ a.x + b.x, a.y + b.y }; }
static inline struct vec vec_sub(struct vec a, struct vec b) { return (struct vec){ a.x - b.x, a.y - b.y }; }
static inline struct vec vec_mul(struct vec v, vec_type val) { return (struct vec){ v.x * val, v.y * val }; }
static inline struct vec vec_div(struct vec v, vec_type val) { return (struct vec){ v.x / val, v.y / val }; }

//
// Attribute
//

typedef int16_t attrib_type;

struct attrib { attrib_type min, max, cur; };

enum // types
{
	HP_ATTR,
	DMG_ATTR,
	TIMER_ATTR,
	MAX_ATTR
};

#define attrib_new(min, max, cur) (struct attrib) { min, max, cur }
#define attrib_if_max(a) ((a).cur >= (a).max)
#define attrib_if_min(a) ((a).cur <= (a).min)
#define attrib_inc(a) (++(a).cur)
#define attrib_dec(a) (--(a).cur)
#define attrib_add(a, v) ((a).cur + (v))
#define attrib_sub(a, v) ((a).cur - (v))

//
// Entity
//

struct entity;

typedef void (*entity_upd_func)(SDL_Context* restrict ctx, float dt, struct entity* restrict ent);
typedef void (*entity_ren_func)(SDL_Context* restrict ctx, struct entity* restrict ent);
typedef void (*entity_free_func)(struct entity* ent);
typedef void (*entity_coll_func)(struct entity* restrict ent, struct entity* restrict other);
typedef unsigned long entity_id;

struct collider
{
	enum { CIRCLE, BOX } type;
	union
	{
		vec_type rad;
		struct { vec_type x1, y1, x2, y2; } box;
	};
};

enum
{
	DIR_RIGHT = 0x0,
	DIR_DOWN = 0x2,
	DIR_LEFT = 0x4,
	DIR_UP = 0x6,
	DIR_MAX = 0x8
};

static inline struct vec dir_to_vec(uint8_t dir_n) // crap
{
	// TODO
	
	return vec_new(0, 0);
}

static inline SDL_ContextTransform dir_to_transform(uint8_t dir_n) // crap crap
{
	return dir_n == DIR_RIGHT || dir_n == DIR_RIGHT + 1 ? 0 :
		dir_n == DIR_DOWN || dir_n == DIR_DOWN + 1 ? SDL_ROTATE_270 :
		dir_n == DIR_LEFT || dir_n == DIR_LEFT + 1 ? SDL_ROTATE_180 :
		dir_n == DIR_UP || dir_n == DIR_UP + 1  ? SDL_ROTATE_90 : 0;
}

struct entity
{
	struct vec pos, vel;
	uint8_t dir_n;
	struct collider coll;
	struct attrib attribs[MAX_ATTR];
	entity_id id;
	bool exist;
	enum { BASE = 0, PLAYER, BULLET, ENEMY, } type;
	void* userdata;
	entity_upd_func update;
	entity_ren_func render;
	entity_free_func free;
	entity_coll_func coll_react;
};

static inline struct entity* entity_alloc(void)
{
	struct entity* ent = xcalloc(1, sizeof(struct entity));
	ent->exist = 1;
	return ent;
}

#define entity_free(e) xfree(e)

static void entity_update_phys(struct entity* ent, float dt)
{
	ent->pos.x += ent->vel.x * dt;
	ent->pos.y += ent->vel.y * dt;
}

static bool resolve_collision(struct entity* restrict a, struct entity* restrict b)
{
	if (a->coll.type == CIRCLE && b->coll.type == CIRCLE)
		return pow(a->pos.x - b->pos.x, 2) + pow(a->pos.y - b->pos.y, 2) < pow(a->coll.rad + a->coll.rad, 2);
	else if (a->coll.type == BOX && b->coll.type == BOX)
		return
			a->coll.box.x1 + a->pos.x > b->coll.box.x2 + b->pos.x ||
			a->coll.box.x2 + a->pos.x < b->coll.box.x1 + b->pos.x ||
			a->coll.box.y1 + a->pos.y > b->coll.box.y2 + b->pos.y ||
			a->coll.box.y2 + a->pos.y < b->coll.box.y1 + b->pos.y;
	else if (a->coll.type == BOX && b->coll.type == CIRCLE)
	{
		vec_type dist = pow(fabs(b->coll.rad), 2);
		return
			pow(a->pos.x + a->coll.box.x1 - b->pos.x, 2) + pow(a->pos.y + a->coll.box.y1 - b->pos.y, 2) < dist ||
			pow(a->pos.x + a->coll.box.x1 - b->pos.x, 2) + pow(a->pos.y + a->coll.box.y2 - b->pos.y, 2) < dist ||
			pow(a->pos.x + a->coll.box.x2 - b->pos.x, 2) + pow(a->pos.y + a->coll.box.y1 - b->pos.y, 2) < dist ||
			pow(a->pos.x + a->coll.box.x2 - b->pos.x, 2) + pow(a->pos.y + a->coll.box.y2 - b->pos.y, 2) < dist;
	}
	else if (a->coll.type == CIRCLE && b->coll.type == BOX)
	{
		vec_type dist = pow(fabs(a->coll.rad), 2);
		return
			pow(b->pos.x + b->coll.box.x1 - a->pos.x, 2) + pow(b->pos.y + b->coll.box.y1 - a->pos.y, 2) < dist ||
			pow(b->pos.x + b->coll.box.x1 - a->pos.x, 2) + pow(b->pos.y + b->coll.box.y2 - a->pos.y, 2) < dist ||
			pow(b->pos.x + b->coll.box.x2 - a->pos.x, 2) + pow(b->pos.y + b->coll.box.y1 - a->pos.y, 2) < dist ||
			pow(b->pos.x + b->coll.box.x2 - a->pos.x, 2) + pow(b->pos.y + b->coll.box.y2 - a->pos.y, 2) < dist;
	}
	return false;
}

//
// Entity list
//

#define POOL_INITAL_SIZE (32)
#define POOL_ALLOCATION_STEP (16)

static struct
{
	struct entity** restrict pool, ** restrict pool_buf;
	unsigned long capacity, size;
}
entities;

#define entities_alloc() (entities.pool = xmalloc((entities.capacity = POOL_INITAL_SIZE) * sizeof(struct entity)), entities.pool_buf = xmalloc(entities.capacity * sizeof(struct entity)), entities.size = 0)
inline static void entities_free(void)
{
	for (unsigned long i = 0; i < entities.size; ++i)
		xfree(entities.pool[i]);
	
	xfree(entities.pool);
	xfree(entities.pool_buf);
}

static inline entity_id register_entity(struct entity* ent)
{
	static entity_id next_id;
	if (!ent) return 0;

	if (entities.size >= entities.capacity)
	{
		entities.capacity += POOL_ALLOCATION_STEP;
		entities.pool = xrealloc(entities.pool, sizeof(struct entity[entities.capacity]));
		entities.pool_buf = xrealloc(entities.pool_buf, sizeof(struct entity[entities.capacity]));
	}

	ent->id = ++next_id;
	entities.pool[entities.size++] = ent;
	return next_id;
}

static inline void update_entities(SDL_Context* ctx, float dt)
{
	static unsigned long i, j;
#define ent entities.pool[i]
#define ent2 entities.pool[j]
	/* update collisions between entities */
	for (i = 0; i < entities.size; ++i)
		for (j = i + 1; j < entities.size; ++j)
			if (resolve_collision(ent, ent2) && 0)
			{
				if (ent->coll_react) ent->coll_react(ent, ent2);
				else ent->exist = false;
				if (ent2->coll_react) ent2->coll_react(ent2, ent);
				else ent2->exist = false;
			}
#undef ent2
	/* update entities */
	for (i = j = 0; i < entities.size; ++i)
		if (ent->exist)
		{
			ent->update(ctx, dt, ent);
			entities.pool_buf[j++] = ent;
		}
		else
		{
			if (ent->free) { ent->free(ent); }
			entity_free(ent);
		}
#undef ent
	entities.size = j;
	swap(struct entity**, entities.pool, entities.pool_buf);
}

static inline void render_entities(SDL_Context* ctx)
{
	static unsigned long i;
#define ent entities.pool[i]
	for (i = 0; i < entities.size; ++i)
		ent->render(ctx, ent);
#undef ent
}

//
// Bullet
//

static inline void bullet_update(SDL_Context* restrict ctx, float dt, struct entity* restrict ent)
{
	(void) ctx;
	
	if (attrib_if_max(ent->attribs[TIMER_ATTR]) && false)
		ent->exist = false;
	else
		attrib_inc(ent->attribs[TIMER_ATTR]);
	
	entity_update_phys(ent, dt);
}

static inline void bullet_render(SDL_Context* restrict ctx, struct entity* restrict ent)
{
	SDL_ContextFillCircle(ctx, ent->pos.x, ent->pos.y, ent->coll.rad, VIOLET);
}

static inline struct entity* bullet_init(vec_type x, vec_type y, vec_type vx, vec_type vy)
{
	static float spd = 100.f;
	struct entity* bul = entity_alloc();
	bul->type = BULLET;
	bul->pos.x = x, bul->pos.y = y;
	bul->vel.x = vx * spd, bul->vel.y = vy * spd;
	bul->attribs[TIMER_ATTR] = attrib_new(0, 30, 0);

	bul->coll.type = CIRCLE;
	bul->coll.rad = 3;

	bul->update = bullet_update;
	bul->render = bullet_render;

	return bul;
}

//
// Enemy
//

static inline void enemy_coll_react(struct entity* restrict ent, struct entity* restrict other)
{
	if (other->type == BULLET)
		attrib_dec(ent->attribs[HP_ATTR]);

	if (attrib_if_min(ent->attribs[HP_ATTR]))
		ent->exist = false;
}

static inline void enemy_update(SDL_Context* restrict ctx, float dt, struct entity* restrict ent)
{
	//static const vec_type spd = 2.f;
	(void) ctx;	
	entity_update_phys(ent, dt);
}

static inline void enemy_render(SDL_Context* restrict ctx, struct entity* restrict ent)
{
	SDL_ContextDrawCircle(ctx, ent->pos.x, ent->pos.y, ent->coll.rad, YELLOW);
}

static inline struct entity* enemy_init(vec_type x, vec_type y)
{
	struct entity* enm = entity_alloc();
	enm->type = ENEMY;
	enm->pos.x = x, enm->pos.y = y;
	enm->coll.type = CIRCLE;
	enm->coll.rad = 4;
	enm->attribs[HP_ATTR] = attrib_new(0, 5, 5);
	
	enm->update = enemy_update;
	enm->render = enemy_render;
	enm->coll_react = enemy_coll_react;
	
	return enm;
}

//
// Player
//

static inline void player_update(SDL_Context* restrict ctx, float dt, struct entity* restrict ent)
{
	(void) ctx;
	static const vec_type spd = 80.f;

	if (SDL_ContextKeyIsDown(SDL_SCANCODE_W))
		ent->vel.y = -spd;
	else if (SDL_ContextKeyIsDown(SDL_SCANCODE_S))
		ent->vel.y = +spd;
	else ent->vel.y = 0;

	if (SDL_ContextKeyIsDown(SDL_SCANCODE_A))
		ent->vel.x = -spd;
	else if (SDL_ContextKeyIsDown(SDL_SCANCODE_D))
		ent->vel.x = +spd;
	else ent->vel.x = 0;

	if (SDL_ContextKeyIsPress(SDL_SCANCODE_SPACE))
	{
		printf("Dir: %u\n", (ent->dir_n = (ent->dir_n + 1) % DIR_MAX));
	}
	
	if (SDL_ContextKeyIsPress(SDL_SCANCODE_RSHIFT))
	{
		const struct vec dir = dir_to_vec(ent->dir_n);
		struct entity* restrict const bul = bullet_init(ent->pos.x, ent->pos.y, dir.y, dir.y);
		bullet_update(ctx, dt, bul);
		register_entity(bul);
	}

	entity_update_phys(ent, dt);
}

static inline void player_render(SDL_Context* restrict ctx, struct entity* restrict ent)
{
	SDL_ContextBitmapClip(asset_global, 0, 0, 15, 15);
	SDL_ContextBitmapCopy(ctx->bitmap, asset_global, ent->pos.x - 8, ent->pos.y - 8);
	if (ent->dir_n % 2)
		SDL_ContextBitmapClip(asset_global, 32, 0, 15, 15);
	else
		SDL_ContextBitmapClip(asset_global, 16, 0, 15, 15);
	SDL_ContextBitmapCopyEx(ctx->bitmap, asset_global, ent->pos.x - 8, ent->pos.y - 8, 1, 1, dir_to_transform(ent->dir_n));
}

static inline void player_free(struct entity* ent)
{
	(void) ent;
	player_global = NULL;
}

static inline struct entity* player_init(vec_type x, vec_type y)
{
	if (player_global) return player_global;
	struct entity* pl = entity_alloc();
	pl->type = PLAYER;
	pl->pos.x = x;
	pl->pos.y = y;
	pl->coll.type = CIRCLE;
	pl->coll.rad = 4;
	pl->update = player_update;
	pl->render = player_render;
	pl->free = player_free;
	player_global = pl;
	return pl;
}

//
// Context callbacks
//

static inline bool events(SDL_Context* ctx)
{
	(void) ctx;
	static SDL_Event event;
	
	SDL_ContextResetInput();
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
				return false;
		SDL_ContextUpdateInput(&event);
	}
	
	return !SDL_ContextKeyIsPress(SDL_SCANCODE_ESCAPE);
}	

static inline bool update(SDL_Context* ctx, float dt)
{
	update_entities(ctx, dt);
	return true;
}

static inline void render(SDL_Context* ctx)
{
	SDL_ContextClear(ctx, 0x000000FF);
	for (int y = 0; y < MAP_HEIGHT; ++y)
		for (int x = 0; x < MAP_WIDTH; ++x)
		{
#define tile tiles[map1[y][x] - '0']
			SDL_ContextBitmapClip(asset_global, tile[0], tile[1], tile[2], tile[3]);
			SDL_ContextBitmapCopy(ctx->bitmap, asset_global, x * TILE_WIDTH, y * TILE_HEIGHT);
#undef tile
		}
	render_entities(ctx);
	SDL_ContextCopyBuffer(ctx);
}

int main(int argc, char* argv[const restrict static 1])
{
	(void) argc;
	(void) argv;

	SDL_Init(SDL_INIT_EVERYTHING);

	// load asset
	asset_global = SDL_ContextLoadBitmapWithTransparent("asset.bmp", 0xFF00FFFF);

	// allocate
	SDL_Context* restrict const ctx = SDL_CreateContext(TITLE, WIDTH, HEIGHT, SCALE, SCALE, update, render, events); 
	SDL_ContextSetBlend(ctx, SDL_BLENDMODE_MASK);
	entities_alloc();
	register_entity(player_init(10, 10));
//	register_entity(enemy_init(0, 0));

	// start
	SDL_ContextMainLoop(ctx, 30);

	// deinit
	entities_free();
	SDL_ContextDestroyBitmap(asset_global);
	SDL_DestroyContext(ctx);
	SDL_Quit();

	return 0;
}
