#pragma once
#include "Struct.h"


struct MeshComponent
{
	MeshComponent(StringId _MeshName, StringId _TextureName = "")
		: MeshName(_MeshName), TextureName(_TextureName) {};
	StringId MeshName = "";
	StringId TextureName = "";
};