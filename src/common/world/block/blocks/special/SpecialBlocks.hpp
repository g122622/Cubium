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
    /// 结构方块模式别名，使用 BlockStateProperties 中定义的 StructureMode
    using Mode = BlockStateProperties::StructureMode;

    explicit StructureBlock(const BlockProperties& properties);
    ~StructureBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取结构方块模式
     *
     * @param state 方块状态
     * @return 当前模式
     */
    [[nodiscard]] Mode getMode(const BlockState& state) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 放置时默认设置为 DATA 模式。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

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

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;
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
 * 物理：
 * - 弹跳系数：0.9（每次弹跳损失 10% 速度）
 * - 滑度：0.8
 *
 * 参考: net.minecraft.block.SlimeBlock
 */
class SlimeBlock : public Block {
public:
    explicit SlimeBlock(const BlockProperties& properties);
    ~SlimeBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体着地时调用
     *
     * 实现弹跳效果：如果实体向下落，Y速度取反并乘以 0.9。
     */
    void onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;

    // ========== 黏液块粘连 ==========

    [[nodiscard]] bool isStickyBlock(const BlockState& state) const override;

    [[nodiscard]] bool canStickTo(const BlockState& state, const BlockState& other) const override;
};

/**
 * @brief 蜂蜜块
 *
 * 粘性方块，实体在上面移动会减速。
 * 活塞推动时会粘住相邻方块。
 *
 * 物理：
 * - 滑度：0.5
 * - 跳跃因子：0.5
 * - 速度因子：0.4（在内部移动时）
 *
 * 参考: net.minecraft.block.HoneyBlock
 */
class HoneyBlock : public Block {
public:
    explicit HoneyBlock(const BlockProperties& properties);
    ~HoneyBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体着地时调用
     *
     * 消除摔落伤害，不弹跳。
     */
    void onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 推动反应 ==========

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override;

    // ========== 蜂蜜块粘连 ==========

    [[nodiscard]] bool isStickyBlock(const BlockState& state) const override;

    [[nodiscard]] bool canStickTo(const BlockState& state, const BlockState& other) const override;

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
 * 吸水机制：
 * - 从海绵位置开始 BFS 搜索周围的水方块
 * - 最大搜索深度：6 格
 * - 最大吸收数量：65 个水方块
 * - 处理三种类型的水：水源、流动水、海洋植物
 * - 成功吸水后变成湿润海绵
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
     *
     * 从海绵位置开始 BFS 搜索周围的水方块并吸收。
     * 成功吸水后将海绵变为湿润海绵。
     *
     * @param world 世界
     * @param pos 海绵位置
     * @return 如果吸收了水返回 true
     */
    bool tryAbsorbWater(IWorld& world, const BlockPos& pos);

    // ========== 方块回调 ==========

    /**
     * @brief 方块被放置时的处理
     *
     * 放置时尝试吸水。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居方块更新
     *
     * 当邻居方块改变时尝试吸水。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动
     */
    void neighborChanged(IWorld& world, const BlockPos& pos,
                         Block& neighborBlock, const BlockPos& neighborPos,
                         bool isMoving) override;

private:
    /// 海绵吸水最大搜索深度（MC 1.16.5）
    static constexpr i32 MAX_ABSORB_DEPTH = 6;

    /// 海绵吸水最大吸收数量（MC 1.16.5）
    static constexpr i32 MAX_ABSORB_COUNT = 65;

    /**
     * @brief 执行吸水 BFS 搜索
     *
     * @param world 世界
     * @param pos 海绵位置
     * @return 吸收的水方块数量
     */
    i32 absorb(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 湿润海绵方块
 *
 * 海绵吸水后的状态，需要烤干才能再次吸水。
 *
 * 特殊行为：
 * - 在下界放置时会变干（变成普通海绵）
 * - 产生蒸汽粒子效果和火焰熄灭音效
 *
 * 参考: net.minecraft.block.WetSpongeBlock
 */
class WetSpongeBlock : public Block {
public:
    explicit WetSpongeBlock(const BlockProperties& properties);
    ~WetSpongeBlock() override = default;

    // ========== 方块回调 ==========

    /**
     * @brief 方块被放置时的处理
     *
     * 在下界放置时会变干并产生蒸汽效果。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;
};

/**
 * @brief 蜘蛛网方块
 *
 * 实体经过时会被减速的网状方块。
 *
 * 物理：
 * - 移动速度减少 97.5%（乘以 0.025）
 * - Y轴下落速度减少（乘以 0.025）
 * - 不影响跳跃
 *
 * 参考: net.minecraft.block.WebBlock
 */
class WebBlock : public Block {
public:
    explicit WebBlock(const BlockProperties& properties);
    ~WebBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时调用
     *
     * 大幅减缓实体速度。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override {
        // 蜘蛛网无碰撞
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }

    [[nodiscard]] bool isOpaque(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
