cmake_minimum_required(VERSION 2.8)

project(assimp)

message(STATUS "Trying to add the addon ${CMAKE_CURRENT_LIST_DIR}")

include_directories(${CMAKE_CURRENT_LIST_DIR}/include/)
include_directories(${OPIFEX_ENGINE_REPOSITORY})

add_external_opifex_includes()

add_source_group(/ "${CMAKE_CURRENT_LIST_DIR}/source/" APPLICATION_SRC)
add_source_group(/ "${CMAKE_CURRENT_LIST_DIR}/include/" APPLICATION_SRC)
add_library(assimp ${APPLICATION_SRC})
add_opifex_defines()


if(${OPIFEX_OS_WIN64})

	# copy_to_binaries(${CMAKE_CURRENT_LIST_DIR}/bin/win64/assimp-vc130-mtd.dll)
	message(STATUS "ASSIMP Trying to add the addon Release Mode: ${OPIFEX_OPTION_RELEASE}")

	if(${OPIFEX_OPTION_RELEASE})
		target_link_libraries(assimp ${CMAKE_CURRENT_LIST_DIR}/bin/win64/release/assimp-vc140-mt.lib)
	else()
		target_link_libraries(assimp ${CMAKE_CURRENT_LIST_DIR}/bin/win64/debug/assimp-vc140-mt.lib)
	endif()

else()
	target_link_libraries(assimp ${CMAKE_CURRENT_LIST_DIR}/bin/osx/x64/debug/libassimp.a libz.dylib)

endif()


set_source_files_properties( ${APPLICATION_SRC} PROPERTIES LANGUAGE "CXX" )

set_property(TARGET assimp PROPERTY FOLDER "Addons")
