#pragma once

/** Amount of gravity to apply to objects with FGravityTag. */
constexpr float GRAVITY = -980.f;

/** Maximum number of enemy entities supported by the system,
 * not including other entities (like exp). */
constexpr int MAX_ENEMY_COUNT = 10000;

constexpr ECollisionChannel WALL_COLLISION[] = {ECC_WorldStatic};

/** Debug params */
constexpr bool GBDebugHitEnemyHealth = false;
