if(DEFINED KUMIR2_CONFIGURED)
	return()
endif(DEFINED KUMIR2_CONFIGURED)

set(KUMIR2_CONFIGURED 1)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/kumir2/distribution_build_config.cmake")
	include("${CMAKE_CURRENT_LIST_DIR}/kumir2/distribution_build_config.cmake")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/kumir2/kumir2_common.cmake")

if(WIN32)
	include("${CMAKE_CURRENT_LIST_DIR}/kumir2/kumir2_win32.cmake")
elseif(EMSCRIPTEN)
	include("${CMAKE_CURRENT_LIST_DIR}/kumir2/kumir2_wasm.cmake")
elseif(APPLE)
	include("${CMAKE_CURRENT_LIST_DIR}/kumir2/kumir2_apple.cmake")
else()
	include("${CMAKE_CURRENT_LIST_DIR}/kumir2/kumir2_linux.cmake")
endif()
