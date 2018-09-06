#pragma once

#include "math/vector.hpp"

namespace MotionIntegrators
{
	// compromise between standard and modified euler
	inline void verlet(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		float halfDelta = delta * 0.5f;
		pos += velocity * halfDelta;	// moves position half with the orig velocity
		velocity += acceleration * delta;
		pos += velocity * halfDelta;	// and half with the new velocity
	}

	// Modifies position (and vel) based on vel, accel and time delta
	// Modification to standard Euler by computing velocity first, then pos
	// This way, momentum is conserved (symplectic) and not lost over time.
	// But this is low-precision.
	inline void modifiedEuler(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		// These statements are reversed from standard Euler.
		velocity += acceleration * delta;
		pos += velocity * delta;
	}

	// 4th order symplectic integator, as precise as rk4 but conserves momentum
	inline void forestRuth(Vector3f &pos, Vector3f &velocity, const Vector3f &acceleration, float delta)
	{
		static const float frCoefficient = 1.0f / (2.0f - Math::pow(2.0f, 1.0f / 3.0f));
		static const float frComplement = 1.0f - 2.0f * frCoefficient;
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frCoefficient);
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frComplement);
		MotionIntegrators::verlet(pos, velocity, acceleration, delta * frCoefficient);
	}
}

