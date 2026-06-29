# Usage

```c++
GOpenGL gogl(NATIVE_WINDOW_HANDLE); // HWND on Windows, wl_surface* on Wayland  (also pass wl_display* if on Wayland)
// ...
gogl.SwapBuffers();
```