# src/common/mod/bedrock/addon/CMakeLists.txt
# 基岩版Addon兼容模组系统

add_library(mc_bedrock_addon STATIC
    # Core (header-only, no .cpp files)

    # Engine - QuickJS引擎实现
    engine/quickjs/QuickJSRuntime.cpp
    engine/quickjs/QuickJSEngine.cpp
    engine/quickjs/QuickJSContext.cpp
    engine/quickjs/QuickJSBindingContext.cpp
    engine/quickjs/QuickJSModuleLoader.cpp

    # Binding (header-only initially, .cpp added later)
    binding/ScriptClassBinding.cpp
    binding/ScriptClassRegistry.cpp
    binding/ScriptHandleRegistry.cpp

    # Modules - @minecraft/server模块绑定
    modules/MinecraftModuleFactory.cpp
    modules/ScriptCustomComponentBinding.cpp
    modules/ScriptEventBinding.cpp
    modules/types/ScriptWorldAccessor.cpp
    modules/types/ScriptVec2.cpp
    modules/types/ScriptVec3.cpp
    modules/types/ScriptColor.cpp

    # Plugin - 插件管理
    plugin/PluginExecutionGroup.cpp
    plugin/ScriptPackPermissions.cpp
    plugin/ScriptPackConfiguration.cpp
    plugin/ScriptPluginSource.cpp
    plugin/ScriptPlugin.cpp
    plugin/ScriptPluginManager.cpp

    # Pack - 行为包系统
    pack/PackVersion.cpp
    pack/AddonModule.cpp
    pack/AddonDependency.cpp
    pack/AddonManifest.cpp
    pack/BehaviorPack.cpp
    pack/BehaviorPackList.cpp
    pack/PackDependencyResolver.cpp

    # Event - 事件桥接
    event/ScriptEventBus.cpp
    event/BeforeEventSignal.cpp
    event/AfterEventSignal.cpp

    # Lifecycle - 生命周期管理
    lifecycle/ScriptManager.cpp
    lifecycle/ScriptTickListener.cpp
    lifecycle/ScriptWatchdog.cpp
    lifecycle/ScriptLogger.cpp
    lifecycle/ScriptScheduler.cpp

    # Component - 自定义组件
    component/BlockComponentRegistry.cpp
    component/ItemComponentRegistry.cpp

    # Diagnostics - 诊断
    diagnostics/ScriptDiagnostics.cpp
    diagnostics/ScriptSentryLogger.cpp
)

target_link_libraries(mc_bedrock_addon
    PUBLIC
        mc_common
    PRIVATE
        qjs
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

mc_set_compiler_warnings(mc_bedrock_addon)

# QuickJS头文件使用了C99复合字面量和指定初始化器，
# 这在C++中是扩展特性，需要禁用相关警告
target_compile_options(mc_bedrock_addon PRIVATE
    -Wno-c99-extensions
    -Wno-c11-extensions
    -Wno-unknown-warning-option
)

# Windows平台下QuickJS使用不安全C函数（strcpy等），需要禁用安全警告
if(WIN32)
    target_compile_definitions(mc_bedrock_addon PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

target_compile_features(mc_bedrock_addon PUBLIC cxx_std_20)

target_include_directories(mc_bedrock_addon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
