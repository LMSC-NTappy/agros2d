project(plugins)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../hermes/cmake)
# Debug yes / no.
SET(AGROS_DEBUG NO)
SET(WITH_QT5 NO)

# Path to Qt5. Should be before CMake.vars to be able to change it.
IF(WITH_QT5)
  IF(WIN32)
    SET(CMAKE_PREFIX_PATH "C:\\Qt\\Qt5.0.2\\5.0.2\\msvc2010_opengl\\")
  ENDIF()
ENDIF()

SET(CMAKE_AGROS_DIRECTORY "${CMAKE_HOME_DIRECTORY}/../")

# For Win64
if(${CMAKE_CL_64})
  set(WIN64 YES)
else(${CMAKE_CL_64})
  set(WIN64 NO)
endif(${CMAKE_CL_64})

# Allow to override the default values in CMake.vars:
include(../CMake.vars OPTIONAL)
include(../hermes/CMake.vars OPTIONAL)

# Add QT.
include(../CMakeQt.cmake)

IF(WITH_QT5)
ELSE(WITH_QT5)
    INCLUDE(${QT_USE_FILE})
ENDIF(WITH_QT5)
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_PLUGIN)
ADD_DEFINITIONS(-DQT_SHARED)
ADD_DEFINITIONS(-DQT_DLL)

# Output paths.
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_AGROS_DIRECTORY}/libs)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_AGROS_DIRECTORY}/libs)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_AGROS_DIRECTORY}/libs)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_AGROS_DIRECTORY}/libs)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_AGROS_DIRECTORY}/libs)
  
IF(WIN32)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_AGROS_DIRECTORY}/libs)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_AGROS_DIRECTORY}/libs)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_AGROS_DIRECTORY}/libs)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_AGROS_DIRECTORY}/libs)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_AGROS_DIRECTORY}/libs)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_AGROS_DIRECTORY}/libs)
ENDIF(WIN32)

find_package(XSD REQUIRED)
find_package(XERCES REQUIRED)
find_package(PTHREAD REQUIRED)

FIND_LIBRARY(PLUGINS_AGROS_LIBRARY agros2d_library PATHS ${CMAKE_AGROS_DIRECTORY}/libs)
FIND_LIBRARY(PLUGINS_PYTHONLAB_LIBRARY agros2d_pythonlab_library PATHS ${CMAKE_AGROS_DIRECTORY}/libs)
FIND_LIBRARY(PLUGINS_AGROS_UTIL agros2d_util PATHS ${CMAKE_AGROS_DIRECTORY}/libs)

# Set global compiler parameters.
find_package(OpenMP REQUIRED)
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  INCLUDE_DIRECTORIES(/usr/include/google)
  IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  INCLUDE_DIRECTORIES(omp)
  ENDIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
ENDIF()

IF(MSVC)
  INCLUDE_DIRECTORIES(c:/hpfem/hermes/dependencies/include)
  INCLUDE_DIRECTORIES(d:/hpfem/hermes/dependencies/include)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /openmp /Zc:wchar_t")
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /NODEFAULTLIB:libcmtd /NODEFAULTLIB:libcmt")
ENDIF(MSVC)

# Python
FIND_PACKAGE(PythonLibs REQUIRED)

SET(CMAKE_SHARED_LINKING_FLAGS "${CMAKE_SHARED_LINKING_FLAGS} ${PYTHON_LIBRARIES}")
SET(CMAKE_MODULE_LINKING_FLAGS "${CMAKE_MODULE_LINKING_FLAGS} ${PYTHON_LIBRARIES}")
SET(CMAKE_EXE_LINKING_FLAGS "${CMAKE_EXE_LINKING_FLAGS} ${PYTHON_LIBRARIES}")

# Include OUR header files location
include(${CMAKE_AGROS_DIRECTORY}/IncludeSubdirs.cmake)

INCLUDE(${CMAKE_AGROS_DIRECTORY}/hermes/CMake.vars)
SET(CMAKE_MODULE_PATH ${CMAKE_AGROS_DIRECTORY}/hermes/cmake)

# Look for UMFPACK, AND MUMPS
IF(WITH_UMFPACK)
  FIND_PACKAGE(UMFPACK REQUIRED)
  INCLUDE_DIRECTORIES(${UMFPACK_INCLUDE_DIRS})
ENDIF()

IF(WITH_MUMPS)
  FIND_PACKAGE(MUMPS REQUIRED)
  
  IF(MSVC)
    FIND_PACKAGE(WINBLAS REQUIRED)
  ELSE(MSVC)
    IF (NOT LAPACK_FOUND)
      ENABLE_LANGUAGE(Fortran)
      FIND_PACKAGE(LAPACK REQUIRED)
      
      # If no error occured, LAPACK library has been found. Save the path to
      # it to cache, so that it will not be searched for during next 'cmake .'
      SET(LAPACK_LIBRARIES  ${LAPACK_LIBRARIES}
              CACHE STRING  "Path to LAPACK/BLAS libraries.")
      SET(LAPACK_FOUND      YES
              CACHE STRING  "Have LAPACK/BLAS libraries been found?")
    ENDIF (NOT LAPACK_FOUND)
    
    ADD_DEFINITIONS(-DWITH_BLAS)
  ENDIF(MSVC)
ENDIF()

SET(MUMPS_LIBRARIES ${MUMPS_CPLX_LIBRARIES})
LIST(APPEND MUMPS_LIBRARIES ${MUMPS_REAL_LIBRARIES})
INCLUDE_DIRECTORIES(${MUMPS_INCLUDE_DIR})

# modules

{{#SOURCE}}
ADD_SUBDIRECTORY({{ID}}){{/SOURCE}}

