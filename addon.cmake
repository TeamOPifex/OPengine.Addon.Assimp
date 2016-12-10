
message(STATUS "Prep for addon: ASSIMP")

SET(ASSIMP_PATH ${CMAKE_CURRENT_LIST_DIR})

macro(ADDON_ASSIMP_PROJECT ADDON_BUILD)
    message(STATUS "Adding project with build folder of ${ADDON_BUILD}")
    add_subdirectory (${ASSIMP_PATH} "${ADDON_BUILD}/assimp_build")
endmacro(ADDON_ASSIMP_PROJECT)

macro(ADDON_ASSIMP_INCLUDES)
    include_directories(${ASSIMP_PATH}/include/)
endmacro(ADDON_ASSIMP_INCLUDES)

macro(ADDON_ASSIMP APPLICATION_TARGET FOLDER)
	
    SET(assimp_lib "${ASSIMP_PATH}/bin/x64/assimp-vc130-mtd.lib")
    SET(assimp_dll "${ASSIMP_PATH}/bin/x64/assimp-vc130-mtd.dll")

    if(${OPIFEX_OPTION_RELEASE})
      SET(BINARY_RELEASE_MODE "release")
    else()
      SET(BINARY_RELEASE_MODE "debug")
    endif()

    message(STATUS "APP: ${APPLICATION_TARGET}")
    message(STATUS "COPY TO: ${PROJECT_BINARY_DIR}/${FOLDER}")
    add_custom_command(TARGET ${APPLICATION_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${assimp_dll}" ${PROJECT_BINARY_DIR}/${FOLDER})

endmacro(ADDON_ASSIMP)

macro(ADDON_ASSIMP_LINK TEMP_RESULT)
    SET(TEMP_RESULT "assimp")
endmacro(ADDON_ASSIMP_LINK)

macro(ADDON_ASSIMP_DEFINES TEMP_RESULT)
    SET(TEMP_RESULT "")
endmacro(ADDON_ASSIMP_DEFINES)

macro(ADDON_ASSIMP_ASSETS TEMP_RESULT)
	SET(TEMP_RESULT "")
endmacro(ADDON_ASSIMP_ASSETS)