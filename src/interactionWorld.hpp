#pragma once

#include "ecs/ecs.hpp"
#include "gameCS/utilComponents.hpp"

class Interaction
{
public:
	virtual void interact(float delta, BaseECSComponent **interactorComponents, 
		BaseECSComponent **interacteeComponents) { }
	const Array<uint32> &getInteractorComponents() const { return interactorComponents; }
	const Array<uint32> &getInteracteeComponents() const { return interacteeComponents; }
protected:
	void addInteractorComponentType(uint32 type)
	{
		interactorComponents.push_back(type);
	}
	void addInteracteeComponentType(uint32 type)
	{
		interacteeComponents.push_back(type);
	}
private:
	Array<uint32> interactorComponents;		// list of required components for the 'interactor'
	Array<uint32> interacteeComponents;		// list of required components for the 'interactee'
};


class InteractionWorld : public ECSListener
{
public:
	InteractionWorld(ECS &ecsIn);

	virtual void onMakeEntity(EntityHandle handle);
	virtual void onRemoveEntity(EntityHandle handle);
	virtual void onAddComponent(EntityHandle handle, uint32 id);
	virtual void onRemoveComponent(EntityHandle handle, uint32 id);

	void processInteractions(float delta);
	void addInteraction(Interaction *interaction) 
	{
		interactions.push_back(interaction); 
		// TODO: update entities
	}
private:
	// an entity keeps lists of interactions it can participate in
	struct EntityInternal
	{
		EntityHandle handle;
		// indices of interactions the entity participates in as interactor or interactee
		Array<uint32> interactors;	
		Array<uint32> interactees;
	};

	// functor for compare op
	struct InteractionWorldCompare
	{
		uint32 axis = 0;
		ECS &ecs;

		InteractionWorldCompare(ECS &ecsIn, uint32 axisIn) :
			axis(axisIn), ecs(ecsIn) { }
		bool operator()(EntityInternal a, EntityInternal b)
		{
			float aMin = ecs.getComponent<ColliderComponent>(a.handle)->aabb.getMinExtents()[axis];
			float bMin = ecs.getComponent<ColliderComponent>(b.handle)->aabb.getMinExtents()[axis];
			return (aMin < bMin);
		}
	};

	Array<EntityInternal> entities;
	Array<EntityHandle> entitiesToRemove;
	Array<Interaction *> interactions;
	ECS &ecs;
	InteractionWorldCompare compareAABB;

	void removeEntities();
	void addEntity(EntityHandle handle);
	void computeInteractions(EntityInternal & entity, uint32 interactionIndex);
};