# 行为包是独立的测试资源目标，生产服务端不依赖 TypeScript/npm。
function(mc_add_integrated_packs integratedRoot)
    file(READ "${integratedRoot}/packs.json" _pack_json)
    string(JSON _pack_count LENGTH "${_pack_json}")
    math(EXPR _last_pack "${_pack_count} - 1")
    file(GLOB_RECURSE _utils CONFIGURE_DEPENDS "${integratedRoot}/utils/*.ts")
    file(GLOB _shared_structures CONFIGURE_DEPENDS "${integratedRoot}/structures/gametests/*.mcstructure")
    set(_inputs "${integratedRoot}/build.mjs" "${integratedRoot}/packs.json"
        "${integratedRoot}/package.json" "${integratedRoot}/package-lock.json"
        "${integratedRoot}/tsconfig.base.json" ${_utils} ${_shared_structures})
    set(_outputs "")
    foreach(_index RANGE ${_last_pack})
        string(JSON _pack GET "${_pack_json}" ${_index})
        set(_root "${integratedRoot}/${_pack}")
        list(APPEND _outputs "${_root}/.cubium-shared-structures.json")
        file(GLOB_RECURSE _sources CONFIGURE_DEPENDS "${_root}/src/*.ts")
        list(FILTER _sources EXCLUDE REGEX "/src/utils/")
        list(APPEND _inputs "${_root}/tsconfig.json" "${_root}/manifest.json" ${_sources})
        foreach(_source IN LISTS _sources)
            if(NOT _source MATCHES "\\.d\\.ts$")
                file(RELATIVE_PATH _relative "${_root}/src" "${_source}")
                string(REGEX REPLACE "\\.ts$" ".js" _relative "${_relative}")
                list(APPEND _outputs "${_root}/scripts/${_relative}")
            endif()
        endforeach()
        foreach(_util IN LISTS _utils)
            file(RELATIVE_PATH _relative "${integratedRoot}/utils" "${_util}")
            list(APPEND _outputs "${_root}/src/utils/${_relative}")
            if(NOT _util MATCHES "\\.d\\.ts$")
                string(REGEX REPLACE "\\.ts$" ".js" _relative "${_relative}")
                list(APPEND _outputs "${_root}/scripts/utils/${_relative}")
            endif()
        endforeach()
        foreach(_shared IN LISTS _shared_structures)
            get_filename_component(_name "${_shared}" NAME)
            list(APPEND _outputs "${_root}/structures/gametests/${_name}")
        endforeach()
        # 包私有结构已在最终路径，不参与复制；不能把生成的共享副本反向登记为输入。
    endforeach()
    list(REMOVE_DUPLICATES _inputs)
    list(REMOVE_DUPLICATES _outputs)
    # 清单变化也是输入：删除/新增源码时必须重建，不能仅依赖剩余文件的时间戳。
    set(_build_root "${CMAKE_BINARY_DIR}/tests/integrated")
    file(CONFIGURE OUTPUT "${_build_root}/inputs.txt" CONTENT "${_inputs}\n${_outputs}\n" @ONLY)
    set(_stamp "${_build_root}/.build.stamp")
    add_custom_command(
        OUTPUT "${_stamp}"
        BYPRODUCTS ${_outputs}
        COMMAND "${NODE_EXECUTABLE}" "${integratedRoot}/build.mjs"
        COMMAND "${CMAKE_COMMAND}" -E touch "${_stamp}"
        DEPENDS ${_inputs} "${_build_root}/inputs.txt"
        WORKING_DIRECTORY "${integratedRoot}"
        COMMENT "Compiling integrated behavior packs"
        VERBATIM
    )
    add_custom_target(mc_integrated_packs DEPENDS "${_stamp}")
endfunction()
