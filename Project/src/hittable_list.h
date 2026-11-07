#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H
//==============================================================================================
// This header defines hittable_list, a container for multiple ray-traceable objects.
// 
// It inherits from hittable, allowing the entire collection to be treated as ONE OBJECT.
// 
// The list stores spheres, boxes, lights, transformations, and other hittable objects.
// 
// When a ray is tested, every object is checked and the closest intersection is recorded.
// 
// The class maintains a combined axis-aligned bounding box for all objects in the list.
// 
// The add method inserts an object and updates the collection's bounding box.
// 
// In main.cc, hittable_list is used to build complete scenes, usually through a variable named world.
// 
// Objects are added to the scene with world.add(...), and the finished list is passed to the camera.
// 
// A hittable_list can also contain a BVH node to improve rendering performance for complex scenes.
//==============================================================================================

#include "aabb.h"
#include "hittable.h"

#include <vector>


class hittable_list : public hittable {
  public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = aabb(bbox, object->bounding_box());
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    aabb bbox;
};


#endif
