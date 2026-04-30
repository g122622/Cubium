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
 * - 浮动偏移: sin((age + partialTick) / 10.0 + hoverStart) * 0.1 + 0.1
 * - 旋转: ((age + partialTick) / 20.0 + hoverStart) 弧度
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

    [[nodiscard]] static f64 calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart);

    /**
     * @brief 计算旋转角度
     *
     * MC 1.16.5 ItemEntity.getItemHover(partialTicks): (age + partialTick) / 20.0F + hoverStart
     *
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @param hoverStart 悬浮起始偏移
     * @return 旋转角度（度）
     */
    [[nodiscard]] static f64 calculateRotation(u32 ticksExisted, f64 partialTick, f32 hoverStart);

private:
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

    static constexpr f64 BOB_AMPLITUDE = 0.1;
    static constexpr f64 BOB_BASE = 0.1;
    static constexpr f64 GROUND_TRANSFORM_Y_OFFSET = 0.25;
};

} // namespace mc::client::renderer::entity::renderer::projectile
} // namespace mc
