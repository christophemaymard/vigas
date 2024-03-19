# SDL 2
add_subdirectory(sdl2 EXCLUDE_FROM_ALL)

# xee
add_subdirectory(xee EXCLUDE_FROM_ALL)

# Alias
add_library(3rdparty::sdl2 ALIAS 3rdparty_sdl2)
add_library(3rdparty::xee ALIAS 3rdparty_xee)
