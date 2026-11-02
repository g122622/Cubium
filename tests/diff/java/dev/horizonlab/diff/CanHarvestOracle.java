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
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;

/**
 * 方块「每种工具能否正常掉落」的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CanHarvestOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/can_harvest.cpp 同构：
 *
 * <pre>
 *   { "block": "minecraft:stone", "harvest": { "empty": false, "wooden_pickaxe": false, ... } }
 * </pre>
 *
 * <p>判定取原版 {@code Player#hasCorrectToolForDrops}：
 * {@code !state.requiresCorrectToolForDrops() || stack.isCorrectToolForDrops(state)}。
 * 后半段走物品的 {@code Tool} 组件（{@code Tool.Rule} 按方块标签匹配），
 * 而不是「材质是否属于某几类」这类近似。
 */
public final class CanHarvestOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与探针同名的工具表；null 表示空手。 */
    private static final Map<String, String> TOOLS = new LinkedHashMap<>();

    static {
        TOOLS.put("empty", null);
        for (String tier : new String[] {"wooden", "stone", "iron", "golden", "diamond", "netherite"}) {
            for (String kind : new String[] {"pickaxe", "axe", "shovel", "hoe", "sword"}) {
                TOOLS.put(tier + "_" + kind, "minecraft:" + tier + "_" + kind);
            }
        }
        TOOLS.put("shears", "minecraft:shears");
    }

    private CanHarvestOracle() {}

    public static void main(String[] args) throws IOException {
        // 工具规则（Tool.Rule）按方块标签匹配 correct_for_drops，标签只存在于数据包里，
        // 因此必须像服务端一样加载原版数据包，否则 needs_diamond_tool 等标签全为空，
        // 连钻石镐都会判成挖不动远古残骸。
        DataPackRegistries.load();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                names.add(id.toString());
            }
            names.sort(Comparator.naturalOrder());
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
        System.err.println("usage: CanHarvestOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("block", name);

        Block block = BuiltInRegistries.BLOCK.getOptional(Identifier.parse(name)).orElse(null);
        if (block == null) {
            result.addProperty("missing", true);
            return result;
        }

        BlockState state = block.defaultBlockState();
        JsonObject harvest = new JsonObject();
        for (Map.Entry<String, String> entry : TOOLS.entrySet()) {
            boolean canHarvest;
            if (entry.getValue() == null) {
                // 空手：只有不需要工具的方块能采集。
                canHarvest = !state.requiresCorrectToolForDrops();
            } else {
                Item item = BuiltInRegistries.ITEM.getOptional(Identifier.parse(entry.getValue())).orElse(null);
                boolean correct = item != null && new ItemStack(item).isCorrectToolForDrops(state);
                canHarvest = !state.requiresCorrectToolForDrops() || correct;
            }
            harvest.addProperty(entry.getKey(), canHarvest);
        }
        result.add("harvest", harvest);
        return result;
    }
}