cmake_minimum_required(VERSION 3.25)
project(smw_xbox360 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(SMW_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

file(GLOB SMW_CORE CONFIGURE_DEPENDS
  "${SMW_ROOT}/smb1/*.c"
  "${SMW_ROOT}/smbll/*.c"
  "${SMW_ROOT}/src/*.c"
  "${SMW_ROOT}/src/snes/*.c"
)
list(REMOVE_ITEM SMW_CORE
  "${SMW_ROOT}/src/main.c"
  "${SMW_ROOT}/src/config.c"
  "${SMW_ROOT}/src/opengl.c"
  "${SMW_ROOT}/src/glsl_shader.c"
)

add_executable(smw_xbox360
  ${SMW_CORE}
  main.c
  assets.c
  config_xenon.c
  renderer.c
)

target_include_directories(smw_xbox360 PRIVATE
  "${SMW_ROOT}"
  "${SMW_ROOT}/src"
  "${SMW_ROOT}/third_party"
  "${SMW_ROOT}/assets"
  "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_definitions(smw_xbox360 PRIVATE SMW_XENON=1)
target_compile_options(smw_xbox360 PRIVATE -fno-strict-aliasing -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(smw_xbox360 PRIVATE m)

xsdk_add_shaders(smw_xbox360 blit.vert blit.frag)
xsdk_add_xex(smw_xbox360)
