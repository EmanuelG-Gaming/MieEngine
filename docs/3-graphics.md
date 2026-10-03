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

Now, rasterization works by treating the viewport as a large grid of pixels (a "raster").
This means that the algorithms used for rasterization are usually pixel-perfect,
which means that the main unit of rendering is a dot placed at the center of each pixel.
(with the exception of certain kinds of antialiasing techniques).

Rasterization takes vertex data, which might have position, texture coordinates, color attributes, etc,
and assembles it into "primitives" (points, lines, triangles)
that can be shown to the screen.

Lines can be drawn using Bresenham's algorithm, which is cheap and effective,
because it avoids using float coordinates.

Meanwhile, triangles can be filled using scanlines, which are horizontal lines that are aligned with the
framebuffer's memory layout. This makes them easy to parallelize, which is good for GPUs.

Rasterization has been the primary method from which graphics libraries draw things to the screen,
for decades.


However, graphics libraries don't work in isolation.
They use the so-called "graphics pipeline", which is a bit more complicated.

Basically, you take your raw vertex data, you tranform it by multiplying matrices (model->view->projection),
you convert it into viewport-space coordinates through homogeneous coordinates and perspective projection,
you assemble the primitives from these vertices, you rasterize these primitives into pixels,
you calculate the distance of each pixel and store it into a depth buffer,
you calculate barycentric coordinates, which are essential for interpolating color and texture attributes across the shape,
and then you "paint" each pixel with a specific color based on the aforementioned barycentric coordinates.
You can apply blending effects and do depth testing with other pixels using this depth buffer.


Though, if you saw graphics library versions from the 2000s and above,
you would realize that this process can be customized.
What we've talked about before, is the so-called "Fixed-function pipeline",
which was good for the GPUs at that time, which could be optimized for those specific workloads.

However, as graphics techniques improved and GPU manufactuers started to have
more different or more performant architectures, the graphics libraries needed to be a bit more "complex".

So they started to introduce these things called "Shaders", which is code that is ran in parallel on the GPU.

At first, there were vertex shaders and fragment/pixel shaders.
Vertex shaders operate on every vertex of a draw call, where matrices would be multiplied
with the position of these vertices to get a 4D vector with homogeneous coordinates,
while fragment shaders color each pixel.

Both of these shaders are executed in parallel.

And then "Compute shaders" also appeared in later graphics API versions,
that allow the programmer to execute more general code on the GPU.

This means that the graphics card is not just a triangle processing unit,
but something where you can program physics simulations in parallel.

Compute shaders and also previous efforts (CUDA, OpenCL)
have led to this.

Anyways, draw calls are functions that are used to draw
your vertices. Graphics libraries provide you with these draw calls, some of them being:

1. A function that draws your vertices directly. This is simpler to
implement, but that also means you have to specify more vertices
to fit with the triangle primitive definition.
2. A function that takes both a vertex buffer and also an index buffer.
The index buffer is also known as the "element" buffer, because it
specifies the connections that form a primitive "element".
If the primitive specifies how these elements should be assembled,
then the index buffer specifies the 16/32-bit indices through the vertex buffer,
which start at `0`, and stop at `n-1`, where `n` is the number of vertices.
3. A function that draws the same buffer but "instanced" multiple times.
This means that the vertex buffer is sent to the GPU once,
and then a large matrix buffer is used to apply matrix transformations
to the vertices in parallel.
This is called "Instancing", and it's a good optimization technique
for drawing hundreds of thousands of meshes that are the same,
but with different coordinates.
It can be used for things like grass blades, trees or asteroids,
because they're very repeated objects.
Without instancing, the CPU would have to send a draw call for every single
object being drawn, which causes driver overhead, slowing down the game tremendously.

The latter technique is used on graphics libraries like OpenGL 3.3 or DirectX11.
Before that, draw calls were pretty expensive.

Programmers in the past knew about this problem. so they came up with something called "Batching".
Basically, instead of having a draw call for every single object at once,
you fill up a large, dynamic buffer with vertex data that you then send ("flush") to the GPU.
A dynamic index buffer is filled pretty much the same way, but with different numbers.

Every time you "flush" a batched renderer, you set the current vertex pointer to 0.
This flushing is done every frame.

In MieEngine, you would draw things in two ways:
1. By using meshes (`DrawRenderMesh()` function)
2. By using the built-in batched state machine. (`draw.h`)

A state machine is a software design pattern that uses state transitions
to handle changes. These transitions can be implemented in functions
that check if the argument is different from the current state,
and if so, it sets the current state to that value,
and then does an action based on that change.

MieEngine's state machine works the same way, much like OpenGL's hidden, global state machine.
It retains variables about textures, blending states, culling, depth testing, etc.

A transition function in the state machine would look something like this:
```cpp
void DrawSetState(State state)
{
    if (drawState.state != state)
    {
        DrawFlush();
        drawState.state = state;

        // Set internal graphics library state.
        // d3device->SetRenderState(D3DRS_STATE, GetD3DStateObject(state));
    }
}
```

The `DrawFlush()` function renders any of the previous vertices that weren't modified.
The `drawState.state = state` simply sets the current state,
and then you apply the changes to the graphics API.

If you think about it, adding vertices to a render batch is kinda like a stack.
You copy the vertices to a large array, and then you increment a stack pointer.

Except in batching, this mechanism sort of works like a queue.
You push elements to the top, like a stack,
and when you "flush" the vertices,
the graphics card would process the elements beginning from the base (the first added),
up until the top (stack pointer), causing them to be removed from the buffer,
by setting the stack pointer to 0.


Anyways, to use the render batch of MieEngine,
you can use almost any function that begins with `Draw`.

In our previous example from chapter 2,
we can add a `DrawRect(w, h)` function
with the specified lengths of this rectangle
being the Width and the Height.

This rectangle is centered on the screen by default.

We then explicitly tell the state machine to `DrawFlush()` any
vertices, since the `DrawRect()` function
uses `DrawVertex3D()` and `DrawIndices()` functions,
which flush the batch only until the vertex capacity is reached.

You can call almost any drawing function you want,
but without `DrawFlush()`, these vertices won't be rendered to the screen.

```cpp
// In the main game loop:

DrawClear(0x001155);
DrawBegin();

DrawReset();
DrawRect(1.0f, 1.0f);

DrawEnd();
```

The `DrawBegin()` and `DrawEnd()` functions
make it clear to the graphics library that this is the
area where we render things to the screen.

You can also do other things here, like update some small things,
but otherwise, it's a good practice to separate the logic of the game
from the rendering. This makes the code more organized and
you can fix some data mismatches.

The `DrawReset()` function calls `DrawFlush()` and
then it sets the variables of the draw state to some default values
that are good enough for showing simple objects.

After that, we use the `DrawRect()` function to draw a white rectangle.

But can we really draw anything without calling `DrawFlush()`?
The short answer is no.
The long answer is complicated.

What if we try to push the drawing API to its limits?
We can do that by making a counter variable `t` that's
incremented by 1 after every frame,
and then use that to repeatedly draw rectangles.

```cpp
// When initializing:
int t = 0;

// In the game loop:
for (int i = 0; i < t; ++i)
{
    DrawRect(0.5, 0.5);
}
printf("t: %d\n", t);

t++;
```

After some time, you would start to see a flashing rectangle
that gets more and more visible as time passes.

The overflow of vertices causes the batched renderer
to trigger a `DrawFlush()` by itself,
through the `DrawPreFlush()` function that's called by `DrawVertex3D()`.

Remember, only the `DrawFlush()` function actually clears the vertices
by setting the vertex pointer to 0.

In the previous example, it took long enough for the vertices to accumulate,
that we didn't see anything on the screen in a short amount of time,
but now that we repeatedly draw stuff to the screen,
these changes are made much more clear.

After roughly `t = 380`, we start to see a solid rectangle.
This means that the number of vertices drawn every frame
has exceeded the vertex capacity (`VBO_MAX_SIZE` bytes), and now the renderer is forced
to submit vertices for the next draw call.

The calculation was done by: `VBO_MAX_SIZE` is 0x8000 bytes (32768 bytes),
which is the 16-bit number limit.
The default vertex has a size of `24` bytes,
so there's a vertex capacity of `32768/24 = 1365` vertices.

Every frame, the `DrawRect()` function adds 4 vertices to the buffer.
This meant that in the previous example,
it would take `1364/4 = 341` frames for a `DrawFlush()` to be triggered,
before the rectangle would flicker on the screen, awaiting the next flush/draw call.

## Color modes
To use colors in the batched renderer, MieEngine provides
three functions:
- `DrawColor(r, g, b, a)`
- `DrawColor2(r, g, b, a)`
- `DrawColorMode(mode)`

Each of these colors are arrays of 4 floats,
and these floats range from 0 to 1 (the SDR range).

The engine uses a pallette of two colors, so that
we can draw more advanced graphical effects.

`DrawColorMode()` uses a `ColorMode` enum, which has
some of the following modes:
- `COLOR_LR`: Draws a gradient from left to right.
- `COLOR_UD`: Draws a gradient from up to down.
- `COLOR_INOUT` and `COLOR_OUTIN` are used
for ellipses/circular shapes.


## Matrices
### Theory
Perhaps one of the most advanced topics we're
gonna talk about are matrices.

Matrices and vectors are part of the mathematical
field called "Linear Algebra".

You may have known about vectors from school,
as these pointed arrows that have
a magnitude and a direction (from physics).

But in computer science, vectors and matrices are arrays of numbers.
They are used extensively in graphics programming,
to represent linear transformations and perspective projections.

A linear transformation is like turning a vector from one basis to another.
Matrices are really good for representing a vector basis.

```
X = a*i + b*j + c*k
```

Where `(a, b, c)` are the coordinates of the `X` vector.
The `i`, `j` and `k` are the basis vectors, which
are the direct columns of a matrix.

There are two conventions we can use for representing matrices:
- Column-major ordering.
- Row-major ordering.

Column-major ordering is what you would commonly find on most math textbooks,
because the vectors are represented as columns.
It's what's used by OpenGL and also programming languages like Fortran or Matlab.

Row-major ordering is what's commonly used to represent memory,
because it's read from left to right, like a book, according to the human eyes.
It's what's used by DirectX by default (you can change it to a column-major matrix in HLSL),
and also by C and C++.


Matrix transposition can be used to change the ordering of a matrix,
meaning that rows become columns and columns become rows.


In graphics programming, we often use square matrices,
to represent linear/affine transformations, such as
rotations, scaling and shearing.
Rotations can be considered a shearing in two/three axes.

We also use 4x4 square matrices to be multiplied with 4D vectors
made from 3D ones, and the reason for that can be found below.


An identity matrix has a 1 on the main diagonal, and 0 everywhere.
One of the properties of identity matrices is that they
don't change the resulting matrix/vector after it was multiplied.

A scale matrix is like an identity matrix, except every number on the diagonal
is a specific scaling factor.

A shearing matrix will lean the vector space's grid sideways.

A rotation matrix rotates points around the origin,
while keeping the basis vectors perpendicular, at the same length.
This rotation can be done along the Z axis, which is what's commonly used in 2D games.
Rotations can also be specified in the X and Y axes.

Generally, you would want to use something called a "Quaternion", which is a number
with 4 components that can compress a 3x3 matrix with 9 components.
Quaternions extend the complex number plane, and they have one real part `w`,
used for the angle, while the imaginary parts `(x, y, z)` are used for specifying the axis of rotation.

Quaternions can be smoothly interpolated, through something called "Slerp" (or spherical interpolation).



However, linear transformations can only represent scaling, rotations or shearing.

If translation simply means adding the components of a vector with the components
of a displacement vector, why can't we just multiply a 2D matrix with a 2D vector,
and achieve the same effect as if we're translating that vector?

You may have now realized that using a shearing matrix means swaying the square
formed by the basis vectors in a sideways direction, turning it into a parallelogram.
The same principle can be applied to a cube in 3D space.

If you take the z component of this cube as being 1, you're basically
shearing the far-away face of the cube. You're not really changing this face,
you're just moving it aroud.

So now that we're bumping up the 2D vector into a 3D one by setting the z component
to be 1, the translation matrix gives us the same
effect as if we're shearing a cube in 3D.

We're also gonna bump up the 2x2 matrix into a 3x3 one and we use
a specific formula for this translation matrix.

This is called "Homogeneous Coordinates",
where we add an additional coordinate to our vectors and matrices.

The same technique can also be used for 3D vectors and 4x4 matrices.
If a vector has a homogeneous coordinate of 1, that means it's a position.
If it's 0, it's a direction, having a length and an orientation.

Directions are not affected by translations, because the `w` component is 0,
so the 4th column of the translation matrix has no effect on the result.
If a direction points up, then translating it will not change its up orientation.

Homogeneous coordinates also allow us to represent more "non-linear" transformations,
like perspective projection.

Perspective makes it so that objects further away are smaller, while closer ones are larger.

Homogeneous coordinates achieve this perspective effect, by
modifying the value of `w` (to something different from 0 or 1),
followed by a division of each component with `w`, to
get to the flat, screen-space coordinates.
In this context, `w` is the distance to the camera.


In graphics programming, model, view, and projection matrices
are used to get 3D objects to be visible on the 2D screen.

Model matrices are simply used to position the object in the world space.

The view matrix applies the inverse transformations (of rotations, dot products, translations),
in order to move points towards the camera space, since a camera really is placed at (0, 0, 0),
and the whole world simply revolves around it.

The projection matrix applies distortion to the objects,
in order to fit these 3D points onto a 2D screen.
The GPU also does perspective division.


### Practice
To use model matrices in MieEngine, you can use functions
like `DrawMatIdent()`, `DrawMatTranslate3D()` or `DrawMatRotate3D()`.

The latter two functions are additive, meaning that the
current matrix is multiplied continuously by the offset matrix, rather than strictly
being set to that matrix, which means that it's more optimized
and you avoid being rigid (as in having a limited amount of matrices
being used to multiply each vertex).


`DrawMatIdent()` sets the current matrix to an identity matrix.
It's good for resetting this matrix, while still
keeping the following objects visible.

`DrawMatTranslate(x, y, z)` translates the current matrix by an `(x, y, z)` offset.

`DrawMatRotate3D(x, y, z)` rotates the current matrix by the x, y and z axes.
This means that rotations happen perpendicular to those axes.
For example, a rotation in the z axis is commonly used for 2D games,
because we assume that the x and y axes represent the viewport space.


Additionally, the batched renderer also uses a "matrix stack".

A matrix stack is an array of 4x4 matrices.
By default, all matrix operations (`DrawMatIdent()`, `DrawMatTranslate()`, etc)
will only modify the first element of this array.

When you call `DrawPushMat()`, you increment the stack pointer,
and then you copy that matrix to the matrix at the new index,
restoring its state for the next use.

You can freely modify the matrix at this index (stack pointer),
and when you call `DrawPopMat()`, you simply decrement the stack pointer.

The engine will only use the top element from the matrix stack
to transform the vertices.
This means that, ideally, your matrices should mostly operate at the 0 index.

But this can also be used to make more hierarchical animations.
Remember that the number of pushes = number of pops,
so that the stack pointer can remain 0 and not do stack overflows/underflows.

```cpp
// Draw base body.
DrawMatrixShenanigans123(&torso);
Draw_torso(&torso);

// Draw arm.
DrawPushMat();
DrawMatTranslate(0.25f, 0.5f, 0.0f);
DrawMatRotateX(arm.angle);
Draw_arm(&arm);
DrawPopMat();

// Draw the rest of your objects normally.
```

In this example, when you call `DrawPushMat()`, all the movements
of the arm will be relative to the body.

In general, a stack is a good data structure that can be used
to retain a "history" of actions, because of the LIFO principle.

For example, in a drawing program, you might have a
series of actions (like putting a brush stroke),
and these actions are placed in a stack called the "Undo stack".

Every time you press the "Undo" button,
you pop the top element from the undo stack and you apply
the inverse transformation (so the pixels of that brush stroke get reverted).

But to add the "Redo" functionality, you make another stack
of actions, this time you push things to that stack when you
press the "undo" button.
And when you press the "redo" button, you pop the top element
from the redo stack and you apply the transformation to the drawing.


## Draw passes
The rendering system of MieEngine is based on Draw Passes.

You can think of a draw pass as a point of view in a specific scene.
It stores the fog state, Z buffering, face culling, etc. of a scene,
and also the viewport state.


Now, these render passes are cycled continuously in the game loop,
so you might have something like this:
```cpp
DrawBegin();

// Draw 1st pass.
{
    drawState.currentPass = 0;
    Scene1_draw();
}

// Draw 2nd pass.
{
    drawState.currentPass = 1;

    Scene1_draw();
}

// Draw 3rd pass (for 2nd scene).
{
    drawState.currentPass = 2;

    Scene2_draw();
}

DrawEnd();
```

This technique is called "Multi-pass rendering".
The draw passes can be drawn at different angles or with different effects.

For example, you might have a virtual camera
looking in one direction. This is one draw pass.
By default, this pass would be rendered to the screen directly,
but by using something called a "Framebuffer", you can render the scene that's seen
by that camera, into a texture. A framebuffer is a memory region that's updated continuously
by "capturing" any of the incoming draw requests.
The same thing can be applied to another camera, which renders the same scene
to a different framebuffer.

Both of these textures can be drawn to a monitor screen, which shows the view of both cameras.
The player can then see this virtual monitor on their screen.

Another use of draw passes is with post-processing.
Postprocessing means applying effects AFTER a scene has been rendered
from the previous passes.
You can do things like grayscale, sobel filters, blur, etc.

MieEngine doesn't really support post-processing, yet, because you would need shaders for them.

The problem with multi-pass rendering is that you have to be careful with the
draw calls you do. Every scene rendered by a draw pass implies copying all
the draw calls to that pass.

Always save a simple, base scene to a framebuffer and then use that buffer
to further add detail to your scenes.


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

