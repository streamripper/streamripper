# FindIBERTY.cmake
# Find the libmad includes and library
#
#  IBERTY_INCLUDE_DIR - where to find mad.h, etc.
#  IBERTY_LIBRARIES   - List of libraries when using mad
#  IBERTY_FOUND       - True if mad found.
#
# In debian, it is in binutils-dev

IF (IBERTY_INCLUDE_DIR)
  # Already in cache, be silent
  SET (IBERTY_FIND_QUIETLY TRUE)
ENDIF (IBERTY_INCLUDE_DIR)

FIND_PATH (IBERTY_INCLUDE_DIR libiberty.h)
FIND_LIBRARY (IBERTY_LIBRARIES NAMES iberty)

# handle the QUIETLY and REQUIRED arguments and set IBERTY_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (IBERTY DEFAULT_MSG 
  IBERTY_LIBRARY 
  IBERTY_INCLUDE_DIR)
IF (IBERTY_FOUND)
  SET (IBERTY_LIBRARIES ${IBERTY_LIBRARY})
ELSE (IBERTY_FOUND)
  SET (IBERTY_LIBRARIES)
ENDIF (IBERTY_FOUND)

MARK_AS_ADVANCED (IBERTY_LIBRARY IBERTY_INCLUDE_DIR)
