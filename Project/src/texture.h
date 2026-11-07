#ifndef TEXTURE_H
#define TEXTURE_H
//==============================================================================================
// This header defines the texture system used to calculate surface colors during rendering.
// 
// A texture provides a color based on the surface texture coordinates (u, v) and the 3D point p.
// 
// The base texture class defines a common interface that all texture types must implement.
// 
// Solid colors, checker patterns, image textures, and procedural noise textures are supported.
// 
// Textures are used by materials to determine the color and light attenuation at each ray hit.
// Because textures are polymorphic, a material can use any texture type through a shared pointer.
//==============================================================================================

#include "perlin.h"
#include "rtw_stb_image.h"


class texture {
  public:
    virtual ~texture() = default;

    virtual color value(double u, double v, const point3& p) const = 0;
};

// Represents a single uniform color across the entire surface.
// It ignores the texture coordinates and 3D position and always returns the same color.
class solid_color : public texture {
  public:
    solid_color(const color& albedo) : albedo(albedo) {}

    solid_color(double red, double green, double blue) : solid_color(color(red,green,blue)) {}

    color value(double u, double v, const point3& p) const override {
        return albedo;
    }

  private:
    color albedo;
};

// Represents a 3D checkerboard pattern made from two alternating textures.
// The scale controls the size of the checker squares, and the pattern is selected using the point's X, Y, and Z coordinates.
class checker_texture : public texture {
  public:
    checker_texture(double scale, shared_ptr<texture> even, shared_ptr<texture> odd)
      : inv_scale(1.0 / scale), even(even), odd(odd) {}

    checker_texture(double scale, const color& c1, const color& c2)
      : checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

    color value(double u, double v, const point3& p) const override {
        auto xInteger = int(std::floor(inv_scale * p.x()));
        auto yInteger = int(std::floor(inv_scale * p.y()));
        auto zInteger = int(std::floor(inv_scale * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }

  private:
    double inv_scale;
    shared_ptr<texture> even;
    shared_ptr<texture> odd;
};


// Represents a texture loaded from an image file.
// The surface's UV coordinates are converted into pixel coordinates to retrieve the corresponding image color.
// If the image cannot be loaded, cyan is returned to make the missing texture visible during debugging.
class image_texture : public texture {
  public:
    image_texture(const char* filename) : image(filename) {}

    color value(double u, double v, const point3& p) const override {
        // If we have no texture data, then return solid cyan as a debugging aid.
        if (image.height() <= 0) return color(0,1,1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0,1).clamp(u);
        v = 1.0 - interval(0,1).clamp(v);  // Flip V to image coordinates

        auto i = int(u * image.width());
        auto j = int(v * image.height());
        auto pixel = image.pixel_data(i,j);

        auto color_scale = 1.0 / 255.0;
        return color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
    }

  private:
    rtw_image image;
};


// Represents a procedural texture generated from Perlin noise and turbulence.
// It does not require an image file and calculates a natural-looking pattern from the 3D point.
// This type of texture can be used to create effects such as marble, clouds, terrain, or other irregular surfaces.
class noise_texture : public texture {
  public:
    noise_texture(double scale) : scale(scale) {}

    color value(double u, double v, const point3& p) const override {
        return color(.5, .5, .5) * (1 + std::sin(scale * p.z() + 10 * noise.turb(p, 7)));
    }

  private:
    perlin noise;
    double scale;
};


#endif
