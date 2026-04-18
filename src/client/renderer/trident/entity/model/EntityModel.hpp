#pragma once

#include "ModelRenderer.hpp"
#include "../../../../../common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer {

/**
 * @brief 实体模型基类
 *
 * 所有实体模型的基类，定义动画和渲染接口。
 *
 * 参考 MC 1.16.5 EntityModel
 */
class EntityModel {
public:
    EntityModel() = default;
    virtual ~EntityModel() = default;

    // ========== 渲染 ==========

    /**
     * @brief 渲染模型
     * @param scale 缩放因子
     */
    virtual void render(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 生成模型网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param scale 缩放因子
     */
    virtual void generateMesh(std::vector<ModelVertex>& vertices,
                              std::vector<u32>& indices,
                              f64 scale = 1.0f / 16.0f) const;

    /**
     * @brief 设置动画参数
     * @param limbSwing 步态动画周期（0-1）
     * @param limbSwingAmount 步态动画强度
     * @param ageInTicks 年龄tick（用于空闲动画）
     * @param netHeadYaw 头部偏航角
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    virtual void setAngles(f64 limbSwing, f64 limbSwingAmount,
                           f64 ageInTicks, f64 netHeadYaw,
                           f64 headPitch, f64 scale);

    // ========== 动画 ==========

    /**
     * @brief 动画目标角度
     * @param target 要写入的目标模型
     * @note 仅复制各部件的旋转角和旋转点，不会改动材质、贴图或部件拓扑
     */
    virtual void copyAnglesTo(EntityModel& target) const;

    // ========== 模型部件访问 ==========

    /**
     * @brief 获取所有部件
     */
    [[nodiscard]] const std::vector<std::shared_ptr<ModelRenderer>>& getParts() const {
        return m_parts;
    }

    // ========== 纹理 ==========

    [[nodiscard]] i32 textureWidth() const { return m_textureWidth; }
    [[nodiscard]] i32 textureHeight() const { return m_textureHeight; }

    void setTextureSize(i32 width, i32 height) {
        m_textureWidth = width;
        m_textureHeight = height;
    }

protected:
    i32 m_textureWidth = 64;
    i32 m_textureHeight = 32;
    std::vector<std::shared_ptr<ModelRenderer>> m_parts;
};

/**
 * @brief 四足动物模型
 *
 * 用于猪、牛、羊等四足动物的模型。
 *
 * 参考 MC 1.16.5 QuadrupedModel
 */
class QuadrupedModel : public EntityModel {
public:
    QuadrupedModel();
    ~QuadrupedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

protected:
    /**
     * @brief 设置模型部件
     */
    void setupParts();

    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
};

/**
 * @brief 双足动物模型
 *
 * 用于玩家、僵尸、骷髅等双足生物的模型。
 *
 * 参考 MC 1.16.5 BipedModel
 */
class BipedModel : public EntityModel {
public:
    BipedModel();
    ~BipedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

protected:
    /**
     * @brief 设置模型部件
     */
    void setupParts();

    // 模型部件
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_headwear;    // 帽子层
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};

} // namespace mc::client::renderer
