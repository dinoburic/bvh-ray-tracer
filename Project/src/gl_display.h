#ifndef GL_DISPLAY_H
#define GL_DISPLAY_H
//==============================================================================================
// GLFW WINDOW THAT SHOWS AN RGB IMAGE (used by animation.cc and interactive.cc)
//
// Uses legacy fixed-function OpenGL: the image is uploaded as a texture and drawn on one quad
// that fills the window (keeping the aspect ratio). Input is forwarded through the on_key,
// on_scroll and on_mouse_drag callbacks, so each program decides what the keys do.
//==============================================================================================

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F   // missing from the OpenGL 1.1 header on Windows
#endif


// Creates every folder along the path (like "mkdir -p").
inline void make_dirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/' || path[i] == '\\') {
            std::string part = path.substr(0, i);
#ifdef _WIN32
            _mkdir(part.c_str());
#else
            mkdir(part.c_str(), 0755);
#endif
        }
    }
}


class gl_display {
  public:
    // Called on key press and key repeat with the GLFW key code.
    std::function<void(int key)> on_key;
    // Called on mouse wheel / trackpad scroll with the vertical offset.
    std::function<void(double dy)> on_scroll;
    // Called while the left mouse button is held and the mouse moves (dx, dy in window units).
    std::function<void(double dx, double dy)> on_mouse_drag;

    // vsync = true limits redraws to the screen refresh rate (good for playback); the
    // interactive viewer turns it off so showing the image never slows down rendering.
    bool open(int w, int h, const std::string& title, bool vsync = true) {
        img_w = w; img_h = h;
        if (!glfwInit()) {
            std::cerr << "ERROR: glfwInit failed.\n";
            return false;
        }

        // Show small images bigger so they are comfortable to look at.
        int scale = std::max(1, 640 / std::max(w, h));
        window = glfwCreateWindow(w * scale, h * scale, title.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "ERROR: could not create GLFW window.\n";
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(vsync ? 1 : 0);
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, key_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetCursorPosCallback(window, cursor_callback);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        return true;
    }

    void close() {
        if (!window) return;
        glDeleteTextures(1, &texture);
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
    }

    bool should_close() const { return glfwWindowShouldClose(window) != 0; }

    void request_close() { glfwSetWindowShouldClose(window, GLFW_TRUE); }

    void set_title(const std::string& title) {
        if (title == last_title) return;   // avoid needless window-system calls every frame
        last_title = title;
        glfwSetWindowTitle(window, title.c_str());
    }

    // Uploads an RGB image (top row first) and draws it, keeping the aspect ratio.
    void show(const unsigned char* rgb) {
        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);   // differs from window size on Retina

        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img_w, img_h, GL_RGB, GL_UNSIGNED_BYTE, rgb);

        // Fit the image into the window (letterbox).
        double sx = 1.0, sy = 1.0;
        double win_aspect = double(fb_w) / std::max(fb_h, 1);
        double img_aspect = double(img_w) / img_h;
        if (win_aspect > img_aspect) sx = img_aspect / win_aspect;
        else                         sy = win_aspect / img_aspect;

        glMatrixMode(GL_PROJECTION); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
        glEnable(GL_TEXTURE_2D);
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);   // texture row 0 is the top image row, so v is flipped
            glTexCoord2f(0, 1); glVertex2d(-sx, -sy);
            glTexCoord2f(1, 1); glVertex2d( sx, -sy);
            glTexCoord2f(1, 0); glVertex2d( sx,  sy);
            glTexCoord2f(0, 0); glVertex2d(-sx,  sy);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);
    }

  private:
    GLFWwindow* window  = nullptr;
    GLuint      texture = 0;
    int         img_w = 0, img_h = 0;
    std::string last_title;
    double      last_x = 0, last_y = 0;
    bool        have_last = false;

    static gl_display* self(GLFWwindow* w) {
        return static_cast<gl_display*>(glfwGetWindowUserPointer(w));
    }

    static void key_callback(GLFWwindow* w, int key, int, int action, int) {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        auto* d = self(w);
        if (d->on_key) d->on_key(key);
    }

    static void scroll_callback(GLFWwindow* w, double, double dy) {
        auto* d = self(w);
        if (d->on_scroll) d->on_scroll(dy);
    }

    static void cursor_callback(GLFWwindow* w, double x, double y) {
        auto* d = self(w);
        bool dragging = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (dragging && d->have_last && d->on_mouse_drag)
            d->on_mouse_drag(x - d->last_x, y - d->last_y);
        d->last_x = x; d->last_y = y;
        d->have_last = true;
    }
};


#endif
