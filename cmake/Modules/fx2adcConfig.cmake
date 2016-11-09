INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_FX2ADC fx2adc)

FIND_PATH(
    FX2ADC_INCLUDE_DIRS
    NAMES fx2adc/api.h
    HINTS $ENV{FX2ADC_DIR}/include
        ${PC_FX2ADC_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    FX2ADC_LIBRARIES
    NAMES gnuradio-fx2adc
    HINTS $ENV{FX2ADC_DIR}/lib
        ${PC_FX2ADC_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FX2ADC DEFAULT_MSG FX2ADC_LIBRARIES FX2ADC_INCLUDE_DIRS)
MARK_AS_ADVANCED(FX2ADC_LIBRARIES FX2ADC_INCLUDE_DIRS)

