# 3. Graphics
Understanding how the graphics works in MieEngine or in any other game
for that matter, is important for knowing which elements make the
games more fun.

## Crash course on graphics
Most computers nowadays have a processor known as the Graphics Processing Unit (GPU).
The GPU can either be integrated inside the CPU (which is known as an iGPU),
or it can be a discrete, separate unit found inside a graphics card.

The graphics library provides an interface (API) through which the
programmer can use the GPU to do various tasks.
Some of the most well-known graphics libraries are OpenGL, DirectX and Vulkan.
MieEngine currently uses DirectX9, but support for DirectX11 and OpenGL are planned.


Now, which algorhithms can be ran on the GPU?
To achieve optimal performance, the GPU does simple operations through massive parallelism.
This means that the CPU, while it's good at being smart and doing everything with branch prediction,
a GPU does a ton of calculations using the path of least resistance.

A modern CPU only has 8 cores, but a GPU can have thousands of smaller cores
that do simple operations.

A "low-end" monitor (by today's standards) has a resolution of 1980x1080 pixels.
It's a very costly task to fill up a resolution like this,
especially if it's on a CPU.

Usually, games would want to have at least 60 FPS, which means that
there is a time budget of 16.7 milliseconds per frame ("tick").


Over the past decades, some algorhithms have been made so that
programmers can display things to the screen. Some of them are
raycasting, raytracing and rasterization.

Raycasting involves shooting out rays from the camera's perspective,
and then checking the intersection between the rays and the game's world.
The rays are only projected from a horizontal plane.

The distance ("depth") of each ray can be determined, which is used.
to scale up in height a small vertical strip of pixels.
If the object is further away from the player, the vertical strip would be shorter.
If it's close, then it would be taller.

However, this makes the game have this sort of "fake 3D" or 2.5D rendering.
This is because the calculations are effectively done on a 2D map.
In games that use pure raycasting, you cannot really look up and down
without the image being distorted massively.
(it lacks the 3rd focal point that makes parallel vertical lines converge)

Anyways, raycasting is a method that worked well for boomer shooter games like Wolf3D or DOOM.
Their simple graphics meant that the games could be ran in real time, even on old computers.

Raytracing is like raycasting, but instead of only using a small horizontal portion of
the world to determine the pixel values, it uses the entire viewport of the screen.
Rays are shot from each pixel, using the perspective projection matrix to slightly distort the ray's direction.
This means that, while the ray at the middle of the screen was not that distorted, the ones near the borders surely are,
and these would bound the so-called "frustum" of the camera.

This allowed the world to truly appear 3D.
The cinema industry used raytracing, in order to render the CGI scenes from the 90s.
Raytracing replicates real-life optical phenomena, like reflections and refractions.
The problem here was that this process was incredibly slow. People had to wait for several days
before a single frame could finish drawing.

Furthermore, raytracing became a viable option for real-time renderers only
because of recent advances in GPU hardware.

## Graphics optimization techniques (Side note)
For raycasing, one naive approach for finding the collision detection between the projected
ray and the game's scene is to increment the distance by a small number.

Before that, let's talk about what a ray is:
A ray is made out of a position, a direction vector, and a distance.
The direction vector is most likely normalized, so that it can be multiplied by the distance,
to get the resulting point from that distance.
```
p(t) = p0 + d0*t
```

This resulting point is used to check collision detection between the game's world,
every single iteration, which is very inefficient, since it's an O(n) time complexity.
It's also very inaccurate. A large distance step would cause the ray to phase out of a wall,
without detecting it, and a smaller one would cause the game to lag.

Furthermore, many things in raycasting and raytracing are based on exact, geometric calculations.
This means that a function would calculate the distances directly, without
using iterative, fixed marching steps. This is good for simple objects such as spheres or boxes.

Also, there are many techniques that can be used to optimize this ray-world collision detection,
which are not just specific to raycasting or raytracing.

For example, DDA (Digital Differential Analyzer) is a very fast algorhithm for tiled worlds
(where the rectangles are axis-aligned and have the same size, and are closely-packed together),
because it only increments the ray's distance based on its direction and the surrounding tile borders,
and it checks the current tile's value, which is an O(1) operation, compared to the O(n) checking of different shapes.

There are also BVH, Octrees and BSP, and these are called "spatial acceleration structures".

A spacial acceleration structure would subdivide the world's space into smaller units,
so that they can be more easily calculated.

The BVH does this by obtaining a mass of axis-aligned bounding boxes (AABBs),
which are added in a hierarchy. The ray only hits an object if it collides with the subsequent boxes of this hierarchy.
This is good for triangle meshes, because they usually have a LOT of triangles (300-2000 to 30000-100000),
but not so much for simple objects like spheres, whose distances can be determined directly.

Octrees recursively split a large box into 8 boxes, and some of them into 8 boxes, and so on,
which allows it to more sparsely represent voxel (3D tiled) worlds.
This means that a ray can quickly ignore large amounts of empty space.

BSPs split the space into planes. The resulting regions are placed in a binary tree.
This technique was used extensively in the DOOM and Quake games,
so that they can quickly render only the things that are visible to the player.

For example, DOOM had no overdraw, despite not using a depth buffer to store the distances from each pixel.


Draw calls are
