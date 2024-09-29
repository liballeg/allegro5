set (IPHONE 1)
set (A5O_CFG_OPENGLES 1)
set (IOS_PLATFORM "iphoneos" CACHE STRING "iOS platform (iphoneos or iphonesimulator)")
mark_as_advanced(IOS_PLATFORM)
set (CMAKE_OSX_SYSROOT "${IOS_PLATFORM}")

set (CMAKE_MACOSX_BUNDLE YES)
set (CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
