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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.serialization.JsonOps;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.animal.pig.Pig;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.enchantment.Enchantments;
import net.minecraft.world.level.storage.loot.LootParams;
import net.minecraft.world.level.storage.loot.LootPool;
import net.minecraft.world.level.storage.loot.LootTable;
import net.minecraft.world.level.storage.loot.entries.LootItem;
import net.minecraft.world.level.storage.loot.functions.LootItemFunction;
import net.minecraft.world.level.storage.loot.functions.LootItemFunctions;
import net.minecraft.world.level.storage.loot.functions.SetItemCountFunction;
import net.minecraft.world.level.storage.loot.parameters.LootContextParamSets;
import net.minecraft.world.level.storage.loot.parameters.LootContextParams;
import net.minecraft.world.level.storage.loot.providers.number.ConstantValue;
import net.minecraft.world.phys.Vec3;

/**
 * 战利品函数**逐函数**比对的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LootFunctionOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.LootFunctionOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/loot_function.cpp 同构：现场造一张「1 个池 + 1 个必定产出一条物品」的表，
 * 表级只挂一个函数，然后在固定种子下掷 4 次。差异因此只可能来自函数本身。
 *
 * <h2>为什么要逐函数</h2>
 *
 * <p>已有的 {@code loot_table*} 三个能力比的是「整张表掷出来什么」，一张表的差异可能来自
 * 池、条目、条件或函数中的任何一层。函数是其中唯一大量消费随机源的一层：随机数的消耗
 * 顺序错一位（例如先判断再取数、取值区间端点写反），整条链的结果都会偏移。
 *
 * <h2>函数的解析方式</h2>
 *
 * <p>直接把输入里的 JSON 喂给 {@link LootItemFunctions#CODEC}（数据包写法与这里完全同构），
 * 走的是原版**自己的**注册表分发——不调用任何构造函数，也不为每个函数写一份。
 *
 * <h2>上下文</h2>
 *
 * <p>参数集用 {@code BLOCK}（对齐 Cubium 的 {@code LootParameterSets::block()}）。
 * 所谓「掠夺等级」其实落在 {@code tool} 这个键上：原版 {@code ApplyBonusCount} 从
 * {@code getOptionalParameter(TOOL)} 读附魔等级，所以两侧都给一把带 {@code looting} 附魔的
 * 铁剑（空手时就是没附魔的同一把）。其余上下文固定：{@code origin} 是 (0.5, 64, 0.5)、
 * {@code this_entity} 是一头猪、{@code block_state} 取世界原点的方块（空气）。
 *
 * <p><b>为什么不用 ENTITY / ALL_PARAMS</b>：ENTITY 不允许 {@code tool} 与
 * {@code explosion_radius} 出现，而 {@code apply_bonus} 要从工具读附魔、{@code explosion_decay}
 * 要读半径，两者都是「键在就执行、不在就原样返回」——用 ENTITY 会让它们在 oracle 侧静默
 * 不执行，差异会指向 oracle 缺键而不是 Cubium。ALL_PARAMS 虽然十项俱全，但全部登记为
 * <b>必需</b>，{@code withParameter} 传 null 会被原版拒绝，就必须造玩家与方块实体的桩值。
 */
public final class LootFunctionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /**
     * 场景单元名，与 probes/loot_function.cpp 的 SCENARIO_UNITS 逐字一致。
     *
     * <p>两份清单靠差分本身保真：任何一边新增场景而另一边没有，单元集合立刻对不上，
     * 报告里会点名多出来的那个。场景的**取值**只有一份（harness 的 loot_function.ts）。
     */
    static final String[] SCENARIO_UNITS = {
        "set_count#constant", "set_count#uniform", "set_count#binomial", "set_count#uniform_float_bounds",
        "set_count#add", "apply_bonus#ore_drops", "apply_bonus#uniform_bonus", "apply_bonus#binomial_with_bonus",
        "apply_bonus#zero_level", "limit_count#range", "limit_count#only_max", "explosion_decay#plain",
        "set_damage#fraction", "set_damage#uniform", "enchant_with_levels#uniform", "enchant_with_levels#with_treasure",
        "enchant_randomly#plain", "enchant_randomly#options", "set_enchantments#levels",
        "enchanted_count_increase#limit", "set_custom_data#compound", "set_components#stack_size", "set_name#fixed",
        "set_lore#fixed", "set_potion#id", "set_instrument#tag", "set_stew_effect#single",
        "set_ominous_bottle_amplifier#range", "furnace_smelt#raw_beef", "set_item#swap", "discard#always",
        "set_contents#single", "set_loot_table#inline", "exploration_map#beach", "sequence#two", "filtered#pass",
        "filtered#block", "copy_name#this", "copy_custom_data#this", "copy_state#this", "fill_player_head#this",
    };

    private LootFunctionOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : SCENARIO_UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            LootServerData data = LootServerData.get();
            run(data, Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LootFunctionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(LootServerData data, Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("unit").getAsString();
            text.append(GSON.toJson(evaluate(data, unit, request))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject evaluate(LootServerData data, String unit, JsonObject request) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        if (!request.has("function")) {
            return failure(result, "missing function");
        }

        // 函数：JSON 直接喂给原版注册表 codec（数据包写法与这里完全同构）。
        var ops = data.registries.compositeAccess().createSerializationContext(JsonOps.INSTANCE);
        // ROOT_CODEC 对应数据包里「一个函数对象」的写法；CODEC 那一条是 item_modifier 的
        // 注册表引用形态，不是这里要的。
        var parsed = LootItemFunctions.ROOT_CODEC.parse(ops, request.get("function"));
        if (parsed.error().isPresent()) {
            return failure(result, "cannot parse function");
        }
        LootItemFunction function = parsed.result().orElseThrow();

        // 输入物品：`{id, count}`。
        JsonObject itemSpec = request.has("item") ? request.getAsJsonObject("item") : new JsonObject();
        String itemId = itemSpec.has("id") ? itemSpec.get("id").getAsString() : "minecraft:stone";
        Item item = data.registries.compositeAccess().lookupOrThrow(Registries.ITEM)
            .get(net.minecraft.resources.ResourceKey.create(Registries.ITEM, Identifier.parse(itemId)))
            .map(holder -> holder.value())
            .orElse(null);
        if (item == null || item == Items.AIR) {
            return failure(result, "unknown input item");
        }
        int count = itemSpec.has("count") ? itemSpec.get("count").getAsInt() : 1;

        // 单池单条目：一个必定产出的物品条目（数量由输入给定），掷骰次数固定 1，
        // 表级只挂目标函数——于是「掷出来什么」完全由这个函数决定。
        // 参数集与下面的上下文一致（BLOCK）；表级声明不参与掷骰，只用于元信息。
        LootTable table = LootTable.lootTable()
            .setParamSet(LootContextParamSets.BLOCK)
            .withPool(LootPool.lootPool()
                .setRolls(ConstantValue.exactly(1.0f))
                .setBonusRolls(ConstantValue.exactly(0.0f))
                // 条目数量取输入的 count。SetItemCountFunction 用 ConstantValue，
                // 不消费随机数，因此不影响后面函数的抽数顺序。
                .add(LootItem.lootTableItem(item)
                    .setWeight(1)
                    .apply(net.minecraft.world.level.storage.loot.functions.SetItemCountFunction.setCount(
                        ConstantValue.exactly(count)))))
            .apply(() -> function)
            .build();

        int looting = request.has("looting") ? request.get("looting").getAsInt() : 0;
        float luck = request.has("luck") ? request.get("luck").getAsFloat() : 0.0f;
        boolean hasRadius = request.has("explosion_radius");
        float explosionRadius = hasRadius ? request.get("explosion_radius").getAsFloat() : 0.0f;

        var level = LootLevel.get();
        // 受害者：`this_entity` 填一头猪。TOOL 也构造出来（空手时就是一把没附魔的剑）。
        Pig victim = new Pig(EntityType.PIG, level.level);
        ItemStack tool = new ItemStack(Items.IRON_SWORD);
        if (looting > 0) {
            tool.enchant(level.level.registryAccess().lookupOrThrow(Registries.ENCHANTMENT)
                .getOrThrow(net.minecraft.world.item.enchantment.Enchantments.LOOTING), looting);
        }

        // 参数集用 BLOCK，与 Cubium 的 LootParameterSets::block() 逐项一致：
        //   required {BLOCK_STATE, ORIGIN, TOOL}，optional {THIS_ENTITY, BLOCK_ENTITY, EXPLOSION_RADIUS}。
        // 这是唯一「同时允许 tool 与 explosion_radius、又不必造玩家/伤害来源桩值」的参数集：
        //   - ENTITY 不允许 tool / explosion_radius 出现，apply_bonus 与 explosion_decay 会
        //     因为读不到键而静默不执行，差异会指向 oracle 缺键而不是 Cubium；
        //   - ALL_PARAMS 虽然十项俱全，但全部登记为必填，withParameter 传 null 会被原版拒绝。
        // 本能力只覆盖函数本身：不需要玩家、方块实体与伤害来源，因此两侧都不提供它们。
        // 幸运值是构建器上的独立字段（withLuck），不在键集合里。
        JsonArray samples = new JsonArray();
        for (int sample = 0; sample < LootRoll.ROLL_SAMPLES; ++sample) {
            var random = LootRoll.randomForSample(sample);
            LootParams params = new LootParams.Builder(level.level)
                .withParameter(LootContextParams.ORIGIN, new Vec3(0.5, 64.0, 0.5))
                .withParameter(LootContextParams.THIS_ENTITY, victim)
                .withParameter(LootContextParams.BLOCK_STATE, level.level.getBlockState(BlockPos.ZERO))
                .withParameter(LootContextParams.TOOL, tool)
                .withOptionalParameter(LootContextParams.EXPLOSION_RADIUS, hasRadius ? explosionRadius : null)
                .withLuck(luck)
                .create(LootContextParamSets.BLOCK);
            samples.add(summarise(table.getRandomItems(params, random)));
        }
        result.addProperty("ok", true);
        result.add("samples", samples);
        return result;
    }

    /** 失败与成功共用 `ok` 字段，两侧口径一致。 */
    private static JsonObject failure(JsonObject result, String reason) {
        result.addProperty("ok", false);
        result.addProperty("error", reason);
        return result;
    }

    /** 与 C++ 探针 {@code summariseStacks} 同口径：只给 id 与 count，按名再按数量排序。 */
    private static JsonArray summarise(List<ItemStack> stacks) {
        List<JsonObject> entries = new ArrayList<>();
        for (ItemStack stack : stacks) {
            if (stack.isEmpty()) {
                continue;
            }
            JsonObject entry = new JsonObject();
            entry.addProperty("id", stack.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
            entry.addProperty("count", stack.getCount());
            entries.add(entry);
        }
        entries.sort((left, right) -> {
            int byId = left.get("id").getAsString().compareTo(right.get("id").getAsString());
            return byId != 0 ? byId : Integer.compare(left.get("count").getAsInt(), right.get("count").getAsInt());
        });
        JsonArray array = new JsonArray();
        entries.forEach(array::add);
        return array;
    }
}