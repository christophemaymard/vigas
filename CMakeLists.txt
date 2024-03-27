cmake_minimum_required(VERSION 3.15)

project(vigas LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Add each source files of the specified target to a group that matches the 
# filesystem.
function(create_target_directory_groups target)
    get_target_property(file_paths "${target}" SOURCES)
    
    foreach(file_path IN LISTS file_paths)
        get_filename_component(dir_path "${file_path}" DIRECTORY)
        
        # Use the directory path as the name.
        # Prior to cmake 3.18, name may contain '\' to specify subgroups.
        # Starting to cmake 3.18, name may contain '\'  or '/' to specify subgroups.
        string(REPLACE "/" "\\" name "${dir_path}")
        
        source_group("${name}" FILES "${file_path}")
    endforeach()
endfunction()

add_compile_definitions("$<IF:$<CONFIG:Debug>,_DEBUG,NDEBUG>")

if(MSVC)
    add_compile_options(/std:c++17)     # C++ language standard: ISO C++17 Standard
endif()

add_subdirectory(3rdparty)

add_compile_definitions(LSB_FIRST)
add_compile_definitions(USE_16BPP_RENDERING)
add_compile_definitions(MAXROMSIZE=33554432)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_CONSOLE)
    add_compile_definitions(UNICODE _UNICODE)
endif()

if (MSVC)
    add_compile_options(
        /permissive-    # Conformance mode: Yes
        /sdl-           # Security Development Lifecycle (SDL) checks: No
        /Zc:inline      # Remove unreferenced code and data: Yes
        /WX-            # Treat warnings as errors: No
        /EHsc           # Enable C++ exceptions: Yes
    )
    
    # Warning level: Level4 if Debug, otherwise Level3.
    add_compile_options("$<IF:$<CONFIG:Debug>,/W4,/W3>")
    
    # Security check: Enable if Debug, otherwise Disable
    add_compile_options("$<IF:$<CONFIG:Debug>,/GS,/GS->")
    
    # Debug information format: Program Database for Edit And Continue fi Debug, otherwise Program Database.
    add_compile_options("$<IF:$<CONFIG:Debug>,/ZI,/Zi>")
    
    # Optimization: Disabled if Debug, otherwise Maximum Optimization (Favor Speed).
    add_compile_options("$<IF:$<CONFIG:Debug>,/Od,/O2>")
    
    # Runtime library: Multi-threaded Debug DLL if Debug, otherwise Multi-threaded DLL.
    add_compile_options("$<IF:$<CONFIG:Debug>,/MDd,/MD>")
    
    # Whole program optimization: Yes if not Debug.
    add_compile_options("$<IF:$<CONFIG:Debug>,,/GL>")
endif()

add_executable(vigas 
    inc/core/cart_hw/eeprom_i2c.h
    inc/core/cart_hw/eeprom_spi.h
    inc/core/cart_hw/ggenie.h
    inc/core/cart_hw/md_cart.h
    inc/core/cart_hw/megasd.h
    inc/core/cart_hw/sms_cart.h
    inc/core/cart_hw/sram.h
    inc/core/cd_hw/cd_cart.h
    inc/core/cd_hw/cdc.h
    inc/core/cd_hw/cdd.h
    inc/core/cd_hw/gfx.h
    inc/core/cd_hw/pcm.h
    inc/core/cd_hw/scd.h
    inc/build/cmd_sdl2/config.h
    inc/build/cmd_sdl2/error.h
    inc/build/cmd_sdl2/fileio.h
    inc/build/cmd_sdl2/main.h
    inc/build/cmd_sdl2/osd.h
    inc/core/audio_subsystem.h
    inc/core/bitmap.h
    inc/core/genesis.h
    inc/core/hvc.h
    inc/core/io_ctrl.h
    inc/core/loadrom.h
    inc/core/macros.h
    inc/core/mem68k.h
    inc/core/membnk.h
    inc/core/memz80.h
    inc/core/rominfo.h
    inc/core/snd.h
    inc/core/state.h
    inc/core/system.h
    inc/core/system_bios.h
    inc/core/system_clock.h
    inc/core/system_cycle.h
    inc/core/system_hardware.h
    inc/core/system_timing.h
    inc/core/types.h
    inc/core/vdp_ctrl.h
    inc/core/vdp_render.h
    inc/core/debug/cpuhook.h
    inc/core/input_hw/activator.h
    inc/core/input_hw/gamepad.h
    inc/core/input_hw/graphic_board.h
    inc/core/input_hw/input.h
    inc/core/input_hw/lightgun.h
    inc/core/input_hw/mouse.h
    inc/core/input_hw/paddle.h
    inc/core/input_hw/sportspad.h
    inc/core/input_hw/teamplayer.h
    inc/core/input_hw/terebi_oekaki.h
    inc/core/input_hw/xe_1ap.h
    inc/core/m68k/m68k.h
    inc/core/m68k/m68kconf.h
    inc/core/m68k/m68kcpu.h
    inc/core/m68k/m68ki_cycles.h
    inc/core/m68k/m68ki_instruction_jump_table.h
    inc/core/m68k/m68kops.h
    inc/core/m68k/s68kconf.h
    inc/core/m68k/s68ki_cycles.h
    inc/core/ntsc/md_ntsc.h
    inc/core/ntsc/md_ntsc_config.h
    inc/core/ntsc/md_ntsc_impl.h
    inc/core/ntsc/sms_ntsc.h
    inc/core/ntsc/sms_ntsc_config.h
    inc/core/ntsc/sms_ntsc_impl.h
    inc/core/cart_hw/svp/ssp16.h
    inc/core/cart_hw/svp/svp.h
    inc/core/z80/osd_cpu.h
    inc/core/z80/z80.h
    inc/core/cart_hw/areplay.h
    inc/core/cart_hw/eeprom_93c.h
    
    inc/gpgx/g_audio_renderer.h
    inc/gpgx/g_psg.h
    inc/gpgx/g_fm_synthesizer.h
    inc/gpgx/audio/audio_renderer.h
    inc/gpgx/audio/blip_buffer.h
    inc/gpgx/audio/effect/equalizer_3band.h
    inc/gpgx/audio/effect/fm_synthesizer.h
    inc/gpgx/audio/effect/fm_synthesizer_base.h
    inc/gpgx/audio/effect/null_fm_synthesizer.h
    inc/gpgx/ic/sn76489/sn76489.h
    inc/gpgx/ic/ym2413/opll_ch.h
    inc/gpgx/ic/ym2413/opll_slot.h
    inc/gpgx/ic/ym2413/ym2413.h
    inc/gpgx/ic/ym2612/fm_3slot.h
    inc/gpgx/ic/ym2612/fm_ch.h
    inc/gpgx/ic/ym2612/fm_opn.h
    inc/gpgx/ic/ym2612/fm_slot.h
    inc/gpgx/ic/ym2612/fm_st.h
    inc/gpgx/ic/ym2612/ym2612.h
    inc/gpgx/ic/ym2612/ym2612_type.h
    inc/gpgx/ic/ym3438/opn2_context.h
    inc/gpgx/ic/ym3438/ym3438.h
    inc/gpgx/ic/ym3438/ym3438_mode.h
    
    src/build/cmd_sdl2/config.cpp
    src/build/cmd_sdl2/error.cpp
    src/build/cmd_sdl2/fileio.cpp
    src/build/cmd_sdl2/main.cpp
    src/build/cmd_sdl2/osd.cpp
    src/core/cart_hw/eeprom_i2c.cpp
    src/core/cart_hw/eeprom_spi.cpp
    src/core/cart_hw/ggenie.cpp
    src/core/cart_hw/md_cart.cpp
    src/core/cart_hw/megasd.cpp
    src/core/cart_hw/sms_cart.cpp
    src/core/cart_hw/sram.cpp
    src/core/cd_hw/cd_cart.cpp
    src/core/cd_hw/cdc.cpp
    src/core/cd_hw/cdd.cpp
    src/core/cd_hw/gfx.cpp
    src/core/cd_hw/pcm.cpp
    src/core/cd_hw/scd.cpp
    src/core/audio_subsystem.cpp
    src/core/bitmap.cpp
    src/core/genesis.cpp
    src/core/io_ctrl.cpp
    src/core/loadrom.cpp
    src/core/mem68k.cpp
    src/core/membnk.cpp
    src/core/memz80.cpp
    src/core/rominfo.cpp
    src/core/snd.cpp
    src/core/state.cpp
    src/core/system.cpp
    src/core/system_bios.cpp
    src/core/system_clock.cpp
    src/core/system_cycle.cpp
    src/core/system_hardware.cpp
    src/core/vdp_ctrl.cpp
    src/core/vdp_render.cpp
    src/core/debug/cpuhook.cpp
    src/core/input_hw/activator.cpp
    src/core/input_hw/gamepad.cpp
    src/core/input_hw/graphic_board.cpp
    src/core/input_hw/input.cpp
    src/core/input_hw/lightgun.cpp
    src/core/input_hw/mouse.cpp
    src/core/input_hw/paddle.cpp
    src/core/input_hw/sportspad.cpp
    src/core/input_hw/teamplayer.cpp
    src/core/input_hw/terebi_oekaki.cpp
    src/core/input_hw/xe_1ap.cpp
    src/core/m68k/m68kcpu.cpp
    src/core/m68k/s68kcpu.cpp
    src/core/ntsc/md_ntsc.cpp
    src/core/ntsc/sms_ntsc.cpp
    src/core/cart_hw/svp/ssp16.cpp
    src/core/cart_hw/svp/svp.cpp
    src/core/z80/z80.cpp
    src/core/cart_hw/areplay.cpp
    src/core/cart_hw/eeprom_93c.cpp
    
    src/gpgx/g_audio_renderer.cpp
    src/gpgx/g_psg.cpp
    src/gpgx/g_fm_synthesizer.cpp
    src/gpgx/audio/audio_renderer.cpp
    src/gpgx/audio/blip_buffer.cpp
    src/gpgx/audio/effect/equalizer_3band.cpp
    src/gpgx/audio/effect/fm_synthesizer_base.cpp
    src/gpgx/audio/effect/null_fm_synthesizer.cpp
    src/gpgx/ic/sn76489/sn76489.cpp
    src/gpgx/ic/ym2413/ym2413.cpp
    src/gpgx/ic/ym2612/ym2612.cpp
    src/gpgx/ic/ym3438/ym3438.cpp
)

target_include_directories(vigas PRIVATE 
    inc
    inc/build/cmd_sdl2
)

target_link_libraries(vigas PRIVATE 3rdparty::sdl2)
target_link_libraries(vigas PRIVATE 3rdparty::xee)

# Define vigas as the startup project.
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT vigas)

create_target_directory_groups(vigas)
