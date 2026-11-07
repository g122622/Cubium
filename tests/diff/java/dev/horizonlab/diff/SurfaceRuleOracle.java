package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.level.levelgen.SurfaceRules;
import net.minecraft.world.level.levelgen.VerticalAnchor;
import net.minecraft.world.level.levelgen.placement.CaveSurface;

/**
 * 地表规则的三种条件判定：水面、石头深度、Y 高度。
 *
 * <pre>
 *   java dev.horizonlab.diff.SurfaceRuleOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>{@code SurfaceRules.Context} 是 {@code protected static final class}，构造函数要求
 * {@code SurfaceSystem}、{@code ChunkAccess}、{@code NoiseChunk} 这些重量级依赖；但条件
 * {@code compute()} 只读 {@code blockY} / {@code stoneDepthAbove} / {@code stoneDepthBelow} /
 * {@code waterHeight} / {@code surfaceDepth} / {@code surfaceSecondary} 几个标量。
 *
 * <p>因此这里用反射构造一个只填了 {@code WorldGenerationContext} 的 Context（YCondition 要它
 * 解析锚点），其余字段直接写值。{@code WorldGenerationContext} 本身也跳过构造——它的构造函数
 * 会解引用 ChunkGenerator 与 LevelHeightAccessor，而这里只需要两个整数。
 */
public final class SurfaceRuleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private SurfaceRuleOracle() {}

    /** 主世界高度范围，与探针侧一致。 */
    private static final int WORLD_MIN_Y = -64;
    private static final int WORLD_HEIGHT = 384;

    private static Constructor<?> contextCtor;
    private static Class<?> worldGenContextClass;
    private static Field minYField;
    private static Field genDepthField;
    private static Field blockYField;
    private static Field waterHeightField;
    private static Field stoneDepthAboveField;
    private static Field stoneDepthBelowField;
    private static Field surfaceDepthField;
    private static Field surfaceSecondaryField;
    private static Field lastSurfaceDepth2UpdateField;
    private static Field lastUpdateXZField;
    private static Method applyMethod;
    private static Method testMethodCache;
    private static Object unsafeInstance;
    private static Method allocateInstanceMethod;

    private static void initReflection() throws Exception {
        if (contextCtor != null) {
            return;
        }
        Class<?> contextClass = Class.forName("net.minecraft.world.level.levelgen.SurfaceRules$Context");
        contextCtor = contextClass.getDeclaredConstructor(
            Class.forName("net.minecraft.world.level.levelgen.SurfaceSystem"),
            Class.forName("net.minecraft.world.level.levelgen.RandomState"),
            net.minecraft.world.level.chunk.ChunkAccess.class,
            Class.forName("net.minecraft.world.level.levelgen.NoiseChunk"),
            java.util.function.Function.class,
            net.minecraft.core.Registry.class,
            Class.forName("net.minecraft.world.level.levelgen.WorldGenerationContext"));
        contextCtor.setAccessible(true);

        blockYField = field(contextClass, "blockY");
        waterHeightField = field(contextClass, "waterHeight");
        stoneDepthAboveField = field(contextClass, "stoneDepthAbove");
        stoneDepthBelowField = field(contextClass, "stoneDepthBelow");
        surfaceDepthField = field(contextClass, "surfaceDepth");
        surfaceSecondaryField = field(contextClass, "surfaceSecondary");
        lastSurfaceDepth2UpdateField = field(contextClass, "lastSurfaceDepth2Update");
        lastUpdateXZField = field(contextClass, "lastUpdateXZ");

        // WorldGenerationContext 的构造函数会解引用 ChunkGenerator 与 LevelHeightAccessor，
        // 而这里只需要它承载两个整数：跳过构造直接写字段。
        worldGenContextClass = Class.forName("net.minecraft.world.level.levelgen.WorldGenerationContext");
        minYField = field(worldGenContextClass, "minY");
        genDepthField = field(worldGenContextClass, "height");

        // ConditionSource 实现了 Function<Context, Condition>，apply 就是 Function.apply(Object)。
        applyMethod = java.util.function.Function.class.getMethod("apply", Object.class);
    }

    /** 分配一个不经构造函数的实例。 */
    private static Object allocateInstance(Class<?> type) throws Exception {
        if (unsafeInstance == null) {
            Class<?> unsafeClass = Class.forName("sun.misc.Unsafe");
            Field theUnsafe = unsafeClass.getDeclaredField("theUnsafe");
            theUnsafe.setAccessible(true);
            unsafeInstance = theUnsafe.get(null);
            allocateInstanceMethod = unsafeClass.getMethod("allocateInstance", Class.class);
        }
        return allocateInstanceMethod.invoke(unsafeInstance, type);
    }

    /** Condition.test() 是 private 接口方法，走反射。 */
    private static Method testMethod() throws Exception {
        if (testMethodCache == null) {
            testMethodCache = Class.forName("net.minecraft.world.level.levelgen.SurfaceRules$Condition")
                .getDeclaredMethod("test");
            testMethodCache.setAccessible(true);
        }
        return testMethodCache;
    }

    private static Field field(Class<?> owner, String name) throws Exception {
        Field found = owner.getDeclaredField(name);
        found.setAccessible(true);
        return found;
    }

    /** 构造只填了 WorldGenerationContext 的 Context。 */
    private static Object newContext() throws Exception {
        Object worldGenerationContext = allocateInstance(worldGenContextClass);
        minYField.setInt(worldGenerationContext, WORLD_MIN_Y);
        genDepthField.setInt(worldGenerationContext, WORLD_HEIGHT);
        return contextCtor.newInstance(null, null, null, null, null, null, worldGenerationContext);
    }

    private static List<String> split(String text) {
        List<String> parts = new ArrayList<>();
        int start = 0;
        while (true) {
            int comma = text.indexOf(',', start);
            if (comma < 0) {
                parts.add(text.substring(start));
                return parts;
            }
            parts.add(text.substring(start, comma));
            start = comma + 1;
        }
    }

    public static void main(String[] args) throws Exception {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            writeUnits(text);
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            initReflection();
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

        throw new IllegalArgumentException("usage: SurfaceRuleOracle list <units.txt> | run <in> <out>");
    }

    private static void writeUnits(StringBuilder out) {
        for (int waterHeight : new int[] {Integer.MIN_VALUE, 62, 63, 64}) {
            for (int blockY : new int[] {60, 61, 62, 63, 64, 65}) {
                for (int surfaceDepth : new int[] {0, 1, 3}) {
                    for (int addStoneDepth = 0; addStoneDepth <= 1; addStoneDepth++) {
                        out.append(String.format("water|0,1,%d,%d,%d,%d,%d%n", addStoneDepth, blockY,
                            addStoneDepth == 1 ? 2 : 0, waterHeight, surfaceDepth));
                    }
                }
            }
        }
        for (int offset : new int[] {-2, 0, 5}) {
            for (int multiplier : new int[] {-2, 0, 1, 4}) {
                out.append(String.format("water|%d,%d,1,63,0,62,3%n", offset, multiplier));
            }
        }

        for (String surface : new String[] {"floor", "ceiling"}) {
            for (int stoneDepth : new int[] {0, 1, 2, 3, 5}) {
                for (int offset : new int[] {-1, 0, 1}) {
                    for (int secondaryRange : new int[] {0, 4}) {
                        for (int addSurfaceDepth = 0; addSurfaceDepth <= 1; addSurfaceDepth++) {
                            for (String secondary : new String[] {"-1.000", "0.000", "0.500", "1.000"}) {
                                out.append(String.format("stonedepth|%d,%d,%d,%s,%d,2,%s%n", offset,
                                    addSurfaceDepth, secondaryRange, surface, stoneDepth, secondary));
                            }
                        }
                    }
                }
            }
        }

        for (String anchor : new String[] {"above_bottom", "below_top"}) {
            for (int anchorValue : new int[] {0, 32, 63}) {
                for (int multiplier : new int[] {-1, 0, 1, 3}) {
                    for (int addStoneDepth = 0; addStoneDepth <= 1; addStoneDepth++) {
                        out.append(String.format("y|%s,%d,%d,%d,70,%d,1%n", anchor, anchorValue, multiplier,
                            addStoneDepth, addStoneDepth == 1 ? 2 : 0));
                    }
                }
            }
        }
    }

    private static String runLine(String line) throws Exception {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int bar = requested.indexOf('|');
        if (bar < 0) {
            return missing(requested);
        }
        String kind = requested.substring(0, bar);
        List<String> args = split(requested.substring(bar + 1));
        if (args.size() < 7) {
            return missing(requested);
        }

        Object context = newContext();
        // 让 getSurfaceSecondary 的缓存判定命中「已是最新」，避免去调 system。
        lastUpdateXZField.setLong(context, 1L);
        lastSurfaceDepth2UpdateField.setLong(context, 1L);

        SurfaceRules.ConditionSource source;
        if (kind.equals("water")) {
            int offset = Integer.parseInt(args.get(0));
            int multiplier = Integer.parseInt(args.get(1));
            boolean addStoneDepth = Integer.parseInt(args.get(2)) != 0;
            blockYField.setInt(context, Integer.parseInt(args.get(3)));
            stoneDepthAboveField.setInt(context, Integer.parseInt(args.get(4)));
            stoneDepthBelowField.setInt(context, 0);
            waterHeightField.setInt(context, Integer.parseInt(args.get(5)));
            surfaceDepthField.setInt(context, Integer.parseInt(args.get(6)));

            source = addStoneDepth ? SurfaceRules.waterStartCheck(offset, multiplier)
                : SurfaceRules.waterBlockCheck(offset, multiplier);
        } else if (kind.equals("stonedepth")) {
            int offset = Integer.parseInt(args.get(0));
            boolean addSurfaceDepth = Integer.parseInt(args.get(1)) != 0;
            int secondaryRange = Integer.parseInt(args.get(2));
            boolean ceiling = args.get(3).equals("ceiling");
            int stoneDepth = Integer.parseInt(args.get(4));

            // Floor 取 stoneDepthAbove、Ceiling 取 stoneDepthBelow，两侧填同一个值，
            // 用例只关心阈值的相对关系。
            stoneDepthAboveField.setInt(context, stoneDepth);
            stoneDepthBelowField.setInt(context, stoneDepth);
            blockYField.setInt(context, 64);
            waterHeightField.setInt(context, Integer.MIN_VALUE);
            surfaceDepthField.setInt(context, Integer.parseInt(args.get(5)));
            surfaceSecondaryField.setDouble(context, Double.parseDouble(args.get(6)));

            source = SurfaceRules.stoneDepthCheck(offset, addSurfaceDepth, secondaryRange,
                ceiling ? CaveSurface.CEILING : CaveSurface.FLOOR);
        } else if (kind.equals("y")) {
            String anchorKind = args.get(0);
            int anchorValue = Integer.parseInt(args.get(1));
            int multiplier = Integer.parseInt(args.get(2));
            boolean addStoneDepth = Integer.parseInt(args.get(3)) != 0;

            blockYField.setInt(context, Integer.parseInt(args.get(4)));
            stoneDepthAboveField.setInt(context, Integer.parseInt(args.get(5)));
            stoneDepthBelowField.setInt(context, 0);
            waterHeightField.setInt(context, Integer.MIN_VALUE);
            surfaceDepthField.setInt(context, Integer.parseInt(args.get(6)));

            VerticalAnchor anchor = anchorKind.equals("below_top")
                ? VerticalAnchor.belowTop(anchorValue)
                : VerticalAnchor.aboveBottom(anchorValue);
            source = addStoneDepth ? SurfaceRules.yStartCheck(anchor, multiplier)
                : SurfaceRules.yBlockCheck(anchor, multiplier);
        } else {
            return missing(requested);
        }

        Object condition = applyMethod.invoke(source, context);
        boolean result = (Boolean) testMethod().invoke(condition);

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("result", result);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}