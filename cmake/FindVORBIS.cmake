# FindOggVorbis.cmake
# Find the ogg and vorbis includes and library
#
#  VORBIS_INCLUDE_DIR - where to find ogg.h, etc
#  VORBIS_LIBRARIES   - List of libraries when using ogg+vorbis
#  VORBIS_FOUND       - True if ogg+vorbis found

IF (VORBIS_INCLUDE_DIR)
  # Already in cache, be silent
  SET (VORBIS_FIND_QUIETLY TRUE)
ENDIF (VORBIS_INCLUDE_DIR)

FIND_PATH (VORBIS_INCLUDE_DIR vorbis/codec.h)

FIND_LIBRARY (VORBIS_LIBRARY NAMES vorbis
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

# handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (VORBIS DEFAULT_MSG 
  VORBIS_LIBRARY 
  VORBIS_INCLUDE_DIR)

IF (VORBIS_FOUND)
  SET (VORBIS_LIBRARIES ${VORBIS_LIBRARY})
ELSE (VORBIS_FOUND)
  SET (VORBIS_LIBRARIES)
ENDIF (VORBIS_FOUND)

MARK_AS_ADVANCED (VORBIS_LIBRARY VORBIS_INCLUDE_DIR)
