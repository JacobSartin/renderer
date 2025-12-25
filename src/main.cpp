// Renderer: SDL3 + WebGPU (Dawn) + Dear ImGui
// A clean architecture separating event handling from rendering

#include "Application.h"
#include <stdio.h>

int main(int argc, char **argv) {
  Application app;

  if (!app.Initialize(1280, 800, "Renderer - SDL3 + WebGPU + ImGui")) {
    fprintf(stderr, "Failed to initialize application\n");
    return 1;
  }

  app.Run();
  // Destructor will call Shutdown() automatically

  return 0;
}
