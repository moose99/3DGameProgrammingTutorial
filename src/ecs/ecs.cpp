#include"ecs.hpp"

ECS::~ECS()
{
	// loop thru all component lists (organized by component ID)
	for (Map<uint32, Array<uint8>>::iterator it = components.begin(); it != components.end(); it++)
	{
		// prepare to free all components in that list
		// they are all the same kind, so same ID, freeFunc and size
		uint32 componentID = it->first;
		size_t typeSize = BaseECSComponent::getTypeSize( componentID );
		ECSComponentFreeFunc freefn = BaseECSComponent::getTypeFreeFunction( componentID );

		// go thru all components in the list and free them
		Array<uint8> componentList = it->second;
		for (uint32 i = 0; i < componentList.size(); i += typeSize)
		{
			freefn( reinterpret_cast<BaseECSComponent*>(&componentList[i]) );
		}
	}

	// delete all entities
	for (uint32 i = 0; i < entities.size(); i++)
	{
		delete entities[i];
	}
}

//
// Creates an entity along with the components it needs and add it to the entities list.
// Returns a pointer to the entity
//
EntityHandle ECS::makeEntity( BaseECSComponent* entityComponents, const uint32 *componentIDs, 
	size_t numComponents )
{
	// create a new entity
	std::pair<uint32, EntityType> *newEntity = new std::pair<uint32 /*index*/, EntityType>();
	EntityHandle handle = static_cast<EntityHandle>(newEntity);

	// create the components (if valid) and add them to the entity
	for (uint32 i = 0; i < numComponents; i++)
	{
		// check if component id is valid
		if (!BaseECSComponent::isTypeValid(componentIDs[i]))
		{
			DEBUG_LOG( "ECS", LOG_ERROR, "%u is not a valid component type", componentIDs[i] );
			delete newEntity;
			return NULL_ENTITY_HANDLE;
		}

		// get the component creat func
		ECSComponentCreateFunc createfn = BaseECSComponent::getTypeCreateFunction( componentIDs[i] );

		// Create the component pair which consists of componentID and it's index in the component list,
		// which is part of the ECS components map.
		std::pair<uint32, uint32> newPair;
		newPair.first = componentIDs[i];
		newPair.second = createfn( components[componentIDs[i]], /* the components list (memory block from map) */
			handle /* the entity to point back to */, 
			&entityComponents[i] /* the component template to copy */);

		// add component pair to the entity
		newEntity->second.push_back( newPair );
	}

	// add the entity to the ECS entities list
	newEntity->first = entities.size();
	entities.push_back( newEntity );

	return handle;
}

//
// Remove an entity by removing all of its components and then removing it from the entities list
//
void ECS::removeEntity( EntityHandle handle )
{
	EntityType& entity = handleToEntity( handle );

	// remove all of its components
	for (uint32 i = 0; i<entity.size(); i++)
	{
		removeComponentInternal( entity[i].first /* componentID */, 
			entity[i].second /* index of component in the components map list */);
	}

	// remove the entity from the entities list by swapping it with the last one
	uint32 entityIndex = handleToEntityIndex( handle );
	uint32 lastIndex = entities.size() - 1;
	delete entities[entityIndex];	// delete the one we're removing
	entities[entityIndex] = entities[lastIndex];	// replace it with the one at the end
	entities.pop_back();	// now delete the one at the end (which is now  duplicate)
}