# WebAssembly (Emscripten) build: Qt for WebAssembly, configured with its qt-cmake.
# All libraries and plugins are linked statically (KUMIR2_STATIC_PLUGINS),
# resources are packed into the Emscripten virtual file system.

if(NOT USE_QT EQUAL 6)
    message(FATAL_ERROR "WebAssembly build requires Qt6: pass -DUSE_QT=6")
endif()
if(NOT KUMIR2_STATIC_PLUGINS)
    message(FATAL_ERROR "WebAssembly build requires KUMIR2_STATIC_PLUGINS=ON")
endif()

# Same layout as Linux build; in the browser the executable runs from "/",
# so resources are expected at /share/kumir2 (see resolvePath in kumir2-launcher.cpp)
set(KUMIR2_EXEC_DIR "bin")
set(KUMIR2_LIBS_DIR "lib/kumir2")
set(KUMIR2_PLUGINS_DIR "lib/kumir2/plugins")
set(KUMIR2_RESOURCES_DIR "share/kumir2")
# Not used in the browser, but referenced by install rules
set(KUMIR2_XDG_APPLICATIONS_DIR "share/applications")
set(KUMIR2_XDG_ICONS_DIR "share/icons")
set(KUMIR2_XDG_PIXMAPS_DIR "share/pixmaps")
set(KUMIR2_XDG_MIME_PACKAGES_DIR "share/mime/packages")
set(KUMIR2_LIBEXECS_DIR "bin")
set(KUMIR2_SDK_SCRIPTS_DIR "${KUMIR2_INSTALL_PREFIX}/${KUMIR2_LIBS_DIR}")
set(KUMIR2_SDK_CMAKE_DIR "lib/cmake/Kumir2")
set(KUMIR2_SDK_SRC_DIR "${KUMIR2_RESOURCES_DIR}/src")

set(CMAKE_CXX_EXTENSIONS "OFF")

# Compile flags (no -fPIC: everything is linked statically).
# -pthread: multithreaded Qt for WebAssembly requires all objects to be built
# with atomics (Qt adds it only to targets using Qt; VM and KStdlib do not)
set(KUMIR2_CXXFLAGS "-pthread -fno-exceptions -Wall -W -Wno-deprecated-declarations -Wno-unused-parameter")
set(KUMIR2_CXXFLAGS_Release "-O2 -UNDEBUG")
set(KUMIR2_CXXFLAGS_RelWithDebInfo "-g1 -O2 -UNDEBUG -UQT_NO_DEBUG")
set(KUMIR2_CXXFLAGS_Debug "-g -O1")

# Linkage flags: Qt adds the Emscripten flags it needs to executables;
# here only Kumir-specific ones
set(KUMIR2_LIBRARY_LINKER_FLAGS "")
set(KUMIR2_PLUGIN_LINKER_FLAGS "")
set(KUMIR2_TOOL_LINKER_FLAGS "")
# Resources (built into ${CMAKE_BINARY_DIR}/share) are preloaded to /share
set(KUMIR2_LAUNCHER_LINKER_FLAGS "--preload-file ${CMAKE_BINARY_DIR}/share@/share")

# Existing Widgets dialogs use nested event loops (unsaved-file prompts,
# preferences, actor dialogs). Qt supports these in the browser with Asyncify.
option(KUMIR2_WASM_ASYNCIFY "Support existing modal Qt dialogs in the browser" ON)
if(KUMIR2_WASM_ASYNCIFY)
    set(KUMIR2_LAUNCHER_LINKER_FLAGS "${KUMIR2_LAUNCHER_LINKER_FLAGS} -sASYNCIFY -Os -sASYNCIFY_STACK_SIZE=65536")
endif()

# Keep function names in the .wasm file for readable browser stack traces
option(KUMIR2_WASM_PROFILING_FUNCS "Keep function names in .wasm (debugging)" OFF)
if(KUMIR2_WASM_PROFILING_FUNCS)
    set(KUMIR2_LAUNCHER_LINKER_FLAGS "${KUMIR2_LAUNCHER_LINKER_FLAGS} --profiling-funcs")
endif()
