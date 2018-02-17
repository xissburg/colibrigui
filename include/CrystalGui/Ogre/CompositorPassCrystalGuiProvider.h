
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"
#include "OgrePrerequisites.h"
#include "Compositor/Pass/OgreCompositorPassProvider.h"

namespace Ogre
{
	class CompositorPassCrystalGuiProvider : public CompositorPassProvider
	{
		Crystal::CrystalManager	*m_crystalManager;

	public:
		CompositorPassCrystalGuiProvider( Crystal::CrystalManager *crystalManager );

		/** Called from CompositorTargetDef::addPass when adding a Compositor Pass of type 'custom'
		@param passType
		@param customId
			Arbitrary ID in case there is more than one type of custom pass you want to implement.
			Defaults to IdString()
		@param rtIndex
		@param parentNodeDef
		@return
		*/
		virtual CompositorPassDef* addPassDef( CompositorPassType passType,
											   IdString customId,
											   CompositorTargetDef *parentTargetDef,
											   CompositorNodeDef *parentNodeDef );

		/** Creates a CompositorPass from a CompositorPassDef for Compositor Pass of type 'custom'
		@remarks    If you have multiple custom pass types then you will need to use dynamic_cast<>()
					on the CompositorPassDef to determine what custom pass it is.
		*/
		virtual CompositorPass* addPass( const CompositorPassDef *definition, Camera *defaultCamera,
										 CompositorNode *parentNode, const RenderTargetViewDef *rtvDef,
										 SceneManager *sceneManager );
	};
}
