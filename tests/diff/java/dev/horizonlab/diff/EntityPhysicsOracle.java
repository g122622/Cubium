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
 * 实体空中移动一个 tick 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityPhysicsOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.EntityPhysicsOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>逐环复刻 {@code LivingEntity.travelInAir} 及其调用链上的
 * {@code getFrictionInfluencedSpeed} / {@code getEffectiveGravity} / {@code getJumpPower}。
 * 原版这些方法要吃完整的实体与世界状态，但公式本身与它们无关，所以这里重排成纯函数；
 * 每一环都与源码同序，便于逐行对照。
 */
public final class EntityPhysicsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityPhysicsOracle() {}

    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    /** 单个场景：十个输入。 */
    private record Scenario(String name, float horizontalSpeed, float slipperiness,
            float verticalVelocity, float gravity, boolean onGround,
            int levitationAmplifier, boolean slowFalling, float jumpBoostPower,
            float blockJumpFactor, float jumpStrength) {}

    private static final float WALK_SPEED = 0.1F;
    private static final float SPRINT_SPEED = 0.13F;
    private static final float FLYING_SPEED = 0.05F;

    private static final float SLIP_DEFAULT = 0.6F;
    private static final float SLIP_ICE = 0.98F;
    private static final float SLIP_BLUE_ICE = 0.989F;
    private static final float SLIP_SLIME = 0.8F;
    private static final float SLIP_AIR = 1.0F;

    private static final float GRAVITY = 0.08F;
    private static final float JUMP_STRENGTH = 0.42F;
    private static final float JUMP_FACTOR_NORMAL = 1.0F;
    private static final float JUMP_FACTOR_HONEY = 0.5F;

    private static List<Scenario> scenarios() {
        List<Scenario> list = new ArrayList<>();
        list.add(new Scenario("ground_walk_default", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("ground_walk_ice", WALK_SPEED, SLIP_ICE, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("ground_walk_blue_ice", WALK_SPEED, SLIP_BLUE_ICE, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("ground_walk_slime", WALK_SPEED, SLIP_SLIME, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("ground_sprint_default", SPRINT_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("ground_flying_default", FLYING_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));

        list.add(new Scenario("air_walk", WALK_SPEED, SLIP_AIR, -0.1F, GRAVITY, false, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("air_sprint", SPRINT_SPEED, SLIP_AIR, -0.1F, GRAVITY, false, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));

        list.add(new Scenario("fall_from_zero", WALK_SPEED, SLIP_AIR, 0.0F, GRAVITY, false, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("fall_rising", WALK_SPEED, SLIP_AIR, 0.42F, GRAVITY, false, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("fall_terminal", WALK_SPEED, SLIP_AIR, -3.0F, GRAVITY, false, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));

        list.add(new Scenario("slowfall_falling", WALK_SPEED, SLIP_AIR, -0.1F, GRAVITY, false, -1, true, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("slowfall_rising", WALK_SPEED, SLIP_AIR, 0.3F, GRAVITY, false, -1, true, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("slowfall_apex", WALK_SPEED, SLIP_AIR, 0.0F, GRAVITY, false, -1, true, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));

        list.add(new Scenario("levitation_0_falling", WALK_SPEED, SLIP_AIR, -0.5F, GRAVITY, false, 0, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("levitation_1_falling", WALK_SPEED, SLIP_AIR, -0.5F, GRAVITY, false, 1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("levitation_0_rising", WALK_SPEED, SLIP_AIR, 0.5F, GRAVITY, false, 0, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("levitation_2_rising", WALK_SPEED, SLIP_AIR, 1.0F, GRAVITY, false, 2, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));

        list.add(new Scenario("jump_plain", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("jump_honey", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_HONEY, JUMP_STRENGTH));
        list.add(new Scenario("jump_boost_1", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.1F, JUMP_FACTOR_NORMAL, JUMP_STRENGTH));
        list.add(new Scenario("jump_boost_2_honey", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.2F, JUMP_FACTOR_HONEY, JUMP_STRENGTH));
        list.add(new Scenario("jump_custom_strength", WALK_SPEED, SLIP_DEFAULT, 0.0F, GRAVITY, true, -1, false, 0.0F, JUMP_FACTOR_NORMAL, 0.5F));
        return list;
    }

    /**
     * 复刻 travelInAir 的一个 tick。
     *
     * @return {moveFactor, verticalVelocity, jumpPower}
     */
    private static float[] travelInAir(Scenario s) {
        // getFrictionInfluencedSpeed：地面走滑度公式（真除法），空中走固定值。
        float friction = s.onGround() ? s.slipperiness() : 1.0F;
        float moveFactor;
        if (s.onGround()) {
            moveFactor = s.horizontalSpeed() * 0.21600002F / (friction * friction * friction);
        } else {
            moveFactor = 0.02F;
        }

        // 竖直速度：悬浮优先于重力。
        float v = s.verticalVelocity();
        if (s.levitationAmplifier() >= 0) {
            float target = 0.05F * (s.levitationAmplifier() + 1);
            v += (target - v) * 0.2F;
        } else {
            // getEffectiveGravity：缓降只在下落时把重力压到 0.01。
            float gravity = s.gravity();
            if (s.slowFalling() && s.verticalVelocity() <= 0.0F) {
                gravity = Math.min(gravity, 0.01F);
            }
            v -= gravity;
        }

        // 原版把 `d0 * f2` 写在悬浮分支之后，因此悬浮也照乘 0.98。
        v *= 0.98F;

        // getJumpPower：乘以方块跳跃因子，再加上跳跃提升（相加而非相乘）。
        float jumpPower = s.jumpStrength() * s.blockJumpFactor() + s.jumpBoostPower();

        return new float[] {moveFactor, v, jumpPower};
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

        throw new IllegalArgumentException("usage: EntityPhysicsOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();
        for (Scenario scenario : scenarios()) {
            if (requested.equals(scenario.name())) {
                float[] result = travelInAir(scenario);
                JsonObject output = new JsonObject();
                output.addProperty("unit", requested);
                output.addProperty("moveFactor", floatBits(result[0]));
                output.addProperty("verticalVelocity", floatBits(result[1]));
                output.addProperty("jumpPower", floatBits(result[2]));
                return GSON.toJson(output);
            }
        }
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}