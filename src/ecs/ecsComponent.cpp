#include "ecsComponent.hpp"

static uint32_t componentID = 0;

uint32_t BaseECSComponent::nextID()
{
	return componentID++;
}