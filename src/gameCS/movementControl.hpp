#pragma once

#include "ecs/ecs.hpp"
#include "math/transform.hpp"

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

