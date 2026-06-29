#pragma once

#ifdef _WIN32
        #include <windows.h>
        #include <GL/wglext.h>
#else
        #include <wayland-client.h>
        #include <wayland-egl.h>
        #include <EGL/egl.h>
#endif

#include <stdexcept>
#include <string>


#define ERROR(msg) throw std::runtime_error(std::string("[ERROR] ") + __FILE__ + "@" + std::to_string(__LINE__) + " (" + __func__ + "): " + (msg))


class GOpenGL { private:
        #ifdef _WIN32
                HDC hdc; HGLRC hglrc;
        #else
                EGLDisplay egl_display; EGLSurface egl_surface;
        #endif

public:
        explicit GOpenGL(
                void* native_window_handle,  // HWND on Windows, wl_surface* on Wayland
                void* native_wayland_display = nullptr  // wl_display* on Wayland
        ) {
                #ifdef _WIN32
                {
                        typedef BOOL WINAPI (*PFN_wglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
                        PFN_wglChoosePixelFormatARB wglChoosePixelFormatARB = nullptr;
                        typedef HGLRC WINAPI (*PFN_wglCreateContextAttribsARB)(HDC hdc, HGLRC hShareContext, const int *attribList);
                        PFN_wglCreateContextAttribsARB wglCreateContextAttribsARB = nullptr;

                        // Windows shenanigans to get `wglChoosePixelFormatARB` & `wglCreateContextAttribsARB`
                        WNDCLASSA _temp_wc{}; _temp_wc.style = CS_OWNDC; _temp_wc.lpfnWndProc = DefWindowProcA; _temp_wc.hInstance = GetModuleHandle(0); _temp_wc.lpszClassName = "_temp_WGL";
                        if (!RegisterClassA(&_temp_wc)) ERROR("Failed to register temporary WGL window class");
                        HWND _temp_window = CreateWindowA(_temp_wc.lpszClassName, "", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, _temp_wc.hInstance, 0); if (!_temp_window) ERROR("Failed to create temporary WGL window");
                        HDC _temp_dc = GetDC(_temp_window); if (!_temp_dc) ERROR("Failed to get temporary WGL window's device context");
                        PIXELFORMATDESCRIPTOR _temp_pfd{}; _temp_pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); _temp_pfd.nVersion = 1; _temp_pfd.dwFlags = (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER); _temp_pfd.cColorBits = 32; _temp_pfd.cAlphaBits = 8; _temp_pfd.cDepthBits = 24; _temp_pfd.cStencilBits = 8;
                        int _temp_pixel_format = ChoosePixelFormat(_temp_dc, &_temp_pfd); if (!_temp_pixel_format) ERROR("Failed to find a suitable pixel format");
                        if (!SetPixelFormat(_temp_dc, _temp_pixel_format, &_temp_pfd)) ERROR("Failed to set pixel format");
                        HGLRC _temp_hglrc = wglCreateContext(_temp_dc); if (!_temp_hglrc) ERROR("Failed to create temporary WGL context");
                        if (!wglMakeCurrent(_temp_dc, _temp_hglrc)) ERROR("Failed to make temporary WGL context current");
                        wglChoosePixelFormatARB = (PFN_wglChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB"); if (!wglChoosePixelFormatARB) ERROR("Failed to get address of `wglChoosePixelFormatARB`");
                        wglCreateContextAttribsARB = (PFN_wglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB"); if (!wglCreateContextAttribsARB) ERROR("Failed to get address of `wglCreateContextAttribsARB`");
                        wglMakeCurrent(_temp_dc, 0); wglDeleteContext(_temp_hglrc); ReleaseDC(_temp_window, _temp_dc); DestroyWindow(_temp_window);

                        this->hdc = GetDC((HWND)native_window_handle); if (!this->hdc) ERROR("Failed to get handle to device context");
                        int _wgl_pixel_format_attribs[] = {WGL_DRAW_TO_WINDOW_ARB, 1, WGL_SUPPORT_OPENGL_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, WGL_COLOR_BITS_ARB, 32, WGL_DEPTH_BITS_ARB, 24, WGL_STENCIL_BITS_ARB, 8, 0};
                        int _wgl_pixel_format; UINT _num_wgl_pixel_formats;
                        wglChoosePixelFormatARB(this->hdc, _wgl_pixel_format_attribs, 0, 1, &_wgl_pixel_format, &_num_wgl_pixel_formats); if (!_num_wgl_pixel_formats) ERROR("`wglChoosePixelFormatARB()` failed");
                        PIXELFORMATDESCRIPTOR _pfd; if (!DescribePixelFormat(this->hdc, _wgl_pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &_pfd)) ERROR("`DescribePixelFormat()` failed");
                        if (!SetPixelFormat(this->hdc, _wgl_pixel_format, &_pfd)) ERROR("`SetPixelFormat()` failed");
                        int _wgl_context_attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB, 6, WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
                        this->hglrc = wglCreateContextAttribsARB(this->hdc, 0, _wgl_context_attribs); if (!this->hglrc) ERROR("Failed to create WGL context");
                        if (!wglMakeCurrent(this->hdc, this->hglrc)) ERROR("Failed to make WGL context current");
                }
                #else
                {
                        if (!native_wayland_display) ERROR("Invalid wl_display*");
                        this->egl_display = eglGetDisplay((wl_display*)native_wayland_display); if (this->egl_display == EGL_NO_DISPLAY) ERROR("eglGetDisplay() failed");
                        EGLint _gl_major, _gl_minor;
                        if (eglInitialize(this->egl_display, &_gl_major, &_gl_minor) != EGL_TRUE) ERROR("Failed to initialize OpenGL");
                        if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) ERROR("Failed to bind OpenGL API");
                        static const EGLint EGL_CONFIG_ATTRIBUTES[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
                        EGLConfig egl_config; EGLint _num_config;
                        if (eglChooseConfig(this->egl_display, EGL_CONFIG_ATTRIBUTES, &egl_config, 1, &_num_config) != EGL_TRUE) ERROR("eglChooseConfig() failed");
                        static const EGLint EGL_CONTEXT_ATTRIBUTES[] = {EGL_CONTEXT_MAJOR_VERSION, 4, EGL_CONTEXT_MINOR_VERSION, 6, EGL_NONE};
                        EGLContext egl_context = eglCreateContext(this->egl_display, egl_config, EGL_NO_CONTEXT, EGL_CONTEXT_ATTRIBUTES); if (egl_context == EGL_NO_CONTEXT) ERROR("eglCreateContext() failed");
                        wl_egl_window* egl_window = wl_egl_window_create((wl_surface*)native_window_handle, window_width, window_height); if (egl_window == nullptr) ERROR("wl_egl_window_create() failed");
                        this->egl_surface = eglCreateWindowSurface(this->egl_display, egl_config, egl_window, NULL); if (this->egl_surface == EGL_NO_SURFACE) ERROR("Failed to create EGL surface");
                        if (!eglMakeCurrent(this->egl_display, this->egl_surface, this->egl_surface, egl_context)) ERROR("Failed to make EGL context current");
                }
                #endif
        }

        void SwapBuffers() {
                #ifdef _WIN32
                        ::SwapBuffers(this->hdc);
                #else
                        eglSwapBuffers(this->egl_display, this->egl_surface);
                #endif
        }

        ~GOpenGL() {
                #ifdef _WIN32
                        wglDeleteContext(this->hglrc);
                #else
                        eglTerminate(this->egl_display);
                #endif
        }
};
