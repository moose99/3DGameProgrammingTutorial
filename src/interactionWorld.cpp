#include "interactionWorld.hpp"

InteractionWorld::InteractionWorld(ECS &ecsIn) : 
	ECSListener(), ecs(ecsIn)
{
	addComponentId(TransformComponent::ID);
	addComponentId(ColliderComponent::ID);
}

void InteractionWorld::onMakeEntity(EntityHandle handle)
{
	entities.push_back(handle);
}

void InteractionWorld::onRemoveEntity(EntityHandle handle)
{
	entitiesToRemove.push_back(handle);
}

void InteractionWorld::onAddComponent(EntityHandle handle, uint32 id)
{
	if (id == TransformComponent::ID)
	{
		if (ecs.getComponent<ColliderComponent>(handle) != nullptr)
		{
			entities.push_back(handle);
		}
	}

	if (id == ColliderComponent::ID)
	{
		if (ecs.getComponent<TransformComponent>(handle) != nullptr)
		{
			entities.push_back(handle);
		}
	}
}

void InteractionWorld::onRemoveComponent(EntityHandle handle, uint32 id)
{
	if (id == TransformComponent::ID || id == ColliderComponent::ID)
	{
		entitiesToRemove.push_back(handle);
	}
}

void InteractionWorld::processInteractions(float delta)
{
	// Remove entitiesToRemove
	removeEntities();

	// Find highest variance axis for AABB (which axis are the most spread out on)
	int axis = findHighestVarianceAxis();

	// Sort AABBS by min on highest variance axis

	// Go thru the list, test intersections in range

}

void InteractionWorld::removeEntities()
{
	if (entitiesToRemove.size() == 0)
	{
		return;
	}

	for (size_t i = 0; i < entities.size(); i++)
	{
		bool didRemove = false;
		do
		{
			for (size_t j = 0; j < entitiesToRemove.size(); j++)
			{
				// if entity is in the remove list
				if (entities[i] == entitiesToRemove[j])
				{
					entities.swap_remove(i);
					entitiesToRemove.swap_remove(j);
					didRemove = true;
					break;
				}
			}
			if (didRemove && entitiesToRemove.size() == 0)
			{
				return;
			}
		} while (didRemove);
	}
	entitiesToRemove.clear();
}

int InteractionWorld::findHighestVarianceAxis()
{
	return 0;
}