#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../physics/collision/CollisionShape.hpp"
#include "../../util/property/StateHolder.hpp"
#include <memory>
#include <unordered_map>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class BlockRegistry;
class IWorld;
class IBlockReader;
class BlockPos;
class Player;
class BlockEntity;
class Entity;
class IPlantable;
class ItemStack;

namespace math {
class IRandom;
}

namespace loot {
class LootTableManager;
class LootTable;
}

namespace fluid {
class FluidState;
}

namespace item {
namespace tool {
    // Tool type constants for harvest tool comparison
    // These values must match ToolType enum in item/tool/ToolType.hpp
    constexpr u8 TOOL_TYPE_NONE = 0;
    constexpr u8 TOOL_TYPE_PICKAXE = 1;
    constexpr u8 TOOL_TYPE_AXE = 2;
    constexpr u8 TOOL_TYPE_SHOVEL = 3;
    constexpr u8 TOOL_TYPE_HOE = 4;
    constexpr u8 TOOL_TYPE_SWORD = 5;
    constexpr u8 TOOL_TYPE_SHEARS = 6;
}
}

// Forward declaration for Direction (needed for method parameters)
enum class Direction : u8;

// Forward declaration for Material (needed for getMaterial() return type)
class Material;

// Forward declaration for BlockSoundType (needed for getSoundType() return type)
class BlockSoundType;

// Forward declaration for IProperty (used in StateHolder base class)
class IProperty;

/**
 * @brief 方块状态
 *
 * 不可变的方块状态对象，包含方块的所有属性值。
 * 继承自StateHolder以支持O(1)的状态转换。
 *
 * 参考: net.minecraft.block.BlockState
 */
class BlockState : public StateHolder<Block, BlockState> {
public:
    /**
     * @brief 构造方块状态
     */
    BlockState(const Block& block,
               std::unordered_map<const IProperty*, size_t> values,
               u32 stateId);

    /**
     * @brief 是否为空气
     */
    [[nodiscard]] bool isAir() const;

    /**
     * @brief 是否为固体
     */
    [[nodiscard]] bool isSolid() const { return m_isSolid; }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque() const { return m_isOpaque; }

    /**
     * @brief 是否透明
     */
    [[nodiscard]] bool isTransparent() const { return !m_isOpaque; }

    /**
     * @brief 是否阻挡移动
     */
    [[nodiscard]] bool blocksMovement() const { return m_blocksMovement; }

    /**
     * @brief 是否为液体
     */
    [[nodiscard]] bool isLiquid() const { return m_isLiquid; }

    /**
     * @brief 是否可燃
     */
    [[nodiscard]] bool isFlammable() const { return m_isFlammable; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }

    /**
     * @brief 检查此状态是否属于指定方块
     * @param block 方块指针
     * @return 如果此状态的方块与给定方块相同则返回true
     */
    [[nodiscard]] bool is(const Block* block) const {
        return block != nullptr && &owner() == block;
    }

    /**
     * @brief 获取此状态所属的方块
     * @return 方块引用
     */
    [[nodiscard]] const Block& getBlock() const {
        return owner();
    }

    /**
     * @brief 获取光照透明度 (0-15)
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getOpacity
     */
    [[nodiscard]] i32 getOpacity() const { return m_opacity; }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool propagatesSkylightDown() const { return m_propagatesSkylightDown; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const { return m_resistance; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const { return m_blockId; }

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape() const;

    /**
     * @brief 获取渲染形状
     */
    [[nodiscard]] const CollisionShape& getShape() const;

    /**
     * @brief 获取遮挡形状
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape() const;

    /**
     * @brief 获取指定面的遮挡形状
     *
     * 用于光照遮挡检测。返回方块在指定方向上的投影形状。
     *
     * @param direction 方向
     * @return 面遮挡形状
     */
    [[nodiscard]] CollisionShape getFaceOcclusionShape(Direction direction) const;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * @return 是否使用形状进行光照遮挡
     */
    [[nodiscard]] bool useShapeForLightOcclusion() const { return m_useShapeForLightOcclusion; }

    /**
     * @brief 是否有不透明碰撞形状
     *
     * 用于环境光遮蔽(AO)计算。如果方块有不透明的完整碰撞箱，
     * 则返回true，导致周围顶点变暗。
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#hasOpaqueCollisionShape
     */
    [[nodiscard]] bool hasOpaqueCollisionShape() const;

    /**
     * @brief 获取环境光遮蔽亮度值
     *
     * 返回值用于AO计算：
     * - 0.2f: 方块有不透明碰撞形状（实心方块），产生阴影
     * - 1.0f: 方块无碰撞或透明（玻璃、树叶等），不产生阴影
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getAmbientOcclusionLightValue
     */
    [[nodiscard]] float getAmbientOcclusionLightValue() const;

    /**
     * @brief 检查指定面是否为实体面
     *
     * 用于流体流动判断、渲染面剔除等。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param side 要检查的面
     * @return 如果该面是实体面返回true
     */
    [[nodiscard]] bool isSolidSide(IWorld& world, const BlockPos& pos, Direction side) const;

    /**
     * @brief 检查是否为不透明完整方块
     *
     * 用于渲染和光照计算。
     *
     * @param world 世界
     * @param pos 方块位置
     * @return 如果是不透明完整方块返回true
     */
    [[nodiscard]] bool isOpaqueCube(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 获取方块资源位置
     */
    [[nodiscard]] const ResourceLocation& blockLocation() const;

    /**
     * @brief 获取流体状态
     *
     * 委托到方块的 getFluidState 方法
     */
    [[nodiscard]] const fluid::FluidState* getFluidState() const;

    /**
     * @brief 获取材质
     *
     * 委托到方块的 material 方法
     */
    [[nodiscard]] const Material& getMaterial() const;

    /**
     * @brief 获取声音类型
     *
     * 委托到方块的 getSoundType 方法
     */
    [[nodiscard]] const BlockSoundType& getSoundType() const;

    /**
     * @brief 获取挖掘工具类型
     *
     * 返回采集此方块所需的工具类型。
     * 返回值与 item::tool 命名空间中的常量比较：
     * - TOOL_TYPE_NONE (0): 无需工具
     * - TOOL_TYPE_PICKAXE (1): 镐
     * - TOOL_TYPE_AXE (2): 斧
     * - TOOL_TYPE_SHOVEL (3): 锹
     * - TOOL_TYPE_HOE (4): 锄
     * - TOOL_TYPE_SWORD (5): 剑
     * - TOOL_TYPE_SHEARS (6): 剪刀
     *
     * @return 工具类型值
     */
    [[nodiscard]] u8 getHarvestTool() const;

    /**
     * @brief 获取挖掘等级
     *
     * 返回采集此方块所需的最低工具等级。
     * - 0: 木/金工具可采集
     * - 1: 石制工具可采集
     * - 2: 铁制工具可采集
     * - 3: 钻石工具可采集
     * - 4: 下界合金工具可采集
     *
     * @return 挖掘等级（0-4）
     */
    [[nodiscard]] i32 getHarvestLevel() const;

    /**
     * @brief 检查工具是否有效
     *
     * 检查指定工具类型和等级是否能采集此方块。
     *
     * @param toolType 工具类型值
     * @param harvestLevel 工具挖掘等级
     * @return 如果工具有效返回true
     */
    [[nodiscard]] bool isToolEffective(u8 toolType, i32 harvestLevel) const;

    /**
     * @brief 是否需要正确工具才能采集
     *
     * 如果返回 true，则必须使用正确类型且等级足够的工具
     * 才能获得方块掉落物。
     *
     * @return 是否需要正确工具
     */
    [[nodiscard]] bool requiresTool() const;

    /**
     * @brief 检查是否为粘性方块
     *
     * 委托到方块的 isStickyBlock 方法。
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#isStickyBlock
     *
     * @return 如果是粘性方块返回 true
     */
    [[nodiscard]] bool isStickyBlock() const;

    /**
     * @brief 检查两个方块是否可以粘连
     *
     * 委托到方块的 canStickTo 方法。
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#canStickTo
     *
     * @param other 目标方块状态
     * @return 如果可以粘连返回 true
     */
    [[nodiscard]] bool canStickTo(const BlockState& other) const;

    /**
     * @brief 转换为模型键（用于查找模型变体）
     * @return 格式: "axis=y,facing=north" 或 "" (无属性时)
     */
    [[nodiscard]] std::string toModelKey() const;

protected:
    /**
     * @brief 获取拥有者名称
     */
    [[nodiscard]] std::string ownerName() const override;

private:
    friend class Block;
    friend class BlockRegistry;

    /**
     * @brief 缓存方块属性
     */
    void cacheProperties();

    // 缓存的属性
    bool m_isSolid = true;
    bool m_isOpaque = true;
    bool m_blocksMovement = false;
    bool m_isLiquid = false;
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;
    bool m_useShapeForLightOcclusion = false;  // 是否使用形状进行光照遮挡
    u8 m_lightLevel = 0;
    u8 m_harvestTool = 0;  // HarvestTool::None
    i32 m_opacity = 15;  // 默认完全不透明
    i32 m_harvestLevel = 0;
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u32 m_blockId = 0;
};

} // namespace mc
