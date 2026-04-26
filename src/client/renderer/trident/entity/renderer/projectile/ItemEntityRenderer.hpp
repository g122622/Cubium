#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {

class ItemStack;
struct TextureRegion;

namespace client::renderer::entity::renderer::projectile {

/**
 * @brief ItemEntity 渲染器
 *
 * 渲染掉落在世界中的物品实体。
 * 物品以 3D 方式浮动渲染，具有上下浮动和旋转动画。
 *
 * 参考 MC 1.16.5 ItemEntityRenderer / ItemRenderer
 *
 * 关键实现细节：
 * - 浮动偏移: sin((age + hoverStart) / 10.0) * 0.1 + 0.1
 * - 旋转: (age + partialTick) * 2.0 度
 * - 多物品堆叠: 根据数量 1-5 个物品
 */
class ItemEntityRenderer : public core::EntityRenderer {
public:
    ItemEntityRenderer();
    ~ItemEntityRenderer() override = default;

    // 禁止拷贝
    ItemEntityRenderer(const ItemEntityRenderer&) = delete;
    ItemEntityRenderer& operator=(const ItemEntityRenderer&) = delete;

    /**
     * @brief 渲染 ItemEntity
     * @param entity 实体（必须是 ClientEntity）
     * @param partialTicks 部分 tick
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染阴影（ItemEntity 有小阴影）
     * @param entity 实体
     * @param partialTicks 部分 tick
     */
    void renderShadow(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 设置物品纹理图集
     * @param atlas 物品纹理图集
     */
    void setItemTextureAtlas(pipeline::EntityTextureAtlas* atlas) { m_itemTextureAtlas = atlas; }

private:
    /**
     * @brief 计算浮动偏移
     *
     * MC 1.16.5 ItemRenderer.java:47:
     * f1 = MathHelper.sin(((float)entityIn.getAge() + partialTicks) / 10.0F + entityIn.hoverStart) * 0.1F + 0.1F
     *
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @param hoverStart 悬浮起始偏移（每个物品实体随机生成）
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart) const;

    /**
     * @brief 计算旋转角度
     *
     * MC 1.16.5: (age + partialTick) * 2.0 度
     *
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return 旋转角度（度）
     */
    [[nodiscard]] f64 calculateRotation(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 获取物品纹理区域
     * @param stack 物品堆
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTextureRegion(const ItemStack& stack) const;

    /**
     * @brief 计算物品堆叠数量对应的渲染数量
     *
     * MC 1.16.5 ItemRenderer:
     * - 1 个物品: 1 个模型
     * - 2-16: 2 个模型
     * - 17-32: 3 个模型
     * - 33-48: 4 个模型
     * - 49+: 5 个模型
     *
     * @param count 物品数量
     * @return 渲染的模型数量 (1-5)
     */
    [[nodiscard]] static i32 getItemCountForRender(i32 count);

    pipeline::EntityTextureAtlas* m_itemTextureAtlas = nullptr;

    // ItemEntity 动画常量（参考 MC 1.16.5）
    static constexpr f64 BOB_AMPLITUDE = 0.1;       // 浮动高度幅度
    static constexpr f64 BOB_FREQUENCY = 0.1;       // 浮动速度（1/10 弧度/tick）
    static constexpr f64 BOB_BASE = 0.1;            // 基础高度偏移（MC 1.16.5: + 0.1F）
    static constexpr f64 ROTATION_SPEED = 2.0;      // 旋转速度（度/tick）
    static constexpr f64 GROUND_OFFSET = 0.25;      // 地面高度偏移
    static constexpr f64 ITEM_SIZE = 0.25;          // 渲染大小
};

} // namespace mc::client::renderer::entity::renderer::projectile
} // namespace mc
