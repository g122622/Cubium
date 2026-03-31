#pragma once

#include "../ParticleTypes.hpp"
#include "../../../../../common/core/Types.hpp"
#include "../../../../../common/core/Result.hpp"
#include "../../../../../common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 粒子数据基类
 *
 * 用于粒子参数的网络序列化和命令行解析。
 * 参考 MC 1.16.5 IParticleData
 *
 * 子类：
 * - BasicParticleData: 无参数粒子（如火焰、烟雾）
 * - BlockParticleData: 方块粒子（如破坏方块）
 * - ItemParticleData: 物品粒子
 * - RedstoneParticleData: 红石粉尘粒子（带颜色）
 */
class ParticleData {
public:
    virtual ~ParticleData() = default;

    /**
     * @brief 获取粒子类型
     *
     * @return 粒子类型 ID
     */
    [[nodiscard]] virtual ParticleTypeId getType() const = 0;

    /**
     * @brief 获取粒子类型名称
     *
     * 返回 Minecraft 资源位置格式的名称，如 "minecraft:flame"
     *
     * @return 粒子类型名称
     */
    [[nodiscard]] virtual String getTypeName() const = 0;

    /**
     * @brief 获取命令行参数字符串
     *
     * 用于 /particle 命令的参数部分。
     * 例如火焰粒子返回空字符串，红石粒子返回 "1.0 0.0 0.0 1.0"
     *
     * @return 参数字符串
     */
    [[nodiscard]] virtual String getParameters() const { return ""; }

    /**
     * @brief 克隆粒子数据
     *
     * @return 粒子数据的深拷贝
     */
    [[nodiscard]] virtual std::unique_ptr<ParticleData> clone() const = 0;
};

/**
 * @brief 粒子数据反序列化器接口
 *
 * 参考 MC 1.16.5 IParticleData.IDeserializer
 *
 * @tparam T 粒子数据类型
 */
template<typename T>
class ParticleDataDeserializer {
public:
    virtual ~ParticleDataDeserializer() = default;

    /**
     * @brief 从命令行参数解析粒子数据
     *
     * @param reader 字符串读取器
     * @return 解析后的粒子数据，失败返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<T> deserialize(const String& input) const = 0;
};

} // namespace mc::client::renderer::trident::particle::data
