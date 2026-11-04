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
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 用剪刀拆绊线时解除武装的原版 oracle（{@code TripWireBlock#playerWillDestroy}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.TripwireDisarmOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是主手物品标签（empty / shears / stick）。
 *
 * <p>原版：
 *
 * <pre>
 *   if (!isClientSide &amp;&amp; 主手非空 &amp;&amp; 主手是剪刀) {
 *       setBlock(pos, state.setValue(DISARMED, true), 260);
 *       gameEvent(GameEvent.SHEAR, pos, ...);
 *   }
 * </pre>
 *
 * <p>先置 DISARMED 再移除：两端钩子重新计算状态时看到「已解除」的绊线，
 * 走拆除路径而不是触发路径。不置的话用剪刀拆陷阱反而会把它引爆一次 ——
 * 这正好是剪刀存在的意义（安全拆除）的反面。
 */
public final class TripwireDisarmOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] HELD_LABELS = {"empty", "shears", "stick"};

    private TripwireDisarmOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String label : HELD_LABELS) {
                units.add(label);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: TripwireDisarmOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        boolean known = false;
        for (String label : HELD_LABELS) {
            if (label.equals(unit)) {
                known = true;
                break;
            }
        }
        if (!known) {
            out.addProperty("missing", true);
            return out;
        }

        // 只有剪刀会置 DISARMED 并发事件
        boolean shears = unit.equals("shears");
        out.addProperty("disarmed", shears);

        JsonArray events = new JsonArray();
        if (shears) {
            events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.SHEAR.value()).toString());
        }
        out.add("gameEvents", events);
        return out;
    }
}
