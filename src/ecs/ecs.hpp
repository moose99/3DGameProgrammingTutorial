#pragma once

#include "ecsComponent.hpp"
#include "ecsSystem.hpp"
#include "dataStructures/map.hpp"
#include "dataStructures/array.hpp"

class ECS
{
public:
	ECS() { }
	~ECS();

	// Entity methods
	EntityHandle makeEntity( BaseECSComponent* components, const uint32 *componentIDs, size_t numComponents );
	void removeEntity( EntityHandle handle );

	// Component methods
	template <class Component>
	void addComponent( EntityHandle handle, Component *component );
	
	template <class Component>
	void removeComponent( EntityHandle handle );
	
	template <class Component>
	void getComponent( EntityHandle entity );

	// System methods
	void addSystem( BaseECSSystem &system )
	{
		systems.push_back( &system );
	}

	void updateSystems( float delta );
	void removeSystem( BaseECSSystem &system );

private:
	Array<BaseECSSystem*> systems;
	Map<uint32, Array<uint8> /* array provides a block of memory */> components;

	Array <
		std::pair<uint32, Array<std::pair<uint32, uint32>>>*
	> entities;

	std::pair<uint32, Array<std::pair<uint32, uint32>>>* 
		handleToRawType( EntityHandle handle )
	{
		return (std::pair<uint32, Array<std::pair<uint32, uint32>>>*)handle;
	}

	uint32 handleToEntityIndex( EntityHandle handle )
	{
		return handleToRawType( handle )->first;
	}

	Array<std::pair<uint32, uint32>>& handleToEntity( EntityHandle handle )
	{
		return handleToRawType( handle )->second;
	}

	void removeComponentInternal( uint32 componentID, uint32 index ) { /* TODO */ }

	NULL_COPY_AND_ASSIGN( ECS );
};