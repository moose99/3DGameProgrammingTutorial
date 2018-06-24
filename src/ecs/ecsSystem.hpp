#pragma once

//
// A SYSTEM (logic) operates on a set of components (data)
//
#include "ecsComponent.hpp"

class BaseECSSystem
{
public:
	// ctor
	BaseECSSystem( const Array<uint32> &componentTypesIn ) : componentTypes( componentTypesIn ) {}

	virtual void updateComponents( float delta, BaseECSComponent **components ) {}
	const Array<uint32>& getComponentTypes()
	{
		return componentTypes; 
	}
private:
	Array<uint32> componentTypes;	// array of component IDs that this sytem operates on
};