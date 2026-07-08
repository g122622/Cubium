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

// EndDragonFight 测试访问器 - 通过 friend 关系暴露 EndDragonFight 私有成员与方法
// 多个测试文件共享此头文件，避免 ODR 冲突。

#pragma once

#include "common/world/IWorld.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"

#include <string>

namespace mc::test {

class EndDragonFightTestAccessor {
public:
    explicit EndDragonFightTestAccessor(EndDragonFight& fight)
        : m_fight(fight)
    {}

    // ========== 私有字段读取器 ==========

    [[nodiscard]] i32 ticksSinceDragonSeen() const { return m_fight.m_ticksSinceDragonSeen; }

    [[nodiscard]] i32 ticksSinceLastPlayerScan() const { return m_fight.m_ticksSinceLastPlayerScan; }

    [[nodiscard]] IDragonBossBar& dragonBossBar() { return m_fight.dragonBossBar(); }

    [[nodiscard]] const std::string& dragonUUID() const { return m_fight.m_dragonUUID; }

    [[nodiscard]] bool dragonKilled() const { return m_fight.m_dragonKilled; }

    // ========== 私有字段设置器 ==========

    void setTicksSinceDragonSeen(i32 value) { m_fight.m_ticksSinceDragonSeen = value; }

    void setTicksSinceLastPlayerScan(i32 value) { m_fight.m_ticksSinceLastPlayerScan = value; }

    void setDragonUUID(const std::string& uuid) { m_fight.m_dragonUUID = uuid; }

    void setDragonKilledFlag(bool value) { m_fight.m_dragonKilled = value; }

    // ========== 私有方法调用器 ==========

    void updatePlayers(IWorld& world) { m_fight._updatePlayers(world); }

    void findOrCreateDragon(IWorld& world) { m_fight._findOrCreateDragon(world); }

    bool createNewDragon(IWorld& world) { return m_fight._createNewDragon(world); }

private:
    EndDragonFight& m_fight;
};

} // namespace mc::test
