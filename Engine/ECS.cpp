#include "ECS.h"

void World::destroy(Entity e) {
    transforms.erase(e);
    cameras.erase(e);
    meshes.erase(e);
}
