#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class ServerWorld;

namespace blocks {

/**
 * @brief 屏障方块
 *
 * 不可见的不可破坏方块，用于地图制作。
 * 只有创造模式玩家可以看到边界轮廓。
 *
 * 参考: net.minecraft.block.BarrierBlock
 */
class BarrierBlock : public Block {
public:
    explicit BarrierBlock(const BlockProperties& properties);
    ~BarrierBlock() override = default;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }
};

/**
 * @brief 结构空位方块
 *
 * 结构方块使用的空位，放置时不会替换现有方块。
 *
 * 参考: net.minecraft.block.StructureVoidBlock
 */
class StructureVoidBlock : public Block {
public:
    explicit StructureVoidBlock(const BlockProperties& properties);
    ~StructureVoidBlock() override = default;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }
};

/**
 * @brief 结构方块
 *
 * 用于保存和加载结构的方块。
 * 创造模式和管理员可以使用。
 *
 * 状态属性：
 * - MODE: 结构方块模式 (SAVE, LOAD, CORNER, DATA)
 *
 * 参考: net.minecraft.block.StructureBlock
 */
class StructureBlock : public Block {
public:
    /**
     * @brief 结构方块模式
     */
    enum class Mode : u8 {
        Save = 0,
        Load = 1,
        Corner = 2,
        Data = 3
    };

    explicit StructureBlock(const BlockProperties& properties);
    ~StructureBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] Mode getMode(const BlockState& state) const;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;
};

/**
 * @brief 拼图方块
 *
 * 用于结构生成的拼图系统。
 *
 * 状态属性：
 * - ORIENTATION: 方向 (DOWN_EAST, DOWN_NORTH, ..., UP_WEST)
 *
 * 参考: net.minecraft.block.JigsawBlock
 */
class JigsawBlock : public Block {
public:
    explicit JigsawBlock(const BlockProperties& properties);
    ~JigsawBlock() override = default;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;
};

/**
 * @brief 命令方块基类
 *
 * 方块形式的命令执行器。
 *
 * 状态属性：
 * - FACING: 朝向
 * - CONDITIONAL: 是否有条件
 * - POWERED: 是否被激活
 *
 * 参考: net.minecraft.block.CommandBlock
 */
class CommandBlock : public Block {
public:
    explicit CommandBlock(const BlockProperties& properties);
    ~CommandBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] Direction getFacing(const BlockState& state) const;
    [[nodiscard]] bool isConditional(const BlockState& state) const;
    [[nodiscard]] bool isPowered(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 红石 ==========

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 交互 ==========

    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const override { return true; }

protected:
    /**
     * @brief 执行命令
     */
    void execute(IWorld& world, const BlockPos& pos, const BlockState& state);
};

/**
 * @brief 循环命令方块
 *
 * 每个 tick 都执行命令的命令方块。
 *
 * 参考: net.minecraft.block.RepeatingCommandBlock
 */
class RepeatingCommandBlock : public CommandBlock {
public:
    explicit RepeatingCommandBlock(const BlockProperties& properties);
    ~RepeatingCommandBlock() override = default;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;
};

/**
 * @brief 连锁命令方块
 *
 * 当前方块命令执行后连锁触发的命令方块。
 *
 * 参考: net.minecraft.block.ChainCommandBlock
 */
class ChainCommandBlock : public CommandBlock {
public:
    explicit ChainCommandBlock(const BlockProperties& properties);
    ~ChainCommandBlock() override = default;
};

/**
 * @brief 粘液块
 *
 * 弹性方块，实体落在上面会弹跳。
 * 活塞推动时会粘住相邻方块。
 *
 * 参考: net.minecraft.block.SlimeBlock
 */
class SlimeBlock : public Block {
public:
    explicit SlimeBlock(const BlockProperties& properties);
    ~SlimeBlock() override = default;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;
};

/**
 * @brief 蜂蜜块
 *
 * 粘性方块，实体在上面移动会减速。
 * 活塞推动时会粘住相邻方块。
 *
 * 参考: net.minecraft.block.HoneyBlock
 */
class HoneyBlock : public Block {
public:
    explicit HoneyBlock(const BlockProperties& properties);
    ~HoneyBlock() override = default;

    // ========== 实体交互 ==========

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_collisionShape;
};

/**
 * @brief 海绵方块
 *
 * 可以吸收水的方块。
 *
 * 状态属性：无（湿润状态是不同的方块）
 *
 * 参考: net.minecraft.block.SpongeBlock
 */
class SpongeBlock : public Block {
public:
    explicit SpongeBlock(const BlockProperties& properties);
    ~SpongeBlock() override = default;

    // ========== 吸水 ==========

    /**
     * @brief 尝试吸水
     * @return 如果吸收了水返回 true
     */
    bool tryAbsorbWater(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 湿润海绵方块
 *
 * 海绵吸水后的状态，需要烤干才能再次吸水。
 *
 * 参考: net.minecraft.block.WetSpongeBlock
 */
class WetSpongeBlock : public Block {
public:
    explicit WetSpongeBlock(const BlockProperties& properties);
    ~WetSpongeBlock() override = default;
};

} // namespace blocks
} // namespace mc
