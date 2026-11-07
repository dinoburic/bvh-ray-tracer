#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "texture.h"


// Base class for all surface materials in the ray tracer.
// It defines the common interface for emitting light and scattering incoming rays.
// Specific materials override these methods to implement their own interaction with light.
class material {
  public:
    virtual ~material() = default;

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    }

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
        return false;
    }
};


// Represents a matte, diffuse material such as chalk, paper, or unfinished wood.
// It scatters incoming rays in random directions around the surface normal and does not create sharp reflections.
// Its texture or albedo determines how much light is absorbed or reflected at each point.
class lambertian : public material {
  public:
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = tex->value(rec.u, rec.v, rec.p);
        return true;
    }

  private:
    shared_ptr<texture> tex;
};


// Represents a reflective metallic material such as a mirror or polished metal.
// It reflects rays around the surface normal, while fuzz controls how blurry or rough the reflection is.
// A fuzz value of zero produces a sharp reflection, while larger values produce a more scattered reflection.
class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

  private:
    color albedo;
    double fuzz;
};


// Represents a transparent material such as glass, water, or ice.
// It can either reflect a ray or refract it as the ray passes between materials with different refractive indices.
// The choice between reflection and refraction is based on the viewing angle and Fresnel reflectance.
class dielectric : public material {
  public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

  private:
    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    double refraction_index;

    static double reflectance(double cosine, double refraction_index) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*std::pow((1 - cosine),5);
    }
};


// Represents a material that emits light instead of merely reflecting it.
// The texture defines the emitted color and intensity, making the object act as a light source in the scene.
// It does not scatter incoming rays because its main purpose is to illuminate other objects.
class diffuse_light : public material {
  public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}
    diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)) {}

    color emitted(double u, double v, const point3& p) const override {
        return tex->value(u, v, p);
    }

  private:
    shared_ptr<texture> tex;
};


// Represents a material inside a participating medium such as smoke, fog, or colored mist.
// It scatters rays equally in all directions, regardless of the incoming direction or surface normal.
// The texture controls the color and attenuation of light while it travels through the medium.
class isotropic : public material {
  public:
    isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        scattered = ray(rec.p, random_unit_vector());
        attenuation = tex->value(rec.u, rec.v, rec.p);
        return true;
    }

  private:
    shared_ptr<texture> tex;
};


#endif
