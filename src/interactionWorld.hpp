#pragma once

#include "ecs/ecs.hpp"
#include "gameCS/utilComponents.hpp"

class InteractionWorld : public ECSListener
{
public:
	InteractionWorld(ECS &ecsIn);

	virtual void onMakeEntity(EntityHandle handle);
	virtual void onRemoveEntity(EntityHandle handle);
	virtual void onAddComponent(EntityHandle handle, uint32 id);
	virtual void onRemoveComponent(EntityHandle handle, uint32 id);

	void processInteractions(float delta);
private:
	Array<EntityHandle> entities;
	Array<EntityHandle> entitiesToRemove;
	ECS &ecs;

	void removeEntities();
	int findHighestVarianceAxis();
};