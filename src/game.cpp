#include "game.hpp"

#include "rendering/renderContext.hpp"
#include "rendering/modelLoader.hpp"
#include "core/input.hpp"

#include "core/timing.hpp"
#include "motionIntegrators.hpp"
#include "gameRenderContext.hpp"
#include "gameCS/renderableMesh.hpp"
#include "gameCS/movementControl.hpp"
#include "gameCS/motion.hpp"

void Game::gameLoop()
{
	Color color(0.0f, 0.15f, 0.3f);
	uint32 fps = 0;
	double lastTime = Time::getTime();
	double fpsTimeCounter = 0.0;
	double updateTimer = 1.0;
	float frameTime = 1.0f / 60.0f;
	while (app->isRunning())
	{
		double currentTime = Time::getTime();
		double passedTime = currentTime - lastTime;
		lastTime = currentTime;

		fpsTimeCounter += passedTime;
		updateTimer += passedTime;

		if (fpsTimeCounter >= 1.0)
		{
			double msPerFrame = 1000.0 / (double)fps;
			DEBUG_LOG("FPS", "NONE", "%f ms (%d fps)", msPerFrame, fps);
			fpsTimeCounter = 0;
			fps = 0;
		}

		bool shouldRender = false;
		while (updateTimer >= frameTime)
		{
			app->processMessages(frameTime, gameEventHandler);
			ecs.updateSystems(mainSystems, frameTime);
			updateTimer -= frameTime;
			shouldRender = true;
		}

		if (shouldRender)
		{
			gameRenderContext->clear(color, true);
			ecs.updateSystems(renderingPipeline, frameTime);
			gameRenderContext->flush();
			window->present();
			fps++;
		}
		else
		{
			Time::sleep(1);
		}
	}
}

int Game::loadAndRunScene(RenderDevice &device)
{
	// BEGIN SCENE CREATION
	Array<IndexedModel> models;
	Array<uint32> modelMaterialIndices;
	Array<MaterialSpec> modelMaterials;
	ModelLoader::loadModels("./res/models/monkey3.obj", models,
		modelMaterialIndices, modelMaterials);
	ModelLoader::loadModels("./res/models/tinycube.obj", models,
		modelMaterialIndices, modelMaterials);

	//	IndexedModel model;
	//	model.allocateElement(3); // Positions
	//	model.allocateElement(2); // TexCoords
	//	model.allocateElement(3); // Normals
	//	model.allocateElement(3); // Tangents
	//	model.setInstancedElementStartIndex(4); // Begin instanced data
	//	model.allocateElement(16); // Transform matrix
	//	
	//	model.addElement3f(0, -0.5f, -0.5f, 0.0f);
	//	model.addElement3f(0, 0.0f, 0.5f, 0.0f);
	//	model.addElement3f(0, 0.5f, -0.5f, 0.0f);
	//	model.addElement2f(1, 0.0f, 0.0f);
	//	model.addElement2f(1, 0.5f, 1.0f);
	//	model.addElement2f(1, 1.0f, 0.0f);
	//	model.addIndices3i(0, 1, 2);

	VertexArray vertexArray(device, models[0], RenderDevice::USAGE_STATIC_DRAW);
	VertexArray tinyCubeVertexArray(device, models[1], RenderDevice::USAGE_STATIC_DRAW);
	//	ArrayBitmap bitmap;
	//	bitmap.set(0,0, Color::WHITE.toInt());
	//	if(!bitmap.load("./res/textures/bricks.jpg")) {
	//		DEBUG_LOG("Main", LOG_ERROR, "Could not load texture!");
	//		return 1;
	//	}
	//	Texture texture(device, bitmap, RenderDevice::FORMAT_RGB, true, false);
	DDSTexture ddsTexture;
	if (!ddsTexture.load("./res/textures/bricks.dds"))
	{
		DEBUG_LOG("Main", LOG_ERROR, "Could not load texture!");
		return 1;
	}
	Texture texture(device, ddsTexture);

	if (!ddsTexture.load("./res/textures/bricks2.dds"))
	{
		DEBUG_LOG("Main", LOG_ERROR, "Could not load texture!");
		return 1;
	}
	Texture bricks2Texture(device, ddsTexture);

	InputControl horizontal;
	InputControl vertical;
	gameEventHandler.addKeyControl(Input::KEY_A, horizontal, -1.0f);
	gameEventHandler.addKeyControl(Input::KEY_D, horizontal, 1.0f);
	gameEventHandler.addKeyControl(Input::KEY_LEFT, horizontal, -1.0f);
	gameEventHandler.addKeyControl(Input::KEY_RIGHT, horizontal, 1.0f);
	gameEventHandler.addKeyControl(Input::KEY_W, vertical, 1.0f);
	gameEventHandler.addKeyControl(Input::KEY_S, vertical, -1.0f);
	gameEventHandler.addKeyControl(Input::KEY_UP, vertical, 1.0f);
	gameEventHandler.addKeyControl(Input::KEY_DOWN, vertical, -1.0f);

	// Create component
	TransformComponent transformComponent;
	transformComponent.transform.setTranslation(Vector3f(0, 0, 20));

	MovementControlComponent movementControl;
	movementControl.movementControls.push_back(std::make_pair(Vector3f(1, 0, 0) * 10, &horizontal));
	movementControl.movementControls.push_back(std::make_pair(Vector3f(0, 1, 0) * 10, &vertical));

	RenderableMeshComponent renderableMeshComponent;
	renderableMeshComponent.vertexArray = &vertexArray;
	renderableMeshComponent.texture = &texture;

	//Create entities
	MotionComponent motionComponent;
	ecs.makeEntity(transformComponent, movementControl, renderableMeshComponent);
	for (uint32 i = 0; i < 5000; i++)
	{
		transformComponent.transform.setTranslation(Vector3f(Math::randf()*10.f - 5.f,
			Math::randf()*10.f - 5.f, Math::randf()*10.f - 5.f + 20.f));
		renderableMeshComponent.vertexArray = &tinyCubeVertexArray;
		renderableMeshComponent.texture = Math::randf() > .5f ? &texture : &bricks2Texture;

		float vf = -4.0f;
		float af = 5.0f;
		motionComponent.acceleration = Vector3f(Math::randf(-af, af), Math::randf(-af, af), Math::randf(-af, af));
		motionComponent.velocity = motionComponent.acceleration * vf;

		ecs.makeEntity(transformComponent, motionComponent, renderableMeshComponent);
	}

	// Create the systems
	MovementControlSystem movementControlSystem;
	MotionSystem motionSystem;
	RenderableMeshSystem renderableMeshSystem(*gameRenderContext);
	mainSystems.addSystem(movementControlSystem);
	mainSystems.addSystem(motionSystem);
	renderingPipeline.addSystem(renderableMeshSystem);

	gameLoop();
	return 0;
}

