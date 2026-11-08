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
#include "common/core/Types.hpp"

namespace mc {
enum class NoteBlockInstrument : u8 {
    Harp = 0,            ///< 竖琴（默认）
    BaseDrum = 1,        ///< 大鼓（石头类方块）
    Snare = 2,           ///< 小军鼓（沙子类方块）
    Hat = 3,             ///< 踩镲（玻璃类方块）
    Bass = 4,            ///< 贝斯（木材类方块）
    Flute = 5,           ///< 长笛（泥土类方块）
    Bell = 6,            ///< 钟（金块）
    Guitar = 7,          ///< 吉他（羊毛类方块）
    Chime = 8,           ///< 风铃（冰类方块）
    Xylophone = 9,       ///< 木琴（骨块）
    IronXylophone = 10,  ///< 铁琴（铁块）
    CowBell = 11,        ///< 牛铃（灵魂沙）
    Didgeridoo = 12,     ///< 迪吉里杜管（南瓜类）
    Bit = 13,            ///< 电子音（绿宝石块）
    Banjo = 14,          ///< 班卓琴（干草块）
    Pling = 15,          ///< 电钢琴（萤石）
    Zombie = 16,         ///< 僵尸头（MOB_HEAD，仅在音符盒上方生效）
    Skeleton = 17,       ///< 骷髅头（MOB_HEAD）
    Creeper = 18,        ///< 苦力怕头（MOB_HEAD）
    Dragon = 19,         ///< 龙首（MOB_HEAD）
    WitherSkeleton = 20, ///< 凋灵骷髅头（MOB_HEAD）
    Piglin = 21,         ///< 猪灵头（MOB_HEAD）
    CustomHead = 22      ///< 玩家头（CUSTOM，音色来自方块实体）
};
}
