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
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.level.EmptyBlockGetter;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;
import net.minecraft.world.level.pathfinder.PathComputationType;

/**
 * 方块状态在三种寻路类型下是否可通行的原版 oracle（{@code BlockBehaviour#isPathfindable}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockPathfindableOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code minecraft:oak_slab[type=bottom]#land}。
 *
 * <p>原版默认判据：
 *
 * <pre>
 *   LAND  -> 碰撞形状不完整即可通行
 *   WATER -> 只认水（getFluidState().is(FluidTags.WATER)）
 *   AIR   -> 碰撞形状不完整即可通行
 * </pre>
 *
 * <p>两处最容易写错：判据是「不完整」而不是「为空」（台阶、楼梯能走上去）；
 * WATER 类型不看形状只看是不是水。
 */
public final class BlockPathfindableOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] TYPES = {"land", "water", "air"};

    private BlockPathfindableOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        // 流体标签必须加载：LiquidBlock#isPathfindable 依赖 "是不是岩浆" 的判定，
        // 标签未加载时 is(FluidTags.LAVA) 恒为 false，岩浆会被误判为可通行。
        DataPackRegistries.load();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Identifier id : BuiltInRegistries.BLOCK.keySet()) {
                Block block = BuiltInRegistries.BLOCK.getValue(id);
                for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                    String base = unitName(state);
                    for (String type : TYPES) {
                        units.add(base + "#" + type);
                    }
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
        System.err.println("usage: BlockPathfindableOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static PathComputationType typeOf(String name) {
        if (name.equals("water")) {
            return PathComputationType.WATER;
        }
        if (name.equals("air")) {
            return PathComputationType.AIR;
        }
        return PathComputationType.LAND;
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        int sep = unit.lastIndexOf('#');
        if (sep < 0) {
            out.addProperty("missing", true);
            return out;
        }
        String stateName = unit.substring(0, sep);
        String typeName = unit.substring(sep + 1);
        boolean knownType = false;
        for (String t : TYPES) {
            if (t.equals(typeName)) {
                knownType = true;
                break;
            }
        }
        if (!knownType) {
            out.addProperty("missing", true);
            return out;
        }

        BlockState state = parseState(stateName);
        if (state == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("pathfindable", state.isPathfindable(typeOf(typeName)));
        return out;
    }
}
