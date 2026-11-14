#ifndef RTWEEKEND_H
#define RTWEEKEND_H
//==============================================================================================
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

#ifdef RTW_MULTITHREADED
#include <atomic>
#include <random>
#endif


// C++ Std Usings

using std::make_shared;
using std::shared_ptr;


// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;


// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

#ifdef RTW_MULTITHREADED
// std::rand() is not safe to call from several threads at once, so the multi-threaded
// build (the animation) gives every thread its own generator. Seeds are handed out in the
// order threads first ask for a number, so the main thread (which builds the scene) always
// gets the same seed and the scene looks the same on every run.
inline double random_double() {
    // Returns a random real in [0,1).
    static std::atomic<unsigned> next_seed(0);
    thread_local std::mt19937 generator(5489u + 7919u * next_seed++);
    return generator() * (1.0 / 4294967296.0);
}
#else
inline double random_double() {
    // Returns a random real in [0,1).
    return std::rand() / (RAND_MAX + 1.0);
}
#endif

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}

inline int random_int(int min, int max) {
    // Returns a random integer in [min,max].
    return int(random_double(min, max+1));
}


// Common Headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"


#endif
