#ifndef BK_RENDER_WINDOW_SURFACE_H
#define BK_RENDER_WINDOW_SURFACE_H
/* Native window presentation supplied by a desktop platform backend.
 *
 * The renderer still owns its VkInstance and device. A backend fills this
 * before renderer creation: the instance extensions its window system needs,
 * a callback that creates the platform surface from the renderer's instance,
 * and the desired initial extent. Vulkan handles stay opaque here so the
 * public renderer/platform headers do not depend on <vulkan/vulkan.h>.
 *
 * create() borrows userdata for the lifetime of the renderer and reports a
 * VkSurfaceKHR through *surface. It is called exactly once, after the
 * VkInstance exists and before device selection. */
typedef struct BkWindowSurface {
  const char *const *extensions;
  unsigned extension_count;
  int (*create)(void *userdata, void *instance, void **surface,
                char error[256]);
  void *userdata;
  unsigned width, height;
} BkWindowSurface;
#endif
