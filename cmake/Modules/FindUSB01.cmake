# povik 2016-07-25: Took FindUSB from GR and replaced "1.0" with "0.1". Likely broken.

if(NOT LIBUSB_FOUND)
  pkg_check_modules (LIBUSB_PKG libusb-0.1)
  find_path(LIBUSB_INCLUDE_DIR NAMES usb.h
    PATHS
    ${LIBUSB_PKG_INCLUDE_DIRS}
    /usr/include/libusb-0.1
    /usr/include
    /usr/local/include
  )

  find_library(LIBUSB_LIBRARIES NAMES usb-0.1 usb
    PATHS
    ${LIBUSB_PKG_LIBRARY_DIRS}
    /usr/lib
    /usr/local/lib
  )

if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
  set(LIBUSB_FOUND TRUE CACHE INTERNAL "libusb-0.1 found")
  message(STATUS "Found libusb-0.1: ${LIBUSB_INCLUDE_DIR}, ${LIBUSB_LIBRARIES}")
else(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
  set(LIBUSB_FOUND FALSE CACHE INTERNAL "libusb-0.1 found")
  message(STATUS "libusb-0.1 not found.")
endif(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)

mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

endif(NOT LIBUSB_FOUND)
