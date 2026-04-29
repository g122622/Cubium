# shaders/CMakeLists.txt
# 着色器编译配置

# 查找 glslangValidator 或 glslc
find_program(GLSLANG_VALIDATOR
    NAMES glslangValidator glslangValidator.exe
    HINTS $ENV{VULKAN_SDK}/Bin
    DOC "Path to glslangValidator"
)

find_program(GLSLC
    NAMES glslc glslc.exe
    HINTS $ENV{VULKAN_SDK}/Bin
    DOC "Path to glslc"
)

# 选择编译器
if(GLSLC)
    set(SHADER_COMPILER ${GLSLC})
    set(SHADER_COMPILE_FLAG "")
    set(SHADER_OUTPUT_FLAG "-o")
elseif(GLSLANG_VALIDATOR)
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_COMPILE_FLAG "-V")
    set(SHADER_OUTPUT_FLAG "-o")
else()
    message(WARNING "No shader compiler found (glslc or glslangValidator). Shaders will not be compiled.")
    return()
endif()

set(SHADER_PLATFORM_DEFINES "")
if(APPLE)
    list(APPEND SHADER_PLATFORM_DEFINES "-D__APPLE__")
endif()

message(STATUS "Shader compiler: ${SHADER_COMPILER}")

# 着色器源文件列表
set(SHADER_SOURCES
    block.vert
    block.frag
    chunk.vert
    chunk.frag
    debug.vert
    debug.frag
    gui.vert
    gui.frag
    entity.vert
    entity.frag
    sky.vert
    sky.frag
    sun.vert
    sun.frag
    moon.vert
    moon.frag
    star.vert
    star.frag
    cloud.vert
    cloud.frag
    weather.vert
    weather.frag
    particle.vert
    particle.frag
    break_overlay.vert
    break_overlay.frag
)

# 输出目录
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

# 创建输出目录
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# 编译每个着色器
set(SHADER_OUTPUT_FILES "")
foreach(SHADER ${SHADER_SOURCES})
    set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${SHADER}.spv)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SHADER_COMPILER} ${SHADER_COMPILE_FLAG} ${SHADER_PLATFORM_DEFINES} ${INPUT_FILE} ${SHADER_OUTPUT_FLAG} ${OUTPUT_FILE}
        DEPENDS ${INPUT_FILE}
        COMMENT "Compiling shader: ${SHADER}"
        VERBATIM
    )

    list(APPEND SHADER_OUTPUT_FILES ${OUTPUT_FILE})
endforeach()

# 创建着色器编译目标
add_custom_target(compile_shaders ALL DEPENDS ${SHADER_OUTPUT_FILES})

# 安装着色器
install(FILES ${SHADER_OUTPUT_FILES}
    DESTINATION bin/shaders
)
