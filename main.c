/*
 * Title: main.c
 * Autor: @ooichu
 * Desc: Test program for SDL_Context
 */

#include "SDL_Context.h"
#include "vec.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH (128)
#define HEIGHT (128)
#define SCALEX 5
#define SCALEY 5

//
// Game data
//

static vec_float pos = {WIDTH / 2, HEIGHT / 2, 0};
static vec_float vel = {0, 0, 0};
static vec_float acc = {0, 0, 0};
static float ang = 0.f, angdt = 0, a_cos = 1.f, a_sin = 0.f, spd = 5;
static SDL_ContextBitmap* bmp;
static const float FRICT = 0.92f;

static void updateTrig(void)
{
	ang += angdt;
	angdt *= FRICT;
	a_cos = cosf(ang);
	a_sin = sinf(ang);
}

static void updatePhysics(void)
{
	vel = vec_Add(vel, acc);
	vel = vec_Mul(vel, FRICT);
	pos = vec_Add(pos, vel);
	acc.x = acc.y = acc.z = 0;
}

//
// Context callbacks
// 

static bool events(SDL_Context* ctx)
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
	
	if (SDL_ContextKeyIsPress(SDL_SCANCODE_ESCAPE))
		return false;

	return true;
}

static bool update(SDL_Context* ctx, float dt)
{
	(void) ctx;

	if (SDL_ContextKeyIsDown(SDL_SCANCODE_W))
	{
		acc.x = a_cos * spd * dt;
		acc.y = a_sin * spd * dt;
	}
	if (SDL_ContextKeyIsDown(SDL_SCANCODE_S))
	{
		acc.x = -a_cos * spd * dt;
		acc.y = -a_sin * spd * dt;
	}
	if (SDL_ContextKeyIsDown(SDL_SCANCODE_A))
		angdt = -2.f * dt;
	if (SDL_ContextKeyIsDown(SDL_SCANCODE_D))
		angdt = +2.f * dt;
	
	updateTrig();
	updatePhysics();

	return true;
}

static void render(SDL_Context* ctx)
{
	SDL_ContextClear(ctx, 0);

//	SDL_ContextBitmapClip(bmp, 0, 0, 15, 15);
//	SDL_ContextBitmapCopyEx(ctx->bitmap, bmp, 0, 0, 2, 3, SDL_ROTATE_270);
	//	SDL_ContextDrawBitmap(ctx, bmp, 16, 16, ang * .5f, 8, 8, 1, 1);
	
	SDL_ContextBitmapClip(bmp, 16, 0, 15, 15);
	SDL_ContextDrawLine(ctx, pos.x, pos.y, a_cos * 8 + pos.x, a_sin * 8 + pos.y, SDL_ContextColor(180, 200, 190, 255));
	SDL_ContextFillCircle(ctx, pos.x, pos.y, 4, SDL_ContextColor(180, 200, 190, 255));
	
	int mouseX = SDL_ContextGetMouseX(ctx);
	int mouseY = SDL_ContextGetMouseY(ctx);
	int mouseWheel = SDL_ContextGetMouseWheel();

	SDL_ContextBitmapCopyEx(SDL_ContextGetFramebuffer(ctx), bmp, 0, 0, 1, 1, 0);
	SDL_ContextBitmapCopyEx(SDL_ContextGetFramebuffer(ctx), bmp, 0, 16, 1, 1, SDL_ROTATE_90);
	SDL_ContextBitmapCopyEx(SDL_ContextGetFramebuffer(ctx), bmp, 0, 32, 1, 1, SDL_ROTATE_180);
	SDL_ContextBitmapCopyEx(SDL_ContextGetFramebuffer(ctx), bmp, 0, 48, 1, 1, SDL_ROTATE_270);
	SDL_ContextDrawLine(ctx, mouseX - abs(mouseWheel), mouseY, mouseX + abs(mouseWheel), mouseY, SDL_ContextColor(200, 64, 120, 255));
	SDL_ContextDrawLine(ctx, mouseX, mouseY - abs(mouseWheel), mouseX, mouseY + abs(mouseWheel), SDL_ContextColor(200, 64, 120, 255));
	
	SDL_ContextDrawPoint(ctx, 0, 0, SDL_ContextColor(255, 255, 255, 0));
	SDL_ContextCopyBuffer(ctx);
}

//
// Initalization & Entry point
//

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;
	srand(time(0));

	bmp = SDL_ContextLoadBitmap("assets/testsheet.bmp");

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

	SDL_Context* const ctx = SDL_CreateContext(
		"Test",
		WIDTH, HEIGHT,
		SCALEX, SCALEY,
		update, render,
		events
	);

	SDL_ContextSetMask(ctx, 0x00FF0024);

	SDL_ContextMainLoop(ctx, SDL_CONTEXT_DEFAULT_FPS_CAP);

	SDL_ContextDestroyBitmap(bmp);
	
	SDL_DestroyContext(ctx);
	
	SDL_Quit();
	return 0;
}

