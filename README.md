# Volumarcher
A DirectX 12 (MiniEngine sample) Cloud renderer based on nubis
made for block B of Y2 at Games | Breda University of applied sciences
![hero2](https://github.com/user-attachments/assets/c611f147-df56-4bed-93ea-ad37fc48f5d9)


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

# Showcase,
<img width="1920" height="1009" alt="clouds-card" src="https://github.com/user-attachments/assets/25fddbf6-413e-44ad-a596-ead2b4dd1aaf" />
<img width="1920" height="1009" alt="hero3" src="https://github.com/user-attachments/assets/c81a2f12-fe3d-4255-a01b-6e15ad065521" />
<img width="956" height="869" alt="spacecloud" src="https://github.com/user-attachments/assets/68db1efa-002b-4b02-b614-95f14706cd96" />
<img width="1920" height="1009" alt="rainbow" src="https://github.com/user-attachments/assets/7aee9ab2-cdfb-4819-b24a-4653d87b0f88" />

