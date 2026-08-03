/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

/**
 * @file BlockStateProperties.hpp
 * @brief 预定义的方块状态属性
 *
 * 参考: net.minecraft.state.properties.BlockStateProperties
 *
 * 这个文件包含所有方块状态常用的属性定义。
 * 属性是静态单例，应该通过引用访问。
 */

#include "../../world/gen/jigsaw/JigsawOrientation.hpp"
#include "../Direction.hpp"
#include "BooleanProperty.hpp"
#include "DirectionProperty.hpp"
#include "EnumProperty.hpp"
#include "IntegerProperty.hpp"
#include "common/core/Types.hpp"

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
    static const BooleanProperty& ATTACHED()
    {
        static auto prop = BooleanProperty::create("attached");
        return *prop;
    }

    /**
     * @brief 是否在底部（门、活板门等的下半部分）
     */
    static const BooleanProperty& BOTTOM()
    {
        static auto prop = BooleanProperty::create("bottom");
        return *prop;
    }

    /**
     * @brief 是否有条件（命令方块）
     */
    static const BooleanProperty& CONDITIONAL()
    {
        static auto prop = BooleanProperty::create("conditional");
        return *prop;
    }

    /**
     * @brief 是否已被拆除（绊线）
     */
    static const BooleanProperty& DISARMED()
    {
        static auto prop = BooleanProperty::create("disarmed");
        return *prop;
    }

    /**
     * @brief 是否有拖拽（灵魂沙上的水）
     */
    static const BooleanProperty& DRAG()
    {
        static auto prop = BooleanProperty::create("drag");
        return *prop;
    }

    /**
     * @brief 是否启用（漏斗、活塞等）
     */
    static const BooleanProperty& ENABLED()
    {
        static auto prop = BooleanProperty::create("enabled");
        return *prop;
    }

    /**
     * @brief 是否伸出（活塞）
     */
    static const BooleanProperty& EXTENDED()
    {
        static auto prop = BooleanProperty::create("extended");
        return *prop;
    }

    /**
     * @brief 是否有眼（末地传送门框架）
     */
    static const BooleanProperty& EYE()
    {
        static auto prop = BooleanProperty::create("eye");
        return *prop;
    }

    /**
     * @brief 是否正在下落（沙子、砾石等）
     */
    static const BooleanProperty& FALLING()
    {
        static auto prop = BooleanProperty::create("falling");
        return *prop;
    }

    /**
     * @brief 是否悬挂（灯笼等）
     */
    static const BooleanProperty& HANGING()
    {
        static auto prop = BooleanProperty::create("hanging");
        return *prop;
    }

    /**
     * @brief 是否反转（日光探测器夜间模式）
     */
    static const BooleanProperty& INVERTED()
    {
        static auto prop = BooleanProperty::create("inverted");
        return *prop;
    }

    /**
     * @brief 是否点亮（火把、熔炉等）
     */
    static const BooleanProperty& LIT()
    {
        static auto prop = BooleanProperty::create("lit");
        return *prop;
    }

    /**
     * @brief 是否锁定（比较器）
     */
    static const BooleanProperty& LOCKED()
    {
        static auto prop = BooleanProperty::create("locked");
        return *prop;
    }

    /**
     * @brief 是否被占用（床）
     */
    static const BooleanProperty& OCCUPIED()
    {
        static auto prop = BooleanProperty::create("occupied");
        return *prop;
    }

    /**
     * @brief 是否打开（门、活板门、栅栏门等）
     */
    static const BooleanProperty& OPEN()
    {
        static auto prop = BooleanProperty::create("open");
        return *prop;
    }

    /**
     * @brief 是否持久（树叶）
     */
    static const BooleanProperty& PERSISTENT()
    {
        static auto prop = BooleanProperty::create("persistent");
        return *prop;
    }

    /**
     * @brief 是否被充能
     */
    static const BooleanProperty& POWERED()
    {
        static auto prop = BooleanProperty::create("powered");
        return *prop;
    }

    /**
     * @brief 是否积雪（草方块等）
     */
    static const BooleanProperty& SNOWY()
    {
        static auto prop = BooleanProperty::create("snowy");
        return *prop;
    }

    /**
     * @brief 是否被触发（命令方块）
     */
    static const BooleanProperty& TRIGGERED()
    {
        static auto prop = BooleanProperty::create("triggered");
        return *prop;
    }

    /**
     * @brief 是否不稳定（TNT）
     */
    static const BooleanProperty& UNSTABLE()
    {
        static auto prop = BooleanProperty::create("unstable");
        return *prop;
    }

    /**
     * @brief 是否含水（栅栏、台阶等）
     */
    static const BooleanProperty& WATERLOGGED()
    {
        static auto prop = BooleanProperty::create("waterlogged");
        return *prop;
    }

    /**
     * @brief 是否为信号火（营火）
     */
    static const BooleanProperty& SIGNAL_FIRE()
    {
        static auto prop = BooleanProperty::create("signal_fire");
        return *prop;
    }

    /**
     * @brief 是否向上（栅栏、墙等）
     */
    static const BooleanProperty& UP()
    {
        static auto prop = BooleanProperty::create("up");
        return *prop;
    }

    /**
     * @brief 是否向下
     */
    static const BooleanProperty& DOWN()
    {
        static auto prop = BooleanProperty::create("down");
        return *prop;
    }

    /**
     * @brief 是否向北
     */
    static const BooleanProperty& NORTH()
    {
        static auto prop = BooleanProperty::create("north");
        return *prop;
    }

    /**
     * @brief 是否向南
     */
    static const BooleanProperty& SOUTH()
    {
        static auto prop = BooleanProperty::create("south");
        return *prop;
    }

    /**
     * @brief 是否向东
     */
    static const BooleanProperty& EAST()
    {
        static auto prop = BooleanProperty::create("east");
        return *prop;
    }

    /**
     * @brief 是否向西
     */
    static const BooleanProperty& WEST()
    {
        static auto prop = BooleanProperty::create("west");
        return *prop;
    }

    // ========================================================================
    // 方向属性
    // ========================================================================

    /**
     * @brief 朝向属性（所有6个方向）
     */
    static const DirectionProperty& FACING()
    {
        static auto prop = DirectionProperty::create("facing");
        return *prop;
    }

    /**
     * @brief 朝向属性（仅水平方向）
     */
    static const DirectionProperty& HORIZONTAL_FACING()
    {
        static auto prop = DirectionProperty::createHorizontal("facing");
        return *prop;
    }

    /**
     * @brief 垂直方向属性（仅上/下方向）
     *
     * 用于滴水石锥等方块。
     *
     * 参考: net.minecraft.world.level.block.state.properties.BlockStateProperties.VERTICAL_DIRECTION
     */
    static const DirectionProperty& VERTICAL_DIRECTION()
    {
        static auto prop = DirectionProperty::create(
            "vertical_direction", [](Direction d) { return d == Direction::Up || d == Direction::Down; });
        return *prop;
    }

    /**
     * @brief 朝向属性（除上之外的所有方向）
     */
    static const DirectionProperty& FACING_EXCEPT_UP()
    {
        static auto prop = DirectionProperty::create("facing", [](Direction d) { return d != Direction::Up; });
        return *prop;
    }

    // ========================================================================
    // 坐标轴属性
    // ========================================================================

    /**
     * @brief 坐标轴属性（所有三个轴）
     */
    static const EnumProperty<Axis>& AXIS()
    {
        static auto prop = AxisProperty::create("axis");
        return *prop;
    }

    /**
     * @brief 坐标轴属性（仅水平轴X和Z）
     */
    static const EnumProperty<Axis>& HORIZONTAL_AXIS()
    {
        static auto prop = EnumProperty<Axis>::create("axis", {Axis::X, Axis::Z});
        return *prop;
    }

    // ========================================================================
    // 整数属性
    // ========================================================================

    /**
     * @brief 年龄属性 (0-1)
     */
    static const IntegerProperty& AGE_0_1()
    {
        static auto prop = IntegerProperty::create("age", 0, 1);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-2)
     */
    static const IntegerProperty& AGE_0_2()
    {
        static auto prop = IntegerProperty::create("age", 0, 2);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-3)
     */
    static const IntegerProperty& AGE_0_3()
    {
        static auto prop = IntegerProperty::create("age", 0, 3);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-4)
     */
    static const IntegerProperty& AGE_0_4()
    {
        static auto prop = IntegerProperty::create("age", 0, 4);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-5)
     */
    static const IntegerProperty& AGE_0_5()
    {
        static auto prop = IntegerProperty::create("age", 0, 5);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-7)
     */
    static const IntegerProperty& AGE_0_7()
    {
        static auto prop = IntegerProperty::create("age", 0, 7);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-15)
     */
    static const IntegerProperty& AGE_0_15()
    {
        static auto prop = IntegerProperty::create("age", 0, 15);
        return *prop;
    }

    /**
     * @brief 年龄属性 (0-25)
     */
    static const IntegerProperty& AGE_0_25()
    {
        static auto prop = IntegerProperty::create("age", 0, 25);
        return *prop;
    }

    /**
     * @brief 层数属性 (1-8)
     */
    static const IntegerProperty& LAYERS_1_8()
    {
        static auto prop = IntegerProperty::create("layers", 1, 8);
        return *prop;
    }

    /**
     * @brief 液体等级属性 (0-8)
     */
    static const IntegerProperty& LEVEL_0_8()
    {
        static auto prop = IntegerProperty::create("level", 0, 8);
        return *prop;
    }

    /**
     * @brief 液体等级属性 (0-15)
     */
    static const IntegerProperty& LEVEL_0_15()
    {
        static auto prop = IntegerProperty::create("level", 0, 15);
        return *prop;
    }

    /**
     * @brief 红石信号强度属性 (0-15)
     */
    static const IntegerProperty& POWER_0_15()
    {
        static auto prop = IntegerProperty::create("power", 0, 15);
        return *prop;
    }

    /**
     * @brief 延迟属性 (1-4)
     */
    static const IntegerProperty& DELAY_1_4()
    {
        static auto prop = IntegerProperty::create("delay", 1, 4);
        return *prop;
    }

    /**
     * @brief 距离属性 (1-7)
     */
    static const IntegerProperty& DISTANCE_1_7()
    {
        static auto prop = IntegerProperty::create("distance", 1, 7);
        return *prop;
    }

    /**
     * @brief 湿度属性 (0-7)
     */
    static const IntegerProperty& MOISTURE_0_7()
    {
        static auto prop = IntegerProperty::create("moisture", 0, 7);
        return *prop;
    }

    /**
     * @brief 音符属性 (0-24)
     */
    static const IntegerProperty& NOTE_0_24()
    {
        static auto prop = IntegerProperty::create("note", 0, 24);
        return *prop;
    }

    /**
     * @brief 旋转属性 (0-15)
     */
    static const IntegerProperty& ROTATION_0_15()
    {
        static auto prop = IntegerProperty::create("rotation", 0, 15);
        return *prop;
    }

    /**
     * @brief 阶段属性 (0-1)
     */
    static const IntegerProperty& STAGE_0_1()
    {
        static auto prop = IntegerProperty::create("stage", 0, 1);
        return *prop;
    }

    /**
     * @brief 落叶段数属性 (1-4)
     *
     * 用于 LeafLitterBlock，表示方块中落叶的段数。
     */
    static const IntegerProperty& SEGMENT_AMOUNT()
    {
        static auto prop = IntegerProperty::create("segment_amount", 1, 4);
        return *prop;
    }

    // ========================================================================
    // 箱子类型属性
    // ========================================================================

    /**
     * @brief 箱子类型枚举
     */
    enum class ChestType : u8 {
        Single = 0, ///< 单箱
        Left = 1,   ///< 双箱左半
        Right = 2   ///< 双箱右半
    };

    /**
     * @brief 箱子类型属性
     */
    static const EnumProperty<ChestType>& CHEST_TYPE()
    {
        static auto prop =
            EnumProperty<ChestType>::create("type", {ChestType::Single, ChestType::Left, ChestType::Right});
        return *prop;
    }

    // ========================================================================
    // 门相关属性
    // ========================================================================

    /**
     * @brief 双方块半部分枚举（门）
     */
    enum class DoubleBlockHalf : u8 {
        Upper = 0, ///< 上半部分
        Lower = 1  ///< 下半部分
    };

    /**
     * @brief 双方块半部分属性
     */
    static const EnumProperty<DoubleBlockHalf>& DOUBLE_BLOCK_HALF()
    {
        static auto prop =
            EnumProperty<DoubleBlockHalf>::create("half", {DoubleBlockHalf::Upper, DoubleBlockHalf::Lower});
        return *prop;
    }

    /**
     * @brief 门铰链位置枚举
     */
    enum class DoorHinge : u8 {
        Left = 0, ///< 左铰链
        Right = 1 ///< 右铰链
    };

    /**
     * @brief 门铰链属性
     */
    static const EnumProperty<DoorHinge>& HINGE()
    {
        static auto prop = EnumProperty<DoorHinge>::create("hinge", {DoorHinge::Left, DoorHinge::Right});
        return *prop;
    }

    /**
     * @brief 栅栏门在墙内状态
     */
    static const BooleanProperty& IN_WALL()
    {
        static auto prop = BooleanProperty::create("in_wall");
        return *prop;
    }

    // ========================================================================
    // 酿造台属性
    // ========================================================================

    /**
     * @brief 酿造台第一个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_0()
    {
        static auto prop = BooleanProperty::create("has_bottle_0");
        return *prop;
    }

    /**
     * @brief 酿造台第二个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_1()
    {
        static auto prop = BooleanProperty::create("has_bottle_1");
        return *prop;
    }

    /**
     * @brief 酿造台第三个槽位是否有瓶子
     */
    static const BooleanProperty& HAS_BOTTLE_2()
    {
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
        Head = 0, ///< 床头
        Foot = 1  ///< 床尾
    };

    /**
     * @brief 床部分属性
     */
    static const EnumProperty<BedPart>& BED_PART()
    {
        static auto prop = EnumProperty<BedPart>::create("part", {BedPart::Head, BedPart::Foot});
        return *prop;
    }

    // ========================================================================
    // 蛋糕属性
    // ========================================================================

    /**
     * @brief 蛋糕已被吃的片数 (0-6)
     */
    static const IntegerProperty& BITES_0_6()
    {
        static auto prop = IntegerProperty::create("bites", 0, 6);
        return *prop;
    }

    // ========================================================================
    // 重生锚属性
    // ========================================================================

    /**
     * @brief 重生锚充能等级 (0-4)
     */
    static const IntegerProperty& CHARGES_0_4()
    {
        static auto prop = IntegerProperty::create("charges", 0, 4);
        return *prop;
    }

    // ========================================================================
    // 唱片机属性
    // ========================================================================

    /**
     * @brief 唱片机是否有唱片
     */
    static const BooleanProperty& HAS_RECORD()
    {
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
        Floor = 0,      ///< 地面
        Ceiling = 1,    ///< 天花板
        SingleWall = 2, ///< 单面墙
        DoubleWall = 3  ///< 双面墙
    };

    /**
     * @brief 钟附着类型属性
     */
    static const EnumProperty<BellAttachment>& BELL_ATTACHMENT()
    {
        static auto prop = EnumProperty<BellAttachment>::create("attachment",
            {BellAttachment::Floor, BellAttachment::Ceiling, BellAttachment::SingleWall, BellAttachment::DoubleWall});
        return *prop;
    }

    // ========================================================================
    // 讲台属性
    // ========================================================================

    /**
     * @brief 讲台是否有书
     */
    static const BooleanProperty& HAS_BOOK()
    {
        static auto prop = BooleanProperty::create("has_book");
        return *prop;
    }

    // ========================================================================
    // 炼药锅属性
    // ========================================================================

    /**
     * @brief 炼药锅水位 (0-3)
     *
     * 用于旧版空炼药锅 CauldronBlock（保留兼容性）。
     * 新代码应优先使用 LEVEL_1_3 配合 LayeredCauldronBlock。
     */
    static const IntegerProperty& LEVEL_0_3()
    {
        static auto prop = IntegerProperty::create("level", 0, 3);
        return *prop;
    }

    /**
     * @brief 分层炼药锅水位 (1-3)
     *
     * 对应 MC 原版 BlockStateProperties.LEVEL_CAULDRON (IntegerProperty, 1-3)。
     * LayeredCauldronBlock 使用此属性：水位1=最低，水位3=满。
     * 当水位降至0时，方块应替换为空炼药锅 (CauldronBlock)。
     */
    static const IntegerProperty& LEVEL_1_3()
    {
        static auto prop = IntegerProperty::create("level", 1, 3);
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
    enum class StairsShape : u8 { Straight = 0, InnerLeft = 1, InnerRight = 2, OuterLeft = 3, OuterRight = 4 };

    /**
     * @brief 楼梯形状属性
     */
    static const EnumProperty<StairsShape>& STAIRS_SHAPE()
    {
        static auto prop = EnumProperty<StairsShape>::create("shape",
            {StairsShape::Straight,
                StairsShape::InnerLeft,
                StairsShape::InnerRight,
                StairsShape::OuterLeft,
                StairsShape::OuterRight});
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
    enum class SlabType : u8 { Bottom = 0, Top = 1, Double = 2 };

    /**
     * @brief 台阶类型属性
     */
    static const EnumProperty<SlabType>& SLAB_TYPE()
    {
        static auto prop = EnumProperty<SlabType>::create("type", {SlabType::Bottom, SlabType::Top, SlabType::Double});
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
    enum class WallHeight : u8 { None = 0, Low = 1, Tall = 2 };

    /**
     * @brief 墙北面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_NORTH()
    {
        static auto prop =
            EnumProperty<WallHeight>::create("north", {WallHeight::None, WallHeight::Low, WallHeight::Tall});
        return *prop;
    }

    /**
     * @brief 墙东面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_EAST()
    {
        static auto prop =
            EnumProperty<WallHeight>::create("east", {WallHeight::None, WallHeight::Low, WallHeight::Tall});
        return *prop;
    }

    /**
     * @brief 墙南面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_SOUTH()
    {
        static auto prop =
            EnumProperty<WallHeight>::create("south", {WallHeight::None, WallHeight::Low, WallHeight::Tall});
        return *prop;
    }

    /**
     * @brief 墙西面高度属性
     */
    static const EnumProperty<WallHeight>& WALL_HEIGHT_WEST()
    {
        static auto prop =
            EnumProperty<WallHeight>::create("west", {WallHeight::None, WallHeight::Low, WallHeight::Tall});
        return *prop;
    }

    // ========================================================================
    // 附着面属性（按钮、拉杆等）
    // ========================================================================

    /**
     * @brief 附着面枚举
     */
    enum class AttachFace : u8 {
        Floor = 0,  ///< 附着在地面（按钮朝上）
        Wall = 1,   ///< 附着在墙上
        Ceiling = 2 ///< 附着在天花板（按钮朝下）
    };

    /**
     * @brief 附着面属性
     */
    static const EnumProperty<AttachFace>& ATTACH_FACE()
    {
        static auto prop =
            EnumProperty<AttachFace>::create("face", {AttachFace::Floor, AttachFace::Wall, AttachFace::Ceiling});
        return *prop;
    }

    // ========================================================================
    // 海泡菜/蛋属性
    // ========================================================================

    /**
     * @brief 海泡菜数量属性 (1-4)
     */
    static const IntegerProperty& PICKLES_1_4()
    {
        static auto prop = IntegerProperty::create("pickles", 1, 4);
        return *prop;
    }

    /**
     * @brief 蛋数量属性 (1-4)
     */
    static const IntegerProperty& EGGS_1_4()
    {
        static auto prop = IntegerProperty::create("eggs", 1, 4);
        return *prop;
    }

    /**
     * @brief 花瓣数量属性 (1-4)
     *
     * 用于 FlowerBedBlock（粉红色花瓣、野花），表示方块中花瓣的数量。
     */
    static const IntegerProperty& FLOWER_AMOUNT()
    {
        static auto prop = IntegerProperty::create("flower_amount", 1, 4);
        return *prop;
    }

    /**
     * @brief 蜡烛数量属性 (1-4)
     *
     * 用于 CandleBlock，表示方块中蜡烛的数量。
     */
    static const IntegerProperty& CANDLES()
    {
        static auto prop = IntegerProperty::create("candles", 1, 4);
        return *prop;
    }

    /**
     * @brief 孵化阶段属性 (0-2)
     */
    static const IntegerProperty& HATCH_0_2()
    {
        static auto prop = IntegerProperty::create("hatch", 0, 2);
        return *prop;
    }

    // ========================================================================
    // 竹子属性
    // ========================================================================

    /**
     * @brief 竹子叶子类型枚举
     */
    enum class BambooLeaves : u8 {
        None = 0,  ///< 无叶子
        Small = 1, ///< 小叶子
        Large = 2  ///< 大叶子
    };

    /**
     * @brief 竹子叶子属性
     */
    static const EnumProperty<BambooLeaves>& BAMBOO_LEAVES_PROP()
    {
        static auto prop = EnumProperty<BambooLeaves>::create(
            "leaves", {BambooLeaves::None, BambooLeaves::Small, BambooLeaves::Large});
        return *prop;
    }

    // ========================================================================
    // 楼梯/活板门半部分属性
    // ========================================================================

    /**
     * @brief 半部分枚举（楼梯、活板门）
     *
     * MC 1.16.5: net.minecraft.state.properties.Half
     * 注意：与 DoubleBlockHalf (Upper/Lower) 不同，Half 是 Top/Bottom
     */
    enum class Half : u8 {
        Top = 0,   ///< 上半部分
        Bottom = 1 ///< 下半部分
    };

    /**
     * @brief 半部分属性
     */
    static const EnumProperty<Half>& HALF()
    {
        static auto prop = EnumProperty<Half>::create("half", {Half::Top, Half::Bottom});
        return *prop;
    }

    // ========================================================================
    // 铁轨形状属性
    // ========================================================================

    /**
     * @brief 铁轨形状枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.RailShape
     */
    enum class RailShape : u8 {
        NorthSouth = 0,     ///< 南北直轨
        EastWest = 1,       ///< 东西直轨
        AscendingEast = 2,  ///< 向东上升
        AscendingWest = 3,  ///< 向西上升
        AscendingNorth = 4, ///< 向北上升
        AscendingSouth = 5, ///< 向南上升
        SouthEast = 6,      ///< 东南弯轨
        SouthWest = 7,      ///< 西南弯轨
        NorthWest = 8,      ///< 西北弯轨
        NorthEast = 9       ///< 东北弯轨
    };

    /**
     * @brief 铁轨形状属性（完整，包含弯轨）
     */
    static const EnumProperty<RailShape>& RAIL_SHAPE()
    {
        static auto prop = EnumProperty<RailShape>::create("shape",
            {RailShape::NorthSouth,
                RailShape::EastWest,
                RailShape::AscendingEast,
                RailShape::AscendingWest,
                RailShape::AscendingNorth,
                RailShape::AscendingSouth,
                RailShape::SouthEast,
                RailShape::SouthWest,
                RailShape::NorthWest,
                RailShape::NorthEast});
        return *prop;
    }

    /**
     * @brief 铁轨形状属性（仅直轨，用于动力铁轨等）
     */
    static const EnumProperty<RailShape>& RAIL_SHAPE_STRAIGHT()
    {
        static auto prop = EnumProperty<RailShape>::create("shape",
            {RailShape::NorthSouth,
                RailShape::EastWest,
                RailShape::AscendingEast,
                RailShape::AscendingWest,
                RailShape::AscendingNorth,
                RailShape::AscendingSouth});
        return *prop;
    }

    // ========================================================================
    // 红石线连接状态属性
    // ========================================================================

    /**
     * @brief 红石线连接状态枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.RedstoneSide
     */
    enum class RedstoneSide : u8 {
        Up = 0,   ///< 向上连接
        Side = 1, ///< 侧面连接
        None = 2  ///< 无连接
    };

    /**
     * @brief 红石线北面连接状态
     */
    static const EnumProperty<RedstoneSide>& REDSTONE_NORTH()
    {
        static auto prop =
            EnumProperty<RedstoneSide>::create("north", {RedstoneSide::Up, RedstoneSide::Side, RedstoneSide::None});
        return *prop;
    }

    /**
     * @brief 红石线东面连接状态
     */
    static const EnumProperty<RedstoneSide>& REDSTONE_EAST()
    {
        static auto prop =
            EnumProperty<RedstoneSide>::create("east", {RedstoneSide::Up, RedstoneSide::Side, RedstoneSide::None});
        return *prop;
    }

    /**
     * @brief 红石线南面连接状态
     */
    static const EnumProperty<RedstoneSide>& REDSTONE_SOUTH()
    {
        static auto prop =
            EnumProperty<RedstoneSide>::create("south", {RedstoneSide::Up, RedstoneSide::Side, RedstoneSide::None});
        return *prop;
    }

    /**
     * @brief 红石线西面连接状态
     */
    static const EnumProperty<RedstoneSide>& REDSTONE_WEST()
    {
        static auto prop =
            EnumProperty<RedstoneSide>::create("west", {RedstoneSide::Up, RedstoneSide::Side, RedstoneSide::None});
        return *prop;
    }

    // ========================================================================
    // 活塞类型属性
    // ========================================================================

    /**
     * @brief 活塞类型枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.PistonType
     */
    enum class PistonType : u8 {
        Default = 0, ///< 普通活塞
        Sticky = 1   ///< 粘性活塞
    };

    /**
     * @brief 活塞类型属性（用于活塞头）
     */
    static const EnumProperty<PistonType>& PISTON_TYPE()
    {
        static auto prop = EnumProperty<PistonType>::create("type", {PistonType::Default, PistonType::Sticky});
        return *prop;
    }

    // ========================================================================
    // 比较器模式属性
    // ========================================================================

    /**
     * @brief 比较器模式枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.ComparatorMode
     */
    enum class ComparatorMode : u8 {
        Compare = 0, ///< 比较模式
        Subtract = 1 ///< 减法模式
    };

    /**
     * @brief 比较器模式属性
     */
    static const EnumProperty<ComparatorMode>& COMPARATOR_MODE()
    {
        static auto prop =
            EnumProperty<ComparatorMode>::create("mode", {ComparatorMode::Compare, ComparatorMode::Subtract});
        return *prop;
    }

    // ========================================================================
    // 音符盒乐器属性
    // ========================================================================

    /**
     * @brief 音符盒乐器枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.NoteBlockInstrument
     */
    enum class NoteBlockInstrument : u8 {
        Harp = 0,
        Basedrum = 1,
        Snare = 2,
        Hat = 3,
        Bass = 4,
        Flute = 5,
        Bell = 6,
        Guitar = 7,
        Chime = 8,
        Xylophone = 9,
        IronXylophone = 10,
        CowBell = 11,
        Didgeridoo = 12,
        Bit = 13,
        Banjo = 14,
        Pling = 15
    };

    /**
     * @brief 音符盒乐器属性
     */
    static const EnumProperty<NoteBlockInstrument>& NOTE_BLOCK_INSTRUMENT()
    {
        static auto prop = EnumProperty<NoteBlockInstrument>::create("instrument",
            {NoteBlockInstrument::Harp,
                NoteBlockInstrument::Basedrum,
                NoteBlockInstrument::Snare,
                NoteBlockInstrument::Hat,
                NoteBlockInstrument::Bass,
                NoteBlockInstrument::Flute,
                NoteBlockInstrument::Bell,
                NoteBlockInstrument::Guitar,
                NoteBlockInstrument::Chime,
                NoteBlockInstrument::Xylophone,
                NoteBlockInstrument::IronXylophone,
                NoteBlockInstrument::CowBell,
                NoteBlockInstrument::Didgeridoo,
                NoteBlockInstrument::Bit,
                NoteBlockInstrument::Banjo,
                NoteBlockInstrument::Pling});
        return *prop;
    }

    // ========================================================================
    // Jigsaw 方向属性
    // ========================================================================

    /**
     * @brief Jigsaw 方向属性
     *
     * MC 1.16.5: net.minecraft.state.properties.BlockStateProperties.ORIENTATION
     * 用于 Jigsaw 方块，表示其 12 种方向组合。
     */
    static const EnumProperty<world::gen::jigsaw::JigsawOrientation>& ORIENTATION()
    {
        static auto prop = EnumProperty<world::gen::jigsaw::JigsawOrientation>::create("orientation",
            {world::gen::jigsaw::JigsawOrientation::DownEast,
                world::gen::jigsaw::JigsawOrientation::DownNorth,
                world::gen::jigsaw::JigsawOrientation::DownSouth,
                world::gen::jigsaw::JigsawOrientation::DownWest,
                world::gen::jigsaw::JigsawOrientation::UpEast,
                world::gen::jigsaw::JigsawOrientation::UpNorth,
                world::gen::jigsaw::JigsawOrientation::UpSouth,
                world::gen::jigsaw::JigsawOrientation::UpWest,
                world::gen::jigsaw::JigsawOrientation::WestUp,
                world::gen::jigsaw::JigsawOrientation::EastUp,
                world::gen::jigsaw::JigsawOrientation::NorthUp,
                world::gen::jigsaw::JigsawOrientation::SouthUp});
        return *prop;
    }

    // ========================================================================
    // 结构方块模式属性
    // ========================================================================

    /**
     * @brief 结构方块模式枚举
     *
     * MC 1.16.5: net.minecraft.state.properties.StructureMode
     * 用于 StructureBlock，表示其四种工作模式。
     */
    enum class StructureMode : u8 {
        Save = 0,   ///< 保存模式 - 保存结构到模板
        Load = 1,   ///< 加载模式 - 从模板加载结构
        Corner = 2, ///< 角落模式 - 定义结构角落
        Data = 3    ///< 数据模式 - 定义实体数据位置
    };

    /**
     * @brief 结构方块模式属性
     *
     * MC 1.16.5: net.minecraft.state.properties.BlockStateProperties.STRUCTURE_BLOCK_MODE
     */
    static const EnumProperty<StructureMode>& STRUCTURE_MODE()
    {
        static auto prop = EnumProperty<StructureMode>::create(
            "mode", {StructureMode::Save, StructureMode::Load, StructureMode::Corner, StructureMode::Data});
        return *prop;
    }

    // ========================================================================
    // 其他布尔属性
    // ========================================================================

    /**
     * @brief 活塞是否为短状态（移动活塞方块使用）
     */
    static const BooleanProperty& SHORT()
    {
        static auto prop = BooleanProperty::create("short");
        return *prop;
    }

    // ========================================================================
    // 铜氧化等级属性 (1.17+)
    // ========================================================================

    /**
     * @brief 铜氧化等级枚举
     *
     * MC 1.17+: net.minecraft.world.level.block.WeatheringCopper.WeatherState
     * 表示铜方块的氧化程度。
     */
    enum class OxidationLevel : u8 {
        Unaffected = 0, ///< 未氧化
        Exposed = 1,    ///< 斑驳
        Weathered = 2,  ///< 锈蚀
        Oxidized = 3    ///< 氧化
    };

    // 注：氧化等级不再作为 block state 属性。vanilla 1.21.11 中不同氧化等级以独立方块名注册
    // （copper/exposed_copper/weathered_copper/oxidized_copper 等），方块状态不含 oxidation。
    // 此枚举保留，供铜方块以成员变量 m_oxidationLevel 持有氧化等级使用。

    // ========================================================================
    // 滴石厚度属性 (1.17+)
    // ========================================================================

    /**
     * @brief 滴石厚度枚举
     *
     * MC 1.17+: net.minecraft.world.level.block.state.properties.DripstoneThickness
     */
    enum class DripstoneThickness : u8 {
        TipMerge = 0, ///< 尖端合并
        Tip = 1,      ///< 尖端
        Frustum = 2,  ///< 截锥
        Middle = 3,   ///< 中间
        Base = 4      ///< 基座
    };

    /**
     * @brief 滴石厚度属性
     */
    static const EnumProperty<DripstoneThickness>& DRIPSTONE_THICKNESS()
    {
        static auto prop = EnumProperty<DripstoneThickness>::create("thickness",
            {DripstoneThickness::TipMerge,
                DripstoneThickness::Tip,
                DripstoneThickness::Frustum,
                DripstoneThickness::Middle,
                DripstoneThickness::Base});
        return *prop;
    }

    // ========================================================================
    // 滴叶倾斜属性 (1.17+)
    // ========================================================================

    /**
     * @brief 滴叶倾斜枚举
     *
     * MC 1.17+: net.minecraft.world.level.block.state.properties.Tilt
     */
    enum class Tilt : u8 {
        None = 0,     ///< 无倾斜
        Unstable = 1, ///< 不稳定
        Partial = 2,  ///< 部分倾斜
        Full = 3      ///< 完全倾斜
    };

    /**
     * @brief 滴叶倾斜属性
     */
    static const EnumProperty<Tilt>& TILT()
    {
        static auto prop = EnumProperty<Tilt>::create("tilt", {Tilt::None, Tilt::Unstable, Tilt::Partial, Tilt::Full});
        return *prop;
    }

    // ========================================================================
    // 幽匿感应器相位属性 (1.19+)
    // ========================================================================

    /**
     * @brief 幽匿感应器相位枚举
     *
     * MC 1.19+: net.minecraft.world.level.block.state.properties.SculkSensorPhase
     */
    enum class SculkSensorPhase : u8 {
        Inactive = 0, ///< 不活跃
        Active = 1,   ///< 活跃
        Cooldown = 2  ///< 冷却
    };

    /**
     * @brief 幽匿感应器相位属性
     */
    static const EnumProperty<SculkSensorPhase>& SCULK_SENSOR_PHASE()
    {
        static auto prop = EnumProperty<SculkSensorPhase>::create(
            "sculk_sensor_phase", {SculkSensorPhase::Inactive, SculkSensorPhase::Active, SculkSensorPhase::Cooldown});
        return *prop;
    }

    // ========================================================================
    // 试炼刷怪笼状态属性 (1.21+)
    // ========================================================================

    /**
     * @brief 试炼刷怪笼状态枚举
     *
     * MC 1.21+: net.minecraft.world.level.block.state.properties.TrialSpawnerState
     */
    enum class TrialSpawnerState : u8 {
        Inactive = 0,                 ///< 不活跃
        WaitingForPlayers = 1,        ///< 等待玩家
        Active = 2,                   ///< 活跃
        WaitingForRewardEjection = 3, ///< 等待奖励弹出
        EjectingReward = 4,           ///< 弹出奖励
        Cooldown = 5                  ///< 冷却
    };

    /**
     * @brief 试炼刷怪笼状态属性
     */
    static const EnumProperty<TrialSpawnerState>& TRIAL_SPAWNER_STATE()
    {
        static auto prop = EnumProperty<TrialSpawnerState>::create("trial_spawner_state",
            {TrialSpawnerState::Inactive,
                TrialSpawnerState::WaitingForPlayers,
                TrialSpawnerState::Active,
                TrialSpawnerState::WaitingForRewardEjection,
                TrialSpawnerState::EjectingReward,
                TrialSpawnerState::Cooldown});
        return *prop;
    }

    // ========================================================================
    // 宝库状态属性 (1.21+)
    // ========================================================================

    /**
     * @brief 宝库状态枚举
     *
     * MC 1.21+: net.minecraft.world.level.block.state.properties.VaultState
     */
    enum class VaultState : u8 {
        Inactive = 0,  ///< 不活跃
        Active = 1,    ///< 活跃
        Unlocking = 2, ///< 解锁中
        Ejecting = 3   ///< 弹出
    };

    /**
     * @brief 宝库状态属性
     */
    static const EnumProperty<VaultState>& VAULT_STATE()
    {
        static auto prop = EnumProperty<VaultState>::create(
            "vault_state", {VaultState::Inactive, VaultState::Active, VaultState::Unlocking, VaultState::Ejecting});
        return *prop;
    }

    // ========================================================================
    // 嘎枝之心状态属性 (1.21.2+)
    // ========================================================================

    /**
     * @brief 嘎枝之心状态枚举
     *
     * MC 1.21.2+: net.minecraft.world.level.block.state.properties.CreakingHeartState
     */
    enum class CreakingHeartState : u8 {
        Uprooted = 0, ///< 被拔出
        Dormant = 1,  ///< 休眠
        Awake = 2     ///< 活跃
    };

    /**
     * @brief 嘎枝之心状态属性
     */
    static const EnumProperty<CreakingHeartState>& CREAKING_HEART_STATE()
    {
        static auto prop = EnumProperty<CreakingHeartState>::create("creaking_heart_state",
            {CreakingHeartState::Uprooted, CreakingHeartState::Dormant, CreakingHeartState::Awake});
        return *prop;
    }

    /**
     * @brief 书架侧链连接部分枚举
     *
     * MC 1.21.4+: net.minecraft.world.level.block.state.properties.SideChainPart
     */
    enum class SideChainPart : u8 {
        Unconnected = 0, ///< 未连接
        Left = 1,        ///< 左端
        Center = 2,      ///< 中间
        Right = 3        ///< 右端
    };

    // ========================================================================
    // SideChainPart 辅助方法
    // 参考: net.minecraft.world.level.block.state.properties.SideChainPart (MC 1.21.11)
    // ========================================================================

    /**
     * @brief 判断侧链部分是否处于连接状态
     */
    [[nodiscard]] static bool isConnected(SideChainPart part) { return part != SideChainPart::Unconnected; }

    /**
     * @brief 判断侧链部分是否朝指定方向连接
     * CENTER 可以向任何方向连接；其他值只能向对应方向连接
     */
    [[nodiscard]] static bool isConnectionTowards(SideChainPart part, SideChainPart direction)
    {
        return part == SideChainPart::Center || part == direction;
    }

    /**
     * @brief 判断侧链部分是否为链的端点
     * 只有 CENTER 不是链的端点
     */
    [[nodiscard]] static bool isChainEnd(SideChainPart part) { return part != SideChainPart::Center; }

    /**
     * @brief 当右侧连接时的侧链状态变化
     */
    [[nodiscard]] static SideChainPart whenConnectedToTheRight(SideChainPart part)
    {
        switch (part) {
            case SideChainPart::Unconnected:
            case SideChainPart::Left:
                return SideChainPart::Left;
            case SideChainPart::Right:
            case SideChainPart::Center:
                return SideChainPart::Center;
        }
        return SideChainPart::Unconnected;
    }

    /**
     * @brief 当左侧连接时的侧链状态变化
     */
    [[nodiscard]] static SideChainPart whenConnectedToTheLeft(SideChainPart part)
    {
        switch (part) {
            case SideChainPart::Unconnected:
            case SideChainPart::Right:
                return SideChainPart::Right;
            case SideChainPart::Center:
            case SideChainPart::Left:
                return SideChainPart::Center;
        }
        return SideChainPart::Unconnected;
    }

    /**
     * @brief 当右侧断开连接时的侧链状态变化
     */
    [[nodiscard]] static SideChainPart whenDisconnectedFromTheRight(SideChainPart part)
    {
        switch (part) {
            case SideChainPart::Unconnected:
            case SideChainPart::Left:
                return SideChainPart::Unconnected;
            case SideChainPart::Right:
            case SideChainPart::Center:
                return SideChainPart::Right;
        }
        return SideChainPart::Unconnected;
    }

    /**
     * @brief 当左侧断开连接时的侧链状态变化
     */
    [[nodiscard]] static SideChainPart whenDisconnectedFromTheLeft(SideChainPart part)
    {
        switch (part) {
            case SideChainPart::Unconnected:
            case SideChainPart::Right:
                return SideChainPart::Unconnected;
            case SideChainPart::Center:
            case SideChainPart::Left:
                return SideChainPart::Left;
        }
        return SideChainPart::Unconnected;
    }

    /**
     * @brief 书架侧链连接属性
     */
    static const EnumProperty<SideChainPart>& SIDE_CHAIN_PART()
    {
        static auto prop = EnumProperty<SideChainPart>::create("side_chain_part",
            {SideChainPart::Unconnected, SideChainPart::Left, SideChainPart::Center, SideChainPart::Right});
        return *prop;
    }

    // ========================================================================
    // 铜傀儡雕像姿态属性 (1.21.11+)
    // ========================================================================

    /**
     * @brief 铜傀儡雕像姿态枚举
     *
     * MC 1.21.11+: net.minecraft.world.level.block.CopperGolemStatueBlock.Pose
     * 表示铜傀儡雕像的姿态，玩家右键点击可循环切换。
     */
    enum class CopperGolemPose : u8 {
        Standing = 0, ///< 站立
        Sitting = 1,  ///< 坐下
        Running = 2,  ///< 奔跑
        Star = 3      ///< 倒地（五角星形）
    };

    /**
     * @brief 铜傀儡雕像姿态属性
     *
     * 对应 MC 原版 BlockStateProperties.COPPER_GOLEM_POSE。
     * 用于 CopperGolemStatueBlock 系列，玩家右键点击雕像时循环切换姿态。
     * 比较器模拟输出 = pose.ordinal() + 1 (1-4)。
     */
    static const EnumProperty<CopperGolemPose>& COPPER_GOLEM_POSE()
    {
        static auto prop = EnumProperty<CopperGolemPose>::create("copper_golem_pose",
            {CopperGolemPose::Standing, CopperGolemPose::Sitting, CopperGolemPose::Running, CopperGolemPose::Star});
        return *prop;
    }

    // ========================================================================
    // 1.17-1.21 新增布尔属性
    // ========================================================================

    /**
     * @brief 是否有浆果（洞穴藤蔓）
     */
    static const BooleanProperty& BERRIES()
    {
        static auto prop = BooleanProperty::create("berries");
        return *prop;
    }

    /**
     * @brief 是否正在尖叫（幽匿尖啸体）
     */
    static const BooleanProperty& SHRIEKING()
    {
        static auto prop = BooleanProperty::create("shrieking");
        return *prop;
    }

    /**
     * @brief 是否可召唤（幽匿尖啸体）
     */
    static const BooleanProperty& CAN_SUMMON()
    {
        static auto prop = BooleanProperty::create("can_summon");
        return *prop;
    }

    /**
     * @brief 是否发光（幽匿催化体）
     */
    static const BooleanProperty& BLOOM()
    {
        static auto prop = BooleanProperty::create("bloom");
        return *prop;
    }

    /**
     * @brief 是否开裂（装饰陶罐）
     */
    static const BooleanProperty& CRACKED()
    {
        static auto prop = BooleanProperty::create("cracked");
        return *prop;
    }

    /**
     * @brief 是否正在合成（自动合成器）
     */
    static const BooleanProperty& CRAFTING()
    {
        static auto prop = BooleanProperty::create("crafting");
        return *prop;
    }

    /**
     * @brief 是否不祥（试炼刷怪笼、宝库）
     */
    static const BooleanProperty& OMINOUS()
    {
        static auto prop = BooleanProperty::create("ominous");
        return *prop;
    }

    /**
     * @brief 是否涂蜡（铜方块）
     */
    static const BooleanProperty& WAXED()
    {
        static auto prop = BooleanProperty::create("waxed");
        return *prop;
    }

    /**
     * @brief 是否自然生成（嘎枝之心）
     *
     * 参考: net.minecraft.world.level.block.state.properties.BlockStateProperties.NATURAL
     */
    static const BooleanProperty& NATURAL()
    {
        static auto prop = BooleanProperty::create("natural");
        return *prop;
    }

    /**
     * @brief 是否为末端（苍白苔藓）
     *
     * 参考: net.minecraft.world.level.block.state.properties.BlockStateProperties.TIP
     */
    static const BooleanProperty& TIP()
    {
        static auto prop = BooleanProperty::create("tip");
        return *prop;
    }

    // ========================================================================
    // 1.20 可疑方块粉尘属性
    // ========================================================================

    /**
     * @brief 粉尘等级 (0-3)，用于可疑沙子和沙砾
     */
    static const IntegerProperty& DUSTED()
    {
        static auto prop = IntegerProperty::create("dusted", 0, 3);
        return *prop;
    }

    // ========================================================================
    // 凿纹书架槽位占用属性 (1.20+)
    // ========================================================================

    static const BooleanProperty& SLOT_0_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_0_occupied");
        return *prop;
    }

    static const BooleanProperty& SLOT_1_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_1_occupied");
        return *prop;
    }

    static const BooleanProperty& SLOT_2_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_2_occupied");
        return *prop;
    }

    static const BooleanProperty& SLOT_3_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_3_occupied");
        return *prop;
    }

    static const BooleanProperty& SLOT_4_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_4_occupied");
        return *prop;
    }

    static const BooleanProperty& SLOT_5_OCCUPIED()
    {
        static auto prop = BooleanProperty::create("slot_5_occupied");
        return *prop;
    }

    // ========================================================================
    // 蜂巢蜂蜜等级属性
    // ========================================================================

    /**
     * @brief 蜂巢蜂蜜等级 (0-5)
     */
    static const IntegerProperty& HONEY_LEVEL_0_5()
    {
        static auto prop = IntegerProperty::create("honey_level", 0, 5);
        return *prop;
    }

    // ========================================================================
    // 特殊距离属性 (0-7，与 DISTANCE_1_7 不同)
    // ========================================================================

    /**
     * @brief 距离属性 (0-7)
     *
     * 某些方块（如霜冰）使用 0-7 范围的距离属性
     */
    static const IntegerProperty& DISTANCE_0_7()
    {
        static auto prop = IntegerProperty::create("distance", 0, 7);
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

template <>
struct mc::EnumProperty<mc::BlockStateProperties::DoorHinge>::Traits {
    static std::string toString(const mc::BlockStateProperties::DoorHinge& value);
    static std::optional<mc::BlockStateProperties::DoorHinge> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::DoubleBlockHalf>::Traits {
    static std::string toString(const mc::BlockStateProperties::DoubleBlockHalf& value);
    static std::optional<mc::BlockStateProperties::DoubleBlockHalf> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::ChestType>::Traits {
    static std::string toString(const mc::BlockStateProperties::ChestType& value);
    static std::optional<mc::BlockStateProperties::ChestType> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::AttachFace>::Traits {
    static std::string toString(const mc::BlockStateProperties::AttachFace& value);
    static std::optional<mc::BlockStateProperties::AttachFace> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::StairsShape>::Traits {
    static std::string toString(const mc::BlockStateProperties::StairsShape& value);
    static std::optional<mc::BlockStateProperties::StairsShape> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::SlabType>::Traits {
    static std::string toString(const mc::BlockStateProperties::SlabType& value);
    static std::optional<mc::BlockStateProperties::SlabType> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::WallHeight>::Traits {
    static std::string toString(const mc::BlockStateProperties::WallHeight& value);
    static std::optional<mc::BlockStateProperties::WallHeight> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::BedPart>::Traits {
    static std::string toString(const mc::BlockStateProperties::BedPart& value);
    static std::optional<mc::BlockStateProperties::BedPart> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::BellAttachment>::Traits {
    static std::string toString(const mc::BlockStateProperties::BellAttachment& value);
    static std::optional<mc::BlockStateProperties::BellAttachment> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::BambooLeaves>::Traits {
    static std::string toString(const mc::BlockStateProperties::BambooLeaves& value);
    static std::optional<mc::BlockStateProperties::BambooLeaves> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::Half>::Traits {
    static std::string toString(const mc::BlockStateProperties::Half& value);
    static std::optional<mc::BlockStateProperties::Half> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::RailShape>::Traits {
    static std::string toString(const mc::BlockStateProperties::RailShape& value);
    static std::optional<mc::BlockStateProperties::RailShape> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::RedstoneSide>::Traits {
    static std::string toString(const mc::BlockStateProperties::RedstoneSide& value);
    static std::optional<mc::BlockStateProperties::RedstoneSide> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::PistonType>::Traits {
    static std::string toString(const mc::BlockStateProperties::PistonType& value);
    static std::optional<mc::BlockStateProperties::PistonType> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::ComparatorMode>::Traits {
    static std::string toString(const mc::BlockStateProperties::ComparatorMode& value);
    static std::optional<mc::BlockStateProperties::ComparatorMode> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::NoteBlockInstrument>::Traits {
    static std::string toString(const mc::BlockStateProperties::NoteBlockInstrument& value);
    static std::optional<mc::BlockStateProperties::NoteBlockInstrument> fromName(std::string_view name);
};

// ============================================================================
// JigsawOrientation 枚举特征特化
// ============================================================================

template <>
struct mc::EnumProperty<mc::world::gen::jigsaw::JigsawOrientation>::Traits {
    static std::string toString(const mc::world::gen::jigsaw::JigsawOrientation& value)
    {
        return mc::world::gen::jigsaw::JigsawOrientations::toString(value);
    }
    static std::optional<mc::world::gen::jigsaw::JigsawOrientation> fromName(std::string_view name)
    {
        return mc::world::gen::jigsaw::JigsawOrientations::fromName(name);
    }
};

// ============================================================================
// StructureMode 枚举特征特化
// ============================================================================

template <>
struct mc::EnumProperty<mc::BlockStateProperties::StructureMode>::Traits {
    static std::string toString(const mc::BlockStateProperties::StructureMode& value);
    static std::optional<mc::BlockStateProperties::StructureMode> fromName(std::string_view name);
};

// ============================================================================
// 1.17-1.21 新增枚举特征特化
// ============================================================================

template <>
struct mc::EnumProperty<mc::BlockStateProperties::DripstoneThickness>::Traits {
    static std::string toString(const mc::BlockStateProperties::DripstoneThickness& value);
    static std::optional<mc::BlockStateProperties::DripstoneThickness> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::Tilt>::Traits {
    static std::string toString(const mc::BlockStateProperties::Tilt& value);
    static std::optional<mc::BlockStateProperties::Tilt> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::SculkSensorPhase>::Traits {
    static std::string toString(const mc::BlockStateProperties::SculkSensorPhase& value);
    static std::optional<mc::BlockStateProperties::SculkSensorPhase> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::TrialSpawnerState>::Traits {
    static std::string toString(const mc::BlockStateProperties::TrialSpawnerState& value);
    static std::optional<mc::BlockStateProperties::TrialSpawnerState> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::VaultState>::Traits {
    static std::string toString(const mc::BlockStateProperties::VaultState& value);
    static std::optional<mc::BlockStateProperties::VaultState> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::CreakingHeartState>::Traits {
    static std::string toString(const mc::BlockStateProperties::CreakingHeartState& value);
    static std::optional<mc::BlockStateProperties::CreakingHeartState> fromName(std::string_view name);
};

template <>
struct mc::EnumProperty<mc::BlockStateProperties::SideChainPart>::Traits {
    static std::string toString(const mc::BlockStateProperties::SideChainPart& value);
    static std::optional<mc::BlockStateProperties::SideChainPart> fromName(std::string_view name);
};

// ============================================================================
// CopperGolemPose 枚举特征特化 (1.21.11+)
// ============================================================================

template <>
struct mc::EnumProperty<mc::BlockStateProperties::CopperGolemPose>::Traits {
    static std::string toString(const mc::BlockStateProperties::CopperGolemPose& value);
    static std::optional<mc::BlockStateProperties::CopperGolemPose> fromName(std::string_view name);
};
