CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT(otf2utils)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
enable_testing()

# Source files
SET(OTF2CSV_SOURCES otf22csv.c otf22csv_arguments.c otf22csv_handlers.c)

# Options
OPTION(OTF2 "Enable to build otf2-based tools" ON)

# check for otf22paje dependencies
IF(OTF2)
  FIND_PACKAGE(OTF2 REQUIRED)
ENDIF(OTF2)

###################
##### TARGETS #####
###################

IF(OTF2_FOUND)
  # otf22csv
  INCLUDE_DIRECTORIES(${OTF2_INCLUDE_PATH})
  LINK_DIRECTORIES(${OTF2_LIBRARY_PATH})
  ADD_EXECUTABLE(otf22csv ${OTF2CSV_SOURCES})
  TARGET_LINK_LIBRARIES(otf22csv otf2)
  TARGET_LINK_LIBRARIES(otf22csv m)
ENDIF(OTF2_FOUND)

# Installation part
IF(OTF2_FOUND)
  INSTALL(PROGRAMS otf22csv DESTINATION bin)
ENDIF(OTF2_FOUND)
