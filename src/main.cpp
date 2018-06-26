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

namespace MotionIntegrators
{
	// compromise between standard and modified euler
	void verlet(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		float halfDelta = delta * 0.5f;
		pos += velocity * halfDelta;	// moves position half with the orig velocity
		velocity += acceleration * delta;
		pos += velocity * halfDelta;	// and half with the new velocity
	}

	// Modifies position (and vel) based on vel, accel and time delta
	// Modification to standard Euler by computing velocity first, then pos
	// This way, momentum is conserved (symplectic) and not lost over time.
	void modifiedEuler(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		// These statements are reversed from standard Euler.
		velocity += acceleration * delta;
		pos += velocity * delta;
	}

	// 4th order symplectic integator, as precise as rk4 but conserves momentum
	void forestRuth(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		static const float frCoefficient = 1.0f / (2.0f - Math::pow(2.0f, 1.0f / 3.0f));
		static const float frComplement = 1.0f - 2.0f * frCoefficient;
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frCoefficient);
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frComplement);
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frCoefficient);
	}
}

//
// COMPONENTS
///////////////////////////////////////////////////
//
struct TransformComponent : public ECSComponent<TransformComponent>
{
	Transform transform;
};

struct MovementControlComponent : public ECSComponent<MovementControlComponent>
{
	Array<std::pair<Vector3f, InputControl*>> movementControls;
};

struct RenderableMeshComponent : public ECSComponent<RenderableMeshComponent>
{
	VertexArray *vertexArray = nullptr;
	Texture *texture = nullptr;
};

struct MotionComponent : public ECSComponent<MotionComponent>
{
	Vector3f velocity = Vector3f(0, 0, 0);
	Vector3f acceleration = Vector3f(0, 0, 0);
};

//
// SYSTEMS
///////////////////////////////////////////////////
//
class MotionSystem : public BaseECSSystem
{
public:
	// add the 2 component types (in order) that this system works on
	MotionSystem() : BaseECSSystem()
	{
		addComponentType(TransformComponent::ID);
		addComponentType(MotionComponent::ID);
	}

	// use the 2 components to calculate a new transform position
	virtual void updateComponents(float delta, BaseECSComponent **components) override
	{
		TransformComponent *transform = (TransformComponent*)components[0];
		MotionComponent *motion = (MotionComponent*)components[1];

		Vector3f newPos = transform->transform.getTranslation();
		MotionIntegrators::forestRuth(newPos, motion->velocity, motion->acceleration, delta);
		transform->transform.setTranslation(newPos);
	}
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

//
// single shader
// same sampler
// same drawParams
//
class GameRenderContext : public RenderContext
{
public:
	GameRenderContext(RenderDevice& deviceIn, RenderTarget& targetIn,
		RenderDevice::DrawParams &drawParamsIn, Shader &shaderIn, Sampler &samplerIn,
		const Matrix &perspectiveIn) :
		RenderContext(deviceIn, targetIn),
		drawParams(drawParamsIn),
		shader(shaderIn),
		sampler(samplerIn),
		perspective(perspectiveIn)
	{
	}

	// add the vertexArray+texture to a map, along with its transform
	void renderMesh(VertexArray &vertexArray, Texture &texture, const Matrix &transformIn)
	{
		meshRenderBuffer[std::make_pair(&vertexArray, &texture)].push_back(perspective * transformIn);
	}

	// draw everything in our render buffer
	void flush()
	{
		Texture *currentTexture = nullptr;
		for (Map<std::pair<VertexArray*, Texture*>, Array<Matrix>>::iterator it = meshRenderBuffer.begin();
			it != meshRenderBuffer.end(); it++)
		{
			const std::pair<VertexArray*, Texture*> &p = it->first;		// the pair itself
			VertexArray *vertexArray = p.first;
			Texture *texture = p.second;
			Matrix *transforms = &it->second[0];		// array of matrices
			size_t numTransforms = it->second.size();	// array of matrices

			if (numTransforms == 0)
			{
				continue;
			}
			if (texture != currentTexture)
			{
				shader.setSampler("diffuse", *texture, sampler, 0);
				currentTexture = texture;
			}
			vertexArray->updateBuffer(4, transforms, numTransforms * sizeof(Matrix));
			draw(shader, *vertexArray, drawParams, numTransforms);
			it->second.clear();		// clear array of matrices each frame
		}
	}
private:
	RenderDevice::DrawParams &drawParams;
	Shader &shader;
	Sampler &sampler;
	Matrix perspective;
	// map of transforms which go a pair of vertex array 
	Map<std::pair<VertexArray*, Texture*>, Array<Matrix>> meshRenderBuffer;		
};

class RenderableMeshSystem : public BaseECSSystem
{
public:
	// add the 2 component types (in order) that this system works on
	RenderableMeshSystem(GameRenderContext &contextIn) : BaseECSSystem(),
		context(contextIn)
	{
		addComponentType(TransformComponent::ID);
		addComponentType(RenderableMeshComponent::ID);
	}

	// use the 2 components to calculate a new transform position
	virtual void updateComponents(float delta, BaseECSComponent **components) override
	{
		TransformComponent *transform = (TransformComponent*)components[0];
		RenderableMeshComponent *mesh = (RenderableMeshComponent*)components[1];

		context.renderMesh(*mesh->vertexArray, *mesh->texture,
			transform->transform.toMatrix());
	}
private:
	GameRenderContext & context;
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

	if (!ddsTexture.load("./res/textures/bricks2.dds"))
	{
		DEBUG_LOG("Main", LOG_ERROR, "Could not load texture!");
		return 1;
	}
	Texture bricks2Texture(device, ddsTexture);


	String shaderText;
	StringFuncs::loadTextFileWithIncludes(shaderText, "./res/shaders/basicShader.glsl", "#include");
	Shader shader(device, shaderText);
	shader.setSampler("diffuse", texture, sampler, 0);

	Matrix perspective(Matrix::perspective(Math::toRadians(70.0f / 2.0f),
		4.0f / 3.0f, 0.1f, 1000.0f));
	Color color(0.0f, 0.15f, 0.3f);

	RenderDevice::DrawParams drawParams;
	drawParams.primitiveType = RenderDevice::PRIMITIVE_TRIANGLES;
	drawParams.faceCulling = RenderDevice::FACE_CULL_BACK;
	drawParams.shouldWriteDepth = true;
	drawParams.depthFunc = RenderDevice::DRAW_FUNC_LESS;
	//	drawParams.sourceBlend = RenderDevice::BLEND_FUNC_ONE;
	//	drawParams.destBlend = RenderDevice::BLEND_FUNC_ONE;
		// End scene creation


	RenderTarget target(device);
	GameRenderContext gameRenderContext(device, target, drawParams, shader, sampler, perspective);
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

	ECS ecs;
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
	RenderableMeshSystem renderableMeshSystem(gameRenderContext);
	ECSSystemList mainSystems;
	ECSSystemList renderingPipeline;
	mainSystems.addSystem(movementControlSystem);
	mainSystems.addSystem(motionSystem);
	renderingPipeline.addSystem(renderableMeshSystem);

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
			app->processMessages(frameTime, eventHandler);
			// Begin scene update
			ecs.updateSystems(mainSystems, frameTime);
			// End scene update

			updateTimer -= frameTime;
			shouldRender = true;
		}

		if (shouldRender)
		{
			// Begin scene render
			gameRenderContext.clear(color, true);
			ecs.updateSystems(renderingPipeline, frameTime);
			gameRenderContext.flush();
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

