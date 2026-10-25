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
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块状态全局 ID 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockStateIdsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/block_state_ids.cpp 同构：
 *
 * <pre>
 *   { "block": "minecraft:oak_stairs",
 *     "ids": { "minecraft:oak_stairs[facing=east,half=bottom,...]": 1234, ... } }
 * </pre>
 *
 * <p>编号来自 {@code Block.BLOCK_STATE_REGISTRY.getId(state)}——原版网络协议与存档里
 * 使用的全局 id；状态名按 {@code id[prop=value,...]}]}（属性按名字排序）拼，与命令语法一致。
 */
public final class BlockStateIdsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockStateIdsOracle() {}

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
        System.err.println("usage: BlockStateIdsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("block").getAsString();
            text.append(GSON.toJson(describe(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject describe(String blockId) {
        Identifier id = Identifier.parse(blockId);
        JsonObject result = new JsonObject();
        result.addProperty("block", blockId);

        if (!BuiltInRegistries.BLOCK.containsKey(id)) {
            result.addProperty("missing", true);
            return result;
        }

        Block block = BuiltInRegistries.BLOCK.getValue(id);
        Map<String, Integer> ids = new LinkedHashMap<>();
        List<BlockState> states = new ArrayList<>(block.getStateDefinition().getPossibleStates());
        states.sort((a, b) -> serializedName(a).compareTo(serializedName(b)));
        for (BlockState state : states) {
            ids.put(serializedName(state), Block.BLOCK_STATE_REGISTRY.getId(state));
        }
        result.add("ids", GSON.toJsonTree(ids));
        return result;
    }

    /** 原版 {@code BlockState.getSerializedName()} 的形式：id[prop=value,...]，属性按名字排序。 */
    private static String serializedName(BlockState state) {
        Identifier id = BuiltInRegistries.BLOCK.getKey(state.getBlock());
        List<String> parts = new ArrayList<>();
        for (Property<?> property : state.getProperties()) {
            parts.add(property.getName() + "=" + valueName(state, property));
        }
        Collections.sort(parts);
        if (parts.isEmpty()) {
            return id.toString();
        }
        return id + "[" + String.join(",", parts) + "]";
    }

    private static <T extends Comparable<T>> String valueName(BlockState state, Property<T> property) {
        return property.getName(state.getValue(property));
    }
}