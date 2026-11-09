#ifndef COLOR_H
#define COLOR_H
//==============================================================================================
// COLOR TYPE AND CONVERSION OF PIXEL COLORS TO BYTES
//==============================================================================================

#include "interval.h"
#include "vec3.h"


using color = vec3;

// function to convert a linear color component to a gamma-corrected component
inline double linear_to_gamma(double linear_component)
{
    if (linear_component > 0)
        return std::sqrt(linear_component);

    return 0;
}

// Converts a pixel color to three bytes (R, G, B) and stores them at out[0..2].
// Gamma correction is applied and the values are clamped to the range [0, 255],
// which is what the PNG image format expects.
inline void color_to_bytes(const color& pixel_color, unsigned char* out) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // Apply a linear to gamma transform for gamma 2
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    // Translate the [0,1] component values to the byte range [0,255]
    static const interval intensity(0.000, 0.999);
    out[0] = static_cast<unsigned char>(256 * intensity.clamp(r));
    out[1] = static_cast<unsigned char>(256 * intensity.clamp(g));
    out[2] = static_cast<unsigned char>(256 * intensity.clamp(b));
}

#endif
