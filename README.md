# Corium3D

A game engine infrastructure under construction. Implemented components so far:

* Core (VS Project - Corium3D):
	*	Collision detection:
		*	Broad phase using dynamic BVHs of AABBs, Separating between static and mobile objects.
		*	Narrow phase finds collisions in 3D of bounding boxes, spheres and capsules, and in 2D using bounding rectangles, circles, and stadiums, including contact manifolds.
		*	Ray casting against the BVHs in 3D.
	*	Rendering – OpenGL:
		*	Camera transformations.
		*	Static objects data buffering.
		*	Frustum culling via the above BVHs, using bounding spheres stored in the nodes.
		*	Skeletal animations.
		*	Text widgets using image atlases and Image widgets.
	*	Physics – Basic kinematics. 
	*	Scenes – Engine orients around data preloading of preprocessed assets, and generation of object pools for everything in a scene.
* Editor (VS Projects: Corium3DGI, Corium3DAssetsGen, DxVisualizer) - A GUI built with Windows Presentation Foundation:
	* Importing of 3D models.
	* Fitting 3D models with 3D collision primitives (box, sphere, capsule) and 2D collision primitives (rectangle, circle, stadium).
	* Generating scenes - define models present in the scene and specify maximum instances number to facilitate object pooling.
	*	DirectX11 rendered viewport: 
		*	Graphically drag and drop initial models instances to levels and transform them via handles.
		*	C++/CLI mediates WPF C# code and DirectX11 C++ code.
		*	Highlight of selected objects via blur filters.
		*	Utilizes space partitioning using a K-D Tree adapted to hold bounding spheres.
	o	Generates the level’s assets.

