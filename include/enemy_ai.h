#ifndef SHADOW_ESCAPE_ENEMY_AI_H
#define SHADOW_ESCAPE_ENEMY_AI_H

#include "game_state.h"

bool checkEnemyCollision(float newX, float newZ);
bool pathExists(float startX, float startZ, float targetX, float targetZ);
bool tryMoveEnemy(Enemy &e, float stepX, float stepZ);
bool enemyPathClear(float startX, float startZ, float targetX, float targetZ);
bool buildEnemyPath(Enemy &e, float targetX, float targetZ);
bool chooseEnemyPatrolTarget(Enemy &e);
void updateEnemies(float dt);

#endif
