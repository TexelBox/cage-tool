# CageTool
C++/OpenGL tool for cage-based deformation, including functionality for cage generation. Originally developed as a fall 2019 term project for CPSC589 @ University of Calgary.

## [WIP]

### CONTROLS/FEATURES:
- LEFT CLICK + DRAG === pan the spherical camera
- SCROLL === zoom the spherical camera
- EDIT WINDOW - can be toggled in/out of view, resized, moved (config saved in imgui.ini file)
- GENERAL IMGUI NOTE: most fields can have directly typed input entered by CTRL + LEFT CLICK, then saved with ENTER
- CLEAR COLOR - self explanatory
- CARTESIAN PLANE TOGGLE BUTTONS - these provide useful visual aid for alignment
- LOAD MODEL / LOAD CAGE - type filename (sans extension) in textbox (note: case-sensitive on Linux), press ENTER and it searches for Wavefront OBJ file in models/imports/
- NOTE: this program only has support for Wavefront OBJ pure tri-meshes. Files must have verts/faces. UVS are optional and currently only a default pink/black checkered texture will get applied. Any normals in the file are ignored since the program will generate both per-face and per-vertex normals, the latter being used for rendering.
- POSITION/ROTATION/SCALE transforms - these model transforms only apply visually (no internal change to positions), they were left in since they could be useful to move the cage out of the way to view the model unobscured.
- CLEAR MODEL / CLEAR CAGE - removes the respective mesh from the scene (e.g. want to load in a new mesh).
- EXPORT MODEL / EXPORT CAGE - similar to import instructions, writes to file in models/exports/. If the file doesn't exist, it will create a new one and write to it, otherwise it will deliberately fail to prevent overwriting existing files.
- NOTE: exporting a model, exports verts, auto-generated per-vertex normals, faces and uvs if present.
- NOTE: exporting a cage, just exports verts/faces.
- CAGE GENERATION - currently incomplete, but very close (just missing the internal face culling and face registration)
- TERMINATION CONSTANTS - get set before clicking GENERATE CAGE button - refer to our paper for an explanation
- CAGE DEFORMATION (MVC) - once a model + cage pair are loaded in the scene, you can press the COMPUTE CAGE WEIGHTS button to compute MVC weights of the cage vertices on the model vertices. You can then either use any of the 3 buttons (SELECT/UNSELECT/TOGGLE ALL VERTS) or individually RIGHT-CLICK on the black cage-verts (turn them YELLOW for SELECTED) and then deform the cage (and consequently the model) by translating the selected cage verts with the keys Q, W, E, A, S, D (1 key per direction on 3 axes).
- NOTE: this cage movement with Q, W, E, A, S, D can also be used to just alter a cage if wanted. To do this, just make sure to CLEAR CAGE WEIGHTS first, or CLEAR MODEL.
- NOTE: there is a slider for the "selected cage vert translation amount" (can also be CTRL+LEFT CLICKED) to allow finer control on how many units the cage verts move by key inputs. 

##### OPTIONAL/FUTURE FEATURES:
-
- port to Linux (Makefile?)
- finish / implement more coarse cage-generation techniques (e.g. improved OBBs method, normals, mesh-simplification, cutting planes, user-tracing, etc.)
- add improved manners of user-interaction for transforming the cage (e.g. view-plane translation, movement along normals (per vertex? per face?), face extrusion/intrusion, etc.)
- improved file picker functionality for both importing/exporting (model and cage)
- additional export options (e.g. which data to export)
- file picker functionality for importing TEXTURE (.png file), only if MODEL has uvs
- update per-vertex normal computation to be angle-weighted
- debug functionality for rendering normals
- implement HC functionality (compute weights + deformation)
- implement GC functionality (compute weights + deformation)
- implement coordinate switching which will trigger the respective weight-recomputing algorithm
-
- add an imgui window that serves as an output log (debug msgs, error msgs, user info, etc.)
- undo/redo functionality
- support for other file formats beside Wavefront OBJ, maybe even support for non-tri meshes
- support for animation (run through deformed model poses)
- ...
- closer to release, decide on a LICENSE, include copy of every dependency licenses
- port to macOS?
- improve this README (build instructions, controls, assumptions, etc.)

### REFERENCES
- boilerplate provided from https://pages.cpsc.ucalgary.ca/~mmactavi/589/
- inspiration from https://github.com/cordafab/Cagelab2018
- source of a potential cage-generation technique - http://www.cad.zju.edu.cn/home/hwlin/pdf_files/Automatic-cage-generation-by-improved-OBBs-for-mesh-deformation.pdf
- mean-value-coordinate weight computing algorithm sourced from https://www.cse.wustl.edu/~taoju/research/meanvalue.pdf
- very useful resource helping with voxelization - http://blog.wolfire.com/2009/11/Triangle-mesh-voxelization
