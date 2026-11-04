# Voxel World Builder: Graphics Engine
**Project Title:** Voxel World Builder
**Description:** A lightweight 3D voxel sandbox engine. Users will be able to navigate a 3D grid and place/remove colored blocks. 

### Group Members and Responsibilities
*   **[Mary Queenvine Muthoni]**: Graphics algorithms (Line & Circle drawing implementation).
*   **[Emmanuel Keter]**: Polygon filling, data structures, and clipping logic.
*   **[Louis Karanja]**: Math logic, Bezier curves, and coordinates.

### Technologies & Tools
*   **Language:** Pure C++ (C++17)
*   **Compiler:** GCC / g++
*   **Output Format:** `.ppm` (Portable Pixmap Image)
*   **Version Control:** Git & GitHub

### Graphics Techniques Implemented (Checkpoint 1)
To meet the Pure C++ requirement, we built a CPU software rasterizer to prove our understanding of foundational algorithms before migrating to OpenGL:
1.  **Line Drawing:** Implemented Bresenham's Line Algorithm to draw the 3D wireframe edges of our core Voxel component.
2.  **Circle Representation:** Implemented the Midpoint Circle Algorithm to render the central UI targeting crosshair used for block selection.
3.  **Polygon Representation & Filling:** Created boundary rendering and a basic scanline polygon fill algorithm to color the faces of the Voxel.

### How to Compile and Run
Ensure you have a C++ compiler installed (like `g++`). Run the following commands in the terminal from the root directory of the project:

```bash
cd src
g++ main.cpp -o renderer
./renderer