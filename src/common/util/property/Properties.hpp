#pragma once

/**
 * @file BlockStateProperties.hpp
 * @brief 预定义的方块状态属性
 *
 * 参考: net.minecraft.state.properties.BlockStateProperties
 *
 * 这个文件包含所有方块状态常用的属性定义。
 * 属性是静态单例，应该通过引用访问。
 */

#include "BooleanProperty.hpp"
#include "IntegerProperty.hpp"
#include "EnumProperty.hpp"
#include "DirectionProperty.hpp"
#include "../Direction.hpp"

namespace mc {

/**
 * @brief 预定义的方块状态属性集合
 *
 * 提供所有标准方块状态属性的静态访问。
 * 所有属性都是懒加载的单例。
 *
 * 用法示例:
 * @code
 * // 获取属性引用
 * const BooleanProperty& lit = BlockStateProperties::LIT;
 * const DirectionProperty& facing = BlockStateProperties::FACING;
 *
 * // 使用属性
 * bool isLit = state.get(lit);
 * const BlockState& newState = state.with(facing, Direction::North);
 * @endcode
 */
class BlockStateProperties {
public:
    // ========================================================================
    // 布尔属性
    // ========================================================================

    /**
     * @brief 是否被激活（绊线钩、绊线等）
     */
    static const BooleanProperty& ATTACHED() {
        static auto prop = BooleanProperty::create("attached");
        return *prop;
    }

    /**
     * @brief 是否在底部（门、活板门等的下半部分）
     */
    static const BooleanProperty& BOTTOM() {
        static auto prop = BooleanProperty::create("bottom");
        return *prop;
    }

    /**
     * @brief 是否有条件（命令方块）
     */
    static const BooleanProperty& CONDITIONAL() {
        static auto prop = BooleanProperty::create("conditional");
        return *prop;
    }

    /**
     * @brief 是否已被拆除（绊线）
     */
    static const BooleanProperty& DISARMED() {
        static auto prop = BooleanProperty::create("disarmed");
        return *prop;
    }

    /**
     * @brief 是否有拖拽（灵魂沙上的水）
     */
    static const BooleanProperty& DRAG() {
        static auto prop = BooleanProperty::create("drag");
        return *prop;
    }

    /**
     * @brief 是否启用（漏斗、活塞等）
     */
    static const BooleanProperty& ENABLED() {
        static auto prop = BooleanProperty::create("enabled");
        return *prop;
    }

    /**
     * @brief 是否伸出（活塞）
     */
    static const BooleanProperty& EXTENDED() {
        static auto prop = BooleanProperty::create("extended");
        return *prop;
    }

    /**
     * @brief 是否有眼（末地传送门框架）
     */
    static const BooleanProperty& EYE() {
        static auto prop = BooleanProperty::create("eye");
        return *prop;
    }

    /**
     * @brief 是否正在下落（沙子、砾石等）
     */
    static const BooleanProperty& FALLING() {
        static auto prop = BooleanProperty::create("falling");
        return *prop;
    }

    /**
     * @brief 是否悬挂（灯笼等）
     */
    static const BooleanProperty& HANGING() {
        static auto prop = BooleanProperty::create("hanging");
        return *prop;
    }

    /**
     * @brief 是否反转（日光探测器夜间模式）
     */
    static const BooleanProperty& INVERTED() {
        static auto prop = BooleanProperty::create("inverted");
        return *prop;
    }

    /**
     * @brief 是否点亮（火把、熔炉等）
     */
    static const BooleanProperty& LIT() {
        static auto prop = BooleanProperty::create("lit");
        return *prop;
    }

    /**
     * @brief 是否锁定（比较器）
     */
    static const BooleanProperty& LOCKED() {
        static auto prop = BooleanProperty::create("locked");
        return *prop;
    }

    /**
     * @brief 是否被占用（床）
     */
    static const BooleanProperty& OCCUPIED() {
        static auto prop = BooleanProperty::create("occupied");
        return *prop;
    }

    /**
     * @brief 是否打开（门、活板门、栅栏门等）
     */
    static const BooleanProperty& OPEN() {
        static auto prop = BooleanProperty::create("open");
        return *prop;
    }

    /**
     * @brief 是否持久（树叶）
     */
    static const BooleanProperty& PERSISTENT() {
        static auto prop = BooleanProperty::create("persistent");
        return *prop;
    }

    /**
     * @brief 是否被充能
     */
    static const BooleanProperty& POWERED() {
        static auto prop = BooleanProperty::create("powered");
        return *prop;
    }

    /**
     * @brief 是否积雪（草方块等）
     */
    static const BooleanProperty& SNOWY() {
        static auto prop = BooleanProperty::create("snowy");
        return *prop;
    }

    /**
     * @brief 是否被触发（命令方块）
     */
    static const BooleanProperty& TRIGGERED() {
        static auto prop = BooleanProperty::create("triggered");
        return *prop;
    }

    /**
     * @brief 是否不稳定（TNT）
     */
    static const BooleanProperty& UNSTABLE() {
        static auto prop = BooleanProperty::create("unstable");
        return *prop;
    }

    /**
     * @brief 是否含水（栅栏、台阶等）
     */
    static const BooleanProperty& WATERLOGGED() {
        static auto prop = BooleanProperty::create("waterlogged");
        return *prop;
    }

    /**
     * @brief 是否为信号火（营火）
     */
    static const BooleanProperty& SIGNAL_FIRE() {
        static auto prop = BooleanProperty::create("signal_fire");
        return *prop;
    }

    /**
     * @brief 是否向上（栅栏、墙等）
     */
    static const BooleanProperty& UP() {
        static auto prop = BooleanProperty::create("up");
        return *prop;
    }

    /**
     * @brief 是否向下
     */
    static const BooleanProperty& DOWN() {
        static auto prop = BooleanProperty::create("down");
        return *prop;
    }

    /**
     * @brief 是否向北
     */
    static const BooleanProperty& NORTH() {
        static auto prop = BooleanProperty::create("north");
        return *prop;
    }

    /**
     * @brief 是否向南
     */
    static const BooleanProperty& SOUTH() {
        static auto prop = BooleanProperty::create("south");
        return *prop;
    }

    /**
     * @brief 是否向东
     */
    static const BooleanProperty& EAST() {
        static auto prop = BooleanProperty::create("east");
        return *prop;
    }

    /**
     * @brief 是否向西
     */
    static const BooleanProperty& WEST() {
        static auto prop = BooleanProperty::create("west");
        return *prop;
    }

    // ========================================================================
    // 方向属性
    // ========================================================================

    /**
     * @brief 朝向属性（所有6个方向）
     */
    static const DirectionProperty& FACING() {
        static auto prop = DirectionProperty::create("facing");
        return *prop;
    }

    /**
     * @brief 朝向属性（仅水平方向）
     */
    static const DirectionProperty& HORIZONTAL_FACING() {
        static auto prop = DirectionProperty::createHorizontal("facing");
        return *prop;
    }

    /**
     * @brief 朝向属性（除上之外的所有方向）
     */
    static const DirectionProperty& FACING_EXCEPT_UP() {
        static auto prop = DirectionProperty::create("facing", [](Direction d) {
            return d != Direction::Up;
        });
        return *prop;
    }

    // ========================================================================
    // 坐标轴属性
    // ========================================================================

    /**
     * @brief 坐标轴属性（所有三个轴）
     */
    static const EnumProperty<Axis>& AXIS() {
        static auto prop = AxisProperty::create("axis");
        return *prop;
    }

    /**
     * @brief 坐标轴属性（仅水平轴X和Z）
     */
    static const EnumProperty<Axis>& HORIZONTAL_AXIS() {
        static auto prop = EnumProperty<Axis>::create("axis", {Axis::X, Axis::Z});
        return *prop;
    }

    // ========================================================================
    // 整数属性
    // ========================================================================

    /**
     * @brief 年龄属性 (0-1)
     */
    static const IntegerProperty& AGE_0_1() {
        static auto prop = IntegerProperty::create("age", 0, 1);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-2)
     */
    static const IntegerProperty& AGE_0_2() {
        static auto prop = IntegerProperty::create("age", 0, 2);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-3)
     */
    static const IntegerProperty& AGE_0_3() {
        static auto prop = IntegerProperty::create("age", 0, 3);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-4)
     */
    static const IntegerProperty& AGE_0_4() {
        static auto prop = IntegerProperty::create("age", 0, 4);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-5)
     */
    static const IntegerProperty& AGE_0_5() {
        static auto prop = IntegerProperty::create("age", 0, 5);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-7)
     */
    static const IntegerProperty& AGE_0_7() {
        static auto prop = IntegerProperty::create("age", 0, 7);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-15)
     */
    static const IntegerProperty& AGE_0_15() {
        static auto prop = IntegerProperty::create("age", 0, 15);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-25)
     */
    static const IntegerProperty& AGE_0_25() {
        static auto prop = IntegerProperty::create("age", 0, 25);
        return *prop;
    }

    /**
     * @brief 层数属性 (1-8)
     */
    static const IntegerProperty& LAYERS_1_8() {
        static auto prop = IntegerProperty::create("layers", 1, 8);
        return *prop;
    }

    /**
     * @brief 液体等级属性 (0-8)
     */
    static const IntegerProperty& LEVEL_0_8() {
        static auto prop = IntegerProperty::create("level", 0, 8);
        return *prop;
    }

    /**
     * @brief 液体等级属性 (0-15)
     */
    static const IntegerProperty& LEVEL_0_15() {
        static auto prop = IntegerProperty::create("level", 0, 15);
        return *prop;
    }

    /**
     * @brief 红石信号强度属性 (0-15)
     */
    static const IntegerProperty& POWER_0_15() {
        static auto prop = IntegerProperty::create("power", 0, 15);
        return *prop;
    }

    /**
     * @brief 延迟属性 (1-4)
     */
    static const IntegerProperty& DELAY_1_4() {
        static auto prop = IntegerProperty::create("delay", 1, 4);
        return *prop;
    }

    /**
     * @brief 距离属性 (1-7)
     */
    static const IntegerProperty& DISTANCE_1_7() {
        static auto prop = IntegerProperty::create("distance", 1, 7);
        return *prop;
    }

    /**
     * @brief 湿度属性 (0-7)
     */
    static const IntegerProperty& MOISTURE_0_7() {
        static auto prop = IntegerProperty::create("moisture", 0, 7);
        return *prop;
    }

    /**
     * @brief 音符属性 (0-24)
     */
    static const IntegerProperty& NOTE_0_24() {
        static auto prop = IntegerProperty::create("note", 0, 24);
        return *prop;
    }

    /**
     * @brief 旋转属性 (0-15)
     */
    static const IntegerProperty& ROTATION_0_15() {
        static auto prop = IntegerProperty::create("rotation", 0, 15);
        return *prop;
    }

    /**
     * @brief 阶段属性 (0-1)
     */
    static const IntegerProperty& STAGE_0_1() {
        static auto prop = IntegerProperty::create("stage", 0, 1);
        return *prop;
    }

    // ========================================================================
    // 箱子类型属性
    // ========================================================================

    /**
     * @brief 箱子类型枚举
     */
    enum class ChestType : u8 {
        Single = 0,   ///< 单箱
        Left = 1,     ///< 双箱左半
        Right = 2     ///< 双箱右半
    };

    /**
     * @brief 箱子类型属性
     */
    static const EnumProperty<ChestType>& CHEST_TYPE() {
        static auto prop = EnumProperty<ChestType>::create("type", {
            ChestType::Single,
            ChestType::Left,
            ChestType::Right
        });
        return *prop;
    }

    // ========================================================================
    // 门相关属性
    // ========================================================================

    /**
     * @brief 双方块半部分枚举（门）
     */
    enum class DoubleBlockHalf : u8 {
        Upper = 0,  ///< 上半部分
        Lower = 1   ///< 下半部分
    };

    /**
     * @brief 双方块半部分属性
     */
    static const EnumProperty<DoubleBlockHalf>& HALF() {
        static auto prop = EnumProperty<DoubleBlockHalf>::create("half", {
            DoubleBlockHalf::Upper,
            DoubleBlockHalf::Lower
        });
        return *prop;
    }

    /**
     * @brief 门铰链位置枚举
     */
    enum class DoorHinge : u8 {
        Left = 0,   ///< 左铰链
        Right = 1   ///< 右铰链
    };

    /**
     * @brief 门铰链属性
     */
    static const EnumProperty<DoorHinge>& HINGE() {
        static auto prop = EnumProperty<DoorHinge>::create("hinge", {
            DoorHinge::Left,
            DoorHinge::Right
        });
        return *prop;
    }

    /**
     * @brief 栅栏门在墙内状态
     */
    static const BooleanProperty& IN_WALL() {
        static auto prop = BooleanProperty::create("in_wall");
        return *prop;
    }

    // ========================================================================
    // 酿造台属性
    // ========================================================================

    /**
     * @brief 酿造台第一个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_0() {
        static auto prop = BooleanProperty::create("has_bottle_0");
        return *prop;
    }

    /**
     * @brief 酿造台第二个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_1() {
        static auto prop = BooleanProperty::create("has_bottle_1");
        return *prop;
    }

    /**
     * @brief 酿造台第三个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_2() {
        static auto prop = BooleanProperty::create("has_bottle_2");
        return *prop;
    }

    // ========================================================================
    // 床属性
    // ========================================================================

    /**
     * @brief 床部分枚举
     */
    enum class BedPart : u8 {
        Head = 0,  ///< 床头
        Foot = 1   ///< 床尾
    };

    /**
     * @brief 床部分属性
     */
    static const EnumProperty<BedPart>& BED_PART() {
        static auto prop = EnumProperty<BedPart>::create("part", {
            BedPart::Head,
            BedPart::Foot
        });
        return *prop;
    }

    // ========================================================================
    // 蛋糕属性
    // ========================================================================

    /**
     * @brief 蛋糕已被吃的片数 (0-6)
     */
    static const IntegerProperty& BITES_0_6() {
        static auto prop = IntegerProperty::create("bites", 0, 6);
        return *prop;
    }

    // ========================================================================
    // 重生锚属性
    // ========================================================================

    /**
     * @brief 重生锚充能等级 (0-4)
     */
    static const IntegerProperty& CHARGES_0_4() {
        static auto prop = IntegerProperty::create("charges", 0, 4);
        return *prop;
    }

    // ========================================================================
    // 唱片机属性
    // ========================================================================

    /**
     * @brief 唱片机是否有唱片
     */
    static const BooleanProperty& HAS_RECORD() {
        static auto prop = BooleanProperty::create("has_record");
        return *prop;
    }

    // ========================================================================
    // 钟属性
    // ========================================================================

    /**
     * @brief 钟附着类型枚举
     */
    enum class BellAttachment : u8 {
        Floor = 0,       ///< 地面
        Ceiling = 1,     ///< 天花板
        SingleWall = 2,  ///< 单面墙
        DoubleWall = 3   ///< 双面墙
    };

    /**
     * @brief 钟附着类型属性
     */
    static const EnumProperty<BellAttachment>& BELL_ATTACHMENT() {
        static auto prop = EnumProperty<BellAttachment>::create("attachment", {
            BellAttachment::Floor,
            BellAttachment::Ceiling,
            BellAttachment::SingleWall,
            BellAttachment::DoubleWall
        });
        return *prop;
    }

    // ========================================================================
    // 讲台属性
    // ========================================================================

    /**
     * @brief 讲台是否有书
     */
    static const BooleanProperty& HAS_BOOK() {
        static auto prop = BooleanProperty::create("has_book");
        return *prop;
    }

    // ========================================================================
    // 炼药锅属性
    // ========================================================================

    /**
     * @brief 炼药锅水位 (0-3)
     */
    static const IntegerProperty& LEVEL_0_3() {
        static auto prop = IntegerProperty::create("level", 0, 3);
        return *prop;
    }

    // ========================================================================
    // 楼梯形状属性
    // ========================================================================

    /**
     * @brief 楼梯形状枚举
     *
     * - STRAIGHT: 直梯
     * - INNER_LEFT: 内角左转
     * - INNER_RIGHT: 内角右转
     * - OUTER_LEFT: 外角左转
     * - OUTER_RIGHT: 外角右转
     */
    enum class StairsShape : u8 {
        Straight = 0,
        InnerLeft = 1,
        InnerRight = 2,
        OuterLeft = 3,
        OuterRight = 4
    };

    /**
     * @brief 楼梯形状属性
     */
    static const EnumProperty<StairsShape>& STAIRS_SHAPE() {
        static auto prop = EnumProperty<StairsShape>::create("shape", {
            StairsShape::Straight,
            StairsShape::InnerLeft,
            StairsShape::InnerRight,
            StairsShape::OuterLeft,
            StairsShape::OuterRight
        });
        return *prop;
    }

    // ========================================================================
    // 台阶类型属性
    // ========================================================================

    /**
     * @brief 台阶类型枚举
     *
     * - BOTTOM: 下半台阶
     * - TOP: 上半台阶
     * - DOUBLE: 双层台阶（完整方块）
     */
    enum class SlabType : u8 {
        Bottom = 0,
        Top = 1,
        Double = 2
    };

    /**
     * @brief 台阶类型属性
     */
    static const EnumProperty<SlabType>& SLAB_TYPE() {
        static auto prop = EnumProperty<SlabType>::create("type", {
            SlabType::Bottom,
            SlabType::Top,
            SlabType::Double
        });
        return *prop;
    }

    // ========================================================================
    // 墙连接高度属性
    // ========================================================================

    /**
     * @brief 墙连接高度枚举
     *
     * - NONE: 无连接
     * - LOW: 低连接（与栅栏等连接）
     * - TALL: 高连接（与墙连接）
     */
    enum class WallHeight : u8 {
        None = 0,
        Low = 1,
        Tall = 2
    };

    /**
     * @brief 墙北面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_NORTH() {
        static auto prop = EnumProperty<WallHeight>::create("north", {
            WallHeight::None,
            WallHeight::Low,
            WallHeight::Tall
        });
        return *prop;
    }

    /**
     * @brief 墙东面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_EAST() {
        static auto prop = EnumProperty<WallHeight>::create("east", {
            WallHeight::None,
            WallHeight::Low,
            WallHeight::Tall
        });
        return *prop;
    }

    /**
     * @brief 墙南面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_SOUTH() {
        static auto prop = EnumProperty<WallHeight>::create("south", {
            WallHeight::None,
            WallHeight::Low,
            WallHeight::Tall
        });
        return *prop;
    }

    /**
     * @brief 墙西面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_WEST() {
        static auto prop = EnumProperty<WallHeight>::create("west", {
            WallHeight::None,
            WallHeight::Low,
            WallHeight::Tall
        });
        return *prop;
    }

    // ========================================================================
    // 附着面属性（按钮、拉杆等）
    // ========================================================================

    /**
     * @brief 附着面枚举
     */
    enum class AttachFace : u8 {
        Floor = 0,   ///< 附着在地面（按钮朝上）
        Wall = 1,    ///< 附着在墙上
        Ceiling = 2  ///< 附着在天花板（按钮朝下）
    };

    /**
     * @brief 附着面属性
     */
    static const EnumProperty<AttachFace>& ATTACH_FACE() {
        static auto prop = EnumProperty<AttachFace>::create("face", {
            AttachFace::Floor,
            AttachFace::Wall,
            AttachFace::Ceiling
        });
        return *prop;
    }

    // ========================================================================
    // 海泡菜/蛋属性
    // ========================================================================

    /**
     * @brief 海泡菜数量属性 (1-4)
     */
    static const IntegerProperty& PICKLES_1_4() {
        static auto prop = IntegerProperty::create("pickles", 1, 4);
        return *prop;
    }

    /**
     * @brief 蛋数量属性 (1-4)
     */
    static const IntegerProperty& EGGS_1_4() {
        static auto prop = IntegerProperty::create("eggs", 1, 4);
        return *prop;
    }

    /**
     * @brief 孵化阶段属性 (0-2)
     */
    static const IntegerProperty& HATCH_0_2() {
        static auto prop = IntegerProperty::create("hatch", 0, 2);
        return *prop;
    }

private:
    // 禁止实例化
    BlockStateProperties() = delete;
    BlockStateProperties(const BlockStateProperties&) = delete;
    BlockStateProperties& operator=(const BlockStateProperties&) = delete;
};

} // namespace mc

// ============================================================================
// 枚举特征特化 - BlockStateProperties 枚举类型
// 实现在 EnumProperty.cpp
// 注意：特化必须在命名空间外，使用完整限定名
// ============================================================================

template<>
struct mc::EnumProperty<mc::BlockStateProperties::DoorHinge>::Traits {
    static mc::String toString(const mc::BlockStateProperties::DoorHinge& value);
    static mc::Optional<mc::BlockStateProperties::DoorHinge> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::DoubleBlockHalf>::Traits {
    static mc::String toString(const mc::BlockStateProperties::DoubleBlockHalf& value);
    static mc::Optional<mc::BlockStateProperties::DoubleBlockHalf> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::ChestType>::Traits {
    static mc::String toString(const mc::BlockStateProperties::ChestType& value);
    static mc::Optional<mc::BlockStateProperties::ChestType> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::AttachFace>::Traits {
    static mc::String toString(const mc::BlockStateProperties::AttachFace& value);
    static mc::Optional<mc::BlockStateProperties::AttachFace> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::StairsShape>::Traits {
    static mc::String toString(const mc::BlockStateProperties::StairsShape& value);
    static mc::Optional<mc::BlockStateProperties::StairsShape> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::SlabType>::Traits {
    static mc::String toString(const mc::BlockStateProperties::SlabType& value);
    static mc::Optional<mc::BlockStateProperties::SlabType> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::WallHeight>::Traits {
    static mc::String toString(const mc::BlockStateProperties::WallHeight& value);
    static mc::Optional<mc::BlockStateProperties::WallHeight> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::BedPart>::Traits {
    static mc::String toString(const mc::BlockStateProperties::BedPart& value);
    static mc::Optional<mc::BlockStateProperties::BedPart> fromName(mc::StringView name);
};

template<>
struct mc::EnumProperty<mc::BlockStateProperties::BellAttachment>::Traits {
    static mc::String toString(const mc::BlockStateProperties::BellAttachment& value);
    static mc::Optional<mc::BlockStateProperties::BellAttachment> fromName(mc::StringView name);
};
