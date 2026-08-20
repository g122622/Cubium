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

namespace mc {

// 前向声明：ExplosionImmunityContext 只持有裸指针，无需完整定义
class Entity;
class LivingEntity;

namespace world::explosion {

/**
 * @brief 爆炸免疫判定上下文
 *
 * Entity::ignoreExplosion() 的入参，把 vanilla Explosion.ignoreExplosion(Explosion)
 * 实际依赖的爆炸属性抽成值语义结构体，避免把整个 Explosion 对象暴露给 Entity 层。
 *
 * 风弹/风爆附魔不走 Explosion 类（独立 applyWindBurst 路径，等价 vanilla TRIGGER 语义），
 * 这两处调用点各自构造等价上下文（shouldAffectBlocklikeEntities 恒为 false）。
 */
struct ExplosionImmunityContext {
    /**
     * @brief 是否影响"方块类实体"（掉落物/盔甲架/画框等）
     *
     * 由爆炸发起方预计算：mobGriefing 开启时为 true，否则取决于爆炸是否破坏方块
     * （ExplosionMode 非 None 即 true）。风弹/风爆路径恒为 false（TRIGGER 语义）。
     */
    bool shouldAffectBlocklikeEntities = false;

    /**
     * @brief 间接源实体（追溯爆炸链：TNT→点燃者/投射物→发射者）
     *
     * 若为 LivingEntity 子类。VehicleEntity（船/矿车）的 ignoreExplosion 据此判定
     * 间接源是否为 Mob（非 Mob 源的爆炸不破坏载具）。
     */
    LivingEntity* indirectSource = nullptr;

    /**
     * @brief 直接源实体（m_source）
     *
     * BlockAttachedEntity（画/物品展示框等）的 ignoreExplosion 据此判定源是否在水中
     * （水中源不破坏悬挂实体）。
     */
    Entity* directSource = nullptr;

    /**
     * @brief mobGriefing 游戏规则当前值
     *
     * 由调用方预填，避免各实体覆写内反查 world。VehicleEntity 覆写据此判定
     * "间接源为 Mob 时，受不受爆炸影响取决于 mobGriefing"。
     */
    bool mobGriefing = false;
};

} // namespace world::explosion
} // namespace mc
