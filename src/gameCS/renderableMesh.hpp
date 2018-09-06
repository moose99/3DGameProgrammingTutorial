#pragma once

#include "ecs/ecs.hpp"
#include "rendering/renderContext.hpp"
#include "gameCS/utilComponents.hpp"

struct RenderableMeshComponent : public ECSComponent<RenderableMeshComponent>
{
	VertexArray *vertexArray = nullptr;
	Texture *texture = nullptr;
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
