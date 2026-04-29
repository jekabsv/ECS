#include "State.h"
#include "SharedDataRef.h"

void State::SetData()
{
	LOG_DEBUG(GlobalLogger(), "State", "Setting state data");

	_data->physics.Tie(ecs);
	ecs.Tie(_data);


	MaterialBase base;
	MaterialBase::MakeTransparent(base);
	MaterialBase::SetVertexAttr(base);
	base.vertexShader = "sprite_vert";
	base.fragmentShader = "sprite_frag";

	//Material`Instance whiteMat;
	//whiteMat.materialBase = _data->assets.GetMaterial("text_default");
	//whiteMat.textures[0] = yourWhitePixelTexture;

	//uiContext.Init(&renderer, whiteMat, screenW, screenH, window);

	//ui.Init(_data->SDLrenderer, (float)_data->GAME_WIDTH, (float)_data->GAME_HEIGHT, _data->window);
}