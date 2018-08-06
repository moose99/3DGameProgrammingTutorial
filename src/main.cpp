#include <iostream>
#include "core/application.hpp"
#include "core/window.hpp"
#include "core/memory.hpp"
#include "math/transform.hpp"
#include "rendering/renderContext.hpp"
#include "rendering/modelLoader.hpp"

#include "core/timing.hpp"
#include "tests.hpp"

#include "math/sphere.hpp"
#include "math/aabb.hpp"
#include "math/plane.hpp"
#include "math/intersects.hpp"

#include "gameEventHandler.hpp"
#include "core/input.hpp"
#include "ecs/ecs.hpp"

struct TransformComponent : public ECSComponent<TransformComponent>
{
	Transform transform;
};

struct MovementControlComponent : public ECSComponent<MovementControlComponent>
{
	Array<std::pair<Vector3f, InputControl*>> movementControls;
};

class MovementControlSystem : public BaseECSSystem
{
public:
	// add the 2 component types (in order) that this system works on
	MovementControlSystem() : BaseECSSystem()
	{
		addComponentType(TransformComponent::ID);
		addComponentType(MovementControlComponent::ID);
	}

	// use the 2 components to calculate a new transform position
	virtual void updateComponents(float delta, BaseECSComponent **components) override
	{
		TransformComponent *transform = (TransformComponent*)components[0];
		MovementControlComponent *movementControl = (MovementControlComponent*)components[1];
		for (uint32 i = 0; i < movementControl->movementControls.size(); i++)
		{
			Vector3f movement = movementControl->movementControls[i].first;
			InputControl *input = movementControl->movementControls[i].second;
			Vector3f newPos = transform->transform.getTranslation() +
				movement * input->getAmt()  * delta;
			transform->transform.setTranslation(newPos);
		}
	}
private:
};

// NOTE: Profiling reveals that in the current instanced rendering system:
// - Updating the buffer takes more time than
// - Calculating the transforms which takes more time than
// - Performing the instanced draw
static int runApp(Application* app)
{
	Tests::runTests();
	Window window(*app, 800, 600, "My Window!");

	// Begin scene creation
	RenderDevice device(window);
	RenderTarget target(device);
	RenderContext context(device, target);

	Array<IndexedModel> models;
	Array<uint32> modelMaterialIndices;
	Array<MaterialSpec> modelMaterials;
	ModelLoader::loadModels("./res/models/monkey3.obj", models,
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
	Sampler sampler(device, RenderDevice::FILTER_LINEAR_MIPMAP_LINEAR);
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

	String shaderText;
	StringFuncs::loadTextFileWithIncludes(shaderText, "./res/shaders/basicShader.glsl", "#include");
	Shader shader(device, shaderText);
	shader.setSampler("diffuse", texture, sampler, 0);

	Matrix perspective(Matrix::perspective(Math::toRadians(70.0f / 2.0f),
		4.0f / 3.0f, 0.1f, 1000.0f));
	float amt = 0.0f;
	Color color(0.0f, 0.15f, 0.3f);
	float randZ = 20.0f;
	float randScaleX = randZ * window.getWidth() / (float)window.getHeight();
	float randScaleY = randZ;

	uint32 numInstances = 1;
	Matrix transformMatrix(Matrix::identity());
	Transform transform;
	Array<Matrix> transformMatrixArray;
	Array<Matrix> transformMatrixBaseArray;
	for (uint32 i = 0; i < numInstances; i++)
	{
		transformMatrixArray.push_back(Matrix::identity());
		transform.setTranslation(Vector3f(0.0f, 0.0f,
			//					(Math::randf() * randScaleX)-randScaleX/2.0f,
			//					(Math::randf() * randScaleY)-randScaleY/2.0f,
			randZ));
		transformMatrixBaseArray.push_back(transform.toMatrix());
	}
	transform.setTranslation(Vector3f(0.0f, 0.0f, 20.0f));

	RenderDevice::DrawParams drawParams;
	drawParams.primitiveType = RenderDevice::PRIMITIVE_TRIANGLES;
	drawParams.faceCulling = RenderDevice::FACE_CULL_BACK;
	drawParams.shouldWriteDepth = true;
	drawParams.depthFunc = RenderDevice::DRAW_FUNC_LESS;
	//	drawParams.sourceBlend = RenderDevice::BLEND_FUNC_ONE;
	//	drawParams.destBlend = RenderDevice::BLEND_FUNC_ONE;
		// End scene creation

	GameEventHandler eventHandler;
	InputControl horizontal;
	InputControl vertical;
	eventHandler.addKeyControl(Input::KEY_A, horizontal, -1.0f);
	eventHandler.addKeyControl(Input::KEY_D, horizontal, 1.0f);
	eventHandler.addKeyControl(Input::KEY_LEFT, horizontal, -1.0f);
	eventHandler.addKeyControl(Input::KEY_RIGHT, horizontal, 1.0f);
	eventHandler.addKeyControl(Input::KEY_W, vertical, 1.0f);
	eventHandler.addKeyControl(Input::KEY_S, vertical, -1.0f);
	eventHandler.addKeyControl(Input::KEY_UP, vertical, 1.0f);
	eventHandler.addKeyControl(Input::KEY_DOWN, vertical, -1.0f);

	float xPos = 0.0f;
	float yPos = 0.0f;

	ECS ecs;
	// Create component
	TransformComponent transformComponent;
	transform.setTranslation(Vector3f(0, 0, 20));

	MovementControlComponent movementControl;
	movementControl.movementControls.push_back(std::make_pair(Vector3f(1, 0, 0) * 10, &horizontal));
	movementControl.movementControls.push_back(std::make_pair(Vector3f(0, 1, 0) * 10, &vertical));

	//Create entities
	EntityHandle entity = ecs.makeEntity(transformComponent, movementControl);

	// Create the systems
	MovementControlSystem movementControlSystem;
	ECSSystemList mainSystems;
	mainSystems.addSystem(movementControlSystem);


	uint32 fps = 0;
	double lastTime = Time::getTime();
	double fpsTimeCounter = 0.0;
	double updateTimer = 1.0;
	float frameTime = 1.0 / 60.0;
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
			app->processMessages(frameTime, eventHandler);
			// Begin scene update
			ecs.updateSystems(mainSystems, frameTime);
			Transform &workingTransform = ecs.getComponent<TransformComponent>(entity)->transform;

			#if 0
			Matrix transformMatrix = perspective * workingTransform.toMatrix();
			vertexArray.updateBuffer(4, &transformMatrix, sizeof(Matrix));
			#else
			workingTransform.setRotation(Quaternion(Vector3f(1.0f, 1.0f, 1.0f).normalized(), amt*10.0f / 11.0f));
			for (uint32 i = 0; i < transformMatrixArray.size(); i++)
			{
				transformMatrixArray[i] = (perspective * transformMatrixBaseArray[i] * workingTransform.toMatrix());
			}
			vertexArray.updateBuffer(4, &transformMatrixArray[0],
				transformMatrixArray.size() * sizeof(Matrix));
			#endif
			amt += (float)frameTime / 2.0f;
			// End scene update

			updateTimer -= frameTime;
			shouldRender = true;
		}

		if (shouldRender)
		{
			// Begin scene render
			context.clear(color, true);
			context.draw(shader, vertexArray, drawParams, numInstances);
			// End scene render

			window.present();
			fps++;
		}
		else
		{
			Time::sleep(1);
		}
	}
	return 0;
}

#ifdef main
#undef main
#endif
int main(int argc, char** argv)
{
	Application* app = Application::create();
	int result = runApp(app);
	delete app;
	return result;
}

