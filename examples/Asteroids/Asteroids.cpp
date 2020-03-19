/*
 * Title: Astroids
 * Autor: @ooichu
 * Description: C++ game example for SDL_Context library.
 * Warning: Bad C++ code! Sorry
 */

#include <SDL_Context.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <cstdint>

//
// Utility
//

enum Colors
{
	BLACK = 0x000000FF,
	WHITE = 0xFFFFFFFF
};

inline float lerp(const float& a, const float& b, const float& t) noexcept { return a + (b - a) * t; }

//
// Basic components
//

template <typename T>
struct vec3 final
{
	T x, y, z;	

public:
	inline vec3(const T& _x = 0, const T& _y = 0, const T& _z = 0) : x(_x), y(_y), z(_z) {}
	inline vec3(const vec3<T>& copy) : x(copy.x), y(copy.y), z(copy.z) {}

public :
	inline vec3 operator+(vec3<T>& v) const noexcept { return vec3(x + v.x, y + v.y, z + v.z); }
	inline vec3& operator+=(vec3<T>& v) noexcept { x += v.x; y += v.y; z += v.z; return *this; }
	inline vec3 operator-(vec3<T>& v) const noexcept { return vec3(x - v.x, y - v.y, z - v.z); }
	inline vec3& operator-=(vec3<T>& v) noexcept { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline vec3 operator*(const T& v) const noexcept { return vec3(x * v, y * v, z * v); }
	inline vec3& operator*=(const T& v) noexcept { x *= v; y *= v; z *= v; return *this; }
	inline vec3 operator/(const T& v) const noexcept { return vec3(x / v, y / v, z / v); }
	inline vec3& operator/=(const T& v) noexcept { x /= v; y /= v; z /= v; return *this; }
	inline vec3 operator-(int) const noexcept { return vec3(-x, -y, -z); }
	inline vec3 operator+(int) const noexcept { return *this; }
};

class Engine;
class Entity;

//
// Defines and typedefs
//

using ptrEntity = std::unique_ptr<Entity>;
using ptrParticle = std::unique_ptr<Entity>;
using EntityID = unsigned long long;
using EntityVecType = float;
using EntityVec = vec3<EntityVecType>;

static constexpr EntityVecType FRICT = 0.999f;

//
// Entity
//

enum class EntityType : unsigned char
{
	ENTITY,
	ASTEROID,
	BULLET,
	PLAYER
};

class Entity
{
	friend Engine;
	static inline EntityID getNextID() noexcept 
	{
		static EntityID id_counter;
		return id_counter++;
	}
	const EntityID id;

protected: // entity prop's
	float ang = 0.f, cos = 1.f, sin = 0.f, rad = 0.f;
	EntityVec pos, vel{0}, acc{0};
	bool exist = true, invisible = false;
	EntityType type = EntityType::ENTITY;

public:
	inline Entity() : id(getNextID()), pos{0.f, 0.f, 0.f} { }
	inline Entity(const EntityVec& v) : id(getNextID()), pos{v.x, v.y, v.z} {}
	virtual ~Entity() {}

public:
	virtual void update(float dt) = 0;
	virtual void render() = 0;
	virtual void finalize() {}
	virtual void onCollision(const ptrEntity&) {}

protected:
	inline void updateTrig() noexcept
	{
		this->cos = cosf(ang);
		this->sin = sinf(ang);
	}

	inline void updatePhysics() noexcept
	{
		pos += vel = (vel + acc) * FRICT;
		acc = {0};
	}

public:
	inline const EntityID& getID() const noexcept { return id; }
	inline const bool& isExist() const noexcept { return exist; }
	inline const EntityVec& getPos() const noexcept { return pos; }
	inline const EntityVecType& getRad() const noexcept { return rad; }
	inline const EntityType& getType() const noexcept { return type; }

public:
	inline void remove() noexcept { exist = false; }
};

//
// Engine "singleton"
//

typedef void (*spawnCallbackType());

void randomSpawn();

class Engine final
{
public:
	static constexpr unsigned width = 196, height = 196, scale = 4;
	static constexpr uint8_t fontWidth = 4, fontHeight = 4;
	static constexpr const char* const title = "Asteroids";

	enum State
	{
		MENU,
		PLAY,
		END
	};

private: // game data
	inline static std::vector<ptrEntity> entities;
	inline static std::vector<ptrParticle> particles;
	inline static SDL_Context* ctx = nullptr;
	inline static bool running = false;
	inline static State state = State::MENU;
	inline static SDL_ContextBitmap* font = nullptr;
	inline static unsigned level = 0;

private:
	inline static unsigned long score = 0;
	inline static std::vector<std::pair<std::string, unsigned long>> scoretable;

public:
	inline static void addEntity(Entity* const& entity) noexcept { entities.push_back(ptrEntity(entity)); }
	inline static void addParticle(Entity* const& particle) noexcept { particles.push_back(ptrParticle(particle)); }
	inline static void drawString(const std::string& str, int x, int y) noexcept
	{
		for (int i = 0; str[i] != '\0'; ++i)
		{
			int c = toupper(str[i]) - '0';
			if (c >= 0 && c < 43/*symbols*/)
			{
				SDL_ContextBitmapClip(font, c * fontWidth, 0, fontWidth - 1, fontHeight - 1);
				SDL_ContextBitmapCopy(ctx->bitmap, font, x + i * fontWidth, y);
			}
		}
	}

public:
	inline static SDL_Context* const& getContext() noexcept { return ctx; }
	inline static const State& getState() noexcept { return state; }
	inline static const unsigned long& getScore() noexcept { return score; }
	inline static const unsigned& getLevel() noexcept { return level; }
	inline static void incrementScore(const unsigned long& inc) noexcept { score += inc; }
	inline static void incrementLevel() noexcept { ++level; }

private: // Font (TTF font support not finished :P)
	inline static void initFont() noexcept
	{
		static constexpr uint8_t symbols = 43;
		static constexpr uint16_t rawFont[symbols] =
		{
			0xEAAE, /* 0 */ 0x4C4E, /* 1 */
			0xE24E, /* 2 */ 0xE62E, /* 3 */
			0xAAE2, /* 4 */ 0xEC2E, /* 5 */
			0xECAE, /* 6 */ 0xE244, /* 7 */
			0xEEAE, /* 8 */ 0xEA6E, /* 9 */
			0x0404, /* : */ 0x040C, /* ; */
			0x0484, /* < */ 0x0E0E, /* = */
			0x0424, /* > */ 0xE204, /* ? */
			0x0EAC, /* @ */ 0xEAEA, /* A */
			0xCEAE, /* B */ 0xE88E, /* C */
			0xCAAE, /* D */ 0xEC8E, /* E */
			0xE8C8, /* F */ 0xE8AE, /* G */
			0xAEAA, /* H */ 0xE44E, /* I */
			0xE2A6, /* J */ 0xACCA, /* K */
			0x888E, /* L */ 0xAEAA, /* M */
			0xCAAA, /* N */ 0x4AA4, /* O */
			0xEAE8, /* P */ 0x4AA6, /* Q */
			0xEACA, /* R */ 0x682C, /* S */
			0xE444, /* T */ 0xAAAE, /* U */
			0xAAA4, /* V */ 0xAAEE, /* W */
			0xA44A, /* X */ 0xAE2E, /* Y */
			0xE82E, /* Z */
		};

		if (font) return;
		
		font = SDL_ContextCreateBitmap(fontWidth * symbols, fontHeight);
		for (int i = 0; i < symbols; ++i)
			for (int y = 0; y < fontWidth; ++y)
				for (int x = 0; x < fontHeight; ++x)
					SDL_ContextBitmapDrawPoint(font, x + i * fontWidth, y, rawFont[i] & (1 << ((fontWidth - 1 - x) + (fontHeight - 1 - y) * fontWidth)) ? (uint32_t)Colors::WHITE : 0);
	}

public:
	inline static void changeState(const State& st) noexcept
	{
		switch (state = st)
		{
			case State::MENU:
				ctx->update = menuUpdate;
				ctx->render = menuRender;
				break;
			case State::PLAY:
				ctx->update = gameUpdate;
				ctx->render = gameRender;
				break;
			case State::END:
				ctx->update = endUpdate;
				ctx->render = endRender;
				break;
			default: break;
		}
	}

private: // Context callbacks (game states)
	inline static bool events(SDL_Context* ctx) noexcept // shared
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
	
		if (SDL_ContextKeyIsPress(SDL_SCANCODE_ESCAPE) || !running)
			return false;
		
		return true;
	}

	//
	// Menu state
	//

	inline static bool menuUpdate(SDL_Context* ctx, float dt) noexcept
	{
		(void) ctx;
		(void) dt;

		if (SDL_ContextKeyIsPress(SDL_SCANCODE_SPACE))
			changeState(State::PLAY);

		return true;
	}

	inline static void menuRender(SDL_Context* ctx) noexcept
	{
		SDL_ContextClear(ctx, Colors::BLACK);
		drawString("example for sdlctx library @ooichu", width / 2 - 17 * fontWidth, 1);
		drawString("<< asteroids >>", width / 2 - 7 * fontWidth, height / 2);
		drawString("press <space> to start", width / 2 - 11 * fontWidth, height - height / 4);
		SDL_ContextCopyBuffer(ctx);
	}

	//
	// Game state
	//

	inline static bool gameUpdate(SDL_Context* ctx, float dt) noexcept
	{
		(void) ctx;
		static EntityID i, j;
	
		// detect collision
		for (i = 0; i < entities.size(); ++i)
		{
			auto& ent1 = entities[i];
			const auto& pos1 = ent1->getPos();
			for (j = i + 1; j < entities.size(); ++j)
			{
				auto& ent2 = entities[j];
				const auto& pos2 = ent2->getPos();
				if (std::pow(pos1.x - pos2.x, 2) + std::pow(pos1.y - pos2.y, 2) <= std::pow(ent1->getRad() + ent2->getRad(), 2))
				{
					// reaction to collision
					if (!ent1->invisible) ent1->onCollision(ent2);
					if (!ent2->invisible) ent2->onCollision(ent1);
				}
			}
		}

		// update entities
		for (i = 0; i < entities.size(); ++i)
		{
			auto& ent = entities[i];
			if (ent->isExist())
			{
				ent->update(dt);
				ent->pos.x = ent->pos.x >= Engine::width ? ent->pos.x - Engine::width : ent->pos.x < 0 ? Engine::width - 1 + ent->pos.x : ent->pos.x;
				ent->pos.y = ent->pos.y >= Engine::height ? ent->pos.y - Engine::height : ent->pos.y < 0 ? Engine::height - 1 + ent->pos.y : ent->pos.y;
			}
			else
			{
				ent->finalize();
				entities.erase(entities.begin() + i);
			}
		}

		// update particles
		for (i = 0; i < particles.size(); ++i)
		{
			auto& prt = particles[i];
			if (prt->isExist())
			{
				prt->update(dt);
				prt->pos.x = prt->pos.x >= Engine::width ? prt->pos.x - Engine::width : prt->pos.x < 0 ? Engine::width - 1 + prt->pos.x : prt->pos.x;
				prt->pos.y = prt->pos.y >= Engine::height ? prt->pos.y - Engine::height : prt->pos.y < 0 ? Engine::height - 1 + prt->pos.y : prt->pos.y;
			}
			else
				particles.erase(particles.begin() + i);
		}

		if (entities.size() == 1) // if player alone
			randomSpawn(); // next level

		return true;
	}

	inline static void gameRender(SDL_Context* ctx) noexcept
	{
		SDL_ContextClear(ctx, Colors::BLACK);

		for (auto& ent : entities)
			ent->render();

		for (auto& prt: particles)
			prt->render();
		
		const std::string& str = std::string("Score: ") + std::to_string(score);
		drawString(str.c_str(), width / 2 - str.size() / 2 * fontWidth, 1);
		
		SDL_ContextCopyBuffer(ctx);
	}

	//
	// Ending state
	//

	inline static bool endUpdate(SDL_Context* ctx, float dt) noexcept
	{
		(void) ctx;
		(void) dt;

		if (SDL_ContextKeyIsPress(SDL_SCANCODE_SPACE))
			changeState(State::MENU);

		return true;
	}
	
	inline static void endRender(SDL_Context* ctx) noexcept
	{
		SDL_ContextClear(ctx, Colors::BLACK);
		drawString("Game over!", width / 2 - 5 * fontWidth, height / 2);
		SDL_ContextCopyBuffer(ctx);
	}

public:
	Engine() = delete;
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	~Engine() = delete;

public: // Run / shutdown
	static inline void run() noexcept
	{
		if (ctx) return;
		if ((ctx = SDL_CreateContext(title, width, height, scale, scale, menuUpdate, menuRender, events)))
		{	
			ctx->bitmap->blendMode = SDL_BLENDMODE_BLEND;
			initFont();
			score = 0;
			running = true;
			SDL_ContextMainLoop(ctx, SDL_CONTEXT_DEFAULT_FPS_CAP);
			SDL_DestroyContext(ctx);
			ctx = nullptr;
			particles.clear();
			entities.clear();
			SDL_ContextDestroyBitmap(font);
		}
		else
		{
			std::cout << "Error on creating context! :(" << std::endl;
			std::terminate();
		}
	}
	
	static inline void shutdown() noexcept { running = false; }
};

//
// Particles
//

class Pufft;

//
// Pufft
//

class Pufft : public Entity
{
	static constexpr EntityVecType spd = 3.f;
	unsigned short timer;

public:
	static constexpr float radDefault = 4.f;
	static constexpr int timeDefault = 30;

public:
	using Entity::Entity;
	inline Pufft(const EntityVec& pos, const EntityVec& v, const float& r = radDefault, const unsigned short& t = timeDefault) :
		Entity::Entity(pos), timer(t)
	{
		vel.x = v.x, vel.y = v.y, vel.z = v.z;
		vel *= spd;
		rad = r;
	}

	inline void update(float dt) noexcept override 
	{
		(void) dt;

		if (timer-- == 0) exist = false;
		rad = lerp(rad, 0, 0.1f);

		updateTrig();
		updatePhysics();
	}
	
	inline void render() noexcept override 
	{
		SDL_ContextFillCircle(Engine::getContext(), pos.x, pos.y, rad, Colors::WHITE);
	}
};

//
// Asteroid
//

class Asteroid final : public Entity
{
	static constexpr unsigned char numVertsMax = 9.f, sizeMax = 12;
	static constexpr EntityVecType spd = 2.f;
	unsigned char numVerts;
	std::vector<float> mesh;
	float angdt;

private:
	inline void generateDirection() noexcept
	{
		ang = (float)(rand() % 360) / 360.f * (M_PI * 2.f);
		updateTrig();
		vel.x = cos;
		vel.y = sin;
		angdt = (float)(rand() % 1000 - 500) / 10000.f;
	}

	inline void generateMesh() noexcept
	{
		numVerts = (rand() % (numVertsMax - 3)) + 3;
		for (int i = -180; i < 180; i += 360 / numVerts)
		{
			const float a = M_PI * i / 180;
			mesh.push_back(cosf(a) * rad);
			mesh.push_back(sinf(a) * rad);
		}
	}

public:
	inline Asteroid(const EntityVec& pos) : Entity::Entity(pos)
	{
		generateDirection();
		rad = (rand() % (sizeMax - 2)) + 2;
		generateMesh();
		type = EntityType::ASTEROID;
	}

	inline Asteroid(const EntityVec& pos, const float& r) : Entity::Entity(pos)
	{
		generateDirection();
		rad = r;
		generateMesh();
		type = EntityType::ASTEROID;
	}

	inline void render() noexcept override
	{
		const unsigned& max = mesh.size();
		for (unsigned i = 0; i < max; i += 2)
		{
			const float& x1 = mesh[i + 0], &y1 = mesh[i + 1];
			const float& x2 = mesh[(i + 2) % max], &y2 = mesh[(i + 3) % max];
			SDL_ContextDrawLine(Engine::getContext(),
				pos.x + (x1 * cos - y1 * sin),
				pos.y + (x1 * sin + y1 * cos),
				pos.x + (x2 * cos - y2 * sin),
				pos.y + (x2 * sin + y2 * cos),
				Colors::WHITE);
		}
	}

	inline void finalize() noexcept override
	{
		float tmp;
		for (int i = -180; i < 180; i += 50 + rand() % 20)
		{
			tmp = M_PI * (float)i / 180.f;
			Engine::addParticle(new Pufft(vec3(pos.x, pos.y), vec3(cosf(tmp), sinf(tmp)), 3.f, 13));
		}
		
		if (rad < sizeMax / 3)
			return;
		else
		{
			Engine::addEntity(new Asteroid(EntityVec(pos.x + cos * rad, pos.y - sin * rad), rad / 2));
			Engine::addEntity(new Asteroid(EntityVec(pos.x - cos * rad, pos.y + sin * rad), rad / 2));
		}
	}

	inline void onCollision(const ptrEntity& ent) noexcept override
	{
		if (ent->getType() != EntityType::ASTEROID)
		{
			remove();
			Engine::incrementScore(static_cast<unsigned long>(rad));
		}
	}

	inline void update(float dt) noexcept override
	{
		(void) dt;

		ang += angdt;
		updateTrig();
		updatePhysics();
	}
};

//
// Bullet
//

class Bullet : public Entity
{
	static constexpr EntityVecType spd = 8.f;
	static constexpr float radDefault = 2.f;
	unsigned short timer = 20;
public:
	inline Bullet(const EntityVec& pos, const EntityVec& v) : Entity::Entity(pos)
	{ 
		vel.x = v.x, vel.y = v.y, vel.z = v.z;
		vel *= spd;	
		rad = radDefault;
		type = EntityType::BULLET;
	}

	inline void update(float dt) noexcept override 
	{
		(void) dt;

		if (timer-- == 0) exist = false;

		updateTrig();
		updatePhysics();
	}
	
	inline void render() noexcept override 
	{
		SDL_ContextFillCircle(Engine::getContext(), pos.x, pos.y, 1, Colors::WHITE);
	}

	inline void onCollision(const ptrEntity&) noexcept override
	{
		// check if other entity is bullet
		remove();
	}

};

//
// Player
//

class Player final : public Entity
{
	static constexpr EntityVecType spd = 0.05f, aspd = 0.05f;
	static constexpr float radDefault = 4.f;
	static constexpr unsigned short invisibleTimeMax = 54; 
	unsigned short invisibleTimer = invisibleTimeMax, lives = 2;

public:
	static constexpr float mesh[6] = { 4.f, 0.f, -3.f, 3.f, -3.f, -3.f };

public:
	inline Player(const EntityVec& vec) : Entity::Entity(vec)
	{
		rad = radDefault;
		type = EntityType::PLAYER;
	}

	inline void update(float dt) noexcept override 
	{
		(void) dt;

		if (SDL_ContextKeyIsDown(SDL_SCANCODE_W))
		{
			acc.x = +spd * cos, acc.y = +spd * sin;
			if ((SDL_GetTicks() / 10) % 4 == 0)
			{
				static constexpr unsigned short timePufft = 16;
				Engine::addParticle(
					new Pufft(vec3(pos.x, pos.y), vec3(-cos, -sin), Pufft::radDefault, timePufft));
			}
		}
		else if (SDL_ContextKeyIsDown(SDL_SCANCODE_S))
			acc.x = -spd * 0.5f * cos, acc.y = -spd * 0.5f * sin;

		if (SDL_ContextKeyIsDown(SDL_SCANCODE_A))
			ang -= aspd;
		else if (SDL_ContextKeyIsDown(SDL_SCANCODE_D))
			ang += aspd;

		if (SDL_ContextKeyIsPress(SDL_SCANCODE_RSHIFT))
		{
			Bullet* newBullet = new Bullet(vec3(pos.x, pos.y), vec3(cos, sin));
			newBullet->update(dt);
			Engine::addEntity(newBullet);
		}

		if (invisibleTimer > 0)
			--invisibleTimer;
		else
			invisible = false;

		updateTrig();
		updatePhysics();
	}

	inline void render() noexcept override 
	{
		if (invisibleTimer == 0 || (SDL_GetTicks() / 10) % 4 == 0)
			for (int i = 0; i < 6; i += 2)
			{
				const float& x1 = mesh[i + 0], &y1 = mesh[i + 1];
				const float& x2 = mesh[(i + 2) % 6], &y2 = mesh[(i + 3) % 6];
				SDL_ContextDrawLine(Engine::getContext(),
					pos.x + (x1 * cos - y1 * sin),
					pos.y + (x1 * sin + y1 * cos),
					pos.x + (x2 * cos - y2 * sin),
					pos.y + (x2 * sin + y2 * cos),
					Colors::WHITE);
			}
		Engine::drawString(std::string("Lives: ") + std::to_string(static_cast<long long>(lives)), 1, 1);
	}

	inline void onCollision(const ptrEntity&) noexcept override
	{
		if (--lives == 0)
		{
			remove();
			Engine::changeState(Engine::State::END);
		}
		invisibleTimer = invisibleTimeMax;
		invisible = true;
	}

};

void randomSpawn()
{
	Engine::incrementLevel();
	for (unsigned i = 0; i < Engine::getLevel(); ++i)
		Engine::addEntity(new Asteroid(EntityVec(rand() % (Engine::width * 2), rand() % (Engine::height * 2))));
}


int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;

	srand(time(0));

	Engine::addEntity(new Player(EntityVec(Engine::width / 2.f, Engine::height / 2.f, 0.f)));
	randomSpawn();

	Engine::run();
	
	return 0;
}

