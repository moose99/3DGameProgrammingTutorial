#include "ecsComponent.hpp"

// class static member decl, will default to null if not initialized
Array<std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>> *BaseECSComponent::componentTypes = nullptr;

uint32_t BaseECSComponent::registerComponentType( ECSComponentCreateFunc createFn,
	ECSComponentFreeFunc freeFn, size_t size )
{
	if (componentTypes == nullptr)
	{
		componentTypes = new Array<std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>>;
	}

	// ID is the array index; we start appending to the end of the current array, so our start index is 
	// the current size
	uint32 componentID = componentTypes->size();		
	componentTypes->push_back( std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>
		( createFn, freeFn, size /* starting array index */ ) );
	return componentID++;
}