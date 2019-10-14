# CageTool
C++/OpenGL tool for cage-based deformation, including functionality for cage generation. Originally developed as a fall 2019 term project for CPSC589 @ University of Calgary.

### TODO
- import boilerplate, fix any build issues
- fix the model size limit issues
- replace any raw pointers
- fix the OBJ loader to not just crash when given different valid variations of OBJ files (e.g. missing normals) that it doesn't like
- improve the outline of the codebase to better fit our project:
    - easy referencing for MODEL + CAGE, especially any necessary data to compute cage vertex weights
    - hard-coded importing of MODEL + (opt.) CAGE. Current support for only wavefront OBJ files with VERTS and NORMALS, ignoring UVS for now
    - implement MVC weight computing working for the armadillo + cage models we have
    - implement MVC deformation (recompute new vertex positions for the MODEL, based on CAGE vert positions + weights)
    - implement a very primitive way of transforming the cage to verify deformation of the model is working correctly
    - (MAIN TODO: attempt different cage-generation methods such as mesh simplification, cutting planes, user-tracing, something with normals, etc.)
    - hard-coded exporting of MODEL + CAGE
    - file picker functionality for importing MODEL and CAGE (just 1 at a time)
    - file picker functionality for exporting MODEL and CAGE (just 1 at a time)
    - implement HC functionality (compute weights + deformation)
    - implement GC functionality (compute weights + deformation)
    - implement coordinate switching which will trigger the respective weight-recomputing algorithm
    - (opt.) support for files without normals (we can auto-gen normals similar to Blender)
    - (opt.) support for importing textures (PNG format) for models with UVS
    - (opt.) support for other file formats beside wavefront OBJ
    - etc.
- ...
- closer to release, decide on a LICENSE
- port to macOS?

### REFERENCES
- boilerplate provided from https://pages.cpsc.ucalgary.ca/~mmactavi/589/