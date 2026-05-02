#include "StructureBoundingBox.hpp"
#include "../../../util/Direction.hpp"

namespace mc::world::gen::structure {

StructureBoundingBox StructureBoundingBox::createBox(
    i32 x, i32 y, i32 z,
    i32 offsetX, i32 offsetY, i32 offsetZ,
    i32 sizeX, i32 sizeY, i32 sizeZ,
    Direction direction) {

    // 参考 MC 1.16.5 MutableBoundingBox.getComponentToAddBoundingBox
    switch (direction) {
        case Direction::North:
            return StructureBoundingBox(
                x + offsetX,
                y + offsetY,
                z + offsetZ,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z + offsetZ + sizeZ - 1
            );

        case Direction::South:
            return StructureBoundingBox(
                x + offsetX,
                y + offsetY,
                z - sizeZ - offsetZ + 1,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z - offsetZ
            );

        case Direction::West:
            return StructureBoundingBox(
                x - sizeZ - offsetZ + 1,
                y + offsetY,
                z + offsetX,
                x - offsetZ,
                y + offsetY + sizeY - 1,
                z + offsetX + sizeX - 1
            );

        case Direction::East:
            return StructureBoundingBox(
                x + offsetZ,
                y + offsetY,
                z + offsetX,
                x + offsetZ + sizeZ - 1,
                y + offsetY + sizeY - 1,
                z + offsetX + sizeX - 1
            );

        default:
            // 默认朝北
            return StructureBoundingBox(
                x + offsetX,
                y + offsetY,
                z + offsetZ,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z + offsetZ + sizeZ - 1
            );
    }
}

} // namespace mc::world::gen::structure
