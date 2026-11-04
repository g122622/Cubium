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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块状态是否可被替换的原版 oracle（{@code BlockStateBase#canBeReplaced}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockReplaceableOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>它描述「这一格现在算不算空」：直接放置、流体流动、世界生成、掉落方块都靠它。
 *
 * <p>注意与「能否被流体替换」（{@code canBeReplaced(Fluid)}）不是一回事：后者对非固体
 * 方块放宽（门、告示牌虽然 replaceable=false，但水能流进去），不能混用同一个判定。
 */
public final class BlockReplaceableOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockReplaceableOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                Block block = BuiltInRegistries.BLOCK.getValue(id);
                for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                    units.add(unitName(state));
                }
            }
            java.util.Collections.sort(units);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockReplaceableOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static String unitName(BlockState state) {
        StringBuilder sb = new StringBuilder(BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString());
        var values = state.getValues();
        if (values.isEmpty()) {
            return sb.toString();
        }
        java.util.TreeMap<String, String> sorted = new java.util.TreeMap<>();
        for (var entry : values.entrySet()) {
            Property<?> property = entry.getKey();
            sorted.put(property.getName(), nameOf(property, entry.getValue()));
        }
        sb.append('[');
        boolean first = true;
        for (var entry : sorted.entrySet()) {
            if (!first) {
                sb.append(',');
            }
            first = false;
            sb.append(entry.getKey()).append('=').append(entry.getValue());
        }
        sb.append(']');
        return sb.toString();
    }

    private static <T extends Comparable<T>> String nameOf(Property<T> property, Comparable<?> value) {
        @SuppressWarnings("unchecked")
        T typed = (T) value;
        return property.getName(typed);
    }

    private static BlockState parseState(String unit) {
        int open = unit.indexOf('[');
        String idText = open < 0 ? unit : unit.substring(0, open);
        Identifier id = Identifier.tryParse(idText);
        if (id == null || !BuiltInRegistries.BLOCK.containsKey(id)) {
            return null;
        }
        BlockState state = BuiltInRegistries.BLOCK.getValue(id).defaultBlockState();
        if (open < 0) {
            return state;
        }
        int close = unit.lastIndexOf(']');
        if (close < open) {
            return null;
        }
        String body = unit.substring(open + 1, close);
        if (body.isEmpty()) {
            return state;
        }
        for (String pair : body.split(",")) {
            int eq = pair.indexOf('=');
            if (eq < 0) {
                return null;
            }
            String key = pair.substring(0, eq);
            String value = pair.substring(eq + 1);
            state = applyProperty(state, key, value);
            if (state == null) {
                return null;
            }
        }
        return state;
    }

    private static <T extends Comparable<T>> BlockState applyPropertyTyped(
        BlockState state, Property<T> property, String valueName) {
        return property.getValue(valueName).map(v -> state.setValue(property, v)).orElse(null);
    }

    private static BlockState applyProperty(BlockState state, String key, String value) {
        Property<?> property = state.getBlock().getStateDefinition().getProperty(key);
        if (property == null) {
            return null;
        }
        return applyPropertyTyped(state, property, value);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        BlockState state = parseState(unit);
        if (state == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("replaceable", state.canBeReplaced());
        return out;
    }
}
