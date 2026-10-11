cmake_minimum_required(VERSION 3.20)
project(Mother3Recomp C CXX)

if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GBARECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gbarecomp"
    CACHE PATH "Path to the gbarecomp platform-core checkout")
set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui"
    CACHE PATH "Path to the shared recomp-ui checkout")
set(GBARECOMP_RUNTIME_UI_ROOT "${RECOMP_UI_ROOT}"
    CACHE PATH "recomp-ui root used by the live GBA runtime adapter")
set(GBARECOMP_ENABLE_MODS OFF CACHE BOOL
    "Mod catalog and trusted-plugin runtime — off during bring-up" FORCE)

if(NOT EXISTS "${GBARECOMP_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "GBARECOMP_ROOT does not point to a gbarecomp checkout:\n"
        "  ${GBARECOMP_ROOT}\n"
        "Run git submodule update --init --recursive or pass "
        "-DGBARECOMP_ROOT=<path>.")
endif()

add_subdirectory("${GBARECOMP_ROOT}" gbarecomp_build EXCLUDE_FROM_ALL)

if(NOT ANDROID)
    add_executable(verify_rom_hash tools/verify_rom_hash/main.cpp)
    enable_testing()
endif()

function(add_gba_variant TARGET NAME PORT WIN_TITLE GAME_NAME ROM_SHA1 ROM_CRC32 REGION)
    set(VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/variants/${NAME}")

    if(EXISTS "${VARIANT_DIR}/generated/recompiled.cpp")
        message(FATAL_ERROR
            "${NAME}: stale monolithic generated/recompiled.cpp found. "
            "Regenerate with the current gba_recompile before building.")
    endif()

    file(GLOB VARIANT_GENERATED CONFIGURE_DEPENDS
        "${VARIANT_DIR}/generated/*.c"
        "${VARIANT_DIR}/generated/*.cpp")

    set(VARIANT_GAME_SOURCES src/main.cpp)

    # Generated C/C++ is deliberately not committed because it is derived from
    # a copyrighted ROM.  A public checkout therefore boots through the
    # runtime's interpreter bridge.  Private builds that regenerate the corpus
    # automatically replace this empty table with generated/dispatch_table.cpp.
    set(VARIANT_BOOTSTRAP_INTERP OFF)
    if(NOT EXISTS "${VARIANT_DIR}/generated/dispatch_table.cpp")
        list(APPEND VARIANT_GAME_SOURCES src/empty_dispatch_table.cpp)
        set(VARIANT_BOOTSTRAP_INTERP ON)
        message(WARNING
            "${NAME}: no generated dispatch table; building whole-program "
            "interpreter bootstrap. Regenerate the corpus for native recompilation speed.")
    endif()

    set(VARIANT_ICON "${VARIANT_DIR}/launcher/icon.ico")
    set(VARIANT_ICON_OBJS "")
    if(WIN32 AND EXISTS "${VARIANT_ICON}")
        set(VARIANT_RC "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_icon.rc")
        file(WRITE "${VARIANT_RC}" "1 ICON \"${VARIANT_ICON}\"\n")
        add_library(${TARGET}_icon OBJECT "${VARIANT_RC}")
        set(VARIANT_ICON_OBJS $<TARGET_OBJECTS:${TARGET}_icon>)
    endif()

    if(ANDROID)
        # SDLActivity loads libmain.so from the APK.
        add_library(${TARGET} SHARED
            ${VARIANT_GAME_SOURCES}
            ${VARIANT_GENERATED})
    else()
        add_executable(${TARGET}
            ${VARIANT_GAME_SOURCES}
            ${VARIANT_ICON_OBJS}
            ${VARIANT_GENERATED})
    endif()

    set(VARIANT_RECOMPILED_SOURCES ${VARIANT_GENERATED})
    list(FILTER VARIANT_RECOMPILED_SOURCES INCLUDE REGEX
        "/recompiled_[0-9]+\\.cpp$")
    if(VARIANT_RECOMPILED_SOURCES)
        set_source_files_properties(${VARIANT_RECOMPILED_SOURCES} PROPERTIES
            COMPILE_OPTIONS "-O1;-g0")
    endif()

    target_include_directories(${TARGET} PRIVATE
        "${GBARECOMP_ROOT}/src/armv4t"
        "${GBARECOMP_ROOT}/src/gba"
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/debug"
        src)

    target_link_libraries(${TARGET} PRIVATE
        "-Wl,--start-group"
        gbarecomp_runtime
        gbarecomp_debug
        gbarecomp_gba
        gbarecomp_armv4t
        "-Wl,--end-group")

    if(VARIANT_BOOTSTRAP_INTERP)
        target_compile_definitions(${TARGET} PRIVATE MOTHER3_BOOTSTRAP_INTERP=1)
    endif()

    target_compile_definitions(${TARGET} PRIVATE
        GBARECOMP_DEFAULT_DEBUG_PORT=${PORT}
        GBARECOMP_WINDOW_TITLE="${WIN_TITLE}"
        GBARECOMP_PROGRAM_NAME="./${WIN_TITLE}"
        GBARECOMP_DEFAULT_GAME_CONFIG="variants/${NAME}/game.toml"
        GBARECOMP_BUILTIN_NAME="${GAME_NAME}"
        GBARECOMP_BUILTIN_SHA1="${ROM_SHA1}"
        GBARECOMP_BUILTIN_CRC32=${ROM_CRC32}
        GBARECOMP_BUILTIN_REGION="${REGION}")

    if(GBAGAME_RECOMP_UI)
        target_link_libraries(${TARGET} PRIVATE game_launcher_ui)
        set(VARIANT_BOXART "${VARIANT_DIR}/launcher/boxart.tga")
        if(EXISTS "${VARIANT_BOXART}")
            if(NOT ANDROID)
                add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        $<TARGET_FILE_DIR:${TARGET}>/assets/img
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${VARIANT_BOXART}"
                        $<TARGET_FILE_DIR:${TARGET}>/assets/img/boxart_${NAME}.tga
                    VERBATIM)
            endif()
            target_compile_definitions(${TARGET} PRIVATE
                GBARECOMP_BOXART="assets/img/boxart_${NAME}.tga")
        endif()
    endif()

    if(MSVC)
        target_link_options(${TARGET} PRIVATE /STACK:268435456)
    elseif(MINGW OR WIN32)
        target_link_options(${TARGET} PRIVATE -Wl,--stack,268435456)
    endif()
endfunction()

option(GBAGAME_RECOMP_UI "Build the recomp-ui launcher/runtime UI" ON)
if(GBAGAME_RECOMP_UI)
    include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
    add_library(game_launcher_ui STATIC src/game_launcher_boot.cpp)
    set_target_properties(game_launcher_ui PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(game_launcher_ui PRIVATE
        "${GBARECOMP_ROOT}/src/runtime"
        "${GBARECOMP_ROOT}/src/gba"
        src)
    recomp_target_launcher_ui(game_launcher_ui CONSOLE gba)

    # The shared Android shell already defines SDL2::SDL2. Desktop keeps the
    # normal system-SDL lookup.
    if(NOT TARGET SDL2::SDL2)
        find_package(SDL2 REQUIRED)
    endif()
    target_link_libraries(game_launcher_ui PUBLIC SDL2::SDL2)
    if(WIN32)
        target_link_libraries(game_launcher_ui PUBLIC opengl32)
    endif()
    set_property(SOURCE src/main.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS GBAGAME_RECOMP_UI)
endif()

if(ANDROID)
    # One APK contains one static recompilation corpus. English is the default;
    # pass -DMOTHER3_ANDROID_VARIANT=mother3 for the Japanese build.
    set(MOTHER3_ANDROID_VARIANT "mother3en" CACHE STRING
        "Android variant: mother3en or mother3")
    if(MOTHER3_ANDROID_VARIANT STREQUAL "mother3en")
        add_gba_variant(main mother3en 19894 "Mother3RecompEN"
            "MOTHER 3 (EN)" "306FB8874533B0FD0796208FAA8BED2DB4AE77FB"
            0x8A3BC5A8 "EN")
    elseif(MOTHER3_ANDROID_VARIANT STREQUAL "mother3")
        add_gba_variant(main mother3 19893 "Mother3Recomp"
            "MOTHER 3 (JPN)" "4f0f493e12c2a8c61b2d809af03f7abf87a85776"
            0x42AC9CB9 "JPN")
    else()
        message(FATAL_ERROR
            "Unknown MOTHER3_ANDROID_VARIANT='${MOTHER3_ANDROID_VARIANT}'. "
            "Use mother3en or mother3.")
    endif()
else()
    add_gba_variant(Mother3Recomp mother3 19893 "Mother3Recomp"
        "MOTHER 3 (JPN)" "4f0f493e12c2a8c61b2d809af03f7abf87a85776"
        0x42AC9CB9 "JPN")
    add_gba_variant(Mother3RecompEN mother3en 19894 "Mother3RecompEN"
        "MOTHER 3 (EN)" "306FB8874533B0FD0796208FAA8BED2DB4AE77FB"
        0x8A3BC5A8 "EN")
endif()
