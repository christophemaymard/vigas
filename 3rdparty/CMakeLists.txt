# SDL 2
add_subdirectory(sdl2 EXCLUDE_FROM_ALL)

# 
add_library(3rdparty::sdl2 ALIAS 3rdparty_sdl2)
