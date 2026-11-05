#pragma once
// A 5x7 bitmap font so the output images can carry labels.
// Text drawing is itself a raster operation: each glyph is a bit mask that is
// tested bit by bit and copied into the framebuffer.
#include <string>

#include "core/Canvas.h"

// Draws text with its top-left corner at (x, y). Lower-case letters are shown
// as upper-case. Each font pixel becomes a scale x scale square.
void drawText(Canvas& canvas, int x, int y, const std::string& text, Color c, int scale = 2);
int textWidth(const std::string& text, int scale = 2);
