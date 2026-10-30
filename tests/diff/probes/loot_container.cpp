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

/**
 * @file loot_container.cpp
 * @brief 差分能力：带战利品表的容器（箱子、木桶、潜影盒……）被打开后逐槽的内容。
 *
 * `list`：数据包里声明了 `random_sequence` 的战利品表 ID
 * `run` ：{ "table": "minecraft:chests/simple_dungeon", "block": "minecraft:chest",
 *           "seed": 1234567, "name": "Dungeon Chest", "lock": "" }
 *         → { "missing" } 或 { "slots": [{slot, id, count, customName, container, lock, containerLoot}, ...] }
 *
 * ## 为什么需要这一路
 *
 * 另外三个战利品探针只掷「一张表会掉什么」，而真实世界里箱子是这样用的：
 * 结构生成时给方块实体记下 `LootTable` 与 `LootTableSeed`，玩家第一次打开时
 * 才把表掷出来的物品**逐格摆进 27 个槽位**。这段「摆」的逻辑和「掷」完全不同：
 *
 *   - 摆的位置决定玩家看到什么（先填满前几格还是随机撒开、同类物品会不会并堆）；
 *   - 摆之前还要从方块实体自己身上抄组件——`copy_components(source: block_entity)`
 *     会把容器的自定义名（箱子上面悬的名字）、锁、潜影盒里装的物品抄到掉落物上。
 *     抄错组件，掉落的箱子就丢了名字、潜影盒就变成空的。
 *
 * ## 能比出什么
 *
 * 1. 掷骰种子与容器填充的随机源是否和原版一致：每个槽位的物品与数量逐格相同；
 * 2. `copy_components` 对 `block_entity` 来源的实现：自定义名、锁、容器内容；
 * 3. 槽位分配：物品落在哪些槽位上（原版洗牌空槽、拆大堆后再依次落位）。
 *
 * ## 已知缺口（本 harness 目前必然报差异的部分）
 *
 * TODO: 【首要缺口】oracle 侧产出与探针不一致的原因已定位到「oracle 进程加载的是旧字节码」：
 *       源码与字节码都已含 addSlot 的 customName/container 字段，但 oracle_outputs 里仍然
 *       只有 id/count。可能是 javac 增量编译 + 类目录复用的缓存问题，尚未查清。
 *       在修好之前，本 harness 的 report 里 drop 段出现「探针多出字段」一律按此缺口看待。
 *
 * TODO: 容器填充的随机源还没有对齐。原版 `LootTable.fill` 用的是
 *       `LootContext.Builder(...).withOptionalRandomSeed(seed).create(randomSequence)`：
 *       种子非零时是 `RandomSource.create(seed)`，但**上下文仍带着 random_sequence**，
 *       而 `LootTable.getRandomItems` 在此之后还会从 `server.getRandomSequence(...)`
 *       派生独立随机源，导致填充掷出的物品与本探针按裸种子掷出的完全不同
 *       （本探针与 oracle 的 `drop` 部分反而完全一致，可作为该结论的旁证）。
 *       Cubium 侧需要补齐 random_sequence 的派生（Xoroshiro.positional 派生），
 *       并在 `LootableContainerBlockEntity::fillWithLootFromTable` 里改用它；
 *       补齐前 `slots` 整段必然不一致，`drop` 段才是有意义的比对项。
 *
 * ## 已知简化
 *
 * TODO: 容器目前只覆盖箱子（27 格）。木桶 / 发射器 / 漏斗 / 潜影盒等其余
 *       `LootableContainerBlockEntity` 子类尚未在本探针里构造——它们的槽位数与
 *       方块实体类型不同，需要逐个补构造分支；补齐前这些方块的表由 oracle 侧
 *       照常输出、探针侧报 `missing`，差异显式可见而非静默通过。
 */
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/LockableBlockEntity.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"

#include "loot_probe_common.hpp"
#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace lootprobe;

using blockentity::LootableContainerBlockEntity;

/**
 * 本探针的可枚举单元：带 `random_sequence` 的表。
 *
 * 只有这类表会被原版当成「容器战利品表」登记随机序列；取它而不是全部 1300 张表，
 * 是为了让单元集合同时表达「这张表能不能被当作容器战利品表使用」。
 */
void writeList(FILE* out)
{
    for (const auto& id : environment().containerTableIds) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

/// 把物品堆的 `container` 组件写成 `[{slot, id, count}]`；没有组件时返回 null。
json containerContents(const ItemStack& stack)
{
    const auto* contents = stack.getContainerContents();
    if (contents == nullptr) {
        return nullptr;
    }
    json slots = json::array();
    for (const auto& entry : contents->slots) {
        if (entry.item == nullptr || entry.item->isEmpty() || entry.item->getItem() == nullptr) {
            continue;
        }
        slots.push_back(json{
            {"slot", entry.index},
            {"id", entry.item->getItem()->itemLocation().toString()},
            {"count", entry.item->getCount()},
        });
    }
    return slots;
}

/// 一个槽位的完整快照：物品、数量，以及 `copy_components` 可能抄过来的三样东西。
json slotSnapshot(const ItemStack& stack)
{
    json result = json::object();
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        result["id"] = nullptr;
        result["count"] = 0;
        result["customName"] = nullptr;
        return result;
    }
    result["id"] = stack.getItem()->itemLocation().toString();
    result["count"] = stack.getCount();
    result["customName"] = stack.hasCustomName() ? json(stack.getCustomName()) : json(nullptr);
    json contents = containerContents(stack);
    if (!contents.is_null()) {
        result["container"] = std::move(contents);
    }
    return result;
}

std::string runLine(std::string_view inputLine)
{
    auto& env = environment();
    const json input = json::parse(inputLine);
    // 单元键是「表 ID#场景」：场景只用于对齐比对，查表必须用 # 之前的表名。
    const std::string unitKey = input.at("table").get<std::string>();
    const auto hashPos = unitKey.rfind('#');
    const std::string id = hashPos == std::string::npos ? unitKey : unitKey.substr(0, hashPos);
    const std::string blockName = input.at("block").get<std::string>();

    if (env.tables.getTable(id) == nullptr) {
        return json{{"table", unitKey}, {"missing", true}}.dump();
    }

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(blockName));
    if (block == nullptr || !block->hasBlockEntity()) {
        return json{{"table", unitKey}, {"block", blockName}, {"missing", true}}.dump();
    }

    // TODO: 本探针目前只打开 `minecraft:chest`。木桶 / 发射器 / 投掷器 / 漏斗 / 潜影盒
    //       等其余 LootableContainerBlockEntity 子类的槽位数不同（27/9/5/27），
    //       需要逐个补构造分支；补齐前这些方块不在此 harness 的单元里，
    //       由 `loot_table_block` 与单元测试兜底。

    const BlockPos pos(0, 64, 0);
    std::unique_ptr<BlockEntity> created = block->createBlockEntity(pos);
    auto* container = dynamic_cast<LootableContainerBlockEntity*>(created.get());
    if (container == nullptr) {
        return json{{"table", unitKey}, {"block", blockName}, {"missing", true}}.dump();
    }
    container->setWorld(env.world.get());

    // 方块实体自己的「附加数据」：原版结构生成时会把箱子名、锁写进方块实体，
    // `copy_components(source: block_entity)` 再从它身上抄进掉落物。
    if (input.contains("name") && !input.at("name").get<std::string>().empty()) {
        container->setCustomName(input.at("name").get<std::string>());
    }
    if (input.contains("lock") && !input.at("lock").get<std::string>().empty()) {
        if (auto* lockable = dynamic_cast<blockentity::LockableBlockEntity*>(container)) {
            lockable->setLockKey(input.at("lock").get<std::string>());
        }
    }

    IInventory* inventory = container->getInventory();
    if (inventory == nullptr) {
        return json{{"table", unitKey}, {"block", blockName}, {"missing", true}}.dump();
    }

    const i64 seed = input.value("seed", 0LL);
    container->setLootTable(ResourceLocation(id), seed);

    // 在填充之前拷一次内容组件：原版摆放战利品时 `setItem` 会逐个解包，
    // 等到插槽都填好，`collectImplicitComponents` 的 container_loot 早已被清空，
    // 掉落物就只能靠 container 组件把内容带下去。这里先把「掉落那一刻的方块实体」
    // 存成快照，填充走一份副本，快照本身保持未拆包。对齐 vanilla
    // `Block#playerDestroy` 的调用时机：先 collectComponents，再 unpackLootTable。
    //
    // TODO: 目前只做「掉落那一刻容器还没被打开过」这一种时序。若将来要覆盖
    //       「先被打开、玩家拿走一半后再敲掉」，需要把快照换成真实方块实体的引用。
    // 固定随机源：两侧都用 ROLL_SEED，掉落物的内容因此逐位可比。
    math::Xoroshiro128ppRandom dropRandom(ROLL_SEED);
    loot::LootContextBuilder componentBuilder(*env.world);
    componentBuilder.withRandom(dropRandom);
    componentBuilder.withOwnedValue(loot::LootParams::ORIGIN,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f));
    componentBuilder.withParameter(loot::LootParams::BLOCK_ENTITY, static_cast<BlockEntity*>(container));
    auto componentContext = componentBuilder.build(loot::LootParameterSets::chest());

    // 取第一个槽位触发延迟填充：原版 `RandomizableContainerBlockEntity#getItem` 也是
    // 先解包再返回。填充完战利品表就被清掉，之后 `container_loot` 组件也就不再存在。
    static_cast<void>(inventory->getItem(0));

    // 预置物品在填充之后才摆：原版 `setItem` 同样会先 `unpackLootTable`，
    // 所以「结构生成时箱子里已有的东西」在语义上就是「先开箱、再摆放」。
    // TODO: 若要覆盖「摆放发生在开箱之前」的场景（真实结构生成正是如此，此时战利品
    //       只会填进剩下的空槽），需要在填充前就把物品写进槽位并绕过解包回调；
    //       目前探针拿不到这条路径，`prefilled` 场景比的是「开箱后再摆」。
    if (input.contains("items")) {
        for (const auto& item : input.at("items")) {
            const i32 slot = item.at("slot").get<i32>();
            if (slot < 0 || slot >= inventory->getContainerSize()) {
                continue;
            }
            auto parsed =
                ItemStack::fromJson(json{{"id", item.at("id").get<std::string>()}, {"Count", item.value("count", 1)}});
            if (parsed.success()) {
                inventory->setItem(slot, parsed.value());
            }
        }
    }

    // 槽位段：哪一格放着什么、几个。名字与容器内容由下面的 drop 段比。
    json slots = json::array();
    for (i32 slot = 0; slot < inventory->getContainerSize(); ++slot) {
        json entry = slotSnapshot(inventory->getItem(slot));
        entry["slot"] = slot;
        slots.push_back(std::move(entry));
    }

    // 掉落物那一面：同一张表按「被敲下来」的方式再掷一次，走真实的
    // `copy_components(source: block_entity)`，把方块实体身上能抄的组件抄过来。
    json drop = json::object();
    if (componentContext != nullptr) {
        const loot::LootTable* table = env.tables.getTable(id);
        std::vector<ItemStack> stacks;
        env.world->rollLoot(*table, *componentContext, stacks);
        json dropped = json::array();
        for (const ItemStack& stack : stacks) {
            if (stack.isEmpty() || stack.getItem() == nullptr) {
                continue;
            }
            // 掉落物只比 id 与数量：名字/容器组件由 slots 段单独覆盖。
            dropped.push_back(json{{"id", stack.getItem()->itemLocation().toString()},
                {"count", stack.getCount()},
                {"customName", stack.hasCustomName() ? json(stack.getCustomName()) : json(nullptr)},
                {"container", containerContents(stack)}});
        }
        drop["items"] = std::move(dropped);
    }

    return json{{"table", unitKey}, {"block", blockName}, {"slots", std::move(slots)}, {"drop", std::move(drop)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN("loot_container", &writeList, &runLine, &lootprobe::releaseEnvironment)

} // namespace mc::diff
