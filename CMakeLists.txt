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
    inc/core/cart_hw/hw_addon.h
    inc/core/cart_hw/lock_on_type.h
    inc/core/cart_hw/md_cart.h
    inc/core/cart_hw/megasd.h
    inc/core/cart_hw/sms_cart.h
    inc/core/cart_hw/special_hw_md.h
    inc/core/cart_hw/special_hw_sms.h
    inc/core/cart_hw/sram.h
    inc/core/cd_hw/cd_cart.h
    inc/core/cd_hw/cd_cart_t.h
    inc/core/cd_hw/cd_hw_t.h
    inc/core/cd_hw/cdc.h
    inc/core/cd_hw/cdc_t.h
    inc/core/cd_hw/cdd.h
    inc/core/cd_hw/cdd_t.h
    inc/core/cd_hw/chan_t.h
    inc/core/cd_hw/gfx.h
    inc/core/cd_hw/gfx_t.h
    inc/core/cd_hw/pcm.h
    inc/core/cd_hw/pcm_t.h
    inc/core/cd_hw/scd.h
    inc/core/cd_hw/toc_t.h
    inc/core/cd_hw/track_t.h
    inc/build/cmd_sdl2/config.h
    inc/build/cmd_sdl2/error.h
    inc/build/cmd_sdl2/fileio.h
    inc/build/cmd_sdl2/main.h
    inc/build/cmd_sdl2/osd.h
    inc/core/audio_subsystem.h
    inc/core/boot_rom.h
    inc/core/core_config.h
    inc/core/core_config_t.h
    inc/core/ext.h
    inc/core/external_t.h
    inc/core/framebuffer.h
    inc/core/framebuffer_t.h
    inc/core/genesis.h
    inc/core/hvc.h
    inc/core/io_ctrl.h
    inc/core/io_reg.h
    inc/core/loadrom.h
    inc/core/macros.h
    inc/core/mem68k.h
    inc/core/membnk.h
    inc/core/memz80.h
    inc/core/pico_current.h
    inc/core/region_code.h
    inc/core/rominfo.h
    inc/core/romtype.h
    inc/core/snd.h
    inc/core/state.h
    inc/core/system.h
    inc/core/system_bios.h
    inc/core/system_clock.h
    inc/core/system_cycle.h
    inc/core/system_hw.h
    inc/core/system_model.h
    inc/core/system_timing.h
    inc/core/types.h
    inc/core/vdp_ctrl.h
    inc/core/vdp_render.h
    inc/core/viewport.h
    inc/core/viewport_t.h
    inc/core/vram.h
    inc/core/work_ram.h
    inc/core/zbank.h
    inc/core/zbank_memory_map.h
    inc/core/zbank_memory_map_t.h
    inc/core/zram.h
    inc/core/zstate.h
    inc/core/crypto/crypto_crc32.h
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
    inc/core/vdp/clip_t.h
    inc/core/vdp/object_info_t.h
    inc/core/vdp/pixel.h
    inc/core/cart_hw/svp/ssp16.h
    inc/core/cart_hw/svp/svp.h
    inc/core/cart_hw/areplay.h
    inc/core/cart_hw/cart_hw_t.h
    inc/core/cart_hw/eeprom_93c.h
    inc/core/cart_hw/md_cart_t.h
    
    inc/gpgx/g_audio_renderer.h
    inc/gpgx/g_psg.h
    inc/gpgx/g_fm_synthesizer.h
    inc/gpgx/g_hid_system.h
    inc/gpgx/g_z80.h
    
    inc/gpgx/audio/audio_renderer.h
    inc/gpgx/audio/blip_buffer.h
    inc/gpgx/audio/effect/equalizer_3band.h
    inc/gpgx/audio/effect/fm_synthesizer.h
    inc/gpgx/audio/effect/fm_synthesizer_base.h
    inc/gpgx/audio/effect/null_fm_synthesizer.h
    
    inc/gpgx/cpu/z80/z80.h
    inc/gpgx/cpu/z80/z80_line_state.h
    inc/gpgx/cpu/z80/z80_macro.h
    inc/gpgx/cpu/z80/z80_register_pair.h
    inc/gpgx/cpu/z80/z80_table_index.h
    
    inc/gpgx/hid/controller.h
    inc/gpgx/hid/controller_type.h
    inc/gpgx/hid/device.h
    inc/gpgx/hid/device_type.h
    inc/gpgx/hid/hid_system.h
    inc/gpgx/hid/input.h

    inc/gpgx/ic/sn76489/sn76489.h
    inc/gpgx/ic/sn76489/sn76489_type.h
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

    inc/gpgx/ppu/vdp/bg_layer_renderer.h
    inc/gpgx/ppu/vdp/bg_pattern_cache_updater.h
    inc/gpgx/ppu/vdp/inv_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m0_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m1_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m1x_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m2_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m3_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m3x_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m4_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m4_bg_pattern_cache_updater.h
    inc/gpgx/ppu/vdp/m4_satb_parser.h
    inc/gpgx/ppu/vdp/m4_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/m4_sprite_tile_drawer.h
    inc/gpgx/ppu/vdp/m4_zoomed_sprite_tile_drawer.h
    inc/gpgx/ppu/vdp/m5_bg_column_drawer.h
    inc/gpgx/ppu/vdp/m5_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_bg_pattern_cache_updater.h
    inc/gpgx/ppu/vdp/m5_color_palette_updater.h
    inc/gpgx/ppu/vdp/m5_im2_bg_column_drawer.h
    inc/gpgx/ppu/vdp/m5_im2_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_im2_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_im2_ste_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_im2_vs_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_satb_parser.h
    inc/gpgx/ppu/vdp/m5_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_sprite_tile_drawer.h
    inc/gpgx/ppu/vdp/m5_ste_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/m5_vs_bg_layer_renderer.h
    inc/gpgx/ppu/vdp/mx_color_palette_updater.h
    inc/gpgx/ppu/vdp/satb_parser.h
    inc/gpgx/ppu/vdp/sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/tms_satb_parser.h
    inc/gpgx/ppu/vdp/tms_sprite_layer_renderer.h
    inc/gpgx/ppu/vdp/tms_sprite_tile_drawer.h
    inc/gpgx/ppu/vdp/tms_zoomed_sprite_tile_drawer.h

    inc/gpgx/vgs/vdp_irq_handler_z80.h
    
    src/build/cmd_sdl2/config.cpp
    src/build/cmd_sdl2/error.cpp
    src/build/cmd_sdl2/fileio.cpp
    src/build/cmd_sdl2/main.cpp
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
    src/core/boot_rom.cpp
    src/core/core_config.cpp
    src/core/ext.cpp
    src/core/framebuffer.cpp
    src/core/genesis.cpp
    src/core/io_ctrl.cpp
    src/core/io_reg.cpp
    src/core/loadrom.cpp
    src/core/mem68k.cpp
    src/core/membnk.cpp
    src/core/memz80.cpp
    src/core/pico_current.cpp
    src/core/region_code.cpp
    src/core/rominfo.cpp
    src/core/romtype.cpp
    src/core/snd.cpp
    src/core/state.cpp
    src/core/system.cpp
    src/core/system_bios.cpp
    src/core/system_clock.cpp
    src/core/system_cycle.cpp
    src/core/system_hw.cpp
    src/core/vdp_ctrl.cpp
    src/core/vdp_render.cpp
    src/core/viewport.cpp
    src/core/vram.cpp
    src/core/work_ram.cpp
    src/core/zbank.cpp
    src/core/zbank_memory_map.cpp
    src/core/zram.cpp
    src/core/zstate.cpp
    src/core/crypto/crypto_crc32.cpp
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
    src/core/cart_hw/svp/ssp16.cpp
    src/core/cart_hw/svp/svp.cpp
    src/core/cart_hw/areplay.cpp
    src/core/cart_hw/eeprom_93c.cpp
    
    src/gpgx/g_audio_renderer.cpp
    src/gpgx/g_psg.cpp
    src/gpgx/g_fm_synthesizer.cpp
    src/gpgx/g_hid_system.cpp
    src/gpgx/g_z80.cpp
    
    src/gpgx/audio/audio_renderer.cpp
    src/gpgx/audio/blip_buffer.cpp
    src/gpgx/audio/effect/equalizer_3band.cpp
    src/gpgx/audio/effect/fm_synthesizer_base.cpp
    src/gpgx/audio/effect/null_fm_synthesizer.cpp
    
    src/gpgx/cpu/z80/z80.cpp
    src/gpgx/cpu/z80/z80_context.cpp
    src/gpgx/cpu/z80/z80_cycles.cpp
    src/gpgx/cpu/z80/z80_opcodes.cpp
    src/gpgx/cpu/z80/z80_operations.cpp
    
    src/gpgx/hid/controller.cpp
    src/gpgx/hid/device.cpp
    src/gpgx/hid/hid_system.cpp

    src/gpgx/ic/sn76489/sn76489.cpp
    src/gpgx/ic/ym2413/ym2413.cpp
    src/gpgx/ic/ym2612/ym2612.cpp
    src/gpgx/ic/ym3438/ym3438.cpp

    src/gpgx/ppu/vdp/inv_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m0_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m1_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m1x_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m2_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m3_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m3x_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m4_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m4_bg_pattern_cache_updater.cpp
    src/gpgx/ppu/vdp/m4_satb_parser.cpp
    src/gpgx/ppu/vdp/m4_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/m4_sprite_tile_drawer.cpp
    src/gpgx/ppu/vdp/m4_zoomed_sprite_tile_drawer.cpp
    src/gpgx/ppu/vdp/m5_bg_column_drawer.cpp
    src/gpgx/ppu/vdp/m5_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_bg_pattern_cache_updater.cpp
    src/gpgx/ppu/vdp/m5_color_palette_updater.cpp
    src/gpgx/ppu/vdp/m5_im2_bg_column_drawer.cpp
    src/gpgx/ppu/vdp/m5_im2_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_im2_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_im2_ste_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_im2_vs_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_satb_parser.cpp
    src/gpgx/ppu/vdp/m5_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_sprite_tile_drawer.cpp
    src/gpgx/ppu/vdp/m5_ste_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/m5_vs_bg_layer_renderer.cpp
    src/gpgx/ppu/vdp/mx_color_palette_updater.cpp
    src/gpgx/ppu/vdp/tms_satb_parser.cpp
    src/gpgx/ppu/vdp/tms_sprite_layer_renderer.cpp
    src/gpgx/ppu/vdp/tms_sprite_tile_drawer.cpp
    src/gpgx/ppu/vdp/tms_zoomed_sprite_tile_drawer.cpp

    src/gpgx/vgs/vdp_irq_handler_z80.cpp
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
