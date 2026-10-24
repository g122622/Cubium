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
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块状态最简名称（命令语法里的 {@code minecraft:oak_stairs[facing=east]}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockStateNamesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_state_names.cpp 同构：
 *
 * <pre>
 *   { "block": "minecraft:oak_stairs", "default": "minecraft:oak_stairs[facing=east,half=bottom,shape=straight,waterlogged=false]",
 *     "states": ["minecraft:oak_stairs[facing=east,half=bottom,shape=straight,waterlogged=false]", ...] }
 * </pre>
 *
 * <p>两侧都对状态列表排序：状态的枚举顺序不是协议的一部分，名字的集合才是。
 *
 * <p>{@link BlockState#toString()} 会带上 {@code Block{id}} 外壳，那是调试输出、不是命令
 * 语法；这里按原版 {@code BlockState.getSerializedName()} 的形式重新拼
 * （{@code id[prop=value,...]}，属性按名字排序），与指令解析器接受的写法一致。
 */
public final class BlockStateNamesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockStateNamesOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                names.add(id.toString());
            }
            Collections.sort(names);
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockStateNamesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("block").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** {@code minecraft:oak_stairs[facing=east,half=bottom]}。 */
    private static String name(BlockState state, String blockId) {
        if (state.getProperties().isEmpty()) {
            return blockId;
        }
        List<String> pairs = new ArrayList<>();
        for (var property : state.getProperties()) {
            pairs.add(property.getName() + "=" + nameOf(property, state.getValue(property)));
        }
        Collections.sort(pairs);
        return blockId + "[" + String.join(",", pairs) + "]";
    }

    /** 属性值的命令语法文本（{@code component.toString()} 与 {@code Property.getName(value)} 等价）。 */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static String nameOf(Property property, Comparable value) {
        return property.getName(value);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("block", name);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(name)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        String id = Identifier.parse(name).toString();
        List<String> states = new ArrayList<>();
        for (BlockState state : block.getStateDefinition().getPossibleStates()) {
            states.add(name(state, id));
        }
        Collections.sort(states);

        JsonArray array = new JsonArray();
        for (String state : states) {
            array.add(state);
        }
        result.addProperty("default", name(block.defaultBlockState(), id));
        result.add("states", array);
        return result;
    }
}