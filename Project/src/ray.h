#ifndef RAY_H
#define RAY_H
//==============================================================================================
// headers for ray class, which represents a ray in 3D space with an origin and direction
// P(t) = origin + t * direction
// p - point along the ray at parameter t
// t - parameter along the ray, can be thought of as "time" or "distance" along the ray
// origin - starting point of the ray
// direction - direction vector of the ray, which can be normalized or not
//==============================================================================================

#include "vec3.h"


class ray {
  public:
    ray() {}

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    const point3& origin() const  { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const {
        return orig + t*dir;
    }

  private:
    point3 orig;
    vec3 dir;
};


#endif
