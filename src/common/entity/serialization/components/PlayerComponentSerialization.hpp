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

#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"

namespace mc::entity::serialization::components {

/**
 * @brief Player 层 Score 组件序列化器
 *
 * 把 Player::addAdditionalSaveData/readAdditionalSaveData 中 Score 字段的 NBT 读写逻辑
 * 搬到按 PlayerScoreComponent 注册的自由函数序列化器。
 *
 * | 组件 | 字段 | 读写路径 |
 * |---|---|---|
 * | PlayerScoreComponent | Score | getScore 读 / setScore 写（同步 DATA_PLAYER_SCORE_PARAM 镜像） |
 *
 * dynamic_cast 早退：序列化器经 Entity& 调用，内部 dynamic_cast<Player*>。非 Player 实体
 * 返回 nullptr 早退。Player 非 final、Entity 虚析构，RTTI 可用。
 *
 * Score 必须走 setter：setScore 同时写 PlayerScoreComponent 真相源 + DATA_PLAYER_SCORE_PARAM
 * 镜像下发客户端，直写组件会丢同步。
 *
 * 批次6 子目标1 Step5。
 */

/** 注册 Player 层组件序列化器到注册表（在 ComponentSerializerRegistry::registerAll 内调用） */
void registerPlayerComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components
