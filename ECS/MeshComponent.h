#pragma once
#include "Struct.h"


struct MeshComponent
{
	MeshComponent(StringId _MeshName, bool _render = false)
		: MeshName(_MeshName), render(_render) {};

	StringId MaterialName = "";

	StringId MeshName = "";
	bool render = false;
};