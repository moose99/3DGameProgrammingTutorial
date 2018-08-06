#pragma once
//
// Component base class
// Components hold data and are operated on by Systems
//
#include <tuple>
#include "core/common.hpp"
#include "dataStructures/array.hpp"

struct BaseECSComponent;	// fwd decl
typedef void *EntityHandle;
typedef Array<uint8> ComponentBlock;
typedef uint32 ( *ECSComponentCreateFunc)(ComponentBlock &memory, EntityHandle entity,
	BaseECSComponent *comp );
typedef void ( *ECSComponentFreeFunc )(BaseECSComponent *comp);

#define NULL_ENTITY_HANDLE nullptr

//
// Base component struct
//
//
struct BaseECSComponent
{
public:
	EntityHandle entity = NULL_ENTITY_HANDLE;	// points back to the entity which has this component

	static uint32 registerComponentType(ECSComponentCreateFunc createFn,
		ECSComponentFreeFunc freeFn, size_t size);			// provides a new ID for each component type 

	static ECSComponentCreateFunc getTypeCreateFunction( uint32 id )
	{
		return std::get<0>( (*componentTypes)[id] );	// returns the appropriate create func (elt 0 in the tuple) 
	}
	static ECSComponentFreeFunc getTypeFreeFunction( uint32 id )
	{
		return std::get<1>( (*componentTypes)[id] );
	}
	static size_t getTypeSize( uint32 id )
	{
		return std::get<2>( (*componentTypes)[id] );	
	}
	static bool isTypeValid( uint32 id )
	{
		return id < componentTypes->size();
	}
private:
	static Array<std::tuple<ECSComponentCreateFunc, ECSComponentFreeFunc, size_t>> *componentTypes;

};

// This makes sure that derived components always have the class specific static members they need 
// by using the CRTP
//
// Curiously Recurring Template Pattern - CRTP consists of:
// inheriting from a template class,
// use the derived class ITSELF as a template parameter of the base class
//
template<typename T>
struct ECSComponent : public BaseECSComponent
{
	static const ECSComponentCreateFunc CREATE_FUNC;
	static const ECSComponentFreeFunc FREE_FUNC;
	static const uint32 ID;
	static const uint32 SIZE;
};

//
// Component create function
// Returns the index of where this component was added into the component list (memory block)
// The central ECS class has memory blocks (for each component type) that it uses to hold components
//
template<typename ComponentType>
uint32 ECSComponentCreate( ComponentBlock &memory /* the list of components */,
	EntityHandle entity, BaseECSComponent *componentIn )
{
	uint32 index = memory.size();
	memory.resize( index + ComponentType::SIZE );

	// provide memory for 'new' operation to use
	// create a new component by copying the provided component
	ComponentType *convertedComponent = static_cast<ComponentType*>(componentIn);
	ComponentType *component = new(&memory[index]) ComponentType( *convertedComponent );
	component->entity = entity;
	return index;	// returns the starting location (index) in the memory array where the component is
}

// 
// component free fuction
//
template<typename ComponentType>
void ECSComponentFree( BaseECSComponent *comp )
{	// TODO - could we just call delete here and provide a virtual destructor on the component?
	ComponentType *component = (ComponentType*)comp;
	component->~ComponentType();	// call DTOR manually
}

// declare and assign component ID
template<typename T>
const uint32 ECSComponent<T>::ID = BaseECSComponent::registerComponentType( ECSComponentCreate<T>,
	ECSComponentFree<T>, sizeof( T ) );

// declare component SIZE func.
// returns the size of the component in bytes
template<typename T>
const size_t ECSComponent<T>::SIZE = sizeof(T);

// declare and assign component create func
template<typename T>
const ECSComponentCreateFunc ECSComponent<T>::CREATE_FUNC = ECSComponentCreate<T>;

// declare and assign component free func
template<typename T>
const ECSComponentFreeFunc ECSComponent<T>::FREE_FUNC = ECSComponentFree<T>;

//
// BEGIN EXAMPLE COMPONENT
//
struct  TestComponent : public ECSComponent<TestComponent>
{
	float x;
	float y;
	// ...
};
