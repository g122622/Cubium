#pragma once

#include "ParticleData.hpp"

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 基础粒子数据（无参数粒子）
 *
 * 用于不需要额外参数的粒子类型，如火焰、烟雾、爱心等。
 * 参考 MC 1.16.5 BasicParticleType
 *
 * 用法示例：
 * @code
 * auto flameData = std::make_unique<BasicParticleData>(ParticleTypeId::Flame);
 * auto smokeData = std::make_unique<BasicParticleData>(ParticleTypeId::Smoke);
 * @endcode
 */
class BasicParticleData : public ParticleData {
public:
    /**
     * @brief 构造基础粒子数据
     *
     * @param type 粒子类型 ID
     */
    explicit BasicParticleData(ParticleTypeId type);

    /**
     * @brief 从资源位置构造基础粒子数据
     *
     * @param typeName 粒子类型名称（如 "minecraft:flame"）
     */
    explicit BasicParticleData(const std::string& typeName);

    ~BasicParticleData() override = default;

    // 允许拷贝
    BasicParticleData(const BasicParticleData&) = default;
    BasicParticleData& operator=(const BasicParticleData&) = default;

    // 允许移动
    BasicParticleData(BasicParticleData&&) noexcept = default;
    BasicParticleData& operator=(BasicParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return m_type; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override { return ""; }
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

private:
    ParticleTypeId m_type;
};

} // namespace mc::client::renderer::trident::particle::data
