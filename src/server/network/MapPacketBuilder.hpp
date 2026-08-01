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

namespace mc::server {
class ServerWorld;
}

namespace mc::server::net {

/**
 * @brief 地图数据周期推送构造器（无状态静态类）
 *
 * 承接原 ServerWorld::_pushMapDataToHolders 的网络包构造与下发逻辑。
 * 遍历在线玩家背包中持有的脏 MapData，构造 ir::play::MapItemData（colorPatch 全图 +
 * decorations 全量）并经 ConnectionManager 下发给持有该图的玩家。
 *
 * 与 PacketBuilders 的边界：PacketBuilders 是无状态单包构造（只产 IrPacket，不含发送
 * 语义）；本构造器是「世界级周期批处理 + 自带发送」（遍历玩家→扫描背包→判脏→构造→
 * 下发），故独立成文件，不并入 PacketBuilders。
 *
 * 调用时机：必须在 MapDataManager::tick 清脏之前调用（依赖 MapData::isDirty() 判定）。
 */
class MapPacketBuilder {
public:
    /**
     * @brief 推送所有脏地图数据给持有该图的在线玩家
     *
     * 遍历每个在线玩家 41 槽背包，去重收集已填充地图 id，对每张处于脏状态的地图
     * 全量推送 colorPatch（128×128）+ decorations，经 ConnectionManager::sendToPlayer 下发。
     *
     * @param world 服务端世界（取 server()/mapDataManager()）
     */
    static void pushDirtyMaps(ServerWorld& world);
};

} // namespace mc::server::net
