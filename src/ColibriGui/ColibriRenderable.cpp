
#include "ColibriGui/ColibriRenderable.h"
#include <OGRE/OgreColourValue.h>

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbPipelineStateObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "OgreRenderQueue.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"

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
		m_overrideSkinColour( false ),
		m_colour( Ogre::ColourValue::White ),
		m_numVertices( 6u * 9u ),
		m_currVertexBufferOffset( 0 ),
		m_visualsEnabled( true )
	{
		m_zOrder = _wrapZOrderInternalId( 0 );
		memset( m_stateInformation, 0, sizeof( m_stateInformation ) );
		for( size_t i = 0u; i < States::NumStates; ++i )
			m_stateInformation[i].defaultColour = Ogre::ColourValue::White;
	}
	//-------------------------------------------------------------------------
	void Renderable::_notifyCanvasChanged()
	{
		setClipBordersMatchSkin();
		Widget::_notifyCanvasChanged();
	}
	//-------------------------------------------------------------------------
	void Renderable::stateChanged( States::States newState )
	{
		setDatablock( m_stateInformation[newState].materialName );
	}
	//-------------------------------------------------------------------------
	void Renderable::setVisualsEnabled( bool bEnabled )
	{
		m_visualsEnabled = bEnabled;
	}
	//-------------------------------------------------------------------------
	bool Renderable::isVisualsEnabled() const
	{
		return m_visualsEnabled;
	}
	//-------------------------------------------------------------------------
	void Renderable::setColour( bool overrideSkinColour, const Ogre::ColourValue &colour )
	{
		m_overrideSkinColour = overrideSkinColour;
		if( overrideSkinColour )
			m_colour = colour;
		else
			m_colour = m_stateInformation[m_currentState].defaultColour;
	}
	//-------------------------------------------------------------------------
	const Ogre::ColourValue &Renderable::getColour() const { return m_colour; }
	//-------------------------------------------------------------------------
	bool Renderable::getOverrideSkinColour() const { return m_overrideSkinColour; }
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

		if( !m_overrideSkinColour )
			m_colour = m_stateInformation[m_currentState].defaultColour;

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setEmptySkin()
	{
    	Ogre::Root *root = getManager()->getRoot();
		Ogre::IdString materialName = "EmptySkin";
		Ogre::Hlms *hlms = root->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::HlmsDatablock *datablock = hlms->getDatablock(materialName);

		if (!datablock) {
			Ogre::HlmsMacroblock macroblock;
			macroblock.mDepthCheck = false;
			datablock = hlms->createDatablock(materialName, "EmptyMaterial", macroblock, {}, {});
		}

		Ogre::HlmsUnlitDatablock *unlitDatablock = static_cast<Ogre::HlmsUnlitDatablock *>(datablock);
		unlitDatablock->setColour(Ogre::ColourValue(1, 1, 1, 1));
		setDatablock(unlitDatablock);

		for (size_t i = 0; i < Colibri::States::NumStates; ++i) {
			auto &stateInfo = m_stateInformation[i];
			memset(&stateInfo, 0, sizeof(stateInfo));
			stateInfo.materialName = materialName;
			stateInfo.centerAspectRatio = 1;
			stateInfo.defaultColour = Ogre::ColourValue(1, 1, 1, 1);

			for (size_t j = 0; j < Colibri::GridLocations::NumGridLocations; ++j) {
				stateInfo.uvTopLeftBottomRight[j] = {0.0009765625, 0.0009765625, 0.999023437, 0.999023437};
			}
		}
	}
	//-------------------------------------------------------------------------
	void Renderable::setSkinPack( Ogre::IdString skinName )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinPack *pack = skinManager->findSkinPack( skinName );
		if( pack )
		{
			for( size_t i = 0; i < States::NumStates; ++i )
			{
				const SkinInfo *skin = skinManager->findSkin( *pack, static_cast<States::States>( i ) );
				if( skin )
				{
					m_stateInformation[i] = skin->stateInfo;
					if( i == m_currentState )
						setDatablock( m_stateInformation[i].materialName );
				}
			}
		}

		if( !m_overrideSkinColour )
			m_colour = m_stateInformation[m_currentState].defaultColour;

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setBorderSize( const float borderSize[colibri_nonnull Borders::NumBorders],
									States::States forState, bool bClipBordersMatchSkin )
	{
		if( forState == States::NumStates )
		{
			for( size_t i = 0u; i < States::NumStates; ++i )
			{
				for( size_t j = 0u; j < Borders::NumBorders; ++j )
					m_stateInformation[i].borderSize[j] = borderSize[j];
			}
		}
		else
		{
			for( size_t j = 0u; j < Borders::NumBorders; ++j )
				m_stateInformation[forState].borderSize[j] = borderSize[j];
		}

		if( bClipBordersMatchSkin )
			setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::_setSkinPack( SkinInfo const * colibri_nonnull
								   const * colibri_nullable skinInfos )
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

		if( !m_overrideSkinColour )
			m_colour = m_stateInformation[m_currentState].defaultColour;

		setClipBordersMatchSkin();
	}
	//-------------------------------------------------------------------------
	void Renderable::setState( States::States state, bool smartHighlight )
	{
		Widget::setState( state, smartHighlight );

		if( !m_overrideSkinColour )
			m_colour = m_stateInformation[m_currentState].defaultColour;

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
		const StateInformation &stateInfo = m_stateInformation[state];

		const Ogre::Vector2 &pixelToCanvas = m_manager->getCanvasSize() * m_manager->getPixelSize();

		float clipBorders[Borders::NumBorders];
		clipBorders[Borders::Left]	= stateInfo.borderSize[Borders::Left] * pixelToCanvas.x;
		clipBorders[Borders::Top]	= stateInfo.borderSize[Borders::Top] * pixelToCanvas.y;
		clipBorders[Borders::Right]	= stateInfo.borderSize[Borders::Right] * pixelToCanvas.x;
		clipBorders[Borders::Bottom]= stateInfo.borderSize[Borders::Bottom] * pixelToCanvas.y;
		setClipBorders( clipBorders );
	}
	//-------------------------------------------------------------------------
	void Renderable::broadcastNewVao( Ogre::VertexArrayObject *vao, Ogre::VertexArrayObject *textVao )
	{
		setVao( !isLabel() ? vao : textVao );
		Widget::broadcastNewVao( vao, textVao );
	}
	//-------------------------------------------------------------------------
	void Renderable::_addCommands( ApiEncapsulatedObjects &apiObject, bool collectingBreadthFirst )
	{
		if( m_culled )
			return;

		if( m_visualsEnabled )
		{
			using namespace Ogre;

			CommandBuffer *commandBuffer = apiObject.commandBuffer;

			QueuedRenderable queuedRenderable( 0u, this, this );

			uint32 lastHlmsCacheHash = apiObject.lastHlmsCache->hash;
			VertexArrayObject *vao = mVaoPerLod[0].back();
			const HlmsCache *hlmsCache = apiObject.hlms->getMaterial( apiObject.lastHlmsCache,
																	  *apiObject.passCache,
																	  queuedRenderable,
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

			const uint32 firstVertex = m_currVertexBufferOffset + apiObject.basePrimCount[widgetType];

			uint32 baseInstance = apiObject.hlms->fillBuffersForColibri(
									  hlmsCache, queuedRenderable, false,
									  firstVertex,
									  lastHlmsCacheHash, apiObject.commandBuffer );

			if( apiObject.drawCmd != commandBuffer->getLastCommand() ||
				apiObject.lastVaoName != vao->getVaoName() )
			{
				if( apiObject.drawCountPtr && apiObject.drawCountPtr->primCount == 0u )
				{
					// Adreno 618 will GPU crash if we send an indirect cmd with vertex_count = 0
					--apiObject.drawCmd->numDraws;
					// Since we only emit CbDrawStrip we can assume the previous cmd
					// issued a CbDrawStrip, so take it back. Otherwise we'd have to
					// save what our last cmd was.
					apiObject.indirectDraw -= sizeof( CbDrawStrip );
				}

				{
					*commandBuffer->addCommand<CbVao>() = CbVao( vao );
					*commandBuffer->addCommand<CbIndirectBuffer>() =
							CbIndirectBuffer( apiObject.indirectBuffer );
					apiObject.lastVaoName = vao->getVaoName();
				}

				void *offset = reinterpret_cast<void *>(
					ptrdiff_t( apiObject.indirectBuffer->_getFinalBufferStart() ) +
					( apiObject.indirectDraw - apiObject.startIndirectDraw ) );

				CbDrawCallStrip *drawCall = commandBuffer->addCommand<CbDrawCallStrip>();
				*drawCall = CbDrawCallStrip( apiObject.baseInstanceAndIndirectBuffers, vao, offset );
				drawCall->numDraws = 1u;
				apiObject.drawCmd = drawCall;
				apiObject.primCount = 0;
				apiObject.lastDatablock = mHlmsDatablock;

				apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
				apiObject.drawCountPtr->primCount		= 0;
				apiObject.drawCountPtr->instanceCount	= 1u;
				apiObject.drawCountPtr->firstVertexIndex= firstVertex;
				apiObject.drawCountPtr->baseInstance	= baseInstance;
				apiObject.indirectDraw += sizeof( CbDrawStrip );
			}
			else if( bIsLabel && apiObject.lastDatablock != mHlmsDatablock )
			{
				if( apiObject.drawCountPtr && apiObject.drawCountPtr->primCount == 0u )
				{
					// Adreno 618 will GPU crash if we send an indirect cmd with vertex_count = 0
					--apiObject.drawCmd->numDraws;
					// Since we only emit CbDrawStrip we can assume the previous cmd
					// issued a CbDrawStrip, so take it back. Otherwise we'd have to
					// save what our last cmd was.
					apiObject.indirectDraw -= sizeof( CbDrawStrip );
				}

				//Text has arbitrary number of of vertices, thus we can't properly calculate the drawId
				//and therefore the material ID unless we issue a start a new draw.
				CbDrawCallStrip *drawCall = static_cast<CbDrawCallStrip*>( apiObject.drawCmd );
				++drawCall->numDraws;
				apiObject.primCount = 0;
				apiObject.lastDatablock = mHlmsDatablock;

				apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
				apiObject.drawCountPtr->primCount		= 0;
				apiObject.drawCountPtr->instanceCount	= 1u;
				apiObject.drawCountPtr->firstVertexIndex= firstVertex;
				apiObject.drawCountPtr->baseInstance	= baseInstance;
				apiObject.indirectDraw += sizeof( CbDrawStrip );
			}
			else if( apiObject.nextFirstVertex != firstVertex )
			{
				if( apiObject.drawCountPtr && apiObject.drawCountPtr->primCount == 0u )
				{
					// Adreno 618 will GPU crash if we send an indirect cmd with vertex_count = 0
					--apiObject.drawCmd->numDraws;
					// Since we only emit CbDrawStrip we can assume the previous cmd
					// issued a CbDrawStrip, so take it back. Otherwise we'd have to
					// save what our last cmd was.
					apiObject.indirectDraw -= sizeof( CbDrawStrip );
				}

				//If we're here, we're most likely rendering using breadth first.
				//Unfortunately, breadth first breaks ordering, thus firstVertex jumped.
				//Add a new draw without creating a new command
				CbDrawCallStrip *drawCall = static_cast<CbDrawCallStrip*>( apiObject.drawCmd );
				++drawCall->numDraws;
				apiObject.primCount = 0;
				apiObject.lastDatablock = mHlmsDatablock;

				apiObject.drawCountPtr = reinterpret_cast<CbDrawStrip*>( apiObject.indirectDraw );
				apiObject.drawCountPtr->primCount		= 0;
				apiObject.drawCountPtr->instanceCount	= 1u;
				apiObject.drawCountPtr->firstVertexIndex= firstVertex;
				apiObject.drawCountPtr->baseInstance	= baseInstance;
				apiObject.indirectDraw += sizeof( CbDrawStrip );
			}

			apiObject.primCount += m_numVertices;
			apiObject.drawCountPtr->primCount = apiObject.primCount;

			apiObject.nextFirstVertex = firstVertex + m_numVertices;
		}

		addChildrenCommands( apiObject, collectingBreadthFirst );
	}
	//-------------------------------------------------------------------------
	const StateInformation& Renderable::getStateInformation( States::States state ) const
	{
		if( state == States::NumStates )
			state = m_currentState;
		return m_stateInformation[state];
	}
	//-------------------------------------------------------------------------
	void Renderable::_fillBuffersAndCommands( UiVertex * colibri_nonnull * colibri_nonnull
											 RESTRICT_ALIAS vertexBuffer,
											 GlyphVertex * colibri_nonnull * colibri_nonnull
											 RESTRICT_ALIAS textVertBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Vector2 &parentCurrentScrollPos,
											 const Matrix2x3 &parentRot )
	{
		_fillBuffersAndCommands( vertexBuffer, textVertBuffer, parentPos,
								 parentCurrentScrollPos, parentRot, Ogre::Vector2::ZERO, false );
	}
}
