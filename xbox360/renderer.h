#ifndef SMW_XBOX360_RENDERER_H
#define SMW_XBOX360_RENDERER_H

#include <stdbool.h>
#include <stdint.h>

bool SmwXenonRendererOpen(uint32_t width, uint32_t height);
bool SmwXenonRendererDraw(const uint8_t *pixels, uint32_t pitch);
void SmwXenonRendererClose(void);

#endif
