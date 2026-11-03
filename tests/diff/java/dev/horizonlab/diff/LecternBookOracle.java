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
 * 讲台放书/取书时的状态复位与游戏事件的原版 oracle（{@code LecternBlock#resetBookState}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.LecternBookOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code true:true:false}（初始 HAS_BOOK:初始 POWERED:目标 hasBook）。
 *
 * <p>原版 {@code resetBookState}：
 *
 * <pre>
 *   BlockState blockstate = state.setValue(POWERED, false).setValue(HAS_BOOK, target);
 *   setBlock(pos, blockstate, 3);
 *   gameEvent(GameEvent.BLOCK_CHANGE, pos, ...);
 * </pre>
 *
 * <p>{@code POWERED} <b>一律复位</b>：讲台翻页时会拉高一次当作比较器脉冲，
 * 放书/取书必须清掉，否则取走书之后下游比较器一直读到不该存在的信号。
 */
public final class LecternBookOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private LecternBookOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String hasBook : new String[] {"false", "true"}) {
                for (String powered : new String[] {"false", "true"}) {
                    for (String target : new String[] {"false", "true"}) {
                        units.add(hasBook + ":" + powered + ":" + target);
                    }
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LecternBookOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        String[] parts = unit.split(":");
        if (parts.length != 3) {
            out.addProperty("missing", true);
            return out;
        }
        boolean targetHasBook = parts[2].equals("true");

        // resetBookState：POWERED 一律复位，HAS_BOOK 置为目标值
        out.addProperty("hasBook", targetHasBook);
        out.addProperty("powered", false);

        JsonArray events = new JsonArray();
        events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_CHANGE.value()).toString());
        out.add("gameEvents", events);
        return out;
    }
}
