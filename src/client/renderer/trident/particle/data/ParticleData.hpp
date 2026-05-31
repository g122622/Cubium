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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 粒子数据基类
 *
 * 用于粒子参数的网络序列化和命令行解析。
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
    [[nodiscard]] virtual std::string getTypeName() const = 0;

    /**
     * @brief 获取命令行参数字符串
     *
     * 用于 /particle 命令的参数部分。
     * 例如火焰粒子返回空字符串，红石粒子返回 "1.0 0.0 0.0 1.0"
     *
     * @return 参数字符串
     */
    [[nodiscard]] virtual std::string getParameters() const { return ""; }

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
 * @tparam T 粒子数据类型
 */
template <typename T>
class ParticleDataDeserializer {
public:
    virtual ~ParticleDataDeserializer() = default;

    /**
     * @brief 从命令行参数解析粒子数据
     *
     * @param reader 字符串读取器
     * @return 解析后的粒子数据，失败返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<T> deserialize(const std::string& input) const = 0;
};

} // namespace mc::client::renderer::trident::particle::data
