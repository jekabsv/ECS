#pragma once
#include "Struct.h"


struct MeshComponent
{
	MeshComponent(MeshInstance _MeshName, MaterialInstance _Material, bool _render = false)
		: MeshName(_MeshName), Material(_Material), render(_render) {};

	MeshComponent(StringId _MeshName, MaterialInstance _Material, bool _render = false)
		: Material(_Material), render(_render) {
		MeshName.meshName = _MeshName;
	};

	MeshComponent(StringId _MeshName, StringId _Material, bool _render = false)
		: render(_render) {
		MeshName.meshName = _MeshName;
		Material.materialName = _Material;

		Material.fragBufferSize = 0;
		Material.vertBufferSize = 0;
		Material.textureCount = 0;
	};

	MeshComponent(MeshInstance _MeshName, StringId _Material, bool _render = false)
		: MeshName(_MeshName), render(_render) {
		Material.materialName = _Material;

		Material.fragBufferSize = 0;
		Material.vertBufferSize = 0;
		Material.textureCount = 0;
	};

	MeshInstance MeshName;
	MaterialInstance Material;
	bool render = false;
};