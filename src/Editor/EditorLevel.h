#pragma once

#include "../Editor/WorldGeometry.h"
#include "../Graphics/Mesh.h"


struct EditorLevel
{
    std::vector<std::pair<Wall, WorldGeometryMesh3D>> walls;

    Wall& add_wall(const Wall& wall);
};
