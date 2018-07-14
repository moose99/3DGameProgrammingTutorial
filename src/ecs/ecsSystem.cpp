#include"ecsSystem.hpp"

//
// Check is system has a non-optional component
//
bool BaseECSSystem::isValid() const
{
	for (uint32 i = 0; i < componentFlags.size(); i++)
	{
		if ( (componentFlags[i] & FLAG_OPTIONAL) == 0)
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////
// ECSSystemList
///////////////////////////////////////////////////////

bool ECSSystemList::removeSystem( BaseECSSystem &system )
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

