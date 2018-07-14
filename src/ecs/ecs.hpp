#pragma once

#include "ecsComponent.hpp"
#include "ecsSystem.hpp"
#include "dataStructures/map.hpp"
#include "dataStructures/array.hpp"
#include "core/common.hpp"

class ECS
{
public:
	ECS() { }
	~ECS();

	// Entity methods
	EntityHandle makeEntity( BaseECSComponent **components, const uint32 *componentIDs, size_t numComponents );
	void removeEntity( EntityHandle handle );

	template<class... Components>
	EntityHandle makeEntity( Components&&... entitycomponents )
	{
		BaseECSComponent * components[] = { (&entitycomponents)... };
		uint32 componentIDs[] = { (std::remove_reference_t<Components>::ID)... };
		return makeEntity( components, componentIDs, sizeof...(Components) );
	}

	// Component methods
	template <class Component>
	void addComponent( EntityHandle entityHandle, Component *component )
	{
		addComponentInternal( entityHandle, handleToEntity( entityHandle ), Component::ID, component );
	}
	
	template <class Component>
	bool removeComponent( EntityHandle entityHandle )
	{
		return removeComponentInternal( entityHandle, Component::ID );
	}
	
	template <class Component>
	Component *getComponent( EntityHandle entityHandle )
	{
		return getComponentInternal( handleToEntity( entityHandle ), components[Component::ID], Component::ID );
	}

	// System methods


	void updateSystems( ECSSystemList &systems, float delta );

private:

	// map of components.  Holds multiple component lists, organized by component ID
	Map<uint32 /* compID */, ComponentBlock> components;

	// an entity is an array of components, each described by componentID and componentIndex
	// the componentIndex looks up the component in the appropriate memory block from the map above
	// cache friendly!
	typedef Array<std::pair<uint32, uint32>> EntityType;

	// the list of entities, each paired with it's index in the list (for easy removal)
	Array <	std::pair<uint32, EntityType>* > entities;

	// 
	std::pair<uint32, EntityType> *handleToRawType( EntityHandle handle )
	{
		return static_cast<std::pair<uint32, EntityType>*>(handle);
	}

	// returns the entity's index in the list, from a handle
	uint32 handleToEntityIndex( EntityHandle handle )
	{
		return handleToRawType( handle )->first;
	}

	// return the entity as list of components, from a handle
	Array<std::pair<uint32, uint32>>& handleToEntity( EntityHandle handle )
	{
		return handleToRawType( handle )->second;
	}

	void deleteComponent( uint32 componentID, uint32 index );
	bool removeComponentInternal( EntityHandle handle, uint32 componentID );
	void addComponentInternal( EntityHandle handle, EntityType &entity, uint32 componentID, BaseECSComponent *component );

	// go thru all the components on an entity and return a pointer to the one with the matching componentID
	BaseECSComponent *getComponentInternal( EntityType &entityComponents, ComponentBlock &compBlock, uint32 componentID );

	void updateSystemWithMultipleComponents( ECSSystemList &systems, uint32 index, float delta, 
		const Array<uint32> &componentTypes, Array <BaseECSComponent*> &componentParam, 
		Array <ComponentBlock*> &componentBlockArray);
	uint32 findLeastCommonComponent( const Array<uint32> &componentTypes, const Array<uint32> &componentFlags );

	NULL_COPY_AND_ASSIGN( ECS );
};