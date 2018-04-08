
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	class Button : public Renderable
	{
		Label	*m_label;

	public:
		Button( CrystalManager *manager );

		virtual void _destroy();

		Label* getLabel();
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
