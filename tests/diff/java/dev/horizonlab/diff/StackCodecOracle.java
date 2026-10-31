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
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import net.minecraft.SharedConstants;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.nbt.CompoundTag;
import net.minecraft.nbt.NbtOps;
import net.minecraft.nbt.Tag;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.RegistryOps;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.item.ItemStack;

/**
 * 物品堆**整体编解码**（{@code ItemStack.CODEC}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.StackCodecOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.StackCodecOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/stack_codec.cpp 同构。单元是「物品 ID#变体」，变体决定喂进去的 NBT 长什么样：
 *
 * <ul>
 *   <li>{@code base}：{@code {id}}——校验 count 缺省为 1、components 缺省为空；
 *   <li>{@code suite}：{@code {id, count, components}} 全给——校验三个字段都在场时的编码；
 *   <li>{@code count#*} / {@code shell#*}：固定场景，与物品无关——数量的 1..99 区间、
 *       越界、空气、未知物品、空组件段、不可堆叠物品给 count > 1 等等，用来校验
 *       「什么时候应当失败」。具体取值只在 harness/harnesses/stack_codec.ts 里有一份。
 * </ul>
 *
 * <h2>为什么单独做这一层</h2>
 *
 * <p>{@code item_component_nbt} 比的是**组件载荷**本身；这里比的是把组件放进物品堆之后
 * 整个物品堆的编解码：{@code count} 的取值区间、空堆的表示、{@code components} 段的省略
 * 规则、以及解码失败的条件。这些规则写错不会在运行时立刻报错，而是「存档能读、物品不对」
 * ——数量被截断、空堆变成空气物品、坏数据被静默接受。
 *
 * <h2>往返</h2>
 *
 * <p>输出里同时包含两个方向：
 *
 * <ul>
 *   <li>{@code decoded}：把输入的 NBT 描述交给 {@code ItemStack.CODEC} 解析的结果
 *       （成功时给出正式的 {@code id} / {@code count} / {@code components} 编码文本）；
 *   <li>{@code reencode}：解析成功后再编码回去得到的 NBT 文本。两者应当互相印证——
 *       只比其中一边会让「解码丢掉字段、编码又凭空补上」这类错误漏网。
 * </ul>
 */
public final class StackCodecOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    static final String VARIANT_BASE = "base";
    static final String VARIANT_SUITE = "suite";
    /**
     * 固定场景的单元名，与 stack_codec.cpp 的 SCENARIO_UNITS 逐字一致。
     *
     * 两份清单靠差分本身保真：任何一边新增场景而另一边没有，单元集合立刻对不上，
     * 报告里会点名多出来的那个。场景的**取值**只有一份（stack_codec.ts），不在这里重复。
     */
    static final String[] SCENARIO_UNITS = {
        "count#min", "count#max", "count#zero", "count#over", "count#negative", "count#absent",
        "shell#extra-key", "shell#air", "shell#unknown-item", "shell#empty-components",
        "shell#damageable-stacked",
    };

    private StackCodecOracle() {}

    public static void main(String[] args) throws IOException {
        // 物品与组件类型注册表都靠 Bootstrap 静态初始化填充。
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            List<String> items = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.ITEM.keySet()) {
                items.add(id.toString());
            }
            items.sort(String::compareTo);
            for (String item : items) {
                for (String variant : new String[] {VARIANT_BASE, VARIANT_SUITE}) {
                    text.append(item).append('#').append(variant).append('\n');
                }
            }
            // 固定场景（数量区间、坏数据）与具体物品无关，单独列出。
            for (String scenario : SCENARIO_UNITS) {
                text.append(scenario).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            LootServerData data = LootServerData.get();
            run(data, Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: StackCodecOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(LootServerData data, Path input, Path output) throws IOException {
        var ops = RegistryOps.create(NbtOps.INSTANCE, data.registries.compositeAccess());
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("unit").getAsString();

            JsonObject result = new JsonObject();
            result.addProperty("unit", unit);

            JsonObject stack = request.has("stack") ? request.getAsJsonObject("stack") : null;
            if (stack == null) {
                result.addProperty("missing", true);
                text.append(GSON.toJson(result)).append('\n');
                continue;
            }

            CompoundTag source = toNbt(stack);
            // 注意：原版 count 的 codec 是 `ExtraCodecs.intRange(1, 99).fieldOf("count").orElse(1)`，
            // 实测 `orElse` 对「越界」是**宽容**的——0 / 100 / -1 都退回 1 而不是整条失败。
            // 这一行为原样记录，不做「纠正」：差分要抓的正是「本侧拒绝、原版接受」这类偏差。
            var parsed = ItemStack.CODEC.parse(ops, source);
            if (parsed.error().isPresent()) {
                // 只记「失败了」：两边的错误措辞不可能逐字相同，比对的是「该不该失败」
                // 而不是「怎么措辞」。
                result.addProperty("ok", false);
                result.addProperty("error", "invalid");
                text.append(GSON.toJson(result)).append('\n');
                continue;
            }
            ItemStack decoded = parsed.result().orElseThrow();
            result.addProperty("ok", true);
            result.add("decoded", describe(decoded));
            result.add("reencode", reencode(ops, decoded));
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 解码结果的规范描述：id / count / components（组件名 → SNBT）。 */
    private static JsonObject describe(ItemStack stack) {
        JsonObject out = new JsonObject();
        out.addProperty("id", BuiltInRegistries.ITEM.getKey(stack.getItem()).toString());
        out.addProperty("count", stack.getCount());
        out.addProperty("empty", stack.isEmpty());
        var patch = stack.getComponentsPatch();
        JsonObject components = new JsonObject();
        // 补丁内部的 map 无序，按组件名排序后逐条写出：顺序不含语义，但固定下来便于阅读报告。
        List<String> names = new ArrayList<>();
        for (var entry : patch.entrySet()) {
            names.add(BuiltInRegistries.DATA_COMPONENT_TYPE.getKey(entry.getKey()).toString());
        }
        names.sort(String::compareTo);
        for (String name : names) {
            Identifier key = Identifier.parse(name);
            var type = BuiltInRegistries.DATA_COMPONENT_TYPE.getValue(key);
            var value = patch.get(type);
            if (value == null) {
                continue;
            }
            if (value.isEmpty()) {
                components.addProperty("!" + name, "");
                continue;
            }
            @SuppressWarnings("unchecked")
            var typed = (net.minecraft.core.component.DataComponentType<Object>) type;
            // 纯 NbtOps：组件值已解析成具体实例，套 RegistryOps 会把 Holder 缩成资源位置。
            // 解不出来的组件（编解码需要注册表或世界上下文的少数几类）整条略去：那一层的
            // 文本比对由 item_component_nbt 负责，这里只关心「它还在不在补丁里」。
            var encoded = typed.codecOrThrow().encodeStart(NbtOps.INSTANCE, value.get());
            encoded.result().ifPresent(tag -> components.addProperty(name, String.valueOf(tag)));
        }
        out.add("components", components);
        // 解不出来的组件名单独列出：它们「在补丁里」这一事实仍然参与比对，
        // 只是文本不参与（见上）。
        return out;
    }

    /** 把解码出来的物品堆重新编码成 NBT 文本。 */
    private static JsonObject reencode(RegistryOps<Tag> ops, ItemStack stack) {
        JsonObject out = new JsonObject();
        var encoded = ItemStack.CODEC.encodeStart(ops, stack);
        if (encoded.error().isPresent()) {
            out.addProperty("ok", false);
            return out;
        }
        out.addProperty("ok", true);
        out.addProperty("nbt", String.valueOf(encoded.result().orElseThrow()));
        return out;
    }

    /**
     * JSON → NBT。整数一律落 Int（超出 i32 落 Long），与原版 {@code DataFixerUpper} 的
     * {@code Codec.INT} 只认 Int 一致；JSON 浮点落 Double。
     */
    private static CompoundTag toNbt(JsonObject spec) {
        CompoundTag root = new CompoundTag();
        for (Map.Entry<String, JsonElement> entry : spec.entrySet()) {
            root.put(entry.getKey(), toTag(entry.getValue()));
        }
        return root;
    }

    private static Tag toTag(JsonElement element) {
        if (element == null || element.isJsonNull()) {
            return new CompoundTag();
        }
        if (element.isJsonPrimitive()) {
            var primitive = element.getAsJsonPrimitive();
            if (primitive.isBoolean()) {
                return net.minecraft.nbt.ByteTag.valueOf(primitive.getAsBoolean());
            }
            if (primitive.isNumber()) {
                String raw = primitive.getAsString();
                if (raw.contains(".") || raw.contains("e") || raw.contains("E")) {
                    return net.minecraft.nbt.DoubleTag.valueOf(primitive.getAsDouble());
                }
                long value = primitive.getAsLong();
                return value >= Integer.MIN_VALUE && value <= Integer.MAX_VALUE
                        ? net.minecraft.nbt.IntTag.valueOf((int) value)
                        : net.minecraft.nbt.LongTag.valueOf(value);
            }
            return net.minecraft.nbt.StringTag.valueOf(primitive.getAsString());
        }
        if (element.isJsonArray()) {
            net.minecraft.nbt.ListTag list = new net.minecraft.nbt.ListTag();
            for (JsonElement item : element.getAsJsonArray()) {
                list.add(toTag(item));
            }
            return list;
        }
        CompoundTag compound = new CompoundTag();
        for (Map.Entry<String, JsonElement> entry : element.getAsJsonObject().entrySet()) {
            compound.put(entry.getKey(), toTag(entry.getValue()));
        }
        return compound;
    }
}