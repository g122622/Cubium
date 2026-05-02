#pragma once

#include "../../../util/Direction.hpp"
#include "../../../core/Types.hpp"
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief Jigsaw 方块方向枚举
 *
 * 表示 Jigsaw 方块的 12 种方向组合。
 * 每个 JigsawOrientation 由一个主要朝向（facing）和一个旋转朝向（rotation）组成。
 *
 * 参考: net.minecraft.state.properties.JigsawOrientation (MC 1.16.5)
 *
 * 方向组合规则：
 * - 当 facing 为 DOWN 时，rotation 可以是 EAST, NORTH, SOUTH, WEST
 * - 当 facing 为 UP 时，rotation 可以是 EAST, NORTH, SOUTH, WEST
 * - 当 facing 为水平方向时，rotation 只能是 UP（即 NORTH_UP, SOUTH_UP, WEST_UP, EAST_UP）
 */
enum class JigsawOrientation : u8 {
    DownEast = 0,   // facing=DOWN, rotation=EAST
    DownNorth = 1,  // facing=DOWN, rotation=NORTH
    DownSouth = 2,  // facing=DOWN, rotation=SOUTH
    DownWest = 3,   // facing=DOWN, rotation=WEST
    UpEast = 4,     // facing=UP, rotation=EAST
    UpNorth = 5,    // facing=UP, rotation=NORTH
    UpSouth = 6,    // facing=UP, rotation=SOUTH
    UpWest = 7,     // facing=UP, rotation=WEST
    WestUp = 8,     // facing=WEST, rotation=UP
    EastUp = 9,     // facing=EAST, rotation=UP
    NorthUp = 10,   // facing=NORTH, rotation=UP
    SouthUp = 11    // facing=SOUTH, rotation=UP
};

/**
 * @brief JigsawOrientation 工具函数
 */
namespace JigsawOrientations {
    constexpr size_t COUNT = 12;

    /**
     * @brief 获取所有 JigsawOrientation
     */
    inline std::array<JigsawOrientation, 12> all() {
        return {
            JigsawOrientation::DownEast,
            JigsawOrientation::DownNorth,
            JigsawOrientation::DownSouth,
            JigsawOrientation::DownWest,
            JigsawOrientation::UpEast,
            JigsawOrientation::UpNorth,
            JigsawOrientation::UpSouth,
            JigsawOrientation::UpWest,
            JigsawOrientation::WestUp,
            JigsawOrientation::EastUp,
            JigsawOrientation::NorthUp,
            JigsawOrientation::SouthUp
        };
    }

    /**
     * @brief 从 facing 和 rotation 获取 JigsawOrientation
     *
     * @param facing 主要朝向
     * @param rotation 旋转朝向（必须与 facing 垂直）
     * @return JigsawOrientation，如果组合无效则返回默认值
     */
    inline JigsawOrientation fromFacingAndRotation(Direction facing, Direction rotation) {
        // facing 和 rotation 必须垂直
        if (facing == rotation || Directions::opposite(facing) == rotation) {
            // 无效组合，返回默认值
            return JigsawOrientation::NorthUp;
        }

        // DOWN facing (rotation 可以是水平方向)
        if (facing == Direction::Down) {
            switch (rotation) {
                case Direction::East: return JigsawOrientation::DownEast;
                case Direction::North: return JigsawOrientation::DownNorth;
                case Direction::South: return JigsawOrientation::DownSouth;
                case Direction::West: return JigsawOrientation::DownWest;
                default: return JigsawOrientation::DownNorth;
            }
        }

        // UP facing (rotation 可以是水平方向)
        if (facing == Direction::Up) {
            switch (rotation) {
                case Direction::East: return JigsawOrientation::UpEast;
                case Direction::North: return JigsawOrientation::UpNorth;
                case Direction::South: return JigsawOrientation::UpSouth;
                case Direction::West: return JigsawOrientation::UpWest;
                default: return JigsawOrientation::UpNorth;
            }
        }

        // 水平 facing (rotation 只能是 UP)
        // 注意：MC 1.16.5 中，水平 facing 时 rotation 固定为 UP
        switch (facing) {
            case Direction::West: return JigsawOrientation::WestUp;
            case Direction::East: return JigsawOrientation::EastUp;
            case Direction::North: return JigsawOrientation::NorthUp;
            case Direction::South: return JigsawOrientation::SouthUp;
            default: return JigsawOrientation::NorthUp;
        }
    }

    /**
     * @brief 获取主要朝向 (facing)
     */
    inline Direction getFacing(JigsawOrientation orientation) {
        switch (orientation) {
            case JigsawOrientation::DownEast:
            case JigsawOrientation::DownNorth:
            case JigsawOrientation::DownSouth:
            case JigsawOrientation::DownWest:
                return Direction::Down;
            case JigsawOrientation::UpEast:
            case JigsawOrientation::UpNorth:
            case JigsawOrientation::UpSouth:
            case JigsawOrientation::UpWest:
                return Direction::Up;
            case JigsawOrientation::WestUp:
                return Direction::West;
            case JigsawOrientation::EastUp:
                return Direction::East;
            case JigsawOrientation::NorthUp:
                return Direction::North;
            case JigsawOrientation::SouthUp:
                return Direction::South;
            default:
                return Direction::North;
        }
    }

    /**
     * @brief 获取旋转朝向 (rotation)
     */
    inline Direction getRotation(JigsawOrientation orientation) {
        switch (orientation) {
            case JigsawOrientation::DownEast:
            case JigsawOrientation::UpEast:
                return Direction::East;
            case JigsawOrientation::DownNorth:
            case JigsawOrientation::UpNorth:
                return Direction::North;
            case JigsawOrientation::DownSouth:
            case JigsawOrientation::UpSouth:
                return Direction::South;
            case JigsawOrientation::DownWest:
            case JigsawOrientation::UpWest:
                return Direction::West;
            case JigsawOrientation::WestUp:
            case JigsawOrientation::EastUp:
            case JigsawOrientation::NorthUp:
            case JigsawOrientation::SouthUp:
                return Direction::Up;
            default:
                return Direction::Up;
        }
    }

    /**
     * @brief 从索引获取 JigsawOrientation
     */
    inline JigsawOrientation fromIndex(size_t index) {
        return static_cast<JigsawOrientation>(index % COUNT);
    }

    /**
     * @brief 获取索引 (0-11)
     */
    inline size_t index(JigsawOrientation orientation) {
        return static_cast<size_t>(orientation);
    }

    /**
     * @brief 从名称获取 JigsawOrientation
     *
     * @param name 名称（如 "down_east", "up_north", "north_up"）
     * @return JigsawOrientation，如果无效则返回 nullopt
     */
    inline std::optional<JigsawOrientation> fromName(StringView name) {
        static const std::unordered_map<String, JigsawOrientation> nameMap = {
            {"down_east", JigsawOrientation::DownEast},
            {"down_north", JigsawOrientation::DownNorth},
            {"down_south", JigsawOrientation::DownSouth},
            {"down_west", JigsawOrientation::DownWest},
            {"up_east", JigsawOrientation::UpEast},
            {"up_north", JigsawOrientation::UpNorth},
            {"up_south", JigsawOrientation::UpSouth},
            {"up_west", JigsawOrientation::UpWest},
            {"west_up", JigsawOrientation::WestUp},
            {"east_up", JigsawOrientation::EastUp},
            {"north_up", JigsawOrientation::NorthUp},
            {"south_up", JigsawOrientation::SouthUp}
        };
        auto it = nameMap.find(String(name));
        return it != nameMap.end() ? std::optional<JigsawOrientation>(it->second) : std::nullopt;
    }

    /**
     * @brief 获取名称
     */
    inline String toString(JigsawOrientation orientation) {
        switch (orientation) {
            case JigsawOrientation::DownEast: return "down_east";
            case JigsawOrientation::DownNorth: return "down_north";
            case JigsawOrientation::DownSouth: return "down_south";
            case JigsawOrientation::DownWest: return "down_west";
            case JigsawOrientation::UpEast: return "up_east";
            case JigsawOrientation::UpNorth: return "up_north";
            case JigsawOrientation::UpSouth: return "up_south";
            case JigsawOrientation::UpWest: return "up_west";
            case JigsawOrientation::WestUp: return "west_up";
            case JigsawOrientation::EastUp: return "east_up";
            case JigsawOrientation::NorthUp: return "north_up";
            case JigsawOrientation::SouthUp: return "south_up";
            default: return "north_up";
        }
    }

    /**
     * @brief 检查两个 JigsawOrientation 是否可以连接
     *
     * 连接条件：
     * 1. facing 方向必须相反
     * 2. rotation 方向必须相同（对于可旋转连接）或允许任意旋转
     *
     * @param source 源 Jigsaw 方向
     * @param target 目标 Jigsaw 方向
     * @param rollable 是否允许旋转（如果为 true，则只检查 facing 相反）
     * @return 是否可以连接
     */
    inline bool canConnect(JigsawOrientation source, JigsawOrientation target, bool rollable = true) {
        Direction sourceFacing = getFacing(source);
        Direction targetFacing = getFacing(target);

        // facing 必须相反
        if (Directions::opposite(sourceFacing) != targetFacing) {
            return false;
        }

        // 如果允许旋转，则只需 facing 相反
        if (rollable) {
            return true;
        }

        // 不允许旋转时，rotation 必须相同
        return getRotation(source) == getRotation(target);
    }

    /**
     * @brief 对 JigsawOrientation 应用旋转
     *
     * @param orientation 原始方向
     * @param rotation 旋转类型
     * @return 旋转后的方向
     */
    inline JigsawOrientation rotate(JigsawOrientation orientation, Rotation rotation) {
        if (rotation == Rotation::None) {
            return orientation;
        }

        Direction facing = getFacing(orientation);
        Direction rot = getRotation(orientation);

        // 垂直方向（UP/DOWN）的 facing
        if (facing == Direction::Up || facing == Direction::Down) {
            // rotation 是水平方向，需要旋转
            Direction newRot = Directions::rotateDirection(rot, rotation);
            return fromFacingAndRotation(facing, newRot);
        }

        // 水平方向 facing，rotation 固定为 UP
        // 只旋转 facing
        Direction newFacing = Directions::rotateDirection(facing, rotation);
        return fromFacingAndRotation(newFacing, Direction::Up);
    }

    /**
     * @brief 对 JigsawOrientation 应用镜像
     *
     * @param orientation 原始方向
     * @param mirror 镜像类型
     * @return 镜像后的方向
     */
    inline JigsawOrientation mirror(JigsawOrientation orientation, Mirror mirror) {
        if (mirror == Mirror::None) {
            return orientation;
        }

        Direction facing = getFacing(orientation);
        Direction rot = getRotation(orientation);

        // 对 facing 应用镜像
        Direction newFacing = facing;
        Direction newRot = rot;

        switch (mirror) {
            case Mirror::LeftRight:
                // X 轴镜像：East <-> West
                if (facing == Direction::East) newFacing = Direction::West;
                else if (facing == Direction::West) newFacing = Direction::East;
                if (rot == Direction::East) newRot = Direction::West;
                else if (rot == Direction::West) newRot = Direction::East;
                break;
            case Mirror::FrontBack:
                // Z 轴镜像：North <-> South
                if (facing == Direction::North) newFacing = Direction::South;
                else if (facing == Direction::South) newFacing = Direction::North;
                if (rot == Direction::North) newRot = Direction::South;
                else if (rot == Direction::South) newRot = Direction::North;
                break;
            default:
                break;
        }

        return fromFacingAndRotation(newFacing, newRot);
    }

    /**
     * @brief 获取相反方向
     *
     * 返回一个 JigsawOrientation，其 facing 与当前相反，
     * rotation 与当前相同。
     *
     * @param orientation 原始方向
     * @return 相反方向
     */
    inline JigsawOrientation opposite(JigsawOrientation orientation) {
        Direction facing = getFacing(orientation);
        Direction rot = getRotation(orientation);
        Direction oppositeFacing = Directions::opposite(facing);

        // 如果 facing 是垂直方向，rotation 可以保持
        if (oppositeFacing == Direction::Up || oppositeFacing == Direction::Down) {
            return fromFacingAndRotation(oppositeFacing, rot);
        }

        // 如果 facing 变为水平方向，rotation 只能是 UP
        return fromFacingAndRotation(oppositeFacing, Direction::Up);
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
