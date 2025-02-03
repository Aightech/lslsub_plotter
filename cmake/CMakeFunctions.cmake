###### Function to get the version number of subproject ######
if(NOT COMMAND subproject_version)
  function(subproject_version subproject_name VERSION_VAR)
    # Read CMakeLists.txt for subproject and extract project() call(s) from it.
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/${subproject_name}/CMakeConfig.cmake" project_calls REGEX "[ \t]*set\\(")
    # For every project() call try to extract its VERSION option
    foreach(project_call ${project_calls})
      string(REGEX MATCH "VERSION[ ]+([^ )]+)" version_param "${project_call}")
      if(version_param)
	set(version_value "${CMAKE_MATCH_1}")
      endif()
    endforeach()
    if(version_value)
      set(${VERSION_VAR} "${version_value}" PARENT_SCOPE)
    else()
      message("WARNING: Cannot extract version for subproject '${subproject_name}'")
      message("WARNING: default version 1.0")
      set(${VERSION_VAR} 1.0 PARENT_SCOPE)
    endif()
  endfunction(subproject_version)
endif()


###### Get all folder of a directory ######
if(NOT COMMAND subdirlist)
  MACRO(subdirlist result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
      IF(IS_DIRECTORY ${curdir}/${child})
	LIST(APPEND dirlist ${child})
      ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
  ENDMACRO()
endif()


if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(ColourSlowBlink  "${Esc}[5m")
  set(ColourUnderline  "${Esc}[4m")
  set(ColourReverse  "${Esc}[7m")
  set(ColourCrossed  "${Esc}[9m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()
