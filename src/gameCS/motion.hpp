#pragma once

#include "ecs/ecs.hpp"
#include "motionIntegrators.hpp"
#include "gameCS/utilComponents.hpp"

struct MotionComponent : public ECSComponent<MotionComponent>
{
	Vector3f velocity = Vector3f(0, 0, 0);
	Vector3f acceleration = Vector3f(0, 0, 0);
};

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
