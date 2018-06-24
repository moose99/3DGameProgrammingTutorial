#include "ecsComponent.hpp"

// class static member decl
Array<std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>> BaseECSComponent::componentTypes;

uint32_t BaseECSComponent::registerComponentType( ECSComponentCreateFunc createFn,
	ECSComponentFreeFunc freeFn, size_t size )
{
	uint32 componentID = componentTypes.size();		// ID is the array index
	componentTypes.push_back( std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>
		( createFn, freeFn, size ) );
	return componentID++;
}