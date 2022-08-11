
#include "ColibriGui/Ogre/CompositorPassColibriGuiProvider.h"

#include "ColibriGui/Ogre/CompositorPassColibriGui.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiDef.h"

#include "OgreLogManager.h"
#include "OgreStringConverter.h"

namespace Ogre
{
	CompositorPassColibriGuiProvider::CompositorPassColibriGuiProvider(
		Colibri::ColibriManager *colibriManager ) :
		m_colibriManager( colibriManager )
	{
	}
	//-------------------------------------------------------------------------
	CompositorPassDef *CompositorPassColibriGuiProvider::addPassDef(
		CompositorPassType passType, IdString customId, CompositorTargetDef *parentTargetDef,
		CompositorNodeDef *parentNodeDef )
	{
		if( customId == "colibri_gui" )
			return OGRE_NEW CompositorPassColibriGuiDef( parentTargetDef );

		return 0;
	}
	//-------------------------------------------------------------------------
	CompositorPass *CompositorPassColibriGuiProvider::addPass( const CompositorPassDef *definition,
															   Camera *defaultCamera,
															   CompositorNode *parentNode,
															   const RenderTargetViewDef *rtvDef,
															   SceneManager *sceneManager )
	{
		COLIBRI_ASSERT( dynamic_cast<const CompositorPassColibriGuiDef *>( definition ) );
		const CompositorPassColibriGuiDef *colibriGuiDef =
			static_cast<const CompositorPassColibriGuiDef *>( definition );
		return OGRE_NEW CompositorPassColibriGui( colibriGuiDef, sceneManager, rtvDef, parentNode,
												  m_colibriManager );
	}
	//-------------------------------------------------------------------------
	static bool ScriptTranslatorGetBoolean( const AbstractNodePtr &node, bool *result )
	{
		if( node->type != ANT_ATOM )
			return false;
		const AtomAbstractNode *atom = (const AtomAbstractNode *)node.get();
		if( atom->id == 1 || atom->id == 2 )
		{
			*result = atom->id == 1 ? true : false;
			return true;
		}
		return false;
	}
	//-------------------------------------------------------------------------
	void CompositorPassColibriGuiProvider::translateCustomPass( const AbstractNodePtr &node,
																CompositorPassDef *customPassDef )
	{
#if OGRE_VERSION >= OGRE_MAKE_VERSION( 2, 3, 0 )
		if( !dynamic_cast<const CompositorPassColibriGuiDef *>( customPassDef ) )
			return;  // Custom pass not created by us

		CompositorPassColibriGuiDef *colibriGuiDef =
			static_cast<CompositorPassColibriGuiDef *>( customPassDef );

		ObjectAbstractNode *obj = reinterpret_cast<ObjectAbstractNode *>( node.get() );

		AbstractNodeList::const_iterator itor = obj->children.begin();
		AbstractNodeList::const_iterator endt = obj->children.end();

		while( itor != endt )
		{
			if( ( *itor )->type == ANT_PROPERTY )
			{
				const PropertyAbstractNode *prop =
					reinterpret_cast<const PropertyAbstractNode *>( itor->get() );
				if( prop->id == ID_SKIP_LOAD_STORE_SEMANTICS )
				{
					if( prop->values.size() != 1u ||
						!ScriptTranslatorGetBoolean( prop->values.front(),
													 &colibriGuiDef->mSkipLoadStoreSemantics ) )
					{
						Ogre::LogManager::getSingleton().logMessage(
							"Error in colibri_gui skip_load_store_semantics at " + prop->file +
							" line " + StringConverter::toString( prop->line ) );
					}
				}
			}
			++itor;
		}
#endif
	}
}  // namespace Ogre
