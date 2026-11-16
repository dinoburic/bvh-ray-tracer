//==============================================================================================
// INTERACTIVE VIEWER WITH TEMPORAL (PROGRESSIVE) ACCUMULATION - SEQUENTIAL
//
// Instead of rendering all samples per pixel at once, every pass renders ONE sample per pixel
// and adds it to an accumulation buffer. The window shows the average of all passes so far,
// so the image appears after the first pass and gets less noisy over time (noise falls as
// 1/sqrt(N)). When the camera moves, the old samples no longer belong to the pixels, so the
// buffer is cleared and accumulation starts again.
//
// Everything runs on ONE thread, like raytracer: the window is serviced between rows of a pass.
//
// Usage:  interactive [--width W] [--depth D] [--max-spp N] [--clamp C] [--out DIR] [--no-window]
//
// Controls:  mouse drag / arrow keys  orbit the camera     W / S or scroll  zoom in / out
//            R  reset view             P  save PNG          ESC or Q  quit
//
// --clamp C limits the brightness of every single sample to C (firefly clamping). Rare, very
// bright samples (e.g. a ray that scatters in the fog and then hits the small light) otherwise
// show up as bright dots that take a very long time to average out. Clamping removes them at
// the cost of a slightly darker image (some energy is lost). 0 = off, the exact image.
//
// With --no-window the program accumulates up to --max-spp at the starting view, prints how
// long it took to reach 1, 10, 50, 100, 250, ... samples per pixel, saves a PNG and exits.
//==============================================================================================

#include "rtweekend.h"

#include "camera.h"
#include "scenes.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "gl_display.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>


//----------------------------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------------------------

struct settings {
    int         width   = 400;    // image width = height (the scene is square)
    int         depth   = 10;     // max ray bounces
    int         max_spp = 1000;   // stop accumulating after this many samples per pixel
    double      clamp   = 0;      // max brightness of one sample, 0 = no clamping
    std::string out_dir = "output";
    bool        window  = true;
};

static void print_usage() {
    std::clog <<
        "Usage: interactive [options]\n"
        "  --width W     image width/height in pixels (default 400)\n"
        "  --depth D     max ray depth (default 10)\n"
        "  --max-spp N   stop accumulating at N samples per pixel (default 1000)\n"
        "  --clamp C     limit the brightness of each sample to C, removes bright dots\n"
        "                (fireflies) but darkens slightly; try 3 (default 0 = off)\n"
        "  --out DIR     folder for saved PNG images (default output)\n"
        "  --no-window   accumulate at the starting view, print timings, save PNG, exit\n";
}

static bool parse_args(int argc, char** argv, settings& s) {
    for (int a = 1; a < argc; a++) {
        std::string arg = argv[a];
        auto next_int = [&](int& target) {
            if (a + 1 >= argc) return false;
            target = std::atoi(argv[++a]);
            return true;
        };

        bool ok = true;
        if      (arg == "--width")     ok = next_int(s.width);
        else if (arg == "--depth")     ok = next_int(s.depth);
        else if (arg == "--max-spp")   ok = next_int(s.max_spp);
        else if (arg == "--no-window") s.window = false;
        else if (arg == "--clamp" && a + 1 < argc) s.clamp = std::atof(argv[++a]);
        else if (arg == "--out" && a + 1 < argc) s.out_dir = argv[++a];
        else ok = false;

        if (!ok) return false;
    }
    return s.width > 0 && s.depth > 0 && s.max_spp > 0 && s.clamp >= 0;
}


//----------------------------------------------------------------------------------------------
// Orbit camera: the camera sits on a sphere around a target point
//----------------------------------------------------------------------------------------------

struct orbit_camera {
    point3 target;
    double yaw, pitch, radius;   // angles in radians

    // Starting view = the camera of the still image (lookfrom (478,278,-600), lookat (278,278,0)).
    void reset() {
        target = point3(278, 278, 0);
        vec3 offset(200, 0, -600);
        radius = offset.length();
        yaw    = std::atan2(offset.z(), offset.x());
        pitch  = 0;
    }

    point3 position() const {
        return target + radius * vec3(std::cos(pitch) * std::cos(yaw),
                                      std::sin(pitch),
                                      std::cos(pitch) * std::sin(yaw));
    }

    void rotate(double d_yaw, double d_pitch) {
        yaw  += d_yaw;
        pitch = std::max(degrees_to_radians(-85), std::min(degrees_to_radians(85), pitch + d_pitch));
    }

    void zoom(double factor) {
        radius = std::max(100.0, std::min(3000.0, radius * factor));
    }
};


//----------------------------------------------------------------------------------------------
// Firefly clamping
//----------------------------------------------------------------------------------------------

// Scales a too-bright sample down so that its largest component is `limit`, keeping its hue.
// limit = 0 means no clamping.
static color clamp_sample(color c, double limit) {
    double m = std::fmax(c.x(), std::fmax(c.y(), c.z()));
    if (limit > 0 && m > limit)
        c *= limit / m;
    return c;
}


//----------------------------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    settings s;
    if (!parse_args(argc, argv, s)) {
        print_usage();
        return 1;
    }

    std::clog << "Building scene and BVH...\n";
    hittable_list world = final_scene_world();

    camera cam;
    cam.aspect_ratio      = 1.0;
    cam.image_width       = s.width;
    cam.samples_per_pixel = 1;          // one sample per pass, the passes are averaged
    cam.max_depth         = s.depth;
    cam.background        = color(0,0,0);
    cam.vfov              = 40;
    cam.vup               = vec3(0,1,0);
    cam.defocus_angle     = 0;

    orbit_camera orbit;
    orbit.reset();
    cam.lookfrom = orbit.position();
    cam.lookat   = orbit.target;
    cam.prepare();

    const int width  = s.width;
    const int height = cam.height();

    std::vector<color>         accum(width * height, color(0,0,0));   // sum of all samples
    std::vector<unsigned char> rgb(width * height * 3, 0);            // displayed average
    int    samples      = 0;       // finished passes = samples per pixel
    bool   camera_moved = false;
    bool   save_request = false;
    int    saved_count  = 0;
    double done_seconds = 0;       // time it took to reach max_spp

    // Time since the last reset, and the sample counts at which it is reported.
    auto since_reset = std::chrono::steady_clock::now();
    const int milestones[] = {1, 10, 50, 100, 250, 500, 1000, 2000, 5000, 10000};
    size_t next_milestone = 0;

    auto seconds_since_reset = [&]() {
        std::chrono::duration<double> d = std::chrono::steady_clock::now() - since_reset;
        return d.count();
    };

    auto reset_accumulation = [&]() {
        std::fill(accum.begin(), accum.end(), color(0,0,0));
        samples        = 0;
        done_seconds   = 0;
        next_milestone = 0;
        since_reset    = std::chrono::steady_clock::now();
    };

    auto save_png = [&]() {
        make_dirs(s.out_dir);
        char name[96];
        std::snprintf(name, sizeof(name), "/accumulated_%dspp_%02d.png", samples, saved_count++);
        std::string path = s.out_dir + name;
        if (stbi_write_png(path.c_str(), width, height, 3, rgb.data(), width * 3))
            std::clog << "Saved " << path << "\n";
        else
            std::cerr << "ERROR: could not write " << path << "\n";
    };

    gl_display view;
    if (s.window) {
        const double step = degrees_to_radians(5);
        view.on_key = [&](int key) {
            switch (key) {
                case GLFW_KEY_ESCAPE:
                case GLFW_KEY_Q:     view.request_close();    return;
                case GLFW_KEY_P:     save_request = true;     return;
                case GLFW_KEY_LEFT:  orbit.rotate(-step, 0);  break;
                case GLFW_KEY_RIGHT: orbit.rotate(+step, 0);  break;
                case GLFW_KEY_UP:    orbit.rotate(0, +step);  break;
                case GLFW_KEY_DOWN:  orbit.rotate(0, -step);  break;
                case GLFW_KEY_W:     orbit.zoom(0.9);         break;
                case GLFW_KEY_S:     orbit.zoom(1.0 / 0.9);   break;
                case GLFW_KEY_R:     orbit.reset();           break;
                default:                                      return;
            }
            camera_moved = true;
        };
        view.on_scroll = [&](double dy) {
            orbit.zoom(std::pow(0.9, dy));
            camera_moved = true;
        };
        view.on_mouse_drag = [&](double dx, double dy) {
            orbit.rotate(degrees_to_radians(0.3 * dx), degrees_to_radians(0.3 * dy));
            camera_moved = true;
        };

        if (!view.open(width, height, "BVH Ray Tracer - interactive", false)) {
            std::clog << "Continuing without a window.\n";
            s.window = false;
        }
    }

    std::clog << width << "x" << height << ", depth " << s.depth << ", 1 thread, up to "
              << s.max_spp << " spp";
    if (s.clamp > 0) std::clog << ", clamp " << s.clamp;
    std::clog << "\n";
    if (s.window)
        std::clog << "Mouse drag / arrows: orbit   W/S or scroll: zoom   "
                     "R: reset   P: save PNG   ESC: quit\n";

    // Updates the window title and redraws the current image.
    auto refresh_window = [&]() {
        bool done = samples >= s.max_spp;
        char title[160];
        std::snprintf(title, sizeof(title), "Accumulated %d spp  (%.1f s)%s%s", samples,
                      done ? done_seconds : seconds_since_reset(),
                      s.clamp > 0 ? "  [clamp]" : "", done ? "  [done]" : "");
        view.set_title(title);
        view.show(rgb.data());
    };

    since_reset = std::chrono::steady_clock::now();   // don't count scene building / window setup

    while (!(s.window && view.should_close())) {
        if (s.window) {
            // While accumulating only peek at events; when done, sleep until there is input.
            if (samples < s.max_spp) glfwPollEvents();
            else                     glfwWaitEvents();
        }

        if (camera_moved) {
            camera_moved = false;
            cam.lookfrom = orbit.position();
            cam.lookat   = orbit.target;
            cam.prepare();
            reset_accumulation();
        }

        if (save_request) {
            save_request = false;
            save_png();
        }

        if (samples < s.max_spp) {
            // One pass: add one new sample to every pixel, row by row, and update the
            // displayed average. After every few rows the window is serviced, so it stays
            // responsive even though there is only one thread. If the camera moves in the
            // middle of a pass, the pass is abandoned (the buffer is reset anyway).
            const int    n   = samples + 1;
            const double inv = 1.0 / n;
            bool aborted = false;

            for (int j = 0; j < height; j++) {
                for (int i = 0; i < width; i++) {
                    int idx = j * width + i;
                    accum[idx] += clamp_sample(cam.sample_pixel(i, j, world), s.clamp);
                    color_to_bytes(accum[idx] * inv, &rgb[idx * 3]);
                }

                if (s.window && (j % 16 == 15)) {
                    glfwPollEvents();
                    if (camera_moved || view.should_close()) { aborted = true; break; }
                    refresh_window();
                }
            }
            if (aborted) continue;

            samples = n;
            if (samples == s.max_spp) done_seconds = seconds_since_reset();

            if (next_milestone < sizeof(milestones) / sizeof(milestones[0]) &&
                samples == milestones[next_milestone]) {
                std::clog << "  " << samples << " spp after " << seconds_since_reset() << " s\n";
                next_milestone++;
            }
        }

        if (s.window) {
            refresh_window();
        } else if (samples >= s.max_spp) {
            std::clog << "Reached " << samples << " spp in " << done_seconds << " s\n";
            save_png();
            break;
        }
    }

    view.close();
    return 0;
}
