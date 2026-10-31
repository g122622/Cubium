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
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.serialization.DataResult;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;
import net.minecraft.SharedConstants;
import net.minecraft.core.component.DataComponentPatch;
import net.minecraft.core.component.DataComponentType;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.nbt.ByteTag;
import net.minecraft.nbt.CompoundTag;
import net.minecraft.nbt.DoubleTag;
import net.minecraft.nbt.IntTag;
import net.minecraft.nbt.ListTag;
import net.minecraft.nbt.LongTag;
import net.minecraft.nbt.NbtOps;
import net.minecraft.nbt.StringTag;
import net.minecraft.nbt.Tag;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.RegistryOps;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.item.ItemStack;

/**
 * 物品**数据组件补丁** NBT 表示的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ItemComponentNbtOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.ItemComponentNbtOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/item_component_nbt.cpp 同构：单元是「物品 ID#变体」，变体决定往物品堆上
 * 施加哪几组组件。输出是该物品堆相对**物品原型**的组件补丁，每个组件写成 SNBT 文本。
 *
 * <h2>为什么取补丁而不是完整的组件表</h2>
 *
 * <p>原版 {@code ItemStack} 内部是「原型（物品自带默认组件）+ 补丁」两层，
 * {@code ItemStack#getComponentsPatch()} 返回的只有相对原型新增或移除的键，也就是写进
 * 存档 {@code components} 段的那一份。直接序列化整个 map 会把原型的默认组件也写出来，
 * 比对的就不再是「我们自己放了什么」，而是物品注册表本身。
 *
 * <p>Cubium 侧 {@code ItemStack::toComponentPatch()} 是同一语义：只导出非默认字段。
 *
 * <h2>变体</h2>
 *
 * <ul>
 *   <li>{@code plain}  ：不施加任何组件——校验「没有组件时不写出 components 段」；
 *   <li>{@code suite}  ：施加一组固定的非默认值——校验组件名与各自的 NBT 结构；
 *   <li>{@code removed}：在 {@code suite} 的基础上再**主动移除**原型自带的组件——
 *       校验 {@code "!name"} 形式的移除键。
 * </ul>
 *
 * <p>变体名与组件取值的唯一事实来源是 {@code harness/harnesses/item_component_nbt.ts}：
 * 两侧读同一份描述，探针按它构造物品堆，oracle 按它构造 {@code ItemStack.CODEC} 的输入。
 */
public final class ItemComponentNbtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    static final String VARIANT_PLAIN = "plain";
    static final String VARIANT_SUITE = "suite";
    static final String VARIANT_REMOVED = "removed";

    private ItemComponentNbtOracle() {}

    public static void main(String[] args) throws IOException {
        // 物品、组件类型与附魔等注册表都靠 Bootstrap 静态初始化填充。
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            List<String> items = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
                items.add(id.toString());
            }
            items.sort(Comparator.naturalOrder());
            for (String item : items) {
                for (String variant : new String[] {VARIANT_PLAIN, VARIANT_SUITE, VARIANT_REMOVED}) {
                    text.append(item).append('#').append(variant).append('\n');
                }
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            LootServerData data = LootServerData.get();
            run(data, Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: ItemComponentNbtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

            JsonObject result = new JsonObject();
            result.addProperty("unit", unit);

            ItemStack stack = request.has("stack") ? build(data, request.getAsJsonObject("stack")) : null;
            // `minecraft:air` 是**合法的空物品堆**（逻辑上数量为 0），它的组件补丁是空对象，
            // 不是「造不出来」。按 `stack == null` 判缺失，不按 isEmpty()。
            if (stack == null) {
                result.addProperty("missing", true);
            } else {
                result.add("components", encodedComponents(stack, data));
            }
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 按输入里的 {@code stack} 描述构造物品堆：{@code id} + 可选的 {@code components}。 */
    private static ItemStack build(LootServerData data, JsonObject spec) {
        if (!spec.has("id")) {
            return null;
        }
        Identifier id = Identifier.tryParse(spec.get("id").getAsString());
        if (id == null || !BuiltInRegistries.ITEM.containsKey(id)) {
            return null;
        }

        // JSON → NBT 必须**逐字段**转，整数一律落 Int：原版 JsonOps 按值域挑最小整数类型，
        // `minecraft:damage: 7` 会变成 `7b`，而它用的是 Codec.INT，整条 ItemStack 因此解析失败。
        CompoundTag root = new CompoundTag();
        root.putString("id", spec.get("id").getAsString());
        JsonElement components = spec.get("components");
        if (components != null && components.isJsonObject()) {
            CompoundTag converted = new CompoundTag();
            for (Map.Entry<String, JsonElement> entry : components.getAsJsonObject().entrySet()) {
                // 移除键：值为空，NBT 上写作 "!name"，由 DataComponentPatch 的 codec 自行识别。
                converted.put(entry.getKey(), toNbt(entry.getValue()));
            }
            root.put("components", converted);
        }

        var ops = RegistryOps.create(NbtOps.INSTANCE, data.registries.compositeAccess());
        return ItemStack.CODEC.parse(ops, root).result().orElse(null);
    }

    /**
     * JSON → NBT，整数一律落 Int（超出 i32 才落 Long）。
     *
     * <p>原版 {@code JsonOps} 把 JSON 数字读成**最小**整数类型，`7` 于是变成 `7b`；而
     * {@code minecraft:damage} 等组件用的是 {@code Codec.INT}，只认 Int。这条差异在原版
     * 自己的「读 JSON」路径上被 {@code Codec.INT} 的宽容读取掩盖，走 NBT ops 就直接失败。
     */
    private static Tag toNbt(JsonElement element) {
        if (element == null || element.isJsonNull()) {
            return new CompoundTag();
        }
        if (element.isJsonPrimitive()) {
            var primitive = element.getAsJsonPrimitive();
            if (primitive.isBoolean()) {
                return ByteTag.valueOf(primitive.getAsBoolean());
            }
            if (primitive.isNumber()) {
                String raw = primitive.getAsString();
                if (raw.contains(".") || raw.contains("e") || raw.contains("E")) {
                    return DoubleTag.valueOf(primitive.getAsDouble());
                }
                long value = primitive.getAsLong();
                return value >= Integer.MIN_VALUE && value <= Integer.MAX_VALUE
                    ? IntTag.valueOf((int) value)
                    : LongTag.valueOf(value);
            }
            return StringTag.valueOf(primitive.getAsString());
        }
        if (element.isJsonArray()) {
            // 列表的元素类型由 codec 决定；这里按内容构造同构列表，空列表落成空 compound 列表。
            ListTag list = new ListTag();
            for (JsonElement item : element.getAsJsonArray()) {
                list.add(toNbt(item));
            }
            return list;
        }
        CompoundTag compound = new CompoundTag();
        for (Map.Entry<String, JsonElement> entry : element.getAsJsonObject().entrySet()) {
            compound.put(entry.getKey(), toNbt(entry.getValue()));
        }
        return compound;
    }

    /**
     * 把物品堆的组件补丁写成「组件名 → SNBT」的对象。
     *
     * <p>用 SNBT 而不是 JSON，是为了让类型差异显式可见：{@code 4.2f} 与 {@code 4.2d}、
     * {@code [I]} 与 {@code [...L]}、空列表与空 compound 都是不同的字节。键按名字排序——
     * patch 内部是无序 map，顺序不承载语义。
     */
    private static JsonObject encodedComponents(ItemStack stack, LootServerData data) {
        JsonObject result = new JsonObject();
        DataComponentPatch patch = stack.getComponentsPatch();
        for (Map.Entry<DataComponentType<?>, Optional<?>> entry : patch.entrySet()) {
            DataComponentType<?> type = entry.getKey();
            Identifier id = BuiltInRegistries.DATA_COMPONENT_TYPE.getKey(type);
            if (id == null) {
                continue;
            }
            if (entry.getValue().isEmpty()) {
                result.addProperty("!" + id, "");
                continue;
            }
            @SuppressWarnings("unchecked")
            DataComponentType<Object> typed = (DataComponentType<Object>) type;
            // 编码用纯 NbtOps：组件值已经被解析成具体实例（Holder、Enchantment 都已解析），
            // 再套一层 RegistryOps 会让 `EitherHolder` / `Holder` 的 dispatch 走「按注册表名」
            // 的分支，把完整定义缩回资源位置字符串。
            var encoded = typed.codecOrThrow().encodeStart(NbtOps.INSTANCE, entry.getValue().get());
            if (encoded instanceof DataResult.Success<?> success) {
                result.addProperty(id.toString(), String.valueOf(success.value()));
            }
        }
        TreeMap<String, JsonElement> ordered = new TreeMap<>();
        for (Map.Entry<String, JsonElement> entry : result.entrySet()) {
            ordered.put(entry.getKey(), entry.getValue());
        }
        JsonObject sorted = new JsonObject();
        for (Map.Entry<String, JsonElement> entry : ordered.entrySet()) {
            sorted.add(entry.getKey(), entry.getValue());
        }
        return sorted;
    }
}