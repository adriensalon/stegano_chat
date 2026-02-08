cmake_minimum_required(VERSION 3.13)
project(stegano_chat)

# cereal
add_library(cereal INTERFACE EXCLUDE_FROM_ALL)
target_include_directories(cereal INTERFACE "external/cereal/include")

# glfw
if(NOT EMSCRIPTEN AND NOT ANDROID)
    set(glfw_main_sources
        "external/glfw/src/context.c"
        "external/glfw/src/init.c"
        "external/glfw/src/input.c"
        "external/glfw/src/monitor.c"
        "external/glfw/src/platform.c"
        "external/glfw/src/vulkan.c"
        "external/glfw/src/window.c"
        "external/glfw/src/egl_context.c"
        "external/glfw/src/osmesa_context.c"
        "external/glfw/src/null_init.c"
        "external/glfw/src/null_monitor.c"
        "external/glfw/src/null_window.c"
        "external/glfw/src/null_joystick.c")
    if(APPLE)
        set(glfw_platform_sources
            "external/glfw/src/cocoa_time.c"
            "external/glfw/src/posix_module.c"
            "external/glfw/src/posix_thread.c")
    elseif(WIN32)
        set(glfw_platform_sources
            "external/glfw/src/win32_time.c"
            "external/glfw/src/win32_module.c"
            "external/glfw/src/win32_thread.c"
            "external/glfw/src/win32_init.c"
            "external/glfw/src/win32_joystick.c"
            "external/glfw/src/win32_monitor.c"
            "external/glfw/src/win32_window.c"
            "external/glfw/src/wgl_context.c")    
    else()
        set(glfw_platform_sources 
            "external/glfw/src/posix_module.c"
            "external/glfw/src/posix_time.c"
            "external/glfw/src/posix_thread.c")
    endif()
    set(glfw_sources ${glfw_main_sources} ${glfw_platform_sources})
    add_library(glfw STATIC EXCLUDE_FROM_ALL ${glfw_sources})
    set_target_properties(glfw PROPERTIES CXX_STANDARD 17)
    target_include_directories(glfw PUBLIC "external/glfw/include")
    target_include_directories(glfw PUBLIC "external/glfw/deps")
    if(WIN32)
        target_compile_definitions(glfw PRIVATE _GLFW_WIN32)
    endif()
    if(MSVC)
        target_compile_options(glfw PRIVATE /wd4034)
        target_compile_options(glfw PRIVATE /wd4996)
    endif()
endif()

# imgui
set(imgui_sources
	"external/imgui/imgui.cpp"
	"external/imgui/imgui_widgets.cpp"
	"external/imgui/imgui_tables.cpp"
	"external/imgui/imgui_draw.cpp"
	"external/imgui/imgui_demo.cpp"
	"external/imgui/misc/cpp/imgui_stdlib.cpp"	
	"external/imgui/backends/imgui_impl_opengl3.cpp")
if(ANDROID)
	set(imgui_sources
		${imgui_sources}
		"external/imgui/backends/imgui_impl_android.cpp")
endif()
if(NOT ANDROID AND NOT EMSCRIPTEN)
	set(imgui_sources
		${imgui_sources}
		"external/imgui/backends/imgui_impl_glfw.cpp")
endif()
add_library(imgui STATIC EXCLUDE_FROM_ALL ${imgui_sources})
set_target_properties(imgui PROPERTIES CXX_STANDARD 17)
target_compile_definitions(imgui PUBLIC -DIMGUI_USE_WCHAR32)
target_include_directories(imgui PUBLIC "external/imgui")
target_include_directories(imgui PUBLIC "external/imgui/backends")
if(ANDROID)
	target_link_libraries(imgui PUBLIC GLESv3)
endif()
if(WIN32)
	target_link_libraries(imgui PUBLIC glfw)
endif()

# libsodium
set(VERSION 1.0.20)
set(SODIUM_LIBRARY_VERSION_MAJOR 26)
set(SODIUM_LIBRARY_VERSION_MINOR 2)
configure_file(
    "external/libsodium/src/libsodium/include/sodium/version.h.in"
    "external/libsodium/src/libsodium/include/sodium/version.h")
file(GLOB_RECURSE libsodium_source "external/libsodium/src/libsodium/*.c")
add_library(libsodium STATIC EXCLUDE_FROM_ALL ${libsodium_source})
set_target_properties(libsodium PROPERTIES C_STANDARD 99)
target_compile_definitions(libsodium PUBLIC -DSODIUM_STATIC)
target_include_directories(libsodium PUBLIC "external/libsodium/src/libsodium/include")
target_include_directories(libsodium PRIVATE "external/libsodium/src/libsodium/include/sodium")
if(MSVC)
    target_compile_definitions(libsodium PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libsodium PRIVATE /wd4197)
    target_compile_options(libsodium PRIVATE /wd4244)
endif()
if(ANDROID OR EMSCRIPTEN)
    target_compile_options(libsodium PRIVATE -Wno-cpp)
endif()

# nativefiledialog
if(NOT EMSCRIPTEN AND NOT ANDROID)
    set(nativefiledialog_source "external/nativefiledialog/src/nfd_common.c")
    if(WIN32)
        set(nativefiledialog_source ${nativefiledialog_source} "external/nativefiledialog/src/nfd_win.cpp")
    elseif(LINUX)
    endif()
    add_library(nativefiledialog STATIC EXCLUDE_FROM_ALL ${nativefiledialog_source})
    set_target_properties(nativefiledialog PROPERTIES C_STANDARD 99)
    set_target_properties(nativefiledialog PROPERTIES CXX_STANDARD 17)
    target_include_directories(nativefiledialog PUBLIC "external/nativefiledialog/src/include")
    if(MSVC)
        target_compile_definitions(nativefiledialog PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

# zlib
configure_file(	
    "external/zlib/zconf.h.cmakein"
    "${CMAKE_CURRENT_BINARY_DIR}/external/zlib/zconf.h" @ONLY)
set(zlib_sources
    "external/zlib/adler32.c"
    "external/zlib/compress.c"
    "external/zlib/crc32.c"
    "external/zlib/deflate.c"
    "external/zlib/inflate.c"
    "external/zlib/infback.c"
    "external/zlib/inftrees.c"
    "external/zlib/inffast.c"
    "external/zlib/trees.c"
    "external/zlib/uncompr.c"
    "external/zlib/zutil.c")
if(NOT ANDROID AND NOT EMSCRIPTEN)
    list(APPEND zlib_sources
        "external/zlib/gzclose.c"
        "external/zlib/gzlib.c"
        "external/zlib/gzread.c"
        "external/zlib/gzwrite.c")
endif()
add_library(zlib STATIC EXCLUDE_FROM_ALL ${zlib_sources})
target_include_directories(zlib PUBLIC "external/zlib" "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")
if(MSVC)
    target_compile_definitions(zlib PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(zlib PRIVATE _CRT_SECURE_NO_DEPRECATE)
    target_compile_definitions(zlib PRIVATE _CRT_NONSTDC_NO_DEPRECATE)
endif()

# libpng
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/external/libpng/scripts/pnglibconf.h.prebuilt"
    "${CMAKE_CURRENT_BINARY_DIR}/external/libpng/pnglibconf.h"
    @ONLY)
set(libpng_public_hdrs
    "external/libpng/png.h"
    "external/libpng/pngconf.h"
    "${CMAKE_CURRENT_BINARY_DIR}/external/libpng/pnglibconf.h")
set(libpng_private_hdrs
    "external/libpng/pngpriv.h"
    "external/libpng/pngdebug.h"
    "external/libpng/pnginfo.h"
    "external/libpng/pngstruct.h")
set(libpng_sources
    # ${libpng_arm_sources}
    # ${libpng_intel_sources}
    # ${libpng_mips_sources}
    # ${libpng_powerpc_sources}
    # ${libpng_loongarch_sources}
    # ${libpng_riscv_sources}
    ${libpng_public_hdrs}
    ${libpng_private_hdrs}
    "external/libpng/png.c"
    "external/libpng/pngerror.c"
    "external/libpng/pngget.c"
    "external/libpng/pngmem.c"
    "external/libpng/pngpread.c"
    "external/libpng/pngread.c"
    "external/libpng/pngrio.c"
    "external/libpng/pngrtran.c"
    "external/libpng/pngrutil.c"
    "external/libpng/pngset.c"
    "external/libpng/pngtrans.c"
    "external/libpng/pngwio.c"
    "external/libpng/pngwrite.c"
    "external/libpng/pngwtran.c"
    "external/libpng/pngwutil.c")
add_library(libpng STATIC EXCLUDE_FROM_ALL ${libpng_sources})
target_include_directories(libpng PUBLIC "external/libpng")
target_include_directories(libpng PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/external/libpng")
target_link_libraries(libpng PRIVATE zlib)
if(MSVC)
    target_compile_definitions(libpng PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# -----------------------------------------------------------------------

file(GLOB_RECURSE stegano_chat_sources "source/*.cpp")
if(ANDROID)
    add_library(stegano_chat SHARED ${stegano_chat_sources})
else()
    add_executable(stegano_chat ${stegano_chat_sources})
endif()
set_target_properties(stegano_chat PROPERTIES CXX_STANDARD 17)
target_link_libraries(stegano_chat PRIVATE cereal imgui libsodium zlib libpng)
if(ANDROID)
    set(android_native_app_glue_dir ${ANDROID_NDK}/sources/android/native_app_glue)
    add_library(android_native_app_glue STATIC ${android_native_app_glue_dir}/android_native_app_glue.c)
    target_include_directories(android_native_app_glue PUBLIC ${android_native_app_glue_dir})
    target_link_libraries(stegano_chat PRIVATE EGL android log android_native_app_glue)
endif()
if(NOT EMSCRIPTEN AND NOT ANDROID)
    target_link_libraries(stegano_chat PRIVATE glfw nativefiledialog)
endif()
target_include_directories(stegano_chat PRIVATE "source")
if(MSVC)
    target_compile_definitions(stegano_chat PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if(ANDROID)
    set(android_platform_dir "${CMAKE_CURRENT_LIST_DIR}/platform/android")
    set(android_build_so "${CMAKE_CURRENT_BINARY_DIR}/libstegano_chat.so")
    set(android_jnilibs_dir "${android_platform_dir}/src/main/jniLibs/${ANDROID_ABI}")
    set(android_jnilibs_so "${android_jnilibs_dir}/libstegano_chat.so")
    add_custom_command(
        TARGET stegano_chat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${android_jnilibs_dir}
        COMMAND ${CMAKE_COMMAND} -E copy ${android_build_so} ${android_jnilibs_so}
        COMMAND ${CMAKE_COMMAND} -E chdir ${android_platform_dir} ${ANDROID_GRADLEW} assemble$<IF:$<CONFIG:Debug>,Debug,Release>
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${android_platform_dir}/build/outputs/apk/$<IF:$<CONFIG:Debug>,debug,release> ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL)
endif()
if(EMSCRIPTEN)
    set_target_properties(stegano_chat PROPERTIES 
        SUFFIX ".html")
    target_link_options(stegano_chat PRIVATE 
        --shell-file "${CMAKE_CURRENT_LIST_DIR}/platform/web/shell.html"
        -sLLD_REPORT_UNDEFINED
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=671088640
        -sMALLOC=emmalloc
        -lidbfs.js
        -sASYNCIFY
        -sMAX_WEBGL_VERSION=2
        -sUSE_WEBGL2=1)
    add_custom_command(TARGET stegano_chat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_FILE:stegano_chat>
            $<TARGET_FILE_DIR:stegano_chat>/index.html)
endif()

# -----------------------------------------------------------------------

if(CMAKE_CROSSCOMPILING)
    return()
endif()

# web
if(DEFINED ENV{EMSDK})
    set(emsdk_dir $ENV{EMSDK})
elseif(DEFINED ENV{EMSCRIPTEN})
    set(emsdk_dir $ENV{EMSCRIPTEN})
endif()
if(emsdk_dir AND EXISTS "${emsdk_dir}")
    message(STATUS "Found Emsdk at ${emsdk_dir}")
    set(emsdk_found TRUE)
else()
    find_program(emcc_executable NAMES emcc PATHS ENV PATH)
    if(emcc_executable)
        get_filename_component(emcc_dir "${emcc_executable}" DIRECTORY)
        get_filename_component(emsdk_dir "${emcc_dir}/../../" ABSOLUTE)
        if(EXISTS "${emsdk_dir}")
            set(emsdk_found TRUE)
        endif()
    endif()
endif()
if(NOT emsdk_found)
    message(STATUS "Emscripten SDK could not be located")
else()
    set(emsdk_toolchain_file "${emsdk_dir}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" CACHE PATH "emsdk_dir toolchain file")
    message(STATUS "Found Emscripten toolchain file at ${emsdk_toolchain_file}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web")
    add_custom_target(stegano_chat_web
        ALL
        COMMAND "${CMAKE_COMMAND}" -Wno-dev 
            -S "${CMAKE_CURRENT_LIST_DIR}" 
            -B "${CMAKE_CURRENT_BINARY_DIR}/web" 
            -G "Ninja" 
            -DCMAKE_TOOLCHAIN_FILE=${emsdk_toolchain_file}
            -DCMAKE_BUILD_TYPE=$<CONFIG>
        COMMAND "${CMAKE_COMMAND}" 
            --build "${CMAKE_CURRENT_BINARY_DIR}/web"
            --target "stegano_chat"
        VERBATIM
        USES_TERMINAL)
endif()

# android
set(ndk_candidates)
if(DEFINED ENV{ANDROID_NDK_ROOT})
    list(APPEND ndk_candidates "$ENV{ANDROID_NDK_ROOT}")
endif()
if(DEFINED ENV{ANDROID_NDK_HOME})
    list(APPEND ndk_candidates "$ENV{ANDROID_NDK_HOME}")
endif()
if(DEFINED ENV{ANDROID_NDK})
    list(APPEND ndk_candidates "$ENV{ANDROID_NDK}")
endif()
if(DEFINED ENV{ANDROID_SDK_ROOT})
    file(GLOB _ndk_dirs "$ENV{ANDROID_SDK_ROOT}/ndk/*")
    list(APPEND ndk_candidates ${_ndk_dirs})
endif()
set(ANDROID_NDK_FOUND FALSE)
foreach(cand IN LISTS ndk_candidates)
    if(EXISTS "${cand}/build/cmake/android.toolchain.cmake")
        set(ANDROID_NDK "${cand}")
        set(ANDROID_NDK_FOUND TRUE)
        break()
    endif()
endforeach()
if(NOT ANDROID_NDK_FOUND)
    find_program(ANDROID_NDK_BUILD_EXECUTABLE ndk-build)
    if(ANDROID_NDK_BUILD_EXECUTABLE)
        get_filename_component(_ndk_dir "${ANDROID_NDK_BUILD_EXECUTABLE}" DIRECTORY)
        get_filename_component(_ndk_root "${_ndk_dir}/.." ABSOLUTE)
        if(EXISTS "${_ndk_root}/build/cmake/android.toolchain.cmake")
            set(ANDROID_NDK "${_ndk_root}")
            set(ANDROID_NDK_FOUND TRUE)
        endif()
    endif()
endif()
if(NOT ANDROID_NDK_FOUND)
    message(STATUS "Android NDK could not be located")
else()
    set(ANDROID_TOOLCHAIN "${ANDROID_NDK}/build/cmake/android.toolchain.cmake" CACHE PATH "Android NDK toolchain file")
    message(STATUS "Found Android toolchain file at ${ANDROID_TOOLCHAIN}")
    if(WIN32)
        set(ANDROID_GRADLEW "gradlew.bat")
    else()
        set(ANDROID_GRADLEW "gradlew")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/android/armeabi-v7a")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/android/arm64-v8a")
    add_custom_target(stegano_chat_android
        ALL
        COMMAND "${CMAKE_COMMAND}" -Wno-dev 
            -S "${CMAKE_CURRENT_LIST_DIR}" 
            -B "${CMAKE_CURRENT_BINARY_DIR}/android/armeabi-v7a" 
            -G "Ninja" 
            -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN}
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DANDROID_GRADLEW=${ANDROID_GRADLEW}
            -DANDROID_PLATFORM=24
            -DANDROID_ABI=armeabi-v7a
        COMMAND "${CMAKE_COMMAND}" -Wno-dev 
            -S "${CMAKE_CURRENT_LIST_DIR}" 
            -B "${CMAKE_CURRENT_BINARY_DIR}/android/arm64-v8a" 
            -G "Ninja" 
            -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN}
            -DCMAKE_BUILD_TYPE=$<CONFIG>
            -DANDROID_GRADLEW=${ANDROID_GRADLEW}
            -DANDROID_PLATFORM=24
            -DANDROID_ABI=arm64-v8a
        COMMAND "${CMAKE_COMMAND}" 
            --build "${CMAKE_CURRENT_BINARY_DIR}/android/armeabi-v7a"
            --target "stegano_chat"
        COMMAND "${CMAKE_COMMAND}" 
            --build "${CMAKE_CURRENT_BINARY_DIR}/android/arm64-v8a"
            --target "stegano_chat"
        VERBATIM
        USES_TERMINAL)
endif()

