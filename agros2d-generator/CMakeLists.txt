PROJECT(${AGROS_GENERATOR})

INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/util)
INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/pythonlab-library)
INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/agros2d-library)
INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/3rdparty)
INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/3rdparty/ctemplate)
IF(WIN32)
    INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/3rdparty/ctemplate/windows)
ELSE(WIN32)
    INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/3rdparty/ctemplate/linux)
ENDIF(WIN32)
INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/3rdparty/paralution/src)

SET(SOURCES main.cpp 
    generator.cpp 
    generator_module.cpp 
    generator_coupling.cpp
    generator_weakforms.cpp
    generator_postprocessor.cpp
    generator_documentation.cpp
    parser.cpp
    parser_lexical_analyser.cpp
)

SET(HEADERS generator.h
    generator_module.h
    generator_coupling.h
    parser.h
)

IF(WIN32)
  # XSDCXX
  # message("XML cpp/h files")
  FILE(GLOB generator_xsd_files "${CMAKE_HOME_DIRECTORY}/resources/xsd/*.xsd")
  foreach(XSD_FILE ${generator_xsd_files})
    GET_FILENAME_COMPONENT(basefile ${XSD_FILE} NAME_WE)
    ADD_CUSTOM_TARGET(generator-${basefile} ALL DEPENDS ${CMAKE_HOME_DIRECTORY}/resources_source/classes/${basefile}.cpp ${CMAKE_HOME_DIRECTORY}/resources_source/classes/${basefile}.h)

    # message("\txsdcxx ${basefile}")

    ADD_CUSTOM_COMMAND(
	  COMMAND  ${XSD_BIN} ARGS cxx-tree --generate-ostream --hxx-suffix .h --cxx-suffix .cpp --root-element-first --generate-polymorphic --output-dir ${CMAKE_HOME_DIRECTORY}/resources_source/classes --generate-serialization ${XSD_FILE}
	  DEPENDS ${XSD_FILE}
	  OUTPUT  ${CMAKE_HOME_DIRECTORY}/resources_source/classes/${basefile}.cpp ${CMAKE_HOME_DIRECTORY}/resources_source/classes/${basefile}.h)
  endforeach()

  SET(SOURCES_XML ${CMAKE_HOME_DIRECTORY}/resources_source/classes/module_xml.cpp
    ${CMAKE_HOME_DIRECTORY}/resources_source/classes/structure_xml.cpp
    ${CMAKE_HOME_DIRECTORY}/resources_source/classes/material_xml.cpp
    ${CMAKE_HOME_DIRECTORY}/resources_source/classes/problem_a2d_31_xml.cpp
    ${CMAKE_HOME_DIRECTORY}/resources_source/classes/form_xml.cpp
    ${CMAKE_HOME_DIRECTORY}/resources_source/classes/test_xml.cpp
)
ENDIF(WIN32)
    
ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES} ${SOURCES_XML} ${HEADERS})

IF(WIN32)
	foreach(XSD_FILE ${generator_xsd_files})
	  GET_FILENAME_COMPONENT(basefile ${XSD_FILE} NAME_WE)
	  ADD_DEPENDENCIES(${PROJECT_NAME} generator-${basefile})
	endforeach()
ENDIF(WIN32)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${AGROS_LIBRARY} ${PYTHONLAB_LIBRARY} ${CTEMPLATE_LIBRARY} ${QCUSTOMPLOT_LIBRARY} ${QUAZIP_LIBRARY} ${STB_TRUETYPE_LIBRARY})
IF(WITH_QT5)
    QT5_USE_MODULES(${PROJECT_NAME} Core Widgets Network Xml XmlPatterns WebKit WebKitWidgets OpenGL)
ENDIF(WITH_QT5)
