# - Try to find Tesseract-OCR
# Once done, this will define
#
#  Tesseract_FOUND - system has Tesseract
#  Tesseract_INCLUDE_DIRS - the Tesseract include directories
#  Tesseract_LIBRARIES - link these to use Tesseract

include(LibFindMacros)

# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(Tesseract_PKGCONF Tesseract)

# Include dir
find_path(Tesseract_INCLUDE_BASEAPI_DIR
  NAMES tesseract/baseapi.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/
)
set(Tesseract_INCLUDE_BASEAPI_DIR /Users/djuarez/CLionProjects/openalpr/libraries/tesseract-ocr/api/)
find_path(Tesseract_INCLUDE_CCSTRUCT_DIR
  NAMES publictypes.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccstruct/
)
set(Tesseract_INCLUDE_CCSTRUCT_DIR ${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccstruct/)
find_path(Tesseract_INCLUDE_CCMAIN_DIR
  NAMES thresholder.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccmain/
)
set(Tesseract_INCLUDE_CCMAIN_DIR ${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccmain/)

find_path(Tesseract_INCLUDE_CCUTIL_DIR
  NAMES platform.h
  HINTS "/usr/include"
        "/usr/include/tesseract"
        "/usr/local/include"
        "/usr/local/include/tesseract"
        ${Tesseract_PKGCONF_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccutil/
)

set(Tesseract_INCLUDE_CCUTIL_DIR ${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/ccutil/)


# Finally the library itself
find_library(Tesseract_LIB
  NAMES tesseract tesseract-static libtesseract304-static tess
  HINTS "/usr/lib"
        "/usr/local/lib"
        ${Tesseract_PKGCONF_LIBRARY_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/.libs
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/vs2010/LIB_Release
)
set(Tesseract_LIB tess)

find_library(Leptonica_LIB
  NAMES liblept170 liblept lept
  HINTS "/usr/lib"
        "/usr/local/lib"
        ${Tesseract_PKGCONF_LIBRARY_DIRS}
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/api/.libs
		${CMAKE_SOURCE_DIR}/../libraries/tesseract-ocr/vs2010/LIB_Release
)

set(Leptonica_LIB lept)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.

set(Tesseract_PROCESS_LIBS Tesseract_LIB Leptonica_LIB Tesseract_LIBRARIES)
libfind_process(Tesseract)