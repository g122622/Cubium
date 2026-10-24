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
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块状态的原版 oracle。
 *
 * <p>与 probes/text_encode.cpp 同构：
 *
 * <pre>
 *   list  → 每个状态一行：`minecraft:oak_log[axis=y]`
 *   run   → { "block": "minecraft:oak_log", "stateCount": 3,
 *             "properties": [{"name":"axis","type":"EnumProperty","values":["x","y","z"]}] }
 * </pre>
 *
 * 状态文本由原版 `BlockState#toString` 产生（属性按名字字典序），与 Cubium 的
 * 编码逐字节比对。相比 minecraft-data，这里还能拿到全部状态，包括它缺的两面墙。
 */
public final class BlockStateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockStateOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length >= 1 && args[0].equals("list")) {
            writeList();
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockStateOracle list | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void writeList() throws IOException {
        List<String> texts = new ArrayList<>();
        // 原版 BlockState#toString() 会带 `Block{...}` 前缀（调试用途），
    // 而注册名 + 属性才是方块状态的标准文本形式。这里按 BlockStateParser 的规则拼装。
        for (Block block : BuiltInRegistries.BLOCK) {
            for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                texts.add(stateText(state));
            }
        }
        texts.sort(Comparator.naturalOrder());
        BufferedWriter writer =
                new BufferedWriter(new OutputStreamWriter(System.out, StandardCharsets.UTF_8));
        for (String text : texts) {
            writer.write(text);
            writer.write('\n');
        }
        writer.flush();
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(input, StandardCharsets.UTF_8);
                BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject request = JsonParser.parseString(line).getAsJsonObject();
                writer.write(GSON.toJson(summarise(request.get("block").getAsString())));
                writer.write('\n');
            }
        }
    }

    private static JsonObject summarise(String blockId) {
        JsonObject result = new JsonObject();
        result.addProperty("block", blockId);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(blockId)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        result.addProperty("stateCount", block.getStateDefinition().getPossibleStates().size());

        // 属性按名字排序：比较的是「有哪些属性」，不是注册顺序。
        List<Property<?>> properties = new ArrayList<>(block.getStateDefinition().getProperties());
        properties.sort(Comparator.comparing(Property::getName));

        JsonArray json = new JsonArray();
        for (Property<?> property : properties) {
            JsonObject entry = new JsonObject();
            entry.addProperty("name", property.getName());
            entry.addProperty("type", property.getClass().getSimpleName());
            JsonArray values = new JsonArray();
            List<String> names = new ArrayList<>();
            appendValueNames(property, names);
            names.sort(String::compareTo);
            names.forEach(values::add);
            entry.add("values", values);
            json.add(entry);
        }
        result.add("properties", json);
        return result;
    }

    /**
     * 方块状态的标准文本：`minecraft:oak_log[axis=y]`，属性按名字字典序。
     *
     * 与原版 `StateHolder#toString` 的排序一致；但不带 `Block{...}` 前缀。
     */
    private static <T extends Comparable<T>> String stateText(BlockState state) {
        StringBuilder builder = new StringBuilder(BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString());
        List<Property<?>> properties = new ArrayList<>(state.getProperties());
        if (!properties.isEmpty()) {
            properties.sort(Comparator.comparing(Property::getName));
            builder.append('[');
            for (int i = 0; i < properties.size(); ++i) {
                if (i > 0) {
                    builder.append(',');
                }
                appendProperty(builder, state, properties.get(i));
            }
            builder.append(']');
        }
        return builder.toString();
    }

    private static <T extends Comparable<T>> void appendProperty(
            StringBuilder builder, BlockState state, Property<T> property) {
        builder.append(property.getName()).append('=').append(property.getName(state.getValue(property)));
    }

    /**
     * 收集属性全部取值的字符串表示。
     *
     * 写成泛型辅助方法而不是内联 for-each：`Property<?>` 的取值类型是通配符，
     * 直接在调用点迭代会因捕获变量不匹配而编译失败。
     */
    private static <T extends Comparable<T>> void appendValueNames(Property<T> property, List<String> names) {
        for (T value : property.getPossibleValues()) {
            names.add(property.getName(value));
        }
    }
}