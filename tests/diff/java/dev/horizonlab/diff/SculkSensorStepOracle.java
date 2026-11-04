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

/**
 * 幽匿感测体被踩踏时是否发出 STEP 振动的原版 oracle（{@code SculkSensorBlock#stepOn}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.SculkSensorStepOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code inactive:normal}（相位 : 踩踏者）。
 *
 * <p>原版 stepOn：
 *
 * <pre>
 *   if (!isClientSide &amp;&amp; canActivate(state) &amp;&amp; entity.getType() != EntityType.WARDEN &amp;&amp; ...)
 *       listener.forceScheduleVibration(STEP, context, entity.position());
 * </pre>
 *
 * <p>三个前置条件缺一不可：
 *
 * <ul>
 *   <li>相位为 Inactive —— 活跃/冷却期间不重复接收，否则站上去不走也会一直触发
 *   <li>踩踏者不是监守者 —— 监守者与感测体同源，踩上去不该自激
 *   <li>踩踏者不在潜行 —— 潜行不产生振动，这是「悄悄绕过感测体」的玩法基础
 * </ul>
 */
public final class SculkSensorStepOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] PHASES = {"inactive", "active", "cooldown"};
    private static final String[] WALKERS = {"normal", "sneaking", "warden"};

    private SculkSensorStepOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String phase : PHASES) {
                for (String walker : WALKERS) {
                    units.add(phase + ":" + walker);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: SculkSensorStepOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        if (parts.length != 2) {
            out.addProperty("missing", true);
            return out;
        }
        String phase = parts[0];
        String walker = parts[1];
        boolean known = false;
        for (String candidate : PHASES) {
            if (candidate.equals(phase)) {
                known = true;
                break;
            }
        }
        boolean knownWalker = false;
        for (String candidate : WALKERS) {
            if (candidate.equals(walker)) {
                knownWalker = true;
                break;
            }
        }
        if (!known || !knownWalker) {
            out.addProperty("missing", true);
            return out;
        }

        // 三个条件同时满足才发 STEP
        boolean phaseOk = phase.equals("inactive");
        boolean notWarden = !walker.equals("warden");
        boolean notSneaking = !walker.equals("sneaking");
        out.addProperty("stepEvent", phaseOk && notWarden && notSneaking);
        return out;
    }
}
