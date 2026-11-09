#ifndef RTW_STB_IMAGE_WRITE_H
#define RTW_STB_IMAGE_WRITE_H
//==============================================================================================
// Wrapper around the stb_image_write library, used to save the rendered image as PNG.
// PNG opens in the default image viewer on Windows, macOS and Linux, unlike PPM.
//==============================================================================================

// Disable strict warnings for this header from the Microsoft Visual C++ compiler.
#ifdef _MSC_VER
    #pragma warning (push, 0)
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

// Restore MSVC compiler warnings
#ifdef _MSC_VER
    #pragma warning (pop)
#endif


#endif
