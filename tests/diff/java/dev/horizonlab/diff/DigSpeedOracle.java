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
 * 挖掘速度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DigSpeedOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.DigSpeedOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>逐环复刻 {@code net.minecraft.world.entity.player.Player#getDestroySpeed}。原版那个方法
 * 需要一个真实玩家对象，但公式本身与玩家无关，所以这里把它重排成纯函数；每一环都与源码
 * 同序，便于逐行对照。场景表必须与探针侧 {@code dig_speed.cpp} 逐项一致（名称、顺序、
 * 每个输入都相同），否则比的是两张不同的表。
 */
public final class DigSpeedOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DigSpeedOracle() {}

    /** 与探针侧一致的浮点位模式输出。 */
    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    /** 挖掘疲劳各档的精确常量（原版是写死的四档 switch，不是几何级数）。 */
    private static float fatigueMultiplier(int amplifier) {
        return switch (amplifier) {
            case 0 -> 0.3F;
            case 1 -> 0.09F;
            case 2 -> 0.0027F;
            default -> 8.1E-4F;
        };
    }

    /** 对应 Player.getDestroySpeed 的七环。 */
    private static float digSpeed(float toolSpeed, float miningEfficiency, int hasteAmplifier,
            int conduitAmplifier, int fatigueAmplifier, float blockBreakSpeed,
            float submergedMiningSpeed, boolean eyesInWater, boolean onGround) {
        float speed = toolSpeed;
        if (speed > 1.0F) {
            speed += miningEfficiency;
        }

        // MobEffectUtil.hasDigSpeed / getDigSpeedAmplification：两者取较高等级，不叠加。
        int digAmplifier = Math.max(hasteAmplifier, conduitAmplifier);
        if (digAmplifier >= 0) {
            speed *= 1.0F + (digAmplifier + 1) * 0.2F;
        }

        if (fatigueAmplifier >= 0) {
            speed *= fatigueMultiplier(fatigueAmplifier);
        }

        speed *= blockBreakSpeed;

        if (eyesInWater) {
            speed *= submergedMiningSpeed;
        }

        if (!onGround) {
            speed /= 5.0F;
        }

        return speed;
    }

    /** 单个场景：名称与九个输入。 */
    private record Scenario(String name, float toolSpeed, float miningEfficiency,
            int hasteAmplifier, int conduitAmplifier, int fatigueAmplifier,
            float blockBreakSpeed, float submergedMiningSpeed,
            boolean eyesInWater, boolean onGround) {}

    private static final float HAND = 1.0F;
    private static final float PICKAXE = 8.0F;
    private static final float SLOW_TOOL = 0.2F;
    private static final float BREAK1 = 1.0F;
    private static final float BREAK2 = 2.0F;
    private static final float SUBMERGED = 0.2F;
    private static final float AQUA_AFFINITY = 1.0F;

    private static List<Scenario> scenarios() {
        List<Scenario> list = new ArrayList<>();
        list.add(new Scenario("hand_air", HAND, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_air", PICKAXE, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("slowtool_air", SLOW_TOOL, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("hand_eff2", HAND, 2.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("hand_eff26", HAND, 26.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("slowtool_eff5", SLOW_TOOL, 5.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_eff2", PICKAXE, 2.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_eff5", PICKAXE, 5.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_eff26", PICKAXE, 26.0F, -1, -1, -1, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_haste0", PICKAXE, 0.0F, 0, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_haste1", PICKAXE, 0.0F, 1, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_haste2", PICKAXE, 0.0F, 2, -1, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_haste9", PICKAXE, 0.0F, 9, -1, -1, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_conduit0", PICKAXE, 0.0F, -1, 0, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_conduit2", PICKAXE, 0.0F, -1, 2, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_haste1_conduit2", PICKAXE, 0.0F, 1, 2, -1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_haste3_conduit1", PICKAXE, 0.0F, 3, 1, -1, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_fatigue0", PICKAXE, 0.0F, -1, -1, 0, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_fatigue1", PICKAXE, 0.0F, -1, -1, 1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_fatigue2", PICKAXE, 0.0F, -1, -1, 2, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_fatigue3", PICKAXE, 0.0F, -1, -1, 3, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_fatigue7", PICKAXE, 0.0F, -1, -1, 7, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_haste2_fatigue1", PICKAXE, 0.0F, 2, -1, 1, BREAK1, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_eff5_haste2_fatigue2", PICKAXE, 5.0F, 2, -1, 2, BREAK1, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_break2", PICKAXE, 0.0F, -1, -1, -1, BREAK2, SUBMERGED, false, true));
        list.add(new Scenario("pickaxe_break2_eff5_haste1", PICKAXE, 5.0F, 1, -1, -1, BREAK2, SUBMERGED, false, true));

        list.add(new Scenario("pickaxe_wet", PICKAXE, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, true, true));
        list.add(new Scenario("pickaxe_wet_aqua", PICKAXE, 0.0F, -1, -1, -1, BREAK1, AQUA_AFFINITY, true, true));
        list.add(new Scenario("hand_wet", HAND, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, true, true));

        list.add(new Scenario("pickaxe_falling", PICKAXE, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, false, false));
        list.add(new Scenario("pickaxe_wet_falling", PICKAXE, 0.0F, -1, -1, -1, BREAK1, SUBMERGED, true, false));

        list.add(new Scenario("all_factors", PICKAXE, 5.0F, 2, 1, 1, BREAK2, AQUA_AFFINITY, true, false));
        return list;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Scenario scenario : scenarios()) {
                units.add(scenario.name());
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: DigSpeedOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();
        for (Scenario scenario : scenarios()) {
            if (requested.equals(scenario.name())) {
                float speed = digSpeed(scenario.toolSpeed(), scenario.miningEfficiency(),
                        scenario.hasteAmplifier(), scenario.conduitAmplifier(),
                        scenario.fatigueAmplifier(), scenario.blockBreakSpeed(),
                        scenario.submergedMiningSpeed(), scenario.eyesInWater(), scenario.onGround());
                JsonObject output = new JsonObject();
                output.addProperty("unit", requested);
                output.addProperty("speed", floatBits(speed));
                return GSON.toJson(output);
            }
        }
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}