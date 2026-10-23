#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "renderer.h"
#include "smw_xbox360_shaders.h"
#include "xsdk/gpu.h"
#include "xsdk/log.h"
#include "xsdk/present.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"

enum { BUFFER_COUNT = 2, SWAP_INTERVAL = 1, BEGIN_TIMEOUT_MS = 5000, STRIDE_BYTES = 16 };
static const XsdkVertexElement kLayout[] = {
  { .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 0 },
  { .usage = XSDK_USAGE_TEXCOORD, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 8 },
};
typedef struct Vertex { float x, y, u, v; } Vertex;
static const Vertex kQuad[4] = {
  { -1.0f, -1.0f, 0.0f, 1.0f },
  {  1.0f, -1.0f, 1.0f, 1.0f },
  { -1.0f,  1.0f, 0.0f, 0.0f },
  {  1.0f,  1.0f, 1.0f, 0.0f },
};

static XsdkShader g_vertex_shader, g_pixel_shader;
static XsdkTexture g_frame_texture;
static XsdkSamplerState g_sampler;
static volatile uint32_t *g_vertices;
static uint8_t *g_staging;
static uint32_t g_source_width, g_source_height, g_display_width, g_display_height;
static bool g_gpu_open, g_present_open, g_renderer_open;
static bool g_first_present_logged;

void SmwXenonRendererClose(void) {
  if (g_present_open) {
    xsdk_present_close();
    g_present_open = false;
  }
  if (g_staging) {
    free(g_staging);
    g_staging = NULL;
  }
  if (g_vertices) {
    xsdk_gpu_free(g_vertices);
    g_vertices = NULL;
  }
  if (g_frame_texture.texels)
    xsdk_texture_free(&g_frame_texture);
  xsdk_shader_free(&g_pixel_shader);
  xsdk_shader_free(&g_vertex_shader);
  if (g_gpu_open) {
    NTSTATUS status = xsdk_gpu_close();
    if (!NT_SUCCESS(status))
      xsdk_log_write("smw: xsdk_gpu_close failed status=0x%08X", (unsigned)status);
    g_gpu_open = false;
  }
  g_renderer_open = false;
}

bool SmwXenonRendererOpen(uint32_t width, uint32_t height) {
  xsdk_log_write("smw: renderer open begin frame=%ux%u", (unsigned)width, (unsigned)height);
  if (!width || !height || width > XSDK_TEXTURE_MAX_SIZE || height > XSDK_TEXTURE_MAX_SIZE)
    return false;

  g_source_width = width;
  g_source_height = height;
  NTSTATUS status = xsdk_gpu_open();
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: xsdk_gpu_open failed status=0x%08X", (unsigned)status);
    return false;
  }
  g_gpu_open = true;
  xsdk_log_write("smw: gpu open ok");

  status = xsdk_present_open(XSDK_DISPLAY_720P, BUFFER_COUNT, SWAP_INTERVAL, &g_display_width, &g_display_height);
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: xsdk_present_open failed status=0x%08X", (unsigned)status);
    SmwXenonRendererClose();
    return false;
  }
  g_present_open = true;
  xsdk_log_write("smw: present open ok display=%ux%u", (unsigned)g_display_width, (unsigned)g_display_height);

  status = xsdk_texture_create(&g_frame_texture, XSDK_FORMAT_A8R8G8B8, width, height, 1, true);
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: xsdk_texture_create failed status=0x%08X", (unsigned)status);
    SmwXenonRendererClose();
    return false;
  }

  status = xsdk_shader_load(&g_vertex_shader, blit_vert.container,
                            blit_vert.container_bytes);
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: vertex shader load failed status=0x%08X", (unsigned)status);
    SmwXenonRendererClose();
    return false;
  }
  status = xsdk_shader_load(&g_pixel_shader, blit_frag.container,
                            blit_frag.container_bytes);
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: pixel shader load failed status=0x%08X", (unsigned)status);
    SmwXenonRendererClose();
    return false;
  }
  status = xsdk_shader_set_vertex_layout(&g_vertex_shader, kLayout, 2,
                                         (const uint32_t[]){ STRIDE_BYTES });
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: vertex layout failed status=0x%08X", (unsigned)status);
    SmwXenonRendererClose();
    return false;
  }

  g_vertices = xsdk_gpu_alloc(sizeof kQuad);
  if (!g_vertices) {
    xsdk_log_write("smw: vertex buffer allocation failed");
    SmwXenonRendererClose();
    return false;
  }
  for (uint32_t i = 0; i < sizeof kQuad / 4; i++) {
    uint32_t word;
    memcpy(&word, (const uint8_t *)kQuad + i * 4, 4);
    g_vertices[i] = word;
  }
  xsdk_gpu_invalidate(xsdk_gpu_address(g_vertices), sizeof kQuad);

  const XsdkSampler sampler = {
    .mag_filter = XSDK_FILTER_POINT, .min_filter = XSDK_FILTER_POINT, .mip_filter = XSDK_FILTER_POINT,
    .address_u = XSDK_ADDRESS_CLAMP, .address_v = XSDK_ADDRESS_CLAMP,
  };
  xsdk_texture_sampler_state(&g_sampler, &sampler);

  g_staging = malloc((size_t)width * height * 4u);
  if (!g_staging) {
    xsdk_log_write("smw: staging buffer allocation failed");
    SmwXenonRendererClose();
    return false;
  }
  g_renderer_open = true;
  xsdk_log_write("smw: renderer ready");
  return true;
}

bool SmwXenonRendererDraw(const uint8_t *pixels, uint32_t pitch) {
  if (!g_renderer_open || !pixels || pitch < g_source_width * 4u)
    return false;

  if (pitch == g_source_width * 4u) {
    memcpy(g_staging, pixels, (size_t)pitch * g_source_height);
  } else {
    for (uint32_t y = 0; y < g_source_height; y++)
      memcpy(g_staging + (size_t)y * g_source_width * 4u,
             pixels + (size_t)y * pitch, (size_t)g_source_width * 4u);
  }
  xsdk_texture_upload(&g_frame_texture, 0, g_staging, g_source_width * 4u);
  xsdk_gpu_invalidate(g_frame_texture.address, g_frame_texture.size_bytes);

  uint32_t address = 0;
  NTSTATUS status = xsdk_present_begin(&address, BEGIN_TIMEOUT_MS);
  if (!NT_SUCCESS(status)) {
    xsdk_log_write("smw: xsdk_present_begin failed status=0x%08X", (unsigned)status);
    return false;
  }

  xsdk_gpu_set_target(g_display_width, g_display_height);
  xsdk_gpu_set_shaders(&g_vertex_shader, &g_pixel_shader);
  xsdk_built_shader_set_literals(&blit_vert);
  xsdk_built_shader_set_literals(&blit_frag);
  xsdk_gpu_set_texture(BLIT_FRAG_SAMPLER_0_0, &g_frame_texture, &g_sampler);
  xsdk_gpu_set_vertices(0, xsdk_gpu_address(g_vertices), sizeof kQuad);
  xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_STRIP, 4);
  xsdk_gpu_resolve(address, g_display_width, g_display_height, 0xFF000000);
  xsdk_present_end();

  if (!g_first_present_logged) {
    xsdk_log_write("smw: first frame queued");
    g_first_present_logged = true;
  }
  return true;
}
