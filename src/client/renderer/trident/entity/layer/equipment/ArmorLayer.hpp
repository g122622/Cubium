#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/EntityModel.hpp"
#include "../../core/IEntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 盔甲层渲染器
 *
 * 在实体模型上渲染盔甲层。支持不同部位的盔甲：
 * - 头盔
 * - 胸甲
 * - 护腿
 * - 靴子
 *
 * 支持皮革染色的盔甲。
 *
 * 参考 MC 1.16.5 ArmorLayer
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template<typename TEntity, typename TModel>
class ArmorLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 盔甲部位
     */
    enum class ArmorSlot : u8 {
        Head = 0,    // 头盔
        Chest = 1,   // 胸甲
        Legs = 2,    // 护腿
        Feet = 3     // 靴子
    };

    /**
     * @brief 默认构造函数
     */
    ArmorLayer() = default;

    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     */
    explicit ArmorLayer(entity::core::IEntityRenderer<TEntity, TModel>& renderer)
        : m_renderer(&renderer) {}

    ~ArmorLayer() override = default;

    /**
     * @brief 渲染盔甲层
     */
    void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    /**
     * @brief 检查是否应该渲染盔甲层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 渲染特定部位的盔甲
     * @param entity 实体
     * @param slot 盔甲部位
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     * @param partialTicks 部分 tick
     * @param ageInTicks 年龄 tick
     * @param netHeadYaw 头部偏航角
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    virtual void renderArmorPart(
        TEntity& entity,
        ArmorSlot slot,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    );

    /**
     * @brief 获取盔甲模型
     * @param slot 盔甲部位
     * @return 盔甲模型
     */
    [[nodiscard]] virtual TModel& getArmorModel(ArmorSlot slot);

    /**
     * @brief 获取盔甲纹理
     * @param entity 实体
     * @param slot 盔甲部位
     * @return 纹理位置
     */
    [[nodiscard]] virtual ResourceLocation getArmorTexture(
        const TEntity& entity,
        ArmorSlot slot
    );

    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] entity::core::IEntityRenderer<TEntity, TModel>* getRenderer() {
        return m_renderer;
    }

    /**
     * @brief 获取关联的模型
     */
    [[nodiscard]] TModel* getParentModel() {
        return m_renderer ? &m_renderer->getModel() : nullptr;
    }

private:
    entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;

    // 盔甲模型（按部位）
    std::unique_ptr<TModel> m_headArmorModel;
    std::unique_ptr<TModel> m_chestArmorModel;
    std::unique_ptr<TModel> m_legsArmorModel;
    std::unique_ptr<TModel> m_feetArmorModel;

    /**
     * @brief 初始化盔甲模型
     */
    void initializeArmorModels();
};

} // namespace mc::client::renderer::entity::layer::equipment
