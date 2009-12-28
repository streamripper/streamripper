# FindOgg.cmake
# Find the ogg includes and library
#
#  OGG_INCLUDE_DIR - where to find ogg.h, etc
#  OGG_LIBRARIES   - List of libraries when using ogg
#  OGG_FOUND       - True if ogg found

IF (OGG_INCLUDE_DIR)
  # Already in cache, be silent
  SET (OGG_FIND_QUIETLY TRUE)
ENDIF (OGG_INCLUDE_DIR)

FIND_PATH (OGG_INCLUDE_DIR ogg/ogg.h)

FIND_LIBRARY (OGG_LIBRARY NAMES ogg
  PATHS
  $ENV{VORBISDIR}/lib
  $ENV{VORBISDIR}
  $ENV{OGGDIR}/lib
  $ENV{OGGDIR}
  /usr/local/lib
  /usr/lib
  /sw/lib
  /opt/local/lib
  /opt/csw/lib
  /opt/lib
  )

# handle the QUIETLY and REQUIRED arguments and set OGG_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (OGG DEFAULT_MSG 
  OGG_LIBRARY 
  OGG_INCLUDE_DIR)

IF (OGG_FOUND)
  SET (OGG_LIBRARIES ${OGG_LIBRARY})
ELSE (OGG_FOUND)
  SET (OGG_LIBRARIES)
ENDIF (OGG_FOUND)

MARK_AS_ADVANCED (OGG_LIBRARY OGG_INCLUDE_DIR)
