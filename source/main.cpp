#include <cstdint>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if defined(_WIN32)
#define GLFW_INCLUDE_NONE
#define GLAD_GL_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <glad/gl.h>
#endif

#include <backends/imgui_impl_opengl3.h>

#include <view/app.hpp>
#include <view/theme.hpp>

namespace {

#if defined(__EMSCRIPTEN__)
EM_JS(int, browser_get_samplerate, (), {
    var AudioContext = window.AudioContext || window.webkitAudioContext;
    var ctx = new AudioContext();
    var sr = ctx.sampleRate;
    ctx.close();
    return sr;
});

EM_JS(float, window_get_dpr, (), {
    return window.devicePixelRatio || 1.0;
});

EM_JS(int, canvas_get_width, (), {
    var _canvas = document.getElementById('canvas');
    var _dpr = window.devicePixelRatio;
    var _width = _canvas.getBoundingClientRect().width * _dpr;
    _canvas.width = _width;
    return _width;
});

EM_JS(int, canvas_get_height, (), {
    var _canvas = document.getElementById('canvas');
    var _dpr = window.devicePixelRatio;
    var _height = _canvas.getBoundingClientRect().height * _dpr;
    _canvas.height = _height;
    return _height;
});

EM_JS(int, navigator_get_touch_points, (), {
    return navigator.maxTouchPoints;
});

EM_BOOL _key_callback(int event_type, const EmscriptenKeyboardEvent* event, void* user_data)
{
    // process lock
    if (!is_audio_locked) {
        is_audio_locked = setup_openal();
    }
    EmscriptenPointerlockChangeEvent _pointer_lock;
    emscripten_assert(emscripten_get_pointerlock_status(&_pointer_lock));
    _is_mouse_locked = _pointer_lock.isActive;
    if (!_is_mouse_locked) {
        emscripten_assert(emscripten_request_pointerlock("#canvas", 1));
        _is_mouse_locked = true;
    }

    const button_key _key(emscripten_keyboard_mappings[std::string(event->key)]);
    if (event_type == EMSCRIPTEN_EVENT_KEYDOWN) {
        _button_events[_key].state = true;
    } else if (event_type == EMSCRIPTEN_EVENT_KEYUP) {
        _button_events[_key].state = false;
    }
    return 0;
}

EM_BOOL _mouse_callback(int event_type, const EmscriptenMouseEvent* event, void* user_data)
{
    if (event_type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        const glm::float32 _dpr = window_get_dpr();
        const glm::vec2 _new_position = _dpr * glm::vec2(event->clientX, event->clientY);
        _pointer_accumulators[0] += _dpr * glm::vec2(event->movementX, event->movementY);
        _pointer_events[0].position = _new_position;
        ImGui::GetIO().AddMousePosEvent(_new_position.x, _new_position.y);
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        const glm::uint _button = event->button;
        _button_events[static_cast<button_key>(_button)].state = true;
        ImGui::GetIO().AddMouseButtonEvent(_button, true);

        // process lock
        if (!is_audio_locked) {
            is_audio_locked = setup_openal();
        }
        EmscriptenPointerlockChangeEvent _pointer_lock;
        emscripten_assert(emscripten_get_pointerlock_status(&_pointer_lock));
        _is_mouse_locked = _pointer_lock.isActive;
        if (!_is_mouse_locked) {
            emscripten_assert(emscripten_request_pointerlock("#canvas", 1));
            _is_mouse_locked = true;
        }
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
        const glm::uint _button = event->button;
        _button_events[static_cast<button_key>(_button)].state = false;
        ImGui::GetIO().AddMouseButtonEvent(_button, false);
    } else if (event_type == EMSCRIPTEN_EVENT_CLICK) {
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEOVER) {
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEOUT) {
    }
    return 0;
}

EM_BOOL _touch_callback(int event_type, const EmscriptenTouchEvent* event, void* user_data)
{
    // process lock
    if (!is_audio_locked) {
        is_audio_locked = setup_openal();
    }
    EmscriptenFullscreenChangeEvent _fullscreen;
    emscripten_assert(emscripten_get_fullscreen_status(&_fullscreen));
    _is_mouse_locked = _fullscreen.isFullscreen;
    if (!_is_mouse_locked) {
        EmscriptenFullscreenStrategy strategy;
        memset(&strategy, 0, sizeof(strategy));
        strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
        strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
        strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
        strategy.canvasResizedCallback = [](int eventType,
                                             const void* reserved,
                                             void* userData) -> EM_BOOL { return EM_TRUE; };
        emscripten_assert(emscripten_request_fullscreen_strategy("#canvas", EM_TRUE, &strategy));
        _is_mouse_locked = true;
    }

    static std::unordered_map<glm::uint, glm::vec2> _last_positions = {};
    const float _dpr = window_get_dpr();
    if (event_type == EMSCRIPTEN_EVENT_TOUCHSTART) {
        for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
            const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
            if (!_touch_point.isChanged) {
                continue;
            }
            const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
            const glm::vec2 _new_position = _dpr * glm::vec2(_touch_point.clientX, _touch_point.clientY);
            _last_positions[_event_id] = _new_position;
            _pointer_accumulators[_event_id] = glm::vec2(0);
            _pointer_events[_event_id].position = _new_position;
        }

    } else if (event_type == EMSCRIPTEN_EVENT_TOUCHMOVE) {
        for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
            const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
            if (!_touch_point.isChanged) {
                continue;
            }
            const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
            const glm::vec2 _new_position = _dpr * glm::vec2(_touch_point.clientX, _touch_point.clientY);
            const glm::vec2 _delta_position = _new_position - _last_positions[_event_id];
            _last_positions[_event_id] = _new_position;
            _pointer_accumulators[_event_id] += _delta_position;
            _pointer_events[_event_id].position = _new_position;
        }

    } else if (event_type == EMSCRIPTEN_EVENT_TOUCHEND || event_type == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
        for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
            const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
            if (!_touch_point.isChanged) {
                continue;
            }
            const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
            _last_positions.erase(_event_id);
            _pointer_accumulators.erase(_event_id);
            _pointer_events.erase(_event_id);
        }
    }

    return 0; // we use preventDefault() for touch callbacks (see Safari on iPad)
}

#endif

#if defined(_WIN32)
static GLFWwindow* _glfw_window = nullptr;

static void _glfw_window_focus_callback(GLFWwindow* window, int focused)
{
}

static void _glfw_error_callback(int error, const char* description)
{
    std::cout << "glfw error " << description << std::endl;
    std::terminate();
}

static void _glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((action == GLFW_PRESS) && (key == GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(_glfw_window, GLFW_TRUE);
    }
}

static void _glfw_mouse_position_callback(GLFWwindow* window, const double xpos, const double ypos)
{
}

static void _glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
}
#endif

static std::int32_t _screen_width = 0;
static std::int32_t _screen_height = 0;
static bool _is_mobile = false;

static void _setup_opengl()
{
#if defined(__EMSCRIPTEN__)
    EmscriptenWebGLContextAttributes _webgl_attributes;
    emscripten_webgl_init_context_attributes(&_webgl_attributes);
    _webgl_attributes.explicitSwapControl = 0;
    _webgl_attributes.depth = 1;
    _webgl_attributes.stencil = 1;
    _webgl_attributes.antialias = 1;
    _webgl_attributes.majorVersion = 2;
    _webgl_attributes.minorVersion = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE _webgl_context = emscripten_webgl_create_context("#canvas", &_webgl_attributes);
    if (_webgl_context < 0) {
        std::cout << "Failed to create WebGL2 context on this device" << std::endl;
        std::terminate();
    }
    emscripten_webgl_make_context_current(_webgl_context);
#endif
}

static void _setup_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

#if defined(_WIN32)
    ImGui_ImplGlfw_InitForOpenGL(_glfw_window, true);
#endif
    ImGui::StyleColorsLight();
    ImGuiIO& _io = ImGui::GetIO();
    _io.IniFilename = NULL;
    install_fonts();
    install_theme();
    ImGui_ImplOpenGL3_Init("#version 300 es");
}

static void _update()
{
#if defined(_WIN32)
    glfwPollEvents();
#endif

    // get screen size
#if defined(__EMSCRIPTEN__)
    _screen_width = canvas_get_width();
    _screen_height = canvas_get_height();
#endif
#if defined(_WIN32)
    glfwGetFramebufferSize(_glfw_window, &_screen_width, &_screen_height);
#endif
    if (_screen_width == 0 && _screen_height == 0) {
        return;
    }

    // clear
    glViewport(0, 0, _screen_width, _screen_height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // imgui platform backend new frame
#if defined(_WIN32)
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(_screen_width), static_cast<float>(_screen_height));

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    draw_app();
    // ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // swap buffers
#if defined(_WIN32)
    glfwSwapBuffers(_glfw_window);
#endif
}
}

int main()
{
#if defined(__EMSCRIPTEN__)
    _is_mobile = navigator_get_touch_points() > 1;
    if (_is_mobile) {
        emscripten_set_touchstart_callback("#canvas", 0, 1, _touch_callback);
        emscripten_set_touchend_callback("#canvas", 0, 1, _touch_callback);
        emscripten_set_touchmove_callback("#canvas", 0, 1, _touch_callback);
        emscripten_set_touchcancel_callback("#canvas", 0, 1, _touch_callback); // EMSCRIPTEN_EVENT_TARGET_WINDOW doesnt seem to work on safari
    } else {
        emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mouseenter_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mouseleave_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mouseover_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_mouseout_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _mouse_callback);
        emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _key_callback);
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _key_callback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, _key_callback);
    }
#endif

#if defined(_WIN32)
    _is_mobile = false;
    glfwSetErrorCallback(_glfw_error_callback);
    if (!glfwInit()) {
        std::cout << "Failed to create glfw context on this device" << std::endl;
        std::terminate();
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    _glfw_window = glfwCreateWindow(1600, 900, "stchat", NULL, NULL);
    if (!_glfw_window) {
        std::cout << "Failed to create glfw window on this device" << std::endl;
        std::terminate();
    }
    glfwSetKeyCallback(_glfw_window, _glfw_key_callback);
    glfwSetCursorPosCallback(_glfw_window, _glfw_mouse_position_callback);
    glfwSetMouseButtonCallback(_glfw_window, _glfw_mouse_button_callback);
    glfwMakeContextCurrent(_glfw_window);
    glfwSetWindowFocusCallback(_glfw_window, _glfw_window_focus_callback);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1);
#endif

    _setup_opengl();
    _setup_imgui();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(_update, 0, EM_TRUE);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
#endif

#if defined(_WIN32)
    while (!glfwWindowShouldClose(_glfw_window)) {
        _update();
    }
    glfwDestroyWindow(_glfw_window);
    glfwTerminate();
#endif
}