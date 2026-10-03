# Relocatable customer bundle; matching C ABI v1 shared and static libraries.
get_filename_component(_GradientData_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(WIN32)
  set(_GradientData_SHARED "${_GradientData_ROOT}/windows-x86_64/gradient_data_c.dll")
  set(_GradientData_IMPLIB "${_GradientData_ROOT}/windows-x86_64/gradient_data_c.dll.lib")
  set(_GradientData_STATIC "${_GradientData_ROOT}/windows-x86_64/gradient_data_c.lib")
elseif(UNIX AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(_GradientData_SHARED "${_GradientData_ROOT}/linux-x86_64/libgradient_data_c.so")
  set(_GradientData_STATIC "${_GradientData_ROOT}/linux-x86_64/libgradient_data_c.a")
else()
  set(GradientData_FOUND FALSE)
  set(GradientData_NOT_FOUND_MESSAGE "This bundle supports Linux and Windows x86-64 only")
  return()
endif()
if(NOT EXISTS "${_GradientData_ROOT}/include/gradient_data.h" OR
   NOT EXISTS "${_GradientData_ROOT}/include/gradient_data.hpp" OR
   NOT EXISTS "${_GradientData_SHARED}" OR NOT EXISTS "${_GradientData_STATIC}" OR
   (WIN32 AND NOT EXISTS "${_GradientData_IMPLIB}"))
  set(GradientData_FOUND FALSE)
  set(GradientData_NOT_FOUND_MESSAGE "Incomplete Gradient Data customer SDK bundle")
  return()
endif()
if(NOT WIN32)
  find_package(Threads REQUIRED)
endif()
if(NOT TARGET GradientData::gradient_data)
  add_library(GradientData::gradient_data SHARED IMPORTED)
  set_target_properties(GradientData::gradient_data PROPERTIES
    IMPORTED_LOCATION "${_GradientData_SHARED}"
    INTERFACE_INCLUDE_DIRECTORIES "${_GradientData_ROOT}/include")
  if(WIN32)
    set_target_properties(GradientData::gradient_data PROPERTIES IMPORTED_IMPLIB "${_GradientData_IMPLIB}")
  endif()
endif()
if(NOT TARGET GradientData::static)
  add_library(GradientData::static STATIC IMPORTED)
  set_target_properties(GradientData::static PROPERTIES
    IMPORTED_LOCATION "${_GradientData_STATIC}"
    INTERFACE_INCLUDE_DIRECTORIES "${_GradientData_ROOT}/include"
    INTERFACE_COMPILE_DEFINITIONS DGRAD_STATIC)
  if(WIN32)
    set_target_properties(GradientData::static PROPERTIES
      INTERFACE_LINK_LIBRARIES "advapi32;userenv;ws2_32;bcrypt;ntdll;kernel32;synchronization")
  else()
    set_target_properties(GradientData::static PROPERTIES
      INTERFACE_LINK_LIBRARIES "Threads::Threads;${CMAKE_DL_LIBS};m;rt;util")
  endif()
endif()
# Preserve the original imported shared target and add a descriptive alias.
if(NOT TARGET GradientData::shared)
  add_library(GradientData::shared INTERFACE IMPORTED)
  set_target_properties(GradientData::shared PROPERTIES
    INTERFACE_LINK_LIBRARIES GradientData::gradient_data)
endif()
set(GradientData_VERSION "0.2.0-rc.2")
set(GradientData_FOUND TRUE)
