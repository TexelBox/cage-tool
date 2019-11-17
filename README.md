# CageTool
C++/OpenGL tool for cage-based deformation, including functionality for cage generation. Originally developed as a fall 2019 term project for CPSC589 @ University of Calgary.

## [WIP]

### TODO (INCOMPLETE)
##### ESSENTIAL FEATURES (FOR SUBMISSION):
-
- file picker functionality for importing MODEL and CAGE (just 1 at a time)
- export method for a MeshObject (MODEL or CAGE), incl. option to export normals (per-vertex?, per-face?, both?)
- file picker functionality for exporting MODEL and CAGE (just 1 at a time)
-
- MeshObject method that computes per-face normals
- MeshObject method that computes per-vertex normals (preferably angle-weighted), uses per-face normals
-
- update deformModel() to call the compute-normals method and then change updateBuffers(MODEL) to update normal buffer as well
-
- test the MVC weight computing for different model-cage pairs
- test the MVC deformation for different model-cage pairs
-
- **VERY IMPORTANT**: implement some coarse cage-generation techniques (e.g. improved OBBs method, normals, mesh-simplification, cutting planes, user-tracing, etc.)
-
- add improved manners of user-interaction for transforming the cage (e.g. view-plane translation, movement along normals (per vertex? per face?), face extrusion/intrusion, etc.)
-
- port to Linux (Makefile?)

##### OPTIONAL/FUTURE FEATURES:
-
- file picker functionality for importing TEXTURE (.png file), only if MODEL has uvs
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
