//==============================================================================================
// Entry point of the ray tracer.
//
// At startup the user picks a scene and whether to use the BVH. The scene is built, rendered
// and saved as a PNG image in the output/ folder, which is then opened in the default image
// viewer. Scene build time (including BVH construction) and render time are printed separately.
//
// The same choices can be given as command-line arguments, which skips the questions:
//     raytracer [scene] [--bvh | --no-bvh] [--no-open]
//==============================================================================================

#include "rtweekend.h"

#include "bvh.h"
#include "camera.h"
#include "constant_medium.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "quad.h"
#include "sphere.h"
#include "texture.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <string>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif


// Global BVH switch, declared in bvh.h. Scenes call accelerate(), which checks this flag.
bool use_bvh = true;


void bouncing_spheres(hittable_list& world, camera& cam) {

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    // Motion blur: diffuse spheres move upward during the shutter interval.
                    // auto center2 = center + vec3(0, random_double(0,.5), 0);
                    // world.add(make_shared<sphere>(center, center2, 0.2, sphere_material));
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    world = hittable_list(accelerate(world));

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;
}


void checkered_spheres(hittable_list& world, camera& cam) {

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void earth(hittable_list& world, camera& cam) {
    auto earth_texture = make_shared<image_texture>("earthmap.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);
    world.add(globe);

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

}


void perlin_spheres(hittable_list& world, camera& cam) {

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void quads(hittable_list& world, camera& cam) {

    // Materials
    auto left_red     = make_shared<lambertian>(color(1.0, 0.2, 0.2));
    auto back_green   = make_shared<lambertian>(color(0.2, 1.0, 0.2));
    auto right_blue   = make_shared<lambertian>(color(0.2, 0.2, 1.0));
    auto upper_orange = make_shared<lambertian>(color(1.0, 0.5, 0.0));
    auto lower_teal   = make_shared<lambertian>(color(0.2, 0.8, 0.8));

    // Quads
    world.add(make_shared<quad>(point3(-3,-2, 5), vec3(0, 0,-4), vec3(0, 4, 0), left_red));
    world.add(make_shared<quad>(point3(-2,-2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<quad>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<quad>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<quad>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0.70, 0.80, 1.00);

    cam.vfov     = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void simple_light(hittable_list& world, camera& cam) {

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    world.add(make_shared<sphere>(point3(0,7,0), 2, difflight));
    world.add(make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight));

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 20;
    cam.lookfrom = point3(26,3,6);
    cam.lookat   = point3(0,2,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void cornell_box(hittable_list& world, camera& cam) {

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world.add(box1);

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));
    world.add(box2);

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void cornell_smoke(hittable_list& world, camera& cam) {

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(113,554,127), vec3(330,0,0), vec3(0,0,305), light));
    world.add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));

    world.add(make_shared<constant_medium>(box1, 0.01, color(0,0,0)));
    world.add(make_shared<constant_medium>(box2, 0.01, color(1,1,1)));

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


void final_scene(hittable_list& world, camera& cam,
                 int image_width, int samples_per_pixel, int max_depth) {
    hittable_list boxes1;
    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i*w;
            auto z0 = -1000.0 + j*w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1,101);
            auto z1 = z0 + w;

            boxes1.add(box(point3(x0,y0,z0), point3(x1,y1,z1), ground));
        }
    }

    world.add(accelerate(boxes1));

    auto light = make_shared<diffuse_light>(color(7, 7, 7));
    world.add(make_shared<quad>(point3(123,554,147), vec3(300,0,0), vec3(0,0,265), light));

    auto center1 = point3(400, 400, 200);
    // Motion blur: this sphere moves from center1 to center2 during the shutter interval.
    // auto center2 = center1 + vec3(30,0,0);
    auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
    // world.add(make_shared<sphere>(center1, center2, 50, sphere_material));
    world.add(make_shared<sphere>(center1, 50, sphere_material));

    world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(
        point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
    ));

    auto boundary = make_shared<sphere>(point3(360,150,145), 70, make_shared<dielectric>(1.5));
    world.add(boundary);
    world.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
    boundary = make_shared<sphere>(point3(0,0,0), 5000, make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(boundary, .0001, color(1,1,1)));

    auto emat = make_shared<lambertian>(make_shared<image_texture>("earthmap.jpg"));
    world.add(make_shared<sphere>(point3(400,200,400), 100, emat));
    auto pertext = make_shared<noise_texture>(0.2);
    world.add(make_shared<sphere>(point3(220,280,300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<sphere>(point3::random(0,165), 10, white));
    }

    world.add(make_shared<translate>(
        make_shared<rotate_y>(
            accelerate(boxes2), 15),
            vec3(-100,270,395)
        )
    );

    cam.aspect_ratio      = 1.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(478, 278, -600);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;
}


//==============================================================================================
// Scene menu
//==============================================================================================

void final_scene_fast(hittable_list& world, camera& cam) { final_scene(world, cam, 400,   250,  4); }
void final_scene_high(hittable_list& world, camera& cam) { final_scene(world, cam, 800, 10000, 40); }

struct scene_entry {
    const char* id;           // short name, used in the output file name
    const char* description;  // text shown in the menu
    bool        uses_bvh;     // whether the BVH switch has any effect on this scene
    void (*build)(hittable_list& world, camera& cam);
};

// The index in this array is the scene number the user types in.
const scene_entry scenes[] = {
    { "final_scene_fast",  "final_scene, brza verzija (400 px, 250 uzoraka)", true,  final_scene_fast  },
    { "bouncing_spheres",  "bouncing_spheres (oko 480 sfera)",                true,  bouncing_spheres  },
    { "checkered_spheres", "checkered_spheres",                               false, checkered_spheres },
    { "earth",             "earth",                                           false, earth             },
    { "perlin_spheres",    "perlin_spheres",                                  false, perlin_spheres    },
    { "quads",             "quads",                                           false, quads             },
    { "simple_light",      "simple_light",                                    false, simple_light      },
    { "cornell_box",       "cornell_box",                                     false, cornell_box       },
    { "cornell_smoke",     "cornell_smoke",                                   false, cornell_smoke     },
    { "final_scene_high",  "final_scene, visok kvalitet (traje satima!)",     true,  final_scene_high  },
};
const int scene_count = sizeof(scenes) / sizeof(scenes[0]);


//==============================================================================================
// Helper functions
//==============================================================================================

// Reads one line from the console and removes surrounding spaces.
std::string read_line() {
    std::string line;
    if (!std::getline(std::cin, line))
        std::exit(1);  // input closed (e.g. Ctrl+D / Ctrl+Z)
    auto first = line.find_first_not_of(" \t\r");
    auto last  = line.find_last_not_of(" \t\r");
    return (first == std::string::npos) ? "" : line.substr(first, last - first + 1);
}

// Converts text to a scene number; returns -1 if the text is not a valid scene number.
int parse_scene(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        return -1;
    int n = std::atoi(text.c_str());
    return (n >= 0 && n < scene_count) ? n : -1;
}

int ask_scene() {
    std::cout << "\nDostupne scene:\n";
    for (int i = 0; i < scene_count; i++) {
        std::cout << "  " << i << "  " << scenes[i].description;
        if (scenes[i].uses_bvh) std::cout << "  [koristi BVH]";
        std::cout << '\n';
    }

    while (true) {
        std::cout << "\nOdaberi scenu (0-" << scene_count - 1 << "): " << std::flush;
        int n = parse_scene(read_line());
        if (n >= 0) return n;
        std::cout << "Neispravan unos, pokusaj ponovo.\n";
    }
}

bool ask_bvh() {
    while (true) {
        std::cout << "Koristiti BVH? (d/n) [d]: " << std::flush;
        std::string answer = read_line();
        if (answer.empty() || answer == "d" || answer == "D" || answer == "da") return true;
        if (answer == "n" || answer == "N" || answer == "ne") return false;
        std::cout << "Upisi d (da) ili n (ne).\n";
    }
}

// Creates the output folder if it does not exist yet.
void make_output_dir() {
#ifdef _WIN32
    _mkdir("output");
#else
    mkdir("output", 0755);
#endif
}

// Opens the image in the operating system's default image viewer.
void open_image(const std::string& path) {
#if defined(_WIN32)
    std::string win_path = path;
    std::replace(win_path.begin(), win_path.end(), '/', '\\');
    std::string command = "start \"\" \"" + win_path + "\"";
#elif defined(__APPLE__)
    std::string command = "open \"" + path + "\"";
#else
    std::string command = "xdg-open \"" + path + "\" >/dev/null 2>&1 &";
#endif
    if (std::system(command.c_str()) != 0)
        std::cout << "Sliku nije moguce automatski otvoriti, otvori je rucno.\n";
}


//==============================================================================================
// main
//==============================================================================================

int main(int argc, char* argv[]) {
    // Command-line arguments: if any are given, the program does not ask questions.
    int  scene_index = 0;
    bool open_after  = true;
    bool interactive = (argc == 1);

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if      (arg == "--bvh")     use_bvh = true;
        else if (arg == "--no-bvh")  use_bvh = false;
        else if (arg == "--no-open") open_after = false;
        else if (parse_scene(arg) >= 0) scene_index = parse_scene(arg);
        else {
            std::cerr << "Nepoznat argument: " << arg << "\n"
                      << "Upotreba: raytracer [scena 0-" << scene_count - 1 << "]"
                      << " [--bvh | --no-bvh] [--no-open]\n";
            return 1;
        }
    }

    if (interactive) {
        scene_index = ask_scene();
        if (scenes[scene_index].uses_bvh)
            use_bvh = ask_bvh();
    }

    const scene_entry& scene = scenes[scene_index];

    std::string filename = "output/" + std::to_string(scene_index) + "_" + scene.id;
    if (scene.uses_bvh)
        filename += use_bvh ? "_bvh" : "_bez_bvh";
    filename += ".png";

    std::cout << "\nRenderujem scenu " << scene_index << " (" << scene.id << ")";
    if (scene.uses_bvh) std::cout << (use_bvh ? " sa BVH" : " bez BVH");
    std::cout << "...\n";

    // Phase 1: build the scene (objects, materials, and the BVH tree if enabled)
    auto t0 = std::chrono::steady_clock::now();
    hittable_list world;
    camera cam;
    scene.build(world, cam);

    // Phase 2: render the image and save it
    auto t1 = std::chrono::steady_clock::now();
    make_output_dir();
    bool saved = cam.render(world, filename);
    auto t2 = std::chrono::steady_clock::now();

    std::chrono::duration<double> build_time  = t1 - t0;
    std::chrono::duration<double> render_time = t2 - t1;
    std::chrono::duration<double> total_time  = t2 - t0;

    std::cout << std::fixed << std::setprecision(3)
              << "\n----------------------------------------------\n"
              << "Scena:            " << scene_index << " - " << scene.id << '\n'
              << "BVH:              "
              << (!scene.uses_bvh ? "scena ne koristi BVH" : (use_bvh ? "da" : "ne")) << '\n'
              << "Izgradnja scene:  " << build_time.count()  << " s\n"
              << "Renderovanje:     " << render_time.count() << " s\n"
              << "Ukupno:           " << total_time.count()  << " s\n"
              << "Slika:            " << (saved ? filename : "GRESKA: slika nije snimljena") << '\n'
              << "----------------------------------------------\n";

    if (!saved)
        return 1;

    if (open_after)
        open_image(filename);

    return 0;
}
