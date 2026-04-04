#pragma once
#include "Struct.h"


struct MeshComponent
{
	MeshComponent(StringId _MeshName, StringId _TextureName = "", bool _render = false)
		: MeshName(_MeshName), TextureName(_TextureName), render(_render) {};
	StringId MeshName = "";
	StringId TextureName = "";
	bool render = false;
};