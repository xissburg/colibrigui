
#include "ColibriGui/ColibriRenderable.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbPipelineStateObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "OgreRenderQueue.h"
#include "OgreHlms.h"
#include "OgreHlmsDatablock.h"

#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"
#include "ColibriGui/Ogre/ColibriOgreRenderable.h"
#include "ColibriGui/Ogre/OgreHlmsColibri.h"

#include "OgreBitwise.h"

#include "ColibriRenderable.inl"

namespace Colibri
{
	Renderable::Renderable( ColibriManager *manager ) :
		Widget( manager ),
		ColibriOgreRenderable( Ogre::Id::generateNewId<Ogre::ColibriOgreRenderable>(),
							   manager->getOgreObjectMemoryManager(),
							   manager->getOgreSceneManager(), 0u, manager ),
		m_colour( Ogre::ColourValue::White ),
		m_numVertices( 6u * 9u )
	{
		memset( m_stateInformation, 0, sizeof(m_stateInformation) );
	}
	//-------------------------------------------------------------------------
	void Renderable::stateChanged( States::States newState )
	{
		setDatablock( m_stateInformation[newState].materialName );
	}
	//-------------------------------------------------------------------------
	void Renderable::setSkin( Ogre::IdString skinName, States::States forState )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinInfoMap &skins = skinManager->getSkins();

		SkinInfoMap::const_iterator itor = skins.find( skinName );
		if( itor != skins.end() )
		{
			if( forState == States::NumStates )
			{
				for( size_t i=0; i<States::NumStates; ++i )
					m_stateInformation[i] = itor->second.stateInfo;
				setDatablock( m_stateInformation[0].materialName );
			}
			else
			{
				m_stateInformation[forState] = itor->second.stateInfo;
				if( forState == m_currentState )
					setDatablock( m_stateInformation[forState].materialName );
			}
		}

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setSkinPack( Ogre::IdString skinName )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinInfoMap &skins = skinManager->getSkins();
		const SkinPackMap &skinPacks = skinManager->getSkinPacks();

		SkinPackMap::const_iterator itor = skinPacks.find( skinName );
		if( itor != skinPacks.end() )
		{
			const SkinPack &pack = itor->second;

			for( size_t i=0; i<States::NumStates; ++i )
			{
				SkinInfoMap::const_iterator itSkinInfo = skins.find( pack.skinInfo[i] );
				if( itSkinInfo != skins.end() )
				{
					m_stateInformation[i] = itSkinInfo->second.stateInfo;
					if( i == m_currentState )
						setDatablock( m_stateInformation[i].materialName );
				}
			}
		}

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::_setSkinPack( SkinInfo const * colibrigui_nonnull
								   const * colibrigui_nullable skinInfos )
	{
		for( size_t i=0; i<States::NumStates; ++i )
		{
			if( skinInfos[i] )
			{
				m_stateInformation[i] = skinInfos[i]->stateInfo;
				if( i == m_currentState )
					setDatablock( m_stateInformation[i].materialName );
			}
		}

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setState( States::States state, bool smartHighlight, bool broadcastEnable )
	{
		Widget::setState( state, smartHighlight, broadcastEnable );

		if( mHlmsDatablock->getName() != m_stateInformation[m_currentState].materialName )
			setDatablock( m_stateInformation[m_currentState].materialName );

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setClipBordersMatchSkin()
	{
		setClipBordersMatchSkin( m_currentState );
	}
	//-------------------------------------------------------------------------
	void Renderable::setClipBordersMatchSkin( States::States state )
	{
		const Ogre::Vector2 pixelSize = m_manager->getPixelSize();

		const StateInformation &stateInfo = m_stateInformation[state];

		float clipBorders[Borders::NumBorders];
		clipBorders[Borders::Left]	= stateInfo.borderSize[Borders::Left] * pixelSize.x;
		clipBorders[Borders::Top]	= stateInfo.borderSize[Borders::Top] * pixelSize.y;
		clipBorders[Borders::Right]	= stateInfo.borderSize[Borders::Right] * pixelSize.x;
		clipBorders[Borders::Bottom]= stateInfo.borderSize[Borders::Bottom] * pixelSize.y;
		setClipBorders( clipBorders );
	}
	//-------------------------------------------------------------------------
	void Renderable::broadcastNewVao( Ogre::VertexArrayObject *vao, Ogre::VertexArrayObject *textVao )
	{
		setVao( !isLabel() ? vao : textVao );
		Widget::broadcastNewVao( vao, textVao );
	}
	//-------------------------------------------------------------------------
	void Renderable::_addCommands( ApiEncapsulatedObjects &apiObject )
	{
		if( m_culled )
			return;

		using namespace Ogre;

		CommandBuffer *commandBuffer = apiObject.commandBuffer;

		QueuedRenderable queuedRenderable( 0u, this, this );

		uint32 lastHlmsCacheHash = apiObject.lastHlmsCache->hash;
		VertexArrayObject *vao = mVaoPerLod[0].back();
		const HlmsCache *hlmsCache = apiObject.hlms->getMaterial( apiObject.lastHlmsCache,
																  *apiObject.passCache,
																  queuedRenderable,
																  vao->getInputLayoutId(),
																  false );
		if( lastHlmsCacheHash != hlmsCache->hash )
		{
			CbPipelineStateObject *psoCmd = commandBuffer->addCommand<CbPipelineStateObject>();
			*psoCmd = CbPipelineStateObject( &hlmsCache->pso );
			apiObject.lastHlmsCache = hlmsCache;

			//Flush the Vao when changing shaders. Needed by D3D11/12 & possibly Vulkan
			apiObject.lastVaoName = 0;
		}

		const bool bIsLabel = isLabel();
		const size_t widgetType = bIsLabel ? 1u : 0u;

		uint32 baseInstance = apiObject.hlms->fillBuffersForColibri(
								  hlmsCache, queuedRenderable, false,
								  apiObject.accumPrimCount[widgetType],
								  lastHlmsCacheHash, apiObject.commandBuffer );

		if( apiObject.drawCmd != commandBuffer->getLastCommand() ||
			apiObject.lastVaoName != vao->getVaoName() )
		{
			{
				*commandBuffer->addCommand<CbVao>() = CbVao( vao );
				*commandBuffer->addCommand<CbIndirectBuffer>() =
						CbIndirectBuffer( apiObject.indirectBuffer );
				apiObject.lastVaoName = vao->getVaoName();
			}

			void *offset = reinterpret_cast<void*>( apiObject.indirectBuffer->_getFinalBufferStart() +
													(apiObject.indirectDraw -
													 apiObject.startIndirectDraw) );

			CbDrawCallStrip *drawCall = commandBuffer->addCommand<CbDrawCallStrip>();
			*drawCall = CbDrawCallStrip( apiObject.baseInstanceAndIndirectBuffers,
										 vao, offset );
			drawCall->numDraws = 1u;
			apiObject.drawCmd = drawCall;
			apiObject.primCount = 0;
			apiObject.lastDatablock = mHlmsDatablock;

			apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
			apiObject.drawCountPtr->primCount		= 0;
			apiObject.drawCountPtr->instanceCount	= 1u;
			apiObject.drawCountPtr->firstVertexIndex=apiObject.accumPrimCount[widgetType];
			apiObject.drawCountPtr->baseInstance	= baseInstance;
			apiObject.indirectDraw += sizeof( CbDrawStrip );
		}
		else if( bIsLabel && apiObject.lastDatablock != mHlmsDatablock )
		{
			//Text  has arbitrary number of of vertices, thus we can't properly calculate the drawId
			//and therefore the material ID unless we issue a start a new draw.
			CbDrawCallStrip *drawCall = static_cast<CbDrawCallStrip*>( apiObject.drawCmd );
			++drawCall->numDraws;
			apiObject.primCount = 0;
			apiObject.lastDatablock = mHlmsDatablock;

			apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
			apiObject.drawCountPtr->primCount		= 0;
			apiObject.drawCountPtr->instanceCount	= 1u;
			apiObject.drawCountPtr->firstVertexIndex=apiObject.accumPrimCount[widgetType];
			apiObject.drawCountPtr->baseInstance	= baseInstance;
			apiObject.indirectDraw += sizeof( CbDrawStrip );
		}

		apiObject.primCount += m_numVertices;
		apiObject.accumPrimCount[widgetType] += m_numVertices;
		apiObject.drawCountPtr->primCount = apiObject.primCount;

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.begin() + m_numNonRenderables;

		while( itor != end )
		{
			(*itor)->addNonRenderableCommands( apiObject );
			++itor;
		}

		itor = m_children.begin() + m_numNonRenderables;
		end  = m_children.end();

		while( itor != end )
		{
			COLIBRI_ASSERT_HIGH( dynamic_cast<Renderable*>( *itor ) );
			Renderable *asRenderable = static_cast<Renderable*>( *itor );
			asRenderable->_addCommands( apiObject );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Renderable::_fillBuffersAndCommands( UiVertex * colibrigui_nonnull * colibrigui_nonnull
											 RESTRICT_ALIAS vertexBuffer,
											 GlyphVertex * colibrigui_nonnull * colibrigui_nonnull
											 RESTRICT_ALIAS textVertBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Vector2 &parentCurrentScrollPos,
											 const Ogre::Matrix3 &parentRot )
	{
		_fillBuffersAndCommands( vertexBuffer, textVertBuffer, parentPos,
								parentCurrentScrollPos, parentRot, Ogre::Vector2::ZERO, false );
	}
}