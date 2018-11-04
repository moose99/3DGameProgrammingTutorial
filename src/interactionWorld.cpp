#include <algorithm>
#include "interactionWorld.hpp"

InteractionWorld::InteractionWorld(ECS &ecsIn) : 
	ECSListener(), ecs(ecsIn), compareAABB(ecsIn, 0)
{
	addComponentId(TransformComponent::ID);
	addComponentId(ColliderComponent::ID);
}

void InteractionWorld::onMakeEntity(EntityHandle handle)
{
	addEntity(handle);
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
			addEntity(handle);
		}
	}

	if (id == ColliderComponent::ID)
	{
		if (ecs.getComponent<TransformComponent>(handle) != nullptr)
		{
			addEntity(handle);
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

void InteractionWorld::addEntity(EntityHandle handle)
{
	EntityInternal entity;
	entity.handle = handle;
	// TODO: Compute Interactions
	entities.push_back(entity);
}

//
// check if the entity has the components to qualify for this interaction as an interactor or interactee
//
void InteractionWorld::computeInteractions(EntityInternal &entity, uint32 interactionIndex)
{
	Interaction * interaction = interactions[interactionIndex];
	
	bool isInteractor = true;
	for (size_t i = 0; i < interaction->getInteractorComponents().size(); i++)
	{
		if (ecs.getComponentByType(entity.handle, interaction->getInteractorComponents()[i]) == nullptr)
		{
			isInteractor = false;
			break;
		}
	}

	bool isInteractee = true;
	for (size_t i = 0; i < interaction->getInteracteeComponents().size(); i++)
	{
		if (ecs.getComponentByType(entity.handle, interaction->getInteracteeComponents()[i]) == nullptr)
		{
			isInteractee = false;
			break;
		}
	}

	if (isInteractor)
		entity.interactors.push_back(interactionIndex);
	if (isInteractor)
		entity.interactees.push_back(interactionIndex);
}


void InteractionWorld::processInteractions(float delta)
{
	// Remove entitiesToRemove
	removeEntities();

	// Sort AABBs by min on highest variance axis
	std::sort(entities.begin(), entities.end(), compareAABB);

	// Go thru the list, test intersections in range
	Vector3f centerSum, centerSqSum;
	for (size_t i = 0; i < entities.size(); i++)
	{
		AABB aabb = ecs.getComponent<ColliderComponent>(entities[i].handle)->aabb;

		Vector3f center = aabb.getCenter();
		centerSum += center;
		centerSqSum += (center * center);

		// find intersections for this entity
		for (size_t j = i + 1; j < entities.size(); j++)
		{
			AABB otherAABB = ecs.getComponent<ColliderComponent>(entities[j].handle)->aabb;
			if (otherAABB.getMinExtents()[compareAABB.axis] > aabb.getMaxExtents()[compareAABB.axis])
			{
				// not in range, early out
				break;
			}

			if (aabb.intersects(otherAABB))
			{
				// if rules allow it, entites[i] interacts with entities[j]
				// if rules allow it, entites[j] interacts with entities[i]
			}
		}
	}

	//
	// Note - variance calc will lag by one frame (but that shouldn't be a big problem)
	//

	// calc avgs of center for variance
	centerSum /= entities.size();
	centerSqSum /= entities.size();
	Vector3f variance = centerSqSum - (centerSum*centerSum);

	// calc max variance. variance is The average of the squared differences from the Mean.
	// To calculate the Variance, take each difference, square it, and then average the result
	// And the Standard Deviation is just the square root of Variance
	int maxVarAxis = 0;
	float maxVar = variance[0];
	if (variance[1] > maxVar)
	{
		maxVar = variance[1];
		maxVarAxis = 1;
	}
	if (variance[2] > maxVar)
	{
		maxVar = variance[2];
		maxVarAxis = 2;
	}
	compareAABB.axis = maxVarAxis;
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
				if (entities[i].handle == entitiesToRemove[j])
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

