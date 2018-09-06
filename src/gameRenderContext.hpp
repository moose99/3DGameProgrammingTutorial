#pragma once

#include "rendering/renderContext.hpp"
#include "math/transform.hpp"

//
// single shader
// same sampler
// same drawParams
//
class GameRenderContext : public RenderContext
{
public:
	GameRenderContext(RenderDevice& deviceIn, RenderTarget& targetIn,
		RenderDevice::DrawParams &drawParamsIn, Shader &shaderIn, Sampler &samplerIn,
		const Matrix &perspectiveIn) :
		RenderContext(deviceIn, targetIn),
		drawParams(drawParamsIn),
		shader(shaderIn),
		sampler(samplerIn),
		perspective(perspectiveIn)
	{
	}

	// add the vertexArray+texture to a map, along with its transform
	inline void renderMesh(VertexArray &vertexArray, Texture &texture, const Matrix &transformIn)
	{
		meshRenderBuffer[std::make_pair(&vertexArray, &texture)].push_back(perspective * transformIn);
	}

	// draw everything in our render buffer
	void flush();

private:
	RenderDevice::DrawParams &drawParams;
	Shader &shader;
	Sampler &sampler;
	Matrix perspective;
	// map of transforms which go a pair of vertex array 
	Map<std::pair<VertexArray*, Texture*>, Array<Matrix>> meshRenderBuffer;
};

