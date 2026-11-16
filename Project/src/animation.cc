//==============================================================================================
// CAMERA FLY-AROUND ANIMATION OF THE FINAL SCENE (GLFW WINDOW + PNG FRAMES)
//
// The scene (and its BVH) is built once. Then, for every frame, the camera is moved a little
// along a closed path around the scene and the frame is path traced on all CPU cores.
// While a frame is rendering, the GLFW window shows it filling in row by row. Every finished
// frame is saved as output/frames/frame_XXXX.png. When all frames are done, the window plays
// them in a loop, so it looks like the camera is flying around the scene.
//
// Usage:  animation [--frames N] [--width W] [--spp S] [--depth D] [--fps F]
//                   [--threads T] [--out DIR] [--no-window]
//
// Keys in the playback window:  SPACE pause/resume,  LEFT/RIGHT step one frame,
//                               UP/DOWN faster/slower,  ESC or Q quit.
//==============================================================================================

#include "rtweekend.h"

#include "camera.h"
#include "scenes.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#include "gl_display.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


//----------------------------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------------------------

struct settings {
    int         frames    = 72;     // number of frames in one full fly-around
    int         width     = 400;    // frame width = height (the scene is square)
    int         spp       = 1000;   // samples per pixel
    int         depth     = 20;     // max ray bounces
    int         fps       = 24;     // playback speed in the window
    int         threads   = 1;      // render threads (1 = sequential); 0 = all hardware threads
    std::string out_dir   = "output/frames";
    bool        window    = true;   // false = render to PNG only (e.g. on a server)
};

static void print_usage() {
    std::clog <<
        "Usage: animation [options]\n"
        "  --frames N    number of frames (default 72)\n"
        "  --width W     frame width/height in pixels (default 400)\n"
        "  --spp S       samples per pixel (default 1000)\n"
        "  --depth D     max ray depth (default 20)\n"
        "  --fps F       playback speed in the window (default 24)\n"
        "  --threads T   render threads, 0 = all cores (default 1)\n"
        "  --out DIR     folder for PNG frames (default output/frames)\n"
        "  --no-window   render frames to PNG without opening a window\n";
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
        if      (arg == "--frames")    ok = next_int(s.frames);
        else if (arg == "--width")     ok = next_int(s.width);
        else if (arg == "--spp")       ok = next_int(s.spp);
        else if (arg == "--depth")     ok = next_int(s.depth);
        else if (arg == "--fps")       ok = next_int(s.fps);
        else if (arg == "--threads")   ok = next_int(s.threads);
        else if (arg == "--no-window") s.window = false;
        else if (arg == "--out" && a + 1 < argc) s.out_dir = argv[++a];
        else ok = false;

        if (!ok) return false;
    }
    return s.frames > 0 && s.width > 0 && s.spp > 0 && s.depth > 0 && s.fps > 0;
}


//----------------------------------------------------------------------------------------------
// Camera path
//----------------------------------------------------------------------------------------------

// Closed path around the final scene, parameterised by t in [0, 1). Frame 0 starts close to
// the original still-image camera (478, 278, -600), and t = 1 returns to the same spot, so the
// animation loops without a jump. Along the way the camera:
//   - circles once around the middle of the scene (seeing it from the front, sides and back),
//   - moves in and out (dolly) twice, getting close to the objects and then pulling back,
//   - rises and drops, so the scene is seen both from object height and from above.
static void camera_on_path(double t, point3& lookfrom, point3& lookat) {
    const point3 center(260, 250, 250);              // roughly the middle of the objects
    const double start_angle = std::atan2(-850.0, 218.0);   // direction of the original camera

    double angle  = start_angle + 2 * pi * t;
    double radius = 820 - 170 * std::sin(4 * pi * t);           // 650 .. 990
    double height = 330 + 170 * std::sin(2 * pi * t + pi / 3);  // 160 .. 500

    lookfrom = point3(center.x() + radius * std::cos(angle),
                      height,
                      center.z() + radius * std::sin(angle));

    // Look at the center, but drift the target a little so the shot doesn't feel locked on.
    lookat = center + vec3(40 * std::sin(2 * pi * t), 30 * std::sin(4 * pi * t), 0);
}


//----------------------------------------------------------------------------------------------
// Multi-threaded frame renderer
//----------------------------------------------------------------------------------------------

// Shared state of one frame that is being rendered. Worker threads take rows from next_row,
// render them into a private buffer and copy them into pixels under the mutex, so the main
// thread can safely show the partially finished frame.
struct frame_job {
    const camera*              cam   = nullptr;
    const hittable*            world = nullptr;
    int                        width = 0, height = 0;
    std::vector<unsigned char> pixels;            // RGB, top row first
    std::mutex                 pixels_mutex;
    std::atomic<int>           next_row{0};
    std::atomic<int>           rows_done{0};
    std::atomic<bool>          cancel{false};
};

static void render_worker(frame_job& job) {
    std::vector<unsigned char> row(job.width * 3);
    for (;;) {
        int j = job.next_row++;
        if (j >= job.height || job.cancel) return;

        for (int i = 0; i < job.width; i++)
            color_to_bytes(job.cam->sample_pixel(i, j, *job.world), &row[i * 3]);

        {
            std::lock_guard<std::mutex> lock(job.pixels_mutex);
            std::memcpy(&job.pixels[j * job.width * 3], row.data(), row.size());
        }
        job.rows_done++;
    }
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
    if (s.threads <= 0)
        s.threads = std::max(1u, std::thread::hardware_concurrency());

    make_dirs(s.out_dir);

    std::clog << "Building scene and BVH...\n";
    hittable_list world = final_scene_world();

    camera cam;
    cam.aspect_ratio      = 1.0;
    cam.image_width       = s.width;
    cam.samples_per_pixel = s.spp;
    cam.max_depth         = s.depth;
    cam.background        = color(0,0,0);
    cam.vfov              = 40;
    cam.vup               = vec3(0,1,0);
    cam.defocus_angle     = 0;
    cam.prepare();
    const int width  = s.width;
    const int height = cam.height();

    // Playback controls, changed from the key callback.
    bool paused = false;
    int  step   = 0;        // -1 / +1 when an arrow key was pressed
    int  fps    = s.fps;

    gl_display view;
    view.on_key = [&](int key) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:     view.request_close();          break;
            case GLFW_KEY_SPACE: paused = !paused;              break;
            case GLFW_KEY_RIGHT: step = +1;                     break;
            case GLFW_KEY_LEFT:  step = -1;                     break;
            case GLFW_KEY_UP:    fps = std::min(fps + 4, 120);  break;
            case GLFW_KEY_DOWN:  fps = std::max(fps - 4, 1);    break;
        }
    };
    if (s.window && !view.open(width, height, "BVH Ray Tracer - animation")) {
        std::clog << "Continuing without a window.\n";
        s.window = false;
    }

    std::clog << "Rendering " << s.frames << " frames, " << width << "x" << height << ", "
              << s.spp << " spp, depth " << s.depth << ", " << s.threads << " threads\n";

    std::vector<std::vector<unsigned char>> frames;
    auto anim_start = std::chrono::steady_clock::now();

    for (int f = 0; f < s.frames; f++) {
        auto frame_start = std::chrono::steady_clock::now();

        // Move the camera to this frame's position on the path.
        double t = double(f) / s.frames;   // t never reaches 1, so the loop doesn't repeat frame 0
        camera_on_path(t, cam.lookfrom, cam.lookat);
        cam.prepare();

        frame_job job;
        job.cam    = &cam;
        job.world  = &world;
        job.width  = width;
        job.height = height;
        job.pixels.assign(width * height * 3, 0);

        std::vector<std::thread> workers;
        for (int k = 0; k < s.threads; k++)
            workers.emplace_back(render_worker, std::ref(job));

        // Main thread: keep the window alive and show the frame as rows come in.
        std::vector<unsigned char> snapshot;
        while (job.rows_done < height) {
            if (s.window) {
                glfwWaitEventsTimeout(1.0 / 30);
                if (view.should_close()) { job.cancel = true; break; }
                {
                    std::lock_guard<std::mutex> lock(job.pixels_mutex);
                    snapshot = job.pixels;
                }
                int pct = 100 * job.rows_done / height;
                view.set_title("Rendering frame " + std::to_string(f + 1) + "/" +
                               std::to_string(s.frames) + "  (" + std::to_string(pct) + "%)");
                view.show(snapshot.data());
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        for (auto& w : workers) w.join();

        if (job.cancel) {
            std::clog << "\nWindow closed, stopping. " << f << " frames were saved.\n";
            view.close();
            return 0;
        }

        char name[64];
        std::snprintf(name, sizeof(name), "/frame_%04d.png", f);
        std::string path = s.out_dir + name;
        if (!stbi_write_png(path.c_str(), width, height, 3, job.pixels.data(), width * 3))
            std::cerr << "\nERROR: could not write " << path << "\n";

        std::chrono::duration<double> frame_time = std::chrono::steady_clock::now() - frame_start;
        std::clog << "\rFrame " << (f + 1) << "/" << s.frames << " done in "
                  << frame_time.count() << " s      " << std::flush;

        frames.push_back(std::move(job.pixels));
    }

    std::chrono::duration<double> total = std::chrono::steady_clock::now() - anim_start;
    std::clog << "\nAll frames rendered in " << total.count() << " seconds.\n"
              << "Frames saved to " << s.out_dir << "/frame_XXXX.png\n"
              << "To make a video (needs ffmpeg):\n"
              << "  ffmpeg -framerate " << s.fps << " -i " << s.out_dir
              << "/frame_%04d.png -c:v libx264 -pix_fmt yuv420p output/animation.mp4\n";

    if (!s.window) return 0;

    // Playback: loop through the finished frames like a video.
    std::clog << "Playing animation. SPACE pause, LEFT/RIGHT step, UP/DOWN speed, ESC quit.\n";
    int    current     = 0;
    double last_switch = glfwGetTime();
    while (!view.should_close()) {
        glfwWaitEventsTimeout(0.005);

        double now = glfwGetTime();
        if (step != 0) {
            current = (current + step + int(frames.size())) % int(frames.size());
            step = 0;
            paused = true;
            last_switch = now;
        } else if (!paused && now - last_switch >= 1.0 / fps) {
            current = (current + 1) % int(frames.size());
            last_switch = now;
        }

        view.set_title("Animation  frame " + std::to_string(current + 1) + "/" +
                       std::to_string(frames.size()) + "  " + std::to_string(fps) + " fps" +
                       (paused ? "  [paused]" : ""));
        view.show(frames[current].data());
    }

    view.close();
    return 0;
}
