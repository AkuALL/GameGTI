#include "../../include/enemy_ai.h"
#include "../../include/audio.h"
#include "../../include/game_logic.h"
#include "../../include/paths.h"

#include <algorithm>
#include <cmath>
#include <vector>

bool checkEnemyCollision(float newX, float newZ) {
    const float enemyRadius = 0.18f;

    if (newX < MAP_MIN + enemyRadius || newX > MAP_MAX - enemyRadius ||
        newZ < MAP_MIN + enemyRadius || newZ > MAP_MAX - enemyRadius)
        return true;

    for (int i = 0; i < wallCount; i++) {
        float wx = walls[i].x, wz = walls[i].z;
        float halfX = walls[i].sx / 2.0f, halfZ = walls[i].sz / 2.0f;
        float closestX = std::max(wx - halfX, std::min(newX, wx + halfX));
        float closestZ = std::max(wz - halfZ, std::min(newZ, wz + halfZ));
        float dx = newX - closestX;
        float dz = newZ - closestZ;

        if (dx * dx + dz * dz < enemyRadius * enemyRadius)
            return true;
    }
    return false;
}

bool pathExists(float startX, float startZ, float targetX, float targetZ) {
    const float gridStep = 0.5f;
    const int gridSize = (int)((MAP_MAX - MAP_MIN) / gridStep) + 1;

    auto toIndex = [&](float v) {
        int idx = (int)roundf((v - MAP_MIN) / gridStep);
        if (idx < 0) idx = 0;
        if (idx >= gridSize) idx = gridSize - 1;
        return idx;
    };

    int sx = toIndex(startX), sz = toIndex(startZ);
    int tx = toIndex(targetX), tz = toIndex(targetZ);

    std::vector<int> visited(gridSize * gridSize, 0);
    std::vector<int> queue;
    queue.reserve(gridSize * gridSize);

    auto nodeId = [&](int x, int z) { return z * gridSize + x; };
    auto worldX = [&](int x) { return MAP_MIN + x * gridStep; };
    auto worldZ = [&](int z) { return MAP_MIN + z * gridStep; };
    auto walkable = [&](int x, int z) {
        return x >= 0 && x < gridSize && z >= 0 && z < gridSize &&
               !checkCollision(worldX(x), worldZ(z));
    };

    if (!walkable(sx, sz) || !walkable(tx, tz)) return false;

    visited[nodeId(sx, sz)] = 1;
    queue.push_back(nodeId(sx, sz));

    const int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    for (size_t head = 0; head < queue.size(); head++) {
        int id = queue[head];
        int x = id % gridSize;
        int z = id / gridSize;

        if (x == tx && z == tz) return true;

        for (int i = 0; i < 4; i++) {
            int nx = x + dirs[i][0];
            int nz = z + dirs[i][1];
            if (!walkable(nx, nz)) continue;

            int nid = nodeId(nx, nz);
            if (visited[nid]) continue;

            visited[nid] = 1;
            queue.push_back(nid);
        }
    }
    return false;
}

bool tryMoveEnemy(Enemy &e, float stepX, float stepZ) {
    float candX = e.x + stepX;
    float candZ = e.z + stepZ;

    if (!checkEnemyCollision(candX, candZ)) {
        e.x = candX;
        e.z = candZ;
        return true;
    }

    if (!checkEnemyCollision(e.x + stepX, e.z)) {
        e.x += stepX;
        return true;
    }

    if (!checkEnemyCollision(e.x, e.z + stepZ)) {
        e.z += stepZ;
        return true;
    }

    const float fractions[5] = {0.75f, 0.5f, 0.35f, 0.2f, 0.1f};
    for (int k = 0; k < 5; k++) {
        float smallX = e.x + stepX * fractions[k];
        float smallZ = e.z + stepZ * fractions[k];
        if (!checkEnemyCollision(smallX, smallZ)) {
            e.x = smallX;
            e.z = smallZ;
            return true;
        }
    }
    return false;
}

bool enemyPathClear(float startX, float startZ, float targetX, float targetZ) {
    float dx = targetX - startX;
    float dz = targetZ - startZ;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 0.001f) return true;

    int steps = (int)(dist / 0.12f) + 1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float x = startX + dx * t;
        float z = startZ + dz * t;
        if (checkEnemyCollision(x, z)) return false;
    }
    return true;
}

bool buildEnemyPath(Enemy &e, float targetX, float targetZ) {
    const float gridStep = 0.5f;
    const int gridSize = (int)((MAP_MAX - MAP_MIN) / gridStep) + 1;

    auto toIndex = [&](float v) {
        int idx = (int)roundf((v - MAP_MIN) / gridStep);
        if (idx < 0) idx = 0;
        if (idx >= gridSize) idx = gridSize - 1;
        return idx;
    };

    auto nodeId = [&](int x, int z) { return z * gridSize + x; };
    auto worldX = [&](int x) { return MAP_MIN + x * gridStep; };
    auto worldZ = [&](int z) { return MAP_MIN + z * gridStep; };
    auto walkable = [&](int x, int z) {
        return x >= 0 && x < gridSize && z >= 0 && z < gridSize &&
               !checkEnemyCollision(worldX(x), worldZ(z));
    };

    int sx = toIndex(e.x), sz = toIndex(e.z);
    int tx = toIndex(targetX), tz = toIndex(targetZ);

    if (!walkable(sx, sz) || !walkable(tx, tz)) return false;

    std::vector<int> queue;
    std::vector<int> prev(gridSize * gridSize, -1);
    queue.reserve(gridSize * gridSize);

    int start = nodeId(sx, sz);
    int goal = nodeId(tx, tz);
    prev[start] = start;
    queue.push_back(start);

    const int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    for (size_t head = 0; head < queue.size(); head++) {
        int id = queue[head];
        if (id == goal) break;

        int x = id % gridSize;
        int z = id / gridSize;
        for (int i = 0; i < 4; i++) {
            int nx = x + dirs[i][0];
            int nz = z + dirs[i][1];
            if (!walkable(nx, nz)) continue;

            int nid = nodeId(nx, nz);
            if (prev[nid] != -1) continue;

            prev[nid] = id;
            queue.push_back(nid);
        }
    }

    if (prev[goal] == -1) return false;

    std::vector<int> reversed;
    for (int at = goal; at != start; at = prev[at]) {
        reversed.push_back(at);
    }
    std::reverse(reversed.begin(), reversed.end());

    e.pathX.clear();
    e.pathZ.clear();
    for (size_t i = 0; i < reversed.size(); i++) {
        int id = reversed[i];
        int x = id % gridSize;
        int z = id / gridSize;
        e.pathX.push_back(worldX(x));
        e.pathZ.push_back(worldZ(z));
    }

    e.targetX = targetX;
    e.targetZ = targetZ;
    e.pathIndex = 0;
    return !e.pathX.empty();
}

bool chooseEnemyPatrolTarget(Enemy &e) {
    for (int tries = 0; tries < 80; tries++) {
        float tx, tz;
        randomPos(tx, tz);

        float dx = tx - e.x;
        float dz = tz - e.z;
        if (sqrtf(dx * dx + dz * dz) < 4.0f) continue;

        if (!checkEnemyCollision(tx, tz) && buildEnemyPath(e, tx, tz)) {
            return true;
        }
    }
    return false;
}
void updateEnemies(float dt) {
    for (int i = 0; i < 8; i++) {
        Enemy &e = enemies[i];

        // ── CHASE MODE ──
        if (chaseMode || lightsOn) {
            if (playerIsHiding) {
                goto patrol_logic;
            }

            float pdx = e.x - playerX, pdz = e.z - playerZ;
            float distToPlayer = sqrtf(pdx*pdx + pdz*pdz);

            if (e.pathIndex >= (int)e.pathX.size() || distToPlayer > 2.0f) {
                buildEnemyPath(e, playerX, playerZ);
            }

            float chaseSpeed = e.speed * 3.0f * dt * 60.0f;

            if (!e.pathX.empty() && e.pathIndex < (int)e.pathX.size()) {
                float tx = e.pathX[e.pathIndex];
                float tz = e.pathZ[e.pathIndex];
                float dx = tx - e.x, dz = tz - e.z;
                float dist = sqrtf(dx*dx + dz*dz);
                if (dist < 0.18f) {
                    e.pathIndex++;
                } else {
                    float stepX = (dx / dist) * chaseSpeed;
                    float stepZ = (dz / dist) * chaseSpeed;
                    tryMoveEnemy(e, stepX, stepZ);
                    e.angle = atan2f(dx, dz) * 180.0f / (float)M_PI;
                }
            }

            if (distToPlayer < 0.85f && !playerIsHiding) {
                if (!jumpscareActive) {
                    jumpscareActive = true;
                    jumpscareTimer  = 0.0f;
                    jumpscareAlpha  = 0.0f;
                    jumpscareStartTime = glutGet(GLUT_ELAPSED_TIME);
                    playSFX(ASSET_SOUND_JUMPSCARE); 
}
            }
            continue;
        }

        // ── PATROL MODE ──
        patrol_logic:

        if (lightsOn && e.pathX.empty()) {
            chooseEnemyPatrolTarget(e);
            continue;
        }

        if (e.pathIndex >= (int)e.pathX.size()) {
            if (!chooseEnemyPatrolTarget(e)) continue;
            continue;
        }

        {
            float tx = e.pathX[e.pathIndex];
            float tz = e.pathZ[e.pathIndex];
            float dx = tx - e.x;
            float dz = tz - e.z;
            float dist = sqrtf(dx*dx + dz*dz);

            if (dist < 0.18f) {
                e.pathIndex++;
                e.stuckFrames = 0;
                if (e.pathIndex >= (int)e.pathX.size()) {
                    chooseEnemyPatrolTarget(e);
                }
                continue;
            }

            float moveSpeed = e.speed * (lightsOn ? 2.5f : 1.0f) * dt * 60.0f;
            float stepX = (dx / dist) * moveSpeed;
            float stepZ = (dz / dist) * moveSpeed;
            float oldX = e.x;
            float oldZ = e.z;

            if (tryMoveEnemy(e, stepX, stepZ)) {
                e.stuckFrames = 0;
                float movedX = e.x - oldX;
                float movedZ = e.z - oldZ;
                if (movedX * movedX + movedZ * movedZ > 0.0001f) {
                    e.angle = atan2f(movedX, movedZ) * 180.0f / (float)M_PI;
                }
            } else {
                e.stuckFrames++;
                int stuckThreshold = lightsOn ? 6 : 12;
                if (e.stuckFrames > stuckThreshold) {
                    if (!buildEnemyPath(e, e.targetX, e.targetZ)) {
                        chooseEnemyPatrolTarget(e);
                    }
                    e.stuckFrames = 0;
                }
            }

            if (!playerIsHiding) {
                float pdx = e.x - playerX, pdz = e.z - playerZ;
                if (sqrtf(pdx*pdx + pdz*pdz) < 0.85f) {
                    if (!jumpscareActive) {
                        jumpscareActive = true;
                        jumpscareTimer  = 0.0f;
                        jumpscareAlpha  = 0.0f;
                        jumpscareStartTime = glutGet(GLUT_ELAPSED_TIME);
                        playSFX(ASSET_SOUND_JUMPSCARE);
                    }
                }
            }
        }
    }
}
