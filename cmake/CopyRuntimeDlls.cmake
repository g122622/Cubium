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

set(dumpbinPath "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe")
set(llvmObjdumpPath "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/llvm-objdump.exe")

if(EXISTS "${dumpbinPath}")
    execute_process(
        COMMAND "${dumpbinPath}" /DEPENDENTS "${targetBinary}"
        OUTPUT_VARIABLE dumpbinOutput
        ERROR_VARIABLE dumpbinError
        RESULT_VARIABLE dumpbinResult
    )
    if(NOT dumpbinResult EQUAL 0)
        message(FATAL_ERROR "dumpbin failed: ${dumpbinError}")
    endif()
    set(toolOutput "${dumpbinOutput}")
elseif(EXISTS "${llvmObjdumpPath}")
    execute_process(
        COMMAND "${llvmObjdumpPath}" --private-headers "${targetBinary}"
        OUTPUT_VARIABLE objdumpOutput
        ERROR_VARIABLE objdumpError
        RESULT_VARIABLE objdumpResult
    )
    if(NOT objdumpResult EQUAL 0)
        message(FATAL_ERROR "llvm-objdump failed: ${objdumpError}")
    endif()
    set(toolOutput "${objdumpOutput}")
else()
    message(FATAL_ERROR "Neither dumpbin nor llvm-objdump is available")
endif()

file(GLOB candidateDlls "${installedDir}/*.dll")

foreach(candidateDll IN LISTS candidateDlls)
    get_filename_component(candidateName "${candidateDll}" NAME)
    string(FIND "${toolOutput}" "${candidateName}" candidatePos)
    if(candidatePos GREATER -1)
        file(COPY "${candidateDll}" DESTINATION "${outputDir}")
    endif()
endforeach()
