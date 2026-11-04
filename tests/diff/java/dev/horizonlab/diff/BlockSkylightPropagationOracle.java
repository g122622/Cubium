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
 * 方块状态是否向下透传天空光的原版 oracle
 * （{@code BlockBehaviour#propagatesSkylightDown}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockSkylightPropagationOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版默认实现：
 *
 * <pre>
 *   return !isShapeFullBlock(state.getShape(...)) && state.getFluidState().isEmpty();
 * </pre>
 *
 * <p>两个容易写错的点：
 *
 * <ol>
 *   <li>必须同时检查「形状」与「流体」。只判形状会让水透光（海底一片明亮）；
 *       只判流体会让台阶、栅栏、楼梯全部挡光（白天进屋一片漆黑）
 *   <li>判据是「形状非整格」而不是「碰撞形状为空」。树叶、草丛、栅栏、告示牌
 *       的碰撞形状都不是整格，它们都应当透光
 * </ol>
 */
public final class BlockSkylightPropagationOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockSkylightPropagationOracle() {}

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
        System.err.println(
            "usage: BlockSkylightPropagationOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
            state = applyProperty(state, pair.substring(0, eq), pair.substring(eq + 1));
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

        // propagatesSkylightDown 在 BlockBehaviour 里是 protected，BlockStateBase 上有公开的
        // 无参访问器 getBlockStateBase().propagatesSkylightDown()；这里直接用 Block 的公开重载。
        out.addProperty("propagates", state.propagatesSkylightDown());
        return out;
    }
}
