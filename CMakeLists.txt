cmake_minimum_required(VERSION 3.15)

project(huvi C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

if (MSVC)
  cmake_policy(SET CMP0091 NEW)
  # Statically build against C runtime (use the right version for Release/Debug)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if (CMAKE_COMPILER_IS_GNUCC)
  add_compile_options(-Wno-unused-function)
endif ()

if (MINGW)
  add_compile_options(-Wno-error=incompatible-pointer-types)
endif ()

if (UNIX)
  add_compile_options(-Wall)
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
  file (STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" HUVI_VERSION)
  message("-- Found huvi version: ${HUVI_VERSION}")
else ()
  execute_process(
    COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE HUVI_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Handle shallow clones
  if (HUVI_VERSION STREQUAL "")
    execute_process(
      COMMAND git rev-parse --short HEAD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE HUVI_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    set(HUVI_VERSION "v0.0.0-0-g${HUVI_VERSION}")
  endif ()
  message("-- Found huvi git version: ${HUVI_VERSION}")
endif ()

option(WithSharedLibluv "Shared or Static libluv" OFF)
option(WithOpenSSL "Include OpenSSL" OFF)
option(WithOpenSSLASM "Enable Assembly Optimizations" ON)
option(WithSharedOpenSSL "Shared or Static OpenSSL" OFF)
option(WithPCRE2 "Include PCRE2" OFF)
option(WithSharedPCRE2 "Shared or Static PCRE2" OFF)
option(WithLPEG "Include LPEG" OFF)
option(WithSharedLPEG "Shared or Static LPEG" OFF)
option(WithZLIB "Include ZLIB" OFF)
option(WithSharedZLIB "Shared or Static ZLIB" OFF)

find_package(Threads)
set (HUVI_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

# When linking against a shared libluv, we assume that luajit and libuv are also shared
# Huvi does not support linking a static libluv *and* a static lua/luajit or libuv
if (WithSharedLibluv)
  find_package(Luv REQUIRED)
  include_directories(${LUV_INCLUDE_DIRS})

  find_package(Luau  REQUIRED)
  include_directories(${LUAU_INCLUDE_DIRS})

  find_package(Libuv  REQUIRED)
  include_directories(${LIBUV_INCLUDE_DIRS})

  list(APPEND HUVI_LIBRARIES ${LUV_LIBRARIES} ${LUAJIT_LIBRARIES} ${LIBUV_LIBRARIES})
else (WithSharedLibluv)
  # Build huv as static library instead of as module
  set(BUILD_MODULE OFF CACHE BOOL "Turn off building huv as module")
  set(BUILD_STATIC_LIBS ON CACHE BOOL "Build huv as static lib")
  include_directories(deps/huv/src)
  include_directories(deps/huv/deps/libuv/include)
  list(APPEND HUVI_LIBRARIES Luau.VM Luau.Compiler)
  list(APPEND HUVI_LIBRARIES libhuv_a uv_a)

  add_subdirectory(deps/huv) # Build huv
endif (WithSharedLibluv)

if (WithOpenSSL)
  include(deps/lua-openssl.cmake)
endif ()

if (WithPCRE2)
  include(deps/lrexlib.cmake)
endif ()

if (WithLPEG)
  include(deps/lpeg.cmake)
  set(lpeg_re_lua ${LPEGLIB_DIR}/re.lua)
endif ()

if (WithZLIB)
  include(deps/lua-zlib.cmake)
endif ()

if (WIN32)
  set(winsvc src/winsvc.h src/winsvcaux.h src/winsvc.c src/winsvcaux.c)
  if (WithSharedLibluv)
    add_definitions( -DLUA_BUILD_AS_DLL -DBUILDING_UV_SHARED )
  endif ()
  add_definitions( -DWITH_WINSVC )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS )

  add_library (huvi_renamed src/luvi_renamed.c)
endif ()

add_definitions(-DWITH_PLAIN_LUA)

include(LuaAddExecutable)

add_subdirectory(deps/miniz miniz.dir)
include_directories(deps/miniz)
list(APPEND HUVI_LIBRARIES miniz)

lua_add_executable(huvi
  ${winsvc}
  src/lminiz.c
  src/main.c
  src/luvi_loader.c
  src/lua/init.lua
  src/lua/luvipath.lua
  src/lua/luvibundle.lua
  ${luajit_vmdef}
  ${lpeg_re_lua}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,-E")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(list APPEND HUVI_LIBRARIES rt)
endif ()

target_link_libraries(huvi ${HUVI_LIBRARIES} ${EXTRA_LIBS})
set_target_properties(huvi PROPERTIES ENABLE_EXPORTS ON)

target_compile_definitions(huvi PRIVATE HUVI_VERSION="${HUVI_VERSION}")
target_compile_definitions(huvi PRIVATE ${HUVI_DEFINITIONS}) # Add any extra definitions, like the WITH_{LIB} defines

message("Configuration Summary:")
message("  HUVI_VERSION: ${HUVI_VERSION}")
message("  HUVI_DEFINITIONS: ${HUVI_DEFINITIONS}")
message("  HUVI_LIBRARIES: ${HUVI_LIBRARIES}")

###############################################################################
## Installation Targets
###############################################################################

install(TARGETS huvi DESTINATION bin)
