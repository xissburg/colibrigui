/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreStableHeaders.h"

#include "CrystalGui/CrystalAssert.h"
#include "CrystalGui/Ogre/OgreHlmsCrystal.h"
#include "CrystalGui/Ogre/OgreHlmsCrystalDatablock.h"
#include "OgreUnlitProperty.h"
#include "OgreHlmsListener.h"

#include "OgreLwString.h"

#include "OgreViewport.h"
#include "OgreRenderTarget.h"
#include "OgreCamera.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHighLevelGpuProgram.h"

#include "OgreDescriptorSetTexture.h"
#include "OgreTextureGpu.h"

#include "OgreSceneManager.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"

#include "OgreHlmsManager.h"
#include "OgreLogManager.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"

namespace Ogre
{

    extern const String c_unlitBlendModes[];

	HlmsCrystal::HlmsCrystal( Archive *dataFolder, ArchiveVec *libraryFolders ) :
		HlmsUnlit( dataFolder, libraryFolders )
	{
    }
	HlmsCrystal::HlmsCrystal( Archive *dataFolder, ArchiveVec *libraryFolders,
							  HlmsTypes type, const String &typeName ) :
		HlmsUnlit( dataFolder, libraryFolders, type, typeName )
	{
    }
    //-----------------------------------------------------------------------------------
	HlmsCrystal::~HlmsCrystal()
	{
	}
	//-----------------------------------------------------------------------------------
	const HlmsCache* HlmsCrystal::createShaderCacheEntry( uint32 renderableHash,
														const HlmsCache &passCache,
														uint32 finalHash,
														const QueuedRenderable &queuedRenderable )
	{
#if 1
		const HlmsCache *retVal = Hlms::createShaderCacheEntry( renderableHash, passCache, finalHash,
																queuedRenderable );

		if( mShaderProfile == "hlsl" || mShaderProfile == "metal" )
		{
			mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache,
												mSetProperties, queuedRenderable );
			return retVal; //D3D embeds the texture slots in the shader.
		}

		//Set samplers.
		assert( dynamic_cast<const HlmsCrystalDatablock*>( queuedRenderable.renderable->getDatablock() ) );
		const HlmsCrystalDatablock *datablock = static_cast<const HlmsCrystalDatablock*>(
												queuedRenderable.renderable->getDatablock() );

		if( !retVal->pso.pixelShader.isNull() )
		{
			GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();

			int texUnit = 2; //Vertex shader consumes 2 slots with its two tbuffers.

			if( !getProperty( HlmsBaseProp::ShadowCaster ) && datablock->mTexturesDescSet )
			{
				FastArray<const TextureGpu*>::const_iterator itor =
						datablock->mTexturesDescSet->mTextures.begin();
				FastArray<const TextureGpu*>::const_iterator end  =
						datablock->mTexturesDescSet->mTextures.end();

				int numTextures = 0;
				int numArrayTextures = 0;
				while( itor != end )
				{
					if( (*itor)->getInternalTextureType() == TextureTypes::Type2DArray )
					{
						psParams->setNamedConstant( "textureMapsArray[" +
													StringConverter::toString( numArrayTextures++ ) +
													"]", texUnit++ );
					}
					else
					{
						psParams->setNamedConstant( "textureMaps[" +
													StringConverter::toString( numTextures++ ) + "]",
													texUnit++ );
					}

					++itor;
				}
			}
		}

		GpuProgramParametersSharedPtr vsParams = retVal->pso.vertexShader->getDefaultParameters();
		vsParams->setNamedConstant( "worldMatBuf", 0 );
		if( getProperty( UnlitProperty::TextureMatrix ) )
			vsParams->setNamedConstant( "animationMatrixBuf", 1 );

		mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache,
											mSetProperties, queuedRenderable );

		mRenderSystem->_setPipelineStateObject( &retVal->pso );

		mRenderSystem->bindGpuProgramParameters( GPT_VERTEX_PROGRAM, vsParams, GPV_ALL );
		if( !retVal->pso.pixelShader.isNull() )
		{
			GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();
			mRenderSystem->bindGpuProgramParameters( GPT_FRAGMENT_PROGRAM, psParams, GPV_ALL );
		}

		if( !mRenderSystem->getCapabilities()->hasCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER ) )
		{
			//Setting it to the vertex shader will set it to the PSO actually.
			retVal->pso.vertexShader->setUniformBlockBinding( "PassBuffer", 0 );
			retVal->pso.vertexShader->setUniformBlockBinding( "MaterialBuf", 1 );
			retVal->pso.vertexShader->setUniformBlockBinding( "InstanceBuffer", 2 );
		}

		return retVal;
#endif
	}
	//-----------------------------------------------------------------------------------
	void HlmsCrystal::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
	{
		HlmsUnlit::calculateHashForPreCreate( renderable, inOutPieces );

		//See CrystalOgreRenderable
		const Ogre::Renderable::CustomParameterMap &customParams = renderable->getCustomParameters();
		if( customParams.find( 6372 ) != customParams.end() )
		{
			setProperty( "crystal_gui", 1 );
			setProperty( HlmsBaseProp::IdentityWorld, 1 );
			setProperty( HlmsBaseProp::PsoClipDistances, 4 );
		}
	}
	//-----------------------------------------------------------------------------------
	uint32 HlmsCrystal::fillBuffersForCrystal( const HlmsCache *cache,
											   const QueuedRenderable &queuedRenderable,
											   bool casterPass, uint32 baseVertex,
											   uint32 lastCacheHash,
											   CommandBuffer *commandBuffer )
	{
		CRYSTAL_ASSERT_HIGH( getProperty( cache->setProperties,
										  HlmsBaseProp::GlobalClipPlanes ) == 0 &&
							 "Clipping planes not supported! Generated shader may be buggy!" );

		assert( dynamic_cast<const HlmsCrystalDatablock*>( queuedRenderable.renderable->getDatablock() ) );
		const HlmsCrystalDatablock *datablock = static_cast<const HlmsCrystalDatablock*>(
                                                queuedRenderable.renderable->getDatablock() );

        if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
        {
            //We changed HlmsType, rebind the shared textures.
            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastBoundPool = 0;

            //layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer-1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( VertexShader,
                                                                           0, passBuffer, 0,
                                                                           passBuffer->
                                                                           getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( PixelShader,
                                                                           0, passBuffer, 0,
                                                                           passBuffer->
                                                                           getTotalSizeBytes() );

            //layout(binding = 2) uniform InstanceBuffer {} instance
            if( mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)((mCurrentMappedConstBuffer - mStartMappedConstBuffer) + 4) <=
                    mCurrentConstBufferSize )
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
                *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
            }

            rebindTexBuffer( commandBuffer );

            mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock );
        }

        //Don't bind the material buffer on caster passes (important to keep
        //MDI & auto-instancing running on shadow map passes)
        if( mLastBoundPool != datablock->getAssignedPool() && !casterPass )
        {
            //layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( PixelShader,
                                                                           1, newPool->materialBuffer, 0,
                                                                           newPool->materialBuffer->
                                                                           getTotalSizeBytes() );
            if( newPool->extraBuffer )
            {
                TexBufferPacked *extraBuffer = static_cast<TexBufferPacked*>( newPool->extraBuffer );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer( VertexShader, 1,
                                                                               extraBuffer, 0,
                                                                               extraBuffer->
                                                                               getTotalSizeBytes() );
            }

            mLastBoundPool = newPool;
        }

        uint32 * RESTRICT_ALIAS currentMappedConstBuffer    = mCurrentMappedConstBuffer;
		//float * RESTRICT_ALIAS currentMappedTexBuffer       = mCurrentMappedTexBuffer;

        bool exceedsConstBuffer = (size_t)((currentMappedConstBuffer - mStartMappedConstBuffer) + 4) >
                                                                                mCurrentConstBufferSize;

        const size_t minimumTexBufferSize = 16;
		bool exceedsTexBuffer = false/*(currentMappedTexBuffer - mStartMappedTexBuffer) +
									 minimumTexBufferSize >= mCurrentTexBufferSize*/;

        if( exceedsConstBuffer || exceedsTexBuffer )
        {
            currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

            if( exceedsTexBuffer )
                mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof(float) );
            else
                rebindTexBuffer( commandBuffer, true, minimumTexBufferSize * sizeof(float) );

			//currentMappedTexBuffer = mCurrentMappedTexBuffer;
        }

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
        bool useIdentityProjection = queuedRenderable.renderable->getUseIdentityProjection();

        //uint materialIdx[]
        *currentMappedConstBuffer = datablock->getAssignedSlot();
        *reinterpret_cast<float * RESTRICT_ALIAS>( currentMappedConstBuffer+1 ) = datablock->
                                                                                    mShadowConstantBias;
        *(currentMappedConstBuffer+2) = useIdentityProjection;
		*(currentMappedConstBuffer+3) = baseVertex;
        currentMappedConstBuffer += 4;

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        if( !casterPass )
        {
            if( datablock->mTexturesDescSet != mLastDescTexture )
            {
                //Bind textures
                size_t texUnit = 2;

                if( datablock->mTexturesDescSet )
                {
                    *commandBuffer->addCommand<CbTextures>() =
                        CbTextures( texUnit, std::numeric_limits<uint16>::max(),
                                    datablock->mTexturesDescSet );

                    if( !mHasSeparateSamplers )
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                                CbSamplers( texUnit, datablock->mSamplersDescSet );
                    }

                    texUnit += datablock->mTexturesDescSet->mTextures.size();
                }

                mLastDescTexture = datablock->mTexturesDescSet;
            }

            if( datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers )
            {
                if( datablock->mSamplersDescSet )
                {
                    //Bind samplers
                    size_t texUnit = 2;
                    *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers( texUnit, datablock->mSamplersDescSet );
                    mLastDescSampler = datablock->mSamplersDescSet;
                }
            }
        }

        mCurrentMappedConstBuffer   = currentMappedConstBuffer;
		//mCurrentMappedTexBuffer     = currentMappedTexBuffer;

        return ((mCurrentMappedConstBuffer - mStartMappedConstBuffer) >> 2) - 1;
	}
    //-----------------------------------------------------------------------------------
	void HlmsCrystal::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
    {
        //We need to know what RenderSystem is currently in use, as the
        //name of the compatible shading language is part of the path
        RenderSystem *renderSystem = Root::getSingleton().getRenderSystem();
        String shaderSyntax = "GLSL";
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            shaderSyntax = "HLSL";
        else if( renderSystem->getName() == "Metal Rendering Subsystem" )
            shaderSyntax = "Metal";

        //Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
		outLibraryFoldersPaths.push_back( "Hlms/Crystal/Any" );
		outLibraryFoldersPaths.push_back( "Hlms/Unlit/Any" );

        //Fill the data folder path
        outDataFolderPath = "Hlms/Unlit/" + shaderSyntax;
	}
	//-----------------------------------------------------------------------------------
	HlmsDatablock* HlmsCrystal::createDatablockImpl( IdString datablockName,
													 const HlmsMacroblock *macroblock,
													 const HlmsBlendblock *blendblock,
													 const HlmsParamVec &paramVec )
	{
		return OGRE_NEW HlmsCrystalDatablock( datablockName, this, macroblock, blendblock, paramVec );
	}
}