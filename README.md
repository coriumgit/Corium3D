# Corium3D

A game engine infrastructure under construction. This is actually my hands-on study in game engine programming. My aim is to produce the main componentes that make up a game engine. For each component I broaden the functionality just on the basis of how fun I find it to implement :)

### Implemented components so far:
* Core (VS Project - Corium3D) components:
	* 	Game loop.
	*	Scenes – Engine orients around data preloading of preprocessed assets, and generation of object pools for everything in a scene.
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
	
	*	Environment independent input handling.
* Editor - GUI built with Windows Presentation Foundation (VS Project: Corium3DGI), MVVM designed. Implemented components:
	* 	Importing and previewing of 3D models.
	* 	Fitting 3D models with 3D collision primitives (box, sphere, capsule) and 2D collision primitives (rectangle, circle, stadium) visually in the preview.
	* 	Scenes definition - declare models present in the scene, and add initial instances.
	*	DirectX11 rendered viewport (VS Project: DxVisualizer): 
		*	Graphically drag and drop initial models instances to levels.
		*	Objects selection and highlight of selected objects via blur filters.
		*	Objects transforms via handles.				
		*	Utilizes space partitioning using a K-D Tree adapted to hold bounding spheres.
		*	C++/CLI mediates WPF C# code and DirectX11 C++ code.
	* 	Generation of scenes' data files to be loaded in the game runtime (VS Project: Corium3DAssetsGen).

