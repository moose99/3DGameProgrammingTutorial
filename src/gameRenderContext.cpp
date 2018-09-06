#pragma once

#include "gameRenderContext.hpp"

// draw everything in our render buffer
void GameRenderContext::flush()
{
	Texture *currentTexture = nullptr;
	for (Map<std::pair<VertexArray*, Texture*>, Array<Matrix>>::iterator it = meshRenderBuffer.begin();
		it != meshRenderBuffer.end(); it++)
	{
		const std::pair<VertexArray*, Texture*> &p = it->first;		// the pair itself
		VertexArray *vertexArray = p.first;
		Texture *texture = p.second;
		Matrix *transforms = &it->second[0];		// array of matrices
		size_t numTransforms = it->second.size();	// array of matrices

		if (numTransforms == 0)
		{
			continue;
		}
		if (texture != currentTexture)
		{
			shader.setSampler("diffuse", *texture, sampler, 0);
			currentTexture = texture;
		}
		vertexArray->updateBuffer(4, transforms, numTransforms * sizeof(Matrix));
		draw(shader, *vertexArray, drawParams, numTransforms);
		it->second.clear();		// clear array of matrices each frame
	}
}
