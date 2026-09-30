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

This allowed the world to truly appear 3D.
The cinema industry used raytracing, in order to render the CGI scenes from the 90s.
Raytracing replicates real-life optical phenomena, like reflections and refractions.
The problem here was that this process was incredibly slow. People had to wait for several days
before a single frame could finish drawing.




Draw calls are
