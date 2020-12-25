#pragma once

#include "PL/pl.h"
#include "PL/PL_math.h"
//TODO: make this so I can visualize tree structure
namespace Debugger
{
	enum class RenderElementType
	{
		LINE, RECTANGLE, BI_TREE
	};

	struct Line
	{
		
	};

	struct RenderElement
	{
		RenderElementType type;
		union
		{

		};
	};

	struct RenderList
	{
		DBuffer<RenderElement, 1, 10> to_draw;
	};

}