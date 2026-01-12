# Volumarcher
A DirectX 12 (MiniEngine sample) Cloud renderer based on nubis
made for block B of Y2 at Games | Breda University of applied sciences

Article about the techniques: [article](https://www.jacktollenaar.top/articles/clouds.html)

[Volumarcher](./Volumarcher) contains the volumetric renderer as a library
[RendererApplication](./RenderApplication) contains a demo project containing a example rasterizer and the setup for volumarcher.

# Running,
Open the .sln with visual studio and run RendererApplication

You can open the runtime settings with Backspace

# Using in your own project.
Currently the project is dependent on [MiniEngine](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine),
If you have a MiniEngine project link Volumarcher as a dependency,
Add the Volumarcher propertysheet to your project and make sure to set the user macros in the propertysheet on both your project and the volumarcher project.

To use Volumarcher just create a VolumarcherContext and call its relevant functions.
