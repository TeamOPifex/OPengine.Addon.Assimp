
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

  message(STATUS "ASSIMP RELEASE MODE ${OPIFEX_OPTION_RELEASE}")
    if(${OPIFEX_OPTION_RELEASE})
        SET(BINARY_RELEASE_MODE "release")
        SET(assimp_lib "${ASSIMP_PATH}/bin/x64/release/assimp-vc140-mt.lib")
        SET(assimp_dll "${ASSIMP_PATH}/bin/x64/release/assimp-vc140-mt.dll")
    else()
        SET(BINARY_RELEASE_MODE "debug")
        SET(assimp_lib "${ASSIMP_PATH}/bin/x64/debug/assimp-vc140-mt.lib")
        SET(assimp_dll "${ASSIMP_PATH}/bin/x64/debug/assimp-vc140-mt.dll")
        message(STATUS "ASSIMP LIB: ${assimp_lib}")
    endif()

    message(STATUS "APP: ${APPLICATION_TARGET}")
    message(STATUS "COPY TO: ${PROJECT_BINARY_DIR}/${FOLDER}")
    add_custom_command(TARGET ${APPLICATION_TARGET} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${assimp_dll}" ${PROJECT_BINARY_DIR}/${FOLDER})

endmacro(ADDON_ASSIMP)

macro(ADDON_ASSIMP_LINK TEMP_RESULT)
  if(${OPIFEX_OPTION_RELEASE})
    SET(assimp_lib "${ASSIMP_PATH}/bin/x64/release/assimp-vc140-mt.lib")
  else()
    SET(assimp_lib "${ASSIMP_PATH}/bin/x64/debug/assimp-vc140-mt.lib")
  endif()
    SET(TEMP_RESULT "assimp;${assimp_lib}")
endmacro(ADDON_ASSIMP_LINK)

macro(ADDON_ASSIMP_DEFINES TEMP_RESULT)
    SET(TEMP_RESULT "")
endmacro(ADDON_ASSIMP_DEFINES)

macro(ADDON_ASSIMP_ASSETS TEMP_RESULT)
	SET(TEMP_RESULT "")
endmacro(ADDON_ASSIMP_ASSETS)
