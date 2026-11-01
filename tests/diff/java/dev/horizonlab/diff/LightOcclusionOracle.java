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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeMap;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;

/**
 * 方块状态光照阻挡属性的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.LightOcclusionOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.LightOcclusionOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是方块 ID；每个方块输出它**全部状态**的四个光照属性，按状态名排序：
 *
 * <ul>
 *   <li>{@code lightBlock} — {@code BlockState#getLightBlock()}，光走过这一格后的衰减量。</li>
 *   <li>{@code propagatesSkylightDown} — 天空光是否不衰减地直通向下。</li>
 *   <li>{@code useShapeForLightOcclusion} — 是否按形状做面遮挡（而不是整格）。</li>
 *   <li>{@code canOcclude} — 是否参与遮挡计算的总结开关。</li>
 * </ul>
 *
 * <p>这四个值都是 {@code BlockBehaviour.BlockStateBase} 上的构造期缓存字段，注册时由
 * 方块与 {@code BlockProperties} 共同决定。这里只读公开的 getter，让原版自己算，
 * 与探针侧读同一组值——两边都不复述规则。
 */
public final class LightOcclusionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private LightOcclusionOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : blockIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LightOcclusionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static List<String> blockIds() {
        // 必须先触发 bootstrap（LootServerData.get() 会跑 Bootstrap.bootStrap），
        // 再碰 BuiltInRegistries：静态初始化阶段读它会抛 "Not bootstrapped"。
        LootServerData.get();
        List<String> ids = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
            ids.add(id.toString());
        }
        ids.sort(String::compareTo);
        return ids;
    }

    private static void run(Path input, Path output) throws IOException {
        LootServerData.get();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("block").getAsString();

            JsonObject result = new JsonObject();
            result.addProperty("block", unit);

            // 空气也是合法方块（注册表里有它），不能当「缺失」处理；只有真查不到才是 missing。
            Block block = BuiltInRegistries.BLOCK.getValue(Identifier.parse(unit));
            if (block == null) {
                result.addProperty("missing", true);
                text.append(GSON.toJson(result)).append('\n');
                continue;
            }

            TreeMap<String, JsonObject> values = new TreeMap<>();
            for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                JsonObject entry = new JsonObject();
                entry.addProperty("lightBlock", state.getLightBlock());
                entry.addProperty("propagatesSkylightDown", state.propagatesSkylightDown());
                entry.addProperty("useShapeForLightOcclusion", state.useShapeForLightOcclusion());
                entry.addProperty("canOcclude", state.canOcclude());
                values.put(stateName(state), entry);
            }

            JsonObject valueObject = new JsonObject();
            for (var entry : values.entrySet()) {
                valueObject.add(entry.getKey(), entry.getValue());
            }
            result.add("values", valueObject);
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    /** 与探针的 {@code stateName} 同一写法：`方块ID[属性=值,...]`，属性按名字排序。 */
    private static String stateName(BlockState state) {
        String name = BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString();
        List<String> pairs = new ArrayList<>();
        for (var property : state.getProperties()) {
            pairs.add(property.getName() + "=" + propertyName(state, property));
        }
        if (pairs.isEmpty()) {
            return name;
        }
        pairs.sort(String::compareTo);
        return name + "[" + String.join(",", pairs) + "]";
    }

    /**
     * 属性的序列化名（{@code facing=east} 里的 east）。
     *
     * <p>{@code Property} 有两个同名的 {@code getName}：无参的给出属性名，带参的给出该值的
     * 序列化名。旧版的 {@code getValueName} 在 1.21.11 已并入后者。
     */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static String propertyName(
            BlockState state, net.minecraft.world.level.block.state.properties.Property property) {
        return property.getName((Comparable) state.getValue(property));
    }
}