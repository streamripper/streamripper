# FindIBERTY.cmake
# Find the libiberty includes and library
#
#  IBERTY_INCLUDE_DIR - where to find iberty.h, etc.
#  IBERTY_LIBRARIES   - List of libraries when using iberty
#  IBERTY_FOUND       - True if iberty found.
#
# In debian, it is in libiberty-dev (used to be in binutils-dev)

IF (IBERTY_INCLUDE_DIR)
  # Already in cache, be silent
  SET (IBERTY_FIND_QUIETLY TRUE)
ENDIF (IBERTY_INCLUDE_DIR)

FIND_PATH (IBERTY_INCLUDE_DIR libiberty.h PATH_SUFFIXES libiberty)
FIND_LIBRARY (IBERTY_LIBRARY NAMES iberty)

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
