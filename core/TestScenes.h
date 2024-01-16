#pragma once

#include "Utils.h"
#include "Renderer.h"

namespace TestScenes{
	std::shared_ptr<Renderer> Diningroom_MeshLight();

	std::shared_ptr<Renderer> Diningroom_EnvironmentLight();

	std::shared_ptr<Renderer> Subsurface();

	std::shared_ptr<Renderer> Surface();

	std::shared_ptr<Renderer> Cornellbox();
}