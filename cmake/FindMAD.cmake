# FindMAD.cmake
# Find the libmad includes and library
#
#  MAD_INCLUDE_DIR - where to find mad.h, etc.
#  MAD_LIBRARIES   - List of libraries when using mad
#  MAD_FOUND       - True if mad found.

IF (MAD_INCLUDE_DIR)
  # Already in cache, be silent
  SET (MAD_FIND_QUIETLY TRUE)
ENDIF (MAD_INCLUDE_DIR)

FIND_PATH (MAD_INCLUDE_DIR mad.h)

SET (MAD_NAMES mad)
FIND_LIBRARY (MAD_LIBRARY NAMES ${MAD_NAMES})

# handle the QUIETLY and REQUIRED arguments and set MAD_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (MAD DEFAULT_MSG 
  MAD_LIBRARY 
  MAD_INCLUDE_DIR)

IF (MAD_FOUND)
  SET (MAD_LIBRARIES ${MAD_LIBRARY})
ELSE (MAD_FOUND)
  SET (MAD_LIBRARIES)
ENDIF (MAD_FOUND)

MARK_AS_ADVANCED (MAD_LIBRARY MAD_INCLUDE_DIR)
