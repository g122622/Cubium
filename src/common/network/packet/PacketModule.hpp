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

/**
 * @file Packet.hpp
 * @brief 网络数据包模块统一头文件
 *
 * 包含所有数据包相关的类。
 */

#include "BlockEntityDataPacket.hpp"
#include "CommandTreePacket.hpp"
#include "ContainerPacketHandler.hpp"
#include "EntityMetadataSerializer.hpp"
#include "EntityPackets.hpp"
#include "GameStateChangePacket.hpp"
#include "InventoryPackets.hpp"
#include "Packet.hpp"
#include "PacketDeserializer.hpp"
#include "PacketSerializer.hpp"
#include "ParticlePacket.hpp"
#include "ProtocolPackets.hpp"
#include "RecipePackets.hpp"
#include "ServerDifficultyPacket.hpp"
#include "SetCameraPacket.hpp"
#include "SetEntityLinkPacket.hpp"
#include "TitlePacket.hpp"
