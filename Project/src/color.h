#ifndef COLOR_H
#define COLOR_H
//==============================================================================================
// WRITEING COLOR IN PPM FORMAT
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

// function to convert a linear color to three gamma-corrected bytes in [0, 255]
inline void color_to_bytes(const color& pixel_color, unsigned char out[3]) {
    // Apply a linear to gamma transform for gamma 2
    auto r = linear_to_gamma(pixel_color.x());
    auto g = linear_to_gamma(pixel_color.y());
    auto b = linear_to_gamma(pixel_color.z());

    // Translate the [0,1] component values to the byte range [0,255]
    static const interval intensity(0.000, 0.999);
    out[0] = static_cast<unsigned char>(256 * intensity.clamp(r));
    out[1] = static_cast<unsigned char>(256 * intensity.clamp(g));
    out[2] = static_cast<unsigned char>(256 * intensity.clamp(b));
}

// function to write a color to an output stream, applying gamma correction and clamping
// the color values to the range [0, 255].
// with this we write ppm files
inline void write_color(std::ostream& out, const color& pixel_color) {
    unsigned char rgb[3];
    color_to_bytes(pixel_color, rgb);

    // Write out the pixel color components.
    out << int(rgb[0]) << ' ' << int(rgb[1]) << ' ' << int(rgb[2]) << '\n';
}


#endif
