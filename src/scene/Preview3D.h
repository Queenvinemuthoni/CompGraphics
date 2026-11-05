#pragma once
// Early 3D preview (ahead of CP2/CP3): perspective projection, z-buffer,
// back-face culling, hidden-face culling and Lambert diffuse lighting,
// all done on the CPU.
#include "core/Canvas.h"

void renderPreview3D(Canvas& canvas);
