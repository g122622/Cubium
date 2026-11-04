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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.EmptyBlockGetter;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.Property;

/**
 * 方块状态「挡视线 / 是否窒息」的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockViewBlockingOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.BlockViewBlockingOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接调 {@code BlockState#isViewBlocking} / {@code #isSuffocating}，两个判定的输入都只取
 * 状态自身，因此用 {@code EmptyBlockGetter.INSTANCE} + {@code BlockPos.ZERO} 求值即可。
 */
public final class BlockViewBlockingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();
    private static final BlockPos ORIGIN = BlockPos.ZERO;
    private static Map<String, BlockState> INDEX;

    private BlockViewBlockingOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Block block : BuiltInRegistries.BLOCK) {
                for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                    units.add(describe(state));
                }
            }
            units.sort(String::compareTo);
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockViewBlockingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        BlockState state = findState(unit);
        if (state == null) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("viewBlocking", state.isViewBlocking(EmptyBlockGetter.INSTANCE, ORIGIN));
        out.addProperty("suffocating", state.isSuffocating(EmptyBlockGetter.INSTANCE, ORIGIN));
        return out;
    }

    /**
     * 状态的字符串表示：{@code namespace:path[key=value,...]}，属性按名字排序。
     *
     * <p>属性顺序必须与 C++ 侧一致——两侧的枚举顺序不同，不排序会把同一状态配成
     * 两个单元，表现为一边有一边没有，那不是差分。
     */
    private static String describe(BlockState state) {
        List<String> attrs = new ArrayList<>();
        for (Property<?> property : state.getProperties()) {
            attrs.add(property.getName() + "=" + getName(state, property));
        }
        attrs.sort(String::compareTo);

        StringBuilder sb = new StringBuilder();
        sb.append(BuiltInRegistries.BLOCK.getKey(state.getBlock()));
        if (!attrs.isEmpty()) {
            sb.append('[');
            sb.append(String.join(",", attrs));
            sb.append(']');
        }
        return sb.toString();
    }

    private static <T extends Comparable<T>> String getName(BlockState state, Property<T> property) {
        return property.getName(state.getValue(property));
    }

    /** 反查：把单元字符串还原成状态。 */
    private static BlockState findState(String unit) {
        if (INDEX == null) {
            INDEX = new HashMap<>();
            for (Block block : BuiltInRegistries.BLOCK) {
                for (BlockState state : block.getStateDefinition().getPossibleStates()) {
                    INDEX.put(describe(state), state);
                }
            }
        }
        return INDEX.get(unit);
    }
}