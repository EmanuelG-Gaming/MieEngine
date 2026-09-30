# 1. Introduction
This is a partially exhaustive documentation for the Mie Game Engine (MieEngine).
Overviews of general elements will be shown here, and in later pages.

## To whom is this article for?
Basically anyone who is interested in programming, graphics, or game development.
A basic understanding of if statements, loops and working with memory (pointers) is required.

## What is MieEngine?
MieEngine is a game engine, which is a software component that provides the
necessary facilities for the programmer to write games.

A game engine can handle graphics, audio, physics, UI,
user input, events, file loading, scripting, and low-level OS things.
It is a layer that sits in-between the operating system and the game that's being developed.

It is said that every game has an engine.
What this means is that no matter how much the programmer tries, they would always
add an abstraction, be it a renderer, a platform layer, or even just a custom function.

In fact, a game engine is an abstraction. Just like how an OS is an abstraction
for the complexities of hardware.

### Design considerations:
Because a game engine needs to interface with the OS,
we can split it into a platform-dependent layer and a platform-independent game logic.

This is so that the programmer can add code for a weird, specific platform more easily,
without having to rewrite the entire codebase. The game uses that common API,
which can make it more cross-platform, depending on how many interfaces it tries to abstract.


An engine is often developed with a specific game in mind.
This is so that the engine would only have the things that are actually needed for a game,
thus minimizing software bloat.

This is something that modern game engines seem to lack.
Unreal Engine, for example, was originally made for an arena shooter game (Unreal Tournament),
but then it became a mess with UE5, having Lumen and Nanite as the primary
graphics technologies. The reason why it's a mess is because it demands
a lot of graphics card power.

This also happens with other mainstream engines like Unity or Godot.
They wanna cater to a wider range of audiences,
so they force them to use a specific workflow.
Companies/game studios don't really wanna change their workflows with every iteration.

Mainstream engines would oftentimes use a text editor or a scene editor,
so that people who are not that good at programming, can write their own scripts and scenes.


Meanwhile, games in the past have had engines specifically made for those games.
Things like Wolf3D, DOOM or Quake. They all had game engines written in C, in the 90s.
In the 2000s, that would have been mostly C++. But even then, they didn't rely on
pre-made engines Unity or Unreal, even if Unreal was around that time period.

So everyone seemed to be making game engines "from scratch".
Of course, nothing in this world is truly "from scratch", because
there's always that causal chain linking to something.

Anyways, because they wrote their own game engines, they weren't dependent on one entity
shipping the updates for the engine. Thus, they had better ways to optimize their games,
by having more low-level control over the engine.


A famous joke in the graphics programming community is as follows:
"Everyone is a game/graphics engine programmer, but no one actually implemented a game yet!"

It means that making a game engine involves many technical things,
like rendering, lighting, physics, and optimizations, which is a form of creative procrastination.
They like working on the cool parts.

On the other hand, creating a game means writing, design, levels, storytelling and finishing the art,
which is much harder and boring for some programmers.
There's also this trope/stereotype that programmers cannot really make art, and if so, they rely on "programmer art",
which is improvised/simple graphics, demoscene showcases and procedural generation,
simply because they don't have an artist in the team.

But really, a programmer can also be a good artist (drawing, music).
It's literally what game development is. Combining programming with some fields,
in order to make a good audio-visual experience.
Programming and game development should be treated as a form of art.


In conclusion, game engines can be split into two subsets:
- General-purpose game engines, which have nearly everything being implemented,
so that they can have more programmers, and level designers using them. This also means that
the engine would be more bloated.
- Specific-purpose game engines, which are designed from the ground-up with a certain game in mind.
This keeps the engine small. MieEngine can be said to be an example of a specific-purpose game engine.

We can say that MieEngine is a more specific-purpose game engine,
more suited for simple graphics, without complicated lighting effects.

It is a more "write-in" engine, meaning that the programmer writes code for handling
most of the engine's interactions, without really using a graphical editor,
like what large game engines do with scene/node editors.

You write the code in C or C++.
You can also use an ANM virtual machine for scripting the animations.
