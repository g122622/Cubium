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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE BE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/command/CommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <memory>

namespace mc::command {

/**
 * @brief 坐标接口
 *
 * 统一抽象三种坐标类型（绝对、相对~、局部^），
 * 提供根据命令源上下文计算最终世界坐标和旋转的方法。
 *
 * 对应 MC Java 的 Coordinates 接口。
 *
 * 使用方式:
 * @code
 * auto coords = context.getArgument<Coordinates::Ptr>("pos");
 * Vector3d position = coords->getPosition(source.position(), source.rotation());
 * Vector3i blockPos = coords->getBlockPos(source.position(), source.rotation());
 * @endcode
 *
 * 对于需要锚点（Feet/Eyes）支持的局部坐标，使用：
 * @code
 * Vector3d anchorPos = (source.anchor() == EntityAnchorType::Eyes && source.entity())
 *     ? Vector3d(source.position().x, source.position().y + source.entity()->eyeHeight(), source.position().z)
 *     : source.position();
 * Vector3d position = coords->getPosition(anchorPos, source.rotation());
 * @endcode
 *
 * 或使用静态便捷方法（自动处理锚点）：
 * @code
 * Vector3d position = Vec3ArgumentType::getVec3(context, "pos", source);
 * Vector3i blockPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
 * @endcode
 */
class Coordinates {
public:
    using Ptr = std::shared_ptr<Coordinates>;

    virtual ~Coordinates() = default;

    /**
     * @brief 根据锚点位置和旋转计算世界坐标
     * @param anchorPosition 锚点位置（对于 Feet 锚点为实体脚底位置，Eyes 锚点为眼睛位置）
     * @param rotation 命令源的旋转 (yaw, pitch)
     * @return 计算后的世界坐标
     */
    [[nodiscard]] virtual Vector3d getPosition(const Vector3d& anchorPosition, const Vector2f& rotation) const = 0;

    /**
     * @brief 根据命令源旋转计算最终旋转角
     * @param rotation 命令源的旋转 (yaw, pitch)
     * @return 计算后的旋转角
     */
    [[nodiscard]] virtual Vector2f getRotation(const Vector2f& rotation) const = 0;

    /**
     * @brief X 分量是否为相对坐标
     */
    [[nodiscard]] virtual bool isXRelative() const = 0;

    /**
     * @brief Y 分量是否为相对坐标
     */
    [[nodiscard]] virtual bool isYRelative() const = 0;

    /**
     * @brief Z 分量是否为相对坐标
     */
    [[nodiscard]] virtual bool isZRelative() const = 0;

    /**
     * @brief 便捷方法：获取方块位置（将世界坐标取整）
     */
    [[nodiscard]] Vector3i getBlockPos(const Vector3d& anchorPosition, const Vector2f& rotation) const
    {
        Vector3d pos = getPosition(anchorPosition, rotation);
        return Vector3i(static_cast<i32>(std::floor(pos.x)),
            static_cast<i32>(std::floor(pos.y)),
            static_cast<i32>(std::floor(pos.z)));
    }
};

} // namespace mc::command
