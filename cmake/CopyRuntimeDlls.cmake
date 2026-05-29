if(NOT DEFINED targetBinary OR NOT DEFINED installedDir OR NOT DEFINED outputDir)
    message(FATAL_ERROR "targetBinary, installedDir and outputDir are required")
endif()

string(STRIP "${targetBinary}" targetBinary)
string(STRIP "${installedDir}" installedDir)
string(STRIP "${outputDir}" outputDir)
string(REPLACE "\"" "" targetBinary "${targetBinary}")
string(REPLACE "\"" "" installedDir "${installedDir}")
string(REPLACE "\"" "" outputDir "${outputDir}")

if(NOT EXISTS "${targetBinary}")
    message(FATAL_ERROR "Target binary not found: ${targetBinary}")
endif()

if(NOT EXISTS "${installedDir}")
    message(FATAL_ERROR "Installed dir not found: ${installedDir}")
endif()

# 动态查找 dumpbin 或 llvm-objdump
set(toolFound FALSE)

find_program(DUMPBIN_EXE dumpbin)
if(DUMPBIN_EXE)
    execute_process(
        COMMAND "${DUMPBIN_EXE}" /DEPENDENTS "${targetBinary}"
        OUTPUT_VARIABLE dumpbinOutput
        ERROR_VARIABLE dumpbinError
        RESULT_VARIABLE dumpbinResult
    )
    if(dumpbinResult EQUAL 0)
        set(toolOutput "${dumpbinOutput}")
        set(toolFound TRUE)
    endif()
endif()

if(NOT toolFound)
    find_program(LLVM_OBJDUMP_EXE llvm-objdump)
    if(LLVM_OBJDUMP_EXE)
        execute_process(
            COMMAND "${LLVM_OBJDUMP_EXE}" --private-headers "${targetBinary}"
            OUTPUT_VARIABLE objdumpOutput
            ERROR_VARIABLE objdumpError
            RESULT_VARIABLE objdumpResult
        )
        if(objdumpResult EQUAL 0)
            set(toolOutput "${objdumpOutput}")
            set(toolFound TRUE)
        endif()
    endif()
endif()

if(NOT toolFound)
    message(WARNING "Neither dumpbin nor llvm-objdump is available; skipping runtime DLL copy for ${targetBinary}")
    return()
endif()

file(GLOB candidateDlls "${installedDir}/*.dll")

foreach(candidateDll IN LISTS candidateDlls)
    get_filename_component(candidateName "${candidateDll}" NAME)
    string(FIND "${toolOutput}" "${candidateName}" candidatePos)
    if(candidatePos GREATER -1)
        file(COPY "${candidateDll}" DESTINATION "${outputDir}")
    endif()
endforeach()
