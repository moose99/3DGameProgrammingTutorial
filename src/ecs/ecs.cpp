#include "core/memory.hpp"
#include"ecs.hpp"
#include "math/math.hpp"

ECS::~ECS()
{
	// loop thru all component lists (organized by component ID)
	for (Map<uint32, ComponentBlock>::iterator it = components.begin(); it != components.end(); it++)
	{
		// prepare to free all components in that list
		// they are all the same kind, so same ID, freeFunc and size
		uint32 componentID = it->first;
		size_t typeSize = BaseECSComponent::getTypeSize( componentID );
		ECSComponentFreeFunc freefn = BaseECSComponent::getTypeFreeFunction( componentID );

		// go thru all components in the list and free them
		ComponentBlock &componentList = it->second;
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
		if (!BaseECSComponent::isTypeValid( componentIDs[i] ))
		{
			DEBUG_LOG( "ECS", LOG_ERROR, "%u is not a valid component type", componentIDs[i] );
			delete newEntity;
			return NULL_ENTITY_HANDLE;
		}

		// add each components to the entity
		addComponentInternal( handle, newEntity->second, componentIDs[i], &entityComponents[i] );
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
	for (uint32 i = 0; i < entity.size(); i++)
	{
		deleteComponent( entity[i].first /* componentID */,
			entity[i].second /* index of component in the components map list */ );
	}

	// remove the entity from the entities list by swapping it with the last one
	uint32 entityIndex = handleToEntityIndex( handle );
	uint32 lastIndex = entities.size() - 1;
	delete entities[entityIndex];	// delete the one we're removing
	entities[entityIndex] = entities[lastIndex];	// replace it with the one at the end
	entities.pop_back();	// now delete the one at the end (which is now  duplicate)
}

void ECS::addComponentInternal( EntityHandle handle, EntityType &entity, uint32 componentID, BaseECSComponent *component )
{
	// get the component create func
	ECSComponentCreateFunc createfn = BaseECSComponent::getTypeCreateFunction( componentID );

	// Create the component pair which consists of componentID and it's index in the component list,
	// which is part of the ECS components map.
	std::pair<uint32, uint32> newPair;
	newPair.first = componentID;
	newPair.second = createfn( components[componentID], /* the components list (memory block from map) */
		handle, /* the entity to point back to */
		component /* the component template to copy */ );

	// add component pair to the entity
	entity.push_back( newPair );
}

//
// Delete the specified component from the ECS component list.
// find the component and call its free function on it, 
// then swap it with the last one in the list.  Since we moved the last component in the list 
// we need to find the entity that it was attached to and update it's component list to reflect 
// the new index.
//
void ECS::deleteComponent( uint32 componentID, uint32 index )
{
	ComponentBlock &array = components[componentID];		// get list of components of the right type
	ECSComponentFreeFunc freefn = BaseECSComponent::getTypeFreeFunction( componentID );
	size_t typeSize = BaseECSComponent::getTypeSize( componentID );
	uint32 srcIndex = array.size() - typeSize;			// get index of the last element
	BaseECSComponent *destComponent = (BaseECSComponent*)&array[index];
	BaseECSComponent *srcComponent = (BaseECSComponent*)&array[srcIndex];
	freefn( destComponent );

	if (index == srcIndex)
	{	// special case when deleting the last item, don't need to move any components
		array.resize( srcIndex );
		return;
	}

	// copy the last component over this one (swap places in the list)
	Memory::memcpy( destComponent, srcComponent, typeSize );

	// find the update the entity that was pointing to the last component, since we moved it
	EntityType &srcComponents = handleToEntity( srcComponent->entity );
	for (uint32 i = 0; i < srcComponents.size(); i++)
	{
		if (componentID == srcComponents[i].first && srcIndex == srcComponents[i].second)
		{
			srcComponents[i].second = index;
			break;
		}
	}

	array.resize( srcIndex );
}

//
// Remove the component from the entity, return true on success
//
bool ECS::removeComponentInternal( EntityHandle handle, uint32 componentID )
{
	// find the component on the entity
	EntityType &entityComponents = handleToEntity( handle );
	for (uint32 i = 0; i < entityComponents.size(); i++)
	{
		if (componentID == entityComponents[i].first)
		{
			// delete the component
			deleteComponent( entityComponents[i].first, entityComponents[i].second );

			// swap it with final element in the entity component array to remove it
			uint32 srcIndex = entityComponents.size() - 1;
			entityComponents[i] = entityComponents[srcIndex];
			entityComponents.pop_back();	// remove end element

			return true;
		}
	}

	return false;
}

//
// go thru all the components on an entity and return a pointer to the one with the matching componentID
//
BaseECSComponent *ECS::getComponentInternal( EntityType &entityComponents, ComponentBlock &compBlock, uint32 componentID )
{
	for (uint32 i = 0; i < entityComponents.size(); i++)
	{
		if (componentID == entityComponents[i].first)
		{
			uint32 compIndex = entityComponents[i].second;
			return (BaseECSComponent*)&compBlock[compIndex];
		}
	}

	return nullptr;
}

bool ECS::removeSystem( BaseECSSystem &system )
{
	for (uint32 i = 0; i < systems.size(); i++)
	{
		if (&system == systems[i])
		{
			systems.erase( systems.begin() + i );
			return true;
		}
	}

	return false;
}

void ECS::updateSystems( float delta )
{
	Array <BaseECSComponent*> componentParam;
	Array <ComponentBlock*> componentBlockArray;
	for (uint32 i = 0; i < systems.size(); i++)
	{
		// get the array of component IDS that this system operates on
		const Array<uint32> &componentTypes = systems[i]->getComponentTypes();

		// simple case
		if (componentTypes.size() == 1)
		{
			size_t typeSize = BaseECSComponent::getTypeSize( componentTypes[0] );

			// get the list of components of that type (memory block)
			ComponentBlock &array = components[componentTypes[0]];
			for (uint32 j = 0; j < array.size(); j+=typeSize)
			{
				BaseECSComponent *component = (BaseECSComponent *)&array[j];
				systems[j]->updateComponents( delta, &component );
			}
		}
		else
		{
			updateSystemWithMultipleComponents( i, delta, componentTypes, componentParam, componentBlockArray );
		}
	}
}

//
// Given the list of component types that the system requires, we first find the component type
// from that list which has the smallest number of components in use (check the block).
// Then we find all the entities which are using the components from the minimal list and check
// if they have the other components that the system requires.  if so, then we update the system
// and provide it the list of components from that entity.
//
// index - index of the system to update
// delta - time delta for update
// componentTypes - the componentTypes the system wants
// componentParam - array used to hold ptrs to the components on the system
// componnentArrays - array of components grouped in blocks by type
//
void ECS::updateSystemWithMultipleComponents( uint32 index, float delta, const Array<uint32> &componentTypes,
	Array <BaseECSComponent*> &componentParam, Array <ComponentBlock*> &componentBlockArray )
{
	componentParam.resize( Math::max( componentParam.size(), componentTypes.size() ) );
	componentBlockArray.resize( Math::max( componentBlockArray.size(), componentTypes.size() ) );

	// init the componentArrays array to point to the correct list of component groups
	for (uint32 i = 0; i < componentTypes.size(); i++)
	{
		componentBlockArray[i] = &components[componentTypes[i]];
	}

	// find the index of the least common component type
	uint32 minSizeIndex = findLeastCommonComponent( componentTypes );

	// start with the smallest component type
	size_t typeSize = BaseECSComponent::getTypeSize( componentTypes[minSizeIndex] );

	ComponentBlock *compBlock = componentBlockArray[minSizeIndex];

	// get the group of components for that type
	// TODO - make it easier to iterate over all components of a certain type
	for (uint32 i = 0; i < compBlock->size(); i += typeSize)
	{
		// get the next component of that type
		componentParam[minSizeIndex] = (BaseECSComponent*)&compBlock[i];

		// get the entity attached to that component, and go thru it's components
		EntityType &entityComponents = handleToEntity( componentParam[minSizeIndex]->entity );

		// if the entity has all the remaining components that we need, then we update it's components
		bool isValid = true;
		for (uint32 j = 0; j < componentTypes.size(); j++)
		{
			if (j == minSizeIndex)
			{	// we know this entity already has this componenty type (since that's how we looked it up),
				// so no need to check that
				continue;
			}

			componentParam[j] = getComponentInternal( entityComponents, *componentBlockArray[j], componentTypes[j] );
			if (componentParam[j] == nullptr)
			{
				isValid = false;
				break;
			}
		}

		if (isValid)
		{	// confirmed that the entity has the components that the system operates on, so update
			systems[index]->updateComponents( delta, &componentParam[0] );
		}
	}
}

//
// checks the number of components of each type and returns the index of the smallest type
//
uint32 ECS::findLeastCommonComponent( const Array<uint32> &componentTypes )
{
	// gets number of components in the first group
	uint32 minSize = components[componentTypes[0]].size() / BaseECSComponent::getTypeSize( componentTypes[0] );
	uint32 minIndx = 0;
	for (uint32 i = 1; i < componentTypes.size(); i++)
	{
		uint32 size = components[componentTypes[i]].size() / BaseECSComponent::getTypeSize( componentTypes[i] );
		if (size < minSize)
		{
			minSize = size;
			minIndx = i;
		}
	}

	return minIndx;
}
