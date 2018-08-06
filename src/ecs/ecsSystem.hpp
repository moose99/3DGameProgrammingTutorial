#pragma once

//
// A SYSTEM (logic) operates on a set of components (data)
//
#include "ecsComponent.hpp"

class BaseECSSystem
{
public:
	enum
	{
		FLAG_OPTIONAL = 1
	};
	// ctor
	BaseECSSystem( const Array<uint32> &componentTypesIn ) : componentTypes( componentTypesIn ) {}
	BaseECSSystem() {}

	// TODO - should these compnents be const? since they should not be changed
	virtual void updateComponents( float /*delta*/, BaseECSComponent ** /*components*/ ) {}
	const Array<uint32>& getComponentTypes() { return componentTypes; }
	const Array<uint32>& getComponentFlags() { return componentFlags; }
	bool isValid() const;	// make sure the system has at least 1 non-optional component

protected:
	void addComponentType( uint32 componentType, uint32 componentFlag = 0 )
	{
		componentTypes.push_back( componentType );
		componentFlags.push_back( componentFlag );
	}
private:
	Array<uint32> componentTypes;	// array of component IDs that this sytem operates on
	Array<uint32> componentFlags;
};

//
// Holds a list of systems
//
class ECSSystemList
{

public:
	// return true on success
	bool addSystem( BaseECSSystem &system )
	{
		if (system.isValid())
		{
			systems.push_back( &system );
			return true;
		}
		return false;
	}
	size_t size() const { return systems.size(); }
	BaseECSSystem * operator[]( uint32 index ) { return systems[index]; }
	bool removeSystem( BaseECSSystem &system );

private:
	Array<BaseECSSystem*> systems;
};