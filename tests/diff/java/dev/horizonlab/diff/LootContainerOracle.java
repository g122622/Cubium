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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.component.DataComponents;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.ComponentSerialization;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.component.ItemContainerContents;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.block.entity.RandomizableContainerBlockEntity;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;

/**
 * 带战利品表的容器被打开后逐槽内容的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LootContainerOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/loot_container.cpp 同构：给方块实体记下 {@code LootTable} 与
 * {@code LootTableSeed}，再读第一个槽位触发延迟填充，最后逐格导出。
 *
 * <p>这里必须走 {@code RandomizableContainerBlockEntity#setLootTable} 而不是自己掷骰：
 * 「何时掷、用什么随机源、掷完怎么摆进槽位」全都在这条路径上，任何一环与原版不同，
 * 玩家开箱看到的东西就不一样。
 */
public final class LootContainerOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private LootContainerOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var data = LootServerData.get();
            StringBuilder text = new StringBuilder();
            for (String id : data.containerTableIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LootContainerOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var level = LootLevel.get();
        LootRoll.run(input, output, "table", (table, request, result) -> {
            String blockId = request.get("block").getAsString();
            Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(blockId)).orElse(null);
            result.addProperty("block", blockId);
            if (block == null) {
                result.addProperty("missing", true);
                return;
            }

            // 与探针一致：目前只打开箱子（其余容器子类的槽位数不同，探针尚未构造）。
            if (!blockId.equals("minecraft:chest")) {
                result.addProperty("unsupportedBlock", true);
                return;
            }
            // 不用 `Block#newBlockEntity`（1.21.11 的签名要 EntityCreationType）：走
            // `Level#setBlock` 让原版自己完成「方块 → 方块实体」的创建，与结构生成同一条路径。
            level.level.setBlockAndUpdate(new net.minecraft.core.BlockPos(0, 64, 0), block.defaultBlockState());
            BlockEntity entity = level.level.getBlockEntity(new net.minecraft.core.BlockPos(0, 64, 0));
            if (!(entity instanceof RandomizableContainerBlockEntity container)) {
                result.addProperty("missing", true);
                return;
            }

            // 名字与锁通过组件写进方块实体：原版 1.21 把二者的存储搬到了
            // DataComponents.CUSTOM_NAME / LOCK，方块实体已经没有 setCustomName 之类的入口了。
            var componentBuilder = net.minecraft.core.component.DataComponentMap.builder();
            if (request.has("name") && !request.get("name").getAsString().isEmpty()) {
                componentBuilder.set(DataComponents.CUSTOM_NAME, Component.literal(request.get("name").getAsString()));
            }
            if (request.has("lock") && !request.get("lock").getAsString().isEmpty()) {
                // LockCode 在 1.21 里持有的是「能开锁的钥匙物品」谓词，不是一个字符串；
                // 这里用锁名对应的物品名构造，语义与「拿这把钥匙名去匹配」一致。
                componentBuilder.set(
                    DataComponents.LOCK,
                    new net.minecraft.world.LockCode(net.minecraft.advancements.criterion.ItemPredicate.Builder.item()
                        .of(
                            LootServerData.get().registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.ITEM),
                            BuiltInRegistries.ITEM.getValue(Identifier.parse(request.get("lock").getAsString())))
                        .build()));
            }
            container.setComponents(componentBuilder.build());

            long seed = request.has("seed") ? request.get("seed").getAsLong() : 0L;
            container.setLootTable(
                net.minecraft.resources.ResourceKey.create(
                    net.minecraft.core.registries.Registries.LOOT_TABLE, Identifier.parse(unitTable(request))),
                seed);

            // 掉落物：在原版 `Block#playerDestroy` 里，`collectComponents()` 发生在
            // `unpackLootTable` 之前——此时容器还带着 container_loot、内容的「容器组件」
            // 视角也还是空的。所以必须在读任何一格之前先把掉落物掷出来。
            JsonObject drop = new JsonObject();
            JsonArray dropped = new JsonArray();
            var dropTables = new ArrayList<ItemStack>();
            // 走 BLOCK 参数集：原版 Block#playerDestroy 调 getRandomItems 时给的正是 BLOCK 集，
            // 它同时带 block_state、tool 与 block_entity——掉落物的 copy_components(source:
            // block_entity) 靠的就是这里塞进去的方块实体。
            var dropParams = new LootParams.Builder(level.level)
                .withParameter(LootContextParams.ORIGIN, LootTableOracle.origin(request))
                .withParameter(LootContextParams.BLOCK_STATE, block.defaultBlockState())
                .withParameter(LootContextParams.TOOL, ItemStack.EMPTY)
                .withParameter(LootContextParams.BLOCK_ENTITY, container)
                .create(net.minecraft.world.level.storage.loot.parameters.LootContextParamSets.BLOCK);
            var dropRandom = new net.minecraft.world.level.levelgen.XoroshiroRandomSource(LootRoll.ROLL_SEED);
            dropTables.addAll(table.getRandomItems(dropParams, dropRandom));
            // 与 C++ 探针同一个收集口径（LootTable::generate）：把掉落物按原版
            // createStackSplitter 的方式合并——空堆丢弃、同物品按最大堆叠数合并。
            for (ItemStack stack : LootRoll.splitIntoStacks(dropTables)) {
                if (!stack.isEmpty()) {
                    JsonObject entry = new JsonObject();
                    addSlot(entry, stack);
                    dropped.add(entry);
                }
            }
            drop.add("items", dropped);

            // 先读一格触发延迟填充（原版 `getItem` 会先 unpackLootTable），
            // 再摆预置物品——与探针的顺序完全一致。
            int size = container.getContainerSize();
            container.getItem(0);
            if (request.has("items")) {
                for (JsonElement element : request.getAsJsonArray("items")) {
                    JsonObject item = element.getAsJsonObject();
                    int slot = item.get("slot").getAsInt();
                    if (slot < 0 || slot >= size) {
                        continue;
                    }
                    var itemId = BuiltInRegistries.ITEM
                        .getOptional(Identifier.parse(item.get("id").getAsString()))
                        .orElse(null);
                    if (itemId == null) {
                        continue;
                    }
                    int count = item.has("count") ? item.get("count").getAsInt() : 1;
                    container.setItem(slot, new ItemStack(itemId, count));
                }
            }

            // 槽位段只比「哪一格放着什么、几个」：名字与容器内容由 drop 段比
            // （那里的 copy_components 才是这些组件的真实来源）。
            JsonArray slots = new JsonArray();
            for (int slot = 0; slot < size; ++slot) {
                JsonObject entry = new JsonObject();
                entry.addProperty("slot", slot);
                addSlot(entry, container.getItem(slot));
                slots.add(entry);
            }
            result.add("slots", slots);
            result.add("drop", drop);
        });
    }

    /** 单元键 `表#场景` → 表 ID；方块实体只知道表 ID，场景后缀只用于对齐比对。 */
    private static String unitTable(JsonObject request) {
        String unit = request.get("table").getAsString();
        int separator = unit.lastIndexOf('#');
        return separator < 0 ? unit : unit.substring(0, separator);
    }

    private static void addSlot(JsonObject entry, ItemStack stack) {
        if (stack.isEmpty()) {
            entry.add("id", JsonNull.INSTANCE);
            entry.addProperty("count", 0);
            entry.add("customName", JsonNull.INSTANCE);
            return;
        }
        entry.addProperty("id", stack.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
        entry.addProperty("count", stack.getCount());
        Component customName = stack.get(DataComponents.CUSTOM_NAME);
        if (customName == null) {
            entry.add("customName", JsonNull.INSTANCE);
        } else {
            entry.add("customName", GSON.toJsonTree(ComponentSerialization.CODEC.encodeStart(
                com.mojang.serialization.JsonOps.INSTANCE, customName).getOrThrow()));
        }
        entry.add("container", containerComponent(stack));
    }

    /** 物品的 container 组件：稀疏的「槽位号 + 物品」列表；没有组件时返回 null。 */
    private static JsonElement containerComponent(ItemStack stack) {
        ItemContainerContents contents = stack.get(DataComponents.CONTAINER);
        if (contents != null) {
            // ItemContainerContents 的槽位是稀疏的「非空项 + 序号」列表（对应 Codec 里的
            // `slot` 字段），正好是 Cubium 侧 (index, item) 的口径：直接逐项写出，
            // 不做压缩或补空。
            JsonArray items = new JsonArray();
            int index = -1;
            for (ItemStack inner : contents.nonEmptyItems()) {
                index++;
                if (inner.isEmpty()) {
                    continue;
                }
                JsonObject item = new JsonObject();
                item.addProperty("slot", index);
                item.addProperty("id", inner.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
                item.addProperty("count", inner.getCount());
                items.add(item);
            }
            return items;
        }
        return JsonNull.INSTANCE;
    }
}