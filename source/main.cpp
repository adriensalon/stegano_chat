#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <imgui.h>

#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <backends/imgui_impl_android.h>
#include <thread>
#include <unistd.h>
#endif

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <atomic>
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#define GLFW_INCLUDE_NONE
#define GLAD_GL_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <glad/gl.h>
#endif

#include <backends/imgui_impl_opengl3.h>

#include <view/app.hpp>
#include <view/theme.hpp>

#if defined(__ANDROID__)
android_app* _android_app = nullptr;
#endif

namespace {
    
static void _setup_opengl();
static void _setup_imgui();

#if defined(__ANDROID__)
static EGLDisplay _android_display = EGL_NO_DISPLAY;
static EGLSurface _android_surface = EGL_NO_SURFACE;
static EGLContext _android_context = EGL_NO_CONTEXT;
static bool _android_has_window = false;
static bool _android_engine_initialized = false;

void _android_redirect_stdio_to_log()
{
    int _pfd[2];
    pipe(_pfd);
    dup2(_pfd[1], STDOUT_FILENO);
    dup2(_pfd[1], STDERR_FILENO);
    std::thread([=]() {
        char _buf[256];
        ssize_t _r;
        while ((_r = read(_pfd[0], _buf, sizeof(_buf) - 1)) > 0) {
            _buf[_r] = 0;
            __android_log_write(ANDROID_LOG_INFO, "SteganoChat", _buf);
        }
    }).detach();
}

static int32_t _android_on_input(android_app* app, AInputEvent* event)
{
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

static void _android_on_app_cmd(android_app* app, int32_t cmd)
{
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window != nullptr) {
            _setup_opengl();
            _setup_imgui();
            _android_engine_initialized = true;
        }
        break;

    case APP_CMD_TERM_WINDOW:
        ImGui_ImplAndroid_Shutdown();
        // destroy_opengl();
        break;

    case APP_CMD_INPUT_CHANGED:
        std::cout << "APP_CMD_INPUT_CHANGED, inputQueue = " << app->inputQueue << "\n";
        break;

    case APP_CMD_DESTROY:
        // destroy_opengl();
        break;
    }
}
#endif

#if defined(__EMSCRIPTEN__)
EM_JS(float, _window_get_dpr, (), {
    return window.devicePixelRatio || 1.0;
});

EM_JS(int, _canvas_get_width, (), {
    var _canvas = document.getElementById('canvas');
    var _dpr = window.devicePixelRatio;
    var _width = _canvas.getBoundingClientRect().width * _dpr;
    _canvas.width = _width;
    return _width;
});

EM_JS(int, _canvas_get_height, (), {
    var _canvas = document.getElementById('canvas');
    var _dpr = window.devicePixelRatio;
    var _height = _canvas.getBoundingClientRect().height * _dpr;
    _canvas.height = _height;
    return _height;
});

EM_JS(int, _navigator_get_touch_points, (), {
    return navigator.maxTouchPoints;
});

EM_BOOL _key_callback(int event_type, const EmscriptenKeyboardEvent* event, void* user_data)
{
    ImGuiIO& _io = ImGui::GetIO();
    _io.AddKeyEvent(ImGuiMod_Ctrl, event->ctrlKey);
    _io.AddKeyEvent(ImGuiMod_Shift, event->shiftKey);
    _io.AddKeyEvent(ImGuiMod_Alt, event->altKey);
    _io.AddKeyEvent(ImGuiMod_Super, event->metaKey);
    ImGuiKey _imgui_key = ImGuiKey_None;
    switch (event->keyCode) {
    case 8:
        _imgui_key = ImGuiKey_Backspace;
        break;
    case 9:
        _imgui_key = ImGuiKey_Tab;
        break;
    case 13:
        _imgui_key = ImGuiKey_Enter;
        break;
    case 27:
        _imgui_key = ImGuiKey_Escape;
        break;
    case 32:
        _imgui_key = ImGuiKey_Space;
        break;
    case 37:
        _imgui_key = ImGuiKey_LeftArrow;
        break;
    case 38:
        _imgui_key = ImGuiKey_UpArrow;
        break;
    case 39:
        _imgui_key = ImGuiKey_RightArrow;
        break;
    case 40:
        _imgui_key = ImGuiKey_DownArrow;
        break;
    case 46:
        _imgui_key = ImGuiKey_Delete;
        break;
    default:
        break;
    }
    if (_imgui_key != ImGuiKey_None) {
        if (event_type == EMSCRIPTEN_EVENT_KEYDOWN) {
            _io.AddKeyEvent(_imgui_key, true);
        } else if (event_type == EMSCRIPTEN_EVENT_KEYUP) {
            _io.AddKeyEvent(_imgui_key, false);
        }
    }
    if (event_type == EMSCRIPTEN_EVENT_KEYPRESS && event->charCode > 0) {
        _io.AddInputCharacter((unsigned int)event->charCode);
    }
    return 0;
}

EM_BOOL _mouse_callback(int event_type, const EmscriptenMouseEvent* event, void* user_data)
{
    ImGuiIO& _io = ImGui::GetIO();
    if (event_type == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        const float _dpr = _window_get_dpr();
        _io.AddMousePosEvent(_dpr * event->clientX, _dpr * event->clientY);
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        _io.AddMouseButtonEvent(event->button, true);
    } else if (event_type == EMSCRIPTEN_EVENT_MOUSEUP) {
        _io.AddMouseButtonEvent(event->button, false);
    }
    return 0;
}

// EM_BOOL _touch_callback(int event_type, const EmscriptenTouchEvent* event, void* user_data)
// {
//     static std::unordered_map<glm::uint, glm::vec2> _last_positions = {};
//     const float _dpr = _window_get_dpr();
//     if (event_type == EMSCRIPTEN_EVENT_TOUCHSTART) {
//         for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
//             const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
//             if (!_touch_point.isChanged) {
//                 continue;
//             }
//             const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
//             const glm::vec2 _new_position = _dpr * glm::vec2(_touch_point.clientX, _touch_point.clientY);
//             _last_positions[_event_id] = _new_position;
//             _pointer_accumulators[_event_id] = glm::vec2(0);
//             _pointer_events[_event_id].position = _new_position;
//         }

//     } else if (event_type == EMSCRIPTEN_EVENT_TOUCHMOVE) {
//         for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
//             const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
//             if (!_touch_point.isChanged) {
//                 continue;
//             }
//             const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
//             const glm::vec2 _new_position = _dpr * glm::vec2(_touch_point.clientX, _touch_point.clientY);
//             const glm::vec2 _delta_position = _new_position - _last_positions[_event_id];
//             _last_positions[_event_id] = _new_position;
//             _pointer_accumulators[_event_id] += _delta_position;
//             _pointer_events[_event_id].position = _new_position;
//         }

//     } else if (event_type == EMSCRIPTEN_EVENT_TOUCHEND || event_type == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
//         for (int _pointer_index = 0; _pointer_index < event->numTouches; ++_pointer_index) {
//             const EmscriptenTouchPoint& _touch_point = event->touches[_pointer_index];
//             if (!_touch_point.isChanged) {
//                 continue;
//             }
//             const glm::uint _event_id = static_cast<glm::uint>(_touch_point.identifier);
//             _last_positions.erase(_event_id);
//             _pointer_accumulators.erase(_event_id);
//             _pointer_events.erase(_event_id);
//         }
//     }

//     return 0; // we use preventDefault() for touch callbacks (see Safari on iPad)
// }

static std::atomic<bool> _is_filesystem_ready = false;

void _setup_filesystem()
{
    EM_ASM(
        FS.mkdir('/data');
        FS.mount(IDBFS, {}, '/data');
        FS.syncfs(true, function(_err) {
            if (_err) {
                console.error('IDBFS sync failed:', _err);
            }
            setValue($0, 1, 'i8'); });, &_is_filesystem_ready);
}

#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
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

static void _setup_platform()
{
#if defined(__ANDROID__)
    _is_mobile = true;
    _android_app->onAppCmd = _android_on_app_cmd;
    _android_app->onInputEvent = _android_on_input;
#endif

#if defined(__EMSCRIPTEN__)
    _is_mobile = _navigator_get_touch_points() > 1;
    if (_is_mobile) {
        // emscripten_set_touchstart_callback("#canvas", 0, 1, _touch_callback);
        // emscripten_set_touchend_callback("#canvas", 0, 1, _touch_callback);
        // emscripten_set_touchmove_callback("#canvas", 0, 1, _touch_callback);
        // emscripten_set_touchcancel_callback("#canvas", 0, 1, _touch_callback); // EMSCRIPTEN_EVENT_TARGET_WINDOW doesnt seem to work on safari
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
    _setup_filesystem();
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
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
}

static void _setup_opengl()
{
#if defined(__ANDROID__)
    const EGLint _egl_config_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    const EGLint _egl_context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    _android_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(_android_display, nullptr, nullptr);
    EGLConfig _egl_config;
    EGLint _egl_configs_count;
    eglChooseConfig(_android_display, _egl_config_attribs, &_egl_config, 1, &_egl_configs_count);
    EGLint _egl_format;
    eglGetConfigAttrib(_android_display, _egl_config, EGL_NATIVE_VISUAL_ID, &_egl_format);
    ANativeWindow_setBuffersGeometry(_android_app->window, 0, 0, _egl_format);
    _android_surface = eglCreateWindowSurface(_android_display, _egl_config, _android_app->window, nullptr);
    _android_context = eglCreateContext(_android_display, _egl_config, EGL_NO_CONTEXT, _egl_context_attribs);
    eglMakeCurrent(_android_display, _android_surface, _android_surface, _android_context);
    _android_has_window = true;
#endif

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
        throw std::runtime_error("failed to create WebGL2 context on this device");
    }
    emscripten_webgl_make_context_current(_webgl_context);
#endif
}

static void _setup_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
#if defined(__ANDROID__)
    ImGui_ImplAndroid_Init(_android_app->window);
#endif
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    ImGui_ImplGlfw_InitForOpenGL(_glfw_window, true);
#endif
    ImGui::StyleColorsLight();
    ImGuiIO& _io = ImGui::GetIO();
    _io.IniFilename = NULL;
    install_fonts();
    install_theme();
    ImGui_ImplOpenGL3_Init("#version 300 es");
}

static void _update_poll_events()
{
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    glfwPollEvents();
#endif
}

static void _update_screen_size()
{
#if defined(__ANDROID__)
    eglQuerySurface(_android_display, _android_surface, EGL_WIDTH, &_screen_width);
    eglQuerySurface(_android_display, _android_surface, EGL_HEIGHT, &_screen_height);
#endif
#if defined(__EMSCRIPTEN__)
    _screen_width = _canvas_get_width();
    _screen_height = _canvas_get_height();
#endif
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    glfwGetFramebufferSize(_glfw_window, &_screen_width, &_screen_height);
#endif
}

static void _update_swap_buffers()
{
#if defined(__ANDROID__)
    eglSwapBuffers(_android_display, _android_surface);
#endif
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    glfwSwapBuffers(_glfw_window);
#endif
}

static void _update_imgui_platform_new_frame()
{
#if defined(__ANDROID__)
    ImGui_ImplAndroid_NewFrame();
#endif
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    ImGui_ImplGlfw_NewFrame();
#endif
}

static void _update_loop()
{
#if defined(__EMSCRIPTEN__)
    if (!_is_filesystem_ready) {
        return; // TODO ASYNCIFY EM_ASYN_JS instead
    }
#endif
    _update_poll_events();
    _update_screen_size();
    if (_screen_width == 0 && _screen_height == 0) {
        return;
    }
    glViewport(0, 0, _screen_width, _screen_height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _update_imgui_platform_new_frame();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(_screen_width), static_cast<float>(_screen_height));
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    draw_app();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    _update_swap_buffers();
}

static void _setup_update()
{
#if defined(__ANDROID__)
    while (true) {
        int _ident;
        int _events_count;
        android_poll_source* _poll_source = nullptr;
        while ((_ident = ALooper_pollOnce(_android_has_window ? 0 : -1, nullptr, &_events_count, (void**)&_poll_source)) >= 0) {
            if (_poll_source) {
                _poll_source->process(_android_app, _poll_source);
            }
            if (_android_app->destroyRequested) {
                // lucaria::destroy_opengl();
                // lucaria::destroy_openal();
                return;
            }
        }
        if (_android_engine_initialized && _android_has_window && _android_surface != EGL_NO_SURFACE) {
            _update_loop();
        }
    }
#endif

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(_update_loop, 0, EM_TRUE);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(_glfw_window)) {
        _update_loop();
    }
    glfwDestroyWindow(_glfw_window);
    glfwTerminate();
#endif
}

}

#if defined(__ANDROID__)
extern "C" void android_main(struct android_app* app)
#else
int main()
#endif
{
#if defined(__ANDROID__)
    _android_app = app;
#endif
    _setup_platform();
#if !defined(__ANDROID__)
    _setup_opengl();
    _setup_imgui();
#endif
    _setup_update();
#if !defined(__ANDROID__)
    return 0;
#endif
}