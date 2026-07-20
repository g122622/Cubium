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

#include "TargetInfo.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/util/math/Vector3.hpp"

#include <functional>

namespace mc::client {
class ClientEntityManager;
class ClientWorld;
} // namespace mc::client

namespace mc::client::ui::minecraft::targetinfo {

class TargetInfoResolver {
public:
    using PlayerNameLookup = std::function<std::string(EntityInstanceId)>;

    /**
     * @brief 解析玩家当前注视目标的信息
     *
     * 综合方块射线检测结果和实体射线检测结果，选取距离更近的目标，
     * 返回包含标题、详细信息和强调色的快照。
     *
     * @param eyePosition      玩家眼睛位置
     * @param forward          视线方向（单位向量）
     * @param world            客户端世界引用
     * @param entityManager    客户端实体管理器引用
     * @param blockRaycast     方块射线检测结果
     * @param reachDistance    玩家触达距离
     * @param playerNameLookup 玩家ID到名称的查找函数
     * @return 目标信息快照
     */
    [[nodiscard]] static TargetInfoSnapshot resolve(const Vector3& eyePosition,
        const Vector3& forward,
        const ClientWorld& world,
        const ClientEntityManager& entityManager,
        const BlockRaycastResult& blockRaycast,
        f32 reachDistance,
        const PlayerNameLookup& playerNameLookup);
};

} // namespace mc::client::ui::minecraft::targetinfo