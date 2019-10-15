== README ==

This is some skeleton code provided to potentially help you get started on your project in CPSC589/689. This version of the code is designed for 3D graphics; it loads a textured .obj model, and allows you to rotate and zoom the camera as well as manipulate the position of the light source.

imgui (https://github.com/ocornut/imgui) is integrated in this code base to provide some simple UI controls and text rendering. The use of imgui is completely optional, but you may find it useful for changing parameters, displaying debugging information, etc.

This code is provided to you to help you get started on your assignments more easily. It is not mandatory to use this code if you have a different framework that you prefer.

== SETUP ==

This code uses GLFW and GLEW. Source packages and precompiled binaries can be downloaded here:
    GLFW: https://www.glfw.org/
    GLEW: http://glew.sourceforge.net/

Be sure to link to those libraries when building your project. 
These are already installed on the Linux machines in the graphics lab where tutorials are run. On the lab computers, link to the "GL", "glew" and "GLEW" libraries. 

In order to use imgui, you must build all files in /include/imgui with the rest of your project. Also add /include/imgui to your include directories. 

== CONTROLS ==

Click and drag with the mouse to move the camera, and scroll to zoom.
Use WASD as well as Q and E to change the position of the light source (visualized on the screen as a white circle).

== USE OF 3D MODELS AND TEXTURES ==

This program supports both textured and untextured 3D models. If you use an untextured model, the colour currently defaults to red. This is specified in the fragment shader, mesh.frag. Both a textured and untextured .obj file are provided for you. 

Textures must be in .png image format.