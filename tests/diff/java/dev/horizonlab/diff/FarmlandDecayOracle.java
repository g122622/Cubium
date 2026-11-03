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
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 耕地退化时的方块替换与游戏事件的原版 oracle（{@code FarmBlock#turnToDirt}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.FarmlandDecayOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是耕地的 {@code MOISTURE}（0..7）。
 *
 * <p>原版 {@code turnToDirt} 两步：
 *
 * <pre>
 *   setBlockAndUpdate(pos, Blocks.DIRT.defaultBlockState());
 *   gameEvent(GameEvent.BLOCK_CHANGE, pos, ...);
 * </pre>
 *
 * <p>换方块与发事件是两件事：幽匿感测体只认 {@code gameEvent}，不发就完全感知不到
 * 耕地被踩坏。退化与湿度无关 —— 干湿都会退化。
 */
public final class FarmlandDecayOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private FarmlandDecayOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int moisture = 0; moisture <= 7; moisture++) {
                units.add(Integer.toString(moisture));
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: FarmlandDecayOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int moisture;
        try {
            moisture = Integer.parseInt(unit);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (moisture < 0 || moisture > 7) {
            out.addProperty("missing", true);
            return out;
        }

        // 退化目标恒为泥土，与湿度无关
        out.addProperty("block", BuiltInRegistries.BLOCK.getKey(Blocks.DIRT).toString());

        JsonArray events = new JsonArray();
        events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_CHANGE.value()).toString());
        out.add("gameEvents", events);
        return out;
    }
}
