
#include "GraphicsSystem.h"
#include "CrystalGuiGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"

#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/Text/CrystalShaperManager.h"
#include "CrystalGui/Text/CrystalShaper.h"
#include "CrystalGui/Ogre/CompositorPassCrystalGuiProvider.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "CrystalGui/Ogre/OgreHlmsCrystal.h"
#include "OgreArchiveManager.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#include "OgreLogManager.h"
#include "hb.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <pwd.h>
    #include <errno.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    #include "shlobj.h"
#endif

#define TODO_fix_leak

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
	extern Crystal::CrystalManager *crystalManager;
	class CrystalLogListener : public Crystal::LogListener
	{
		virtual void log( const char *text, Crystal::LogSeverity::LogSeverity severity )
		{
			Ogre::LogManager::getSingleton().logMessage( text );
		}
	};
	static CrystalLogListener g_crystalLogListener;

    class CrystalGuiGraphicsSystem : public GraphicsSystem
	{
		virtual Ogre::CompositorWorkspace* setupCompositor()
		{
			Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
			return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
													"CrystalGuiWorkspace", true );
		}

		void registerHlms(void)
		{
			Ogre::ConfigFile cf;
			cf.load( mResourcePath + "resources2.cfg" );

	#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
			Ogre::String rootHlmsFolder = Ogre::macBundlePath() + '/' +
									  cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
	#else
			Ogre::String rootHlmsFolder = mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
	#endif

			if( rootHlmsFolder.empty() )
				rootHlmsFolder = "./";
			else if( *(rootHlmsFolder.end() - 1) != '/' )
				rootHlmsFolder += "/";

			//At this point rootHlmsFolder should be a valid path to the Hlms data folder

			Ogre::HlmsCrystal *hlmsCrystal = 0;
			Ogre::HlmsPbs *hlmsPbs = 0;

			//For retrieval of the paths to the different folders needed
			Ogre::String mainFolderPath;
			Ogre::StringVector libraryFoldersPaths;
			Ogre::StringVector::const_iterator libraryFolderPathIt;
			Ogre::StringVector::const_iterator libraryFolderPathEn;

			Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

			{
				//Create & Register HlmsCrystal
				//Get the path to all the subdirectories used by HlmsCrystal
				Ogre::HlmsCrystal::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
				Ogre::Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath,
																   "FileSystem", true );
				Ogre::ArchiveVec archiveUnlitLibraryFolders;
				libraryFolderPathIt = libraryFoldersPaths.begin();
				libraryFolderPathEn = libraryFoldersPaths.end();
				while( libraryFolderPathIt != libraryFolderPathEn )
				{
					Ogre::Archive *archiveLibrary =
							archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
					archiveUnlitLibraryFolders.push_back( archiveLibrary );
					++libraryFolderPathIt;
				}

				//Create and register the unlit Hlms
				hlmsCrystal = OGRE_NEW Ogre::HlmsCrystal( archiveUnlit, &archiveUnlitLibraryFolders );
				Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsCrystal );
			}

			{
				//Create & Register HlmsPbs
				//Do the same for HlmsPbs:
				Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
				Ogre::Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath,
																 "FileSystem", true );

				//Get the library archive(s)
				Ogre::ArchiveVec archivePbsLibraryFolders;
				libraryFolderPathIt = libraryFoldersPaths.begin();
				libraryFolderPathEn = libraryFoldersPaths.end();
				while( libraryFolderPathIt != libraryFolderPathEn )
				{
					Ogre::Archive *archiveLibrary =
							archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
					archivePbsLibraryFolders.push_back( archiveLibrary );
					++libraryFolderPathIt;
				}

				//Create and register
				hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
				Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
			}


			Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
			if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
			{
				//Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
				//and below to avoid saturating AMD's discard limit (8MB) or
				//saturate the PCIE bus in some low end machines.
				bool supportsNoOverwriteOnTextureBuffers;
				renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
												  &supportsNoOverwriteOnTextureBuffers );

				if( !supportsNoOverwriteOnTextureBuffers )
				{
					hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
					hlmsCrystal->setTextureBufferDefaultSize( 512 * 1024 );
				}
			}
		}

        virtual void setupResources(void)
        {
			TODO_fix_leak;

			struct ShaperSettings
			{
				const char *locale;
				const char *fullpath;
				hb_script_t script;
				Crystal::HorizReadingDir::HorizReadingDir horizReadingDir;
				bool useKerning;
				bool allowsVerticalLayout;
				ShaperSettings( const char *_locale, const char *_fullpath, hb_script_t _script,
								bool _useKerning=false,
								Crystal::HorizReadingDir::HorizReadingDir _horizReadingDir=
						Crystal::HorizReadingDir::LTR,
								bool _allowsVerticalLayout=false ) :
					locale( _locale ),
					fullpath( _fullpath ),
					script( _script ),
					horizReadingDir( _horizReadingDir ),
					useKerning( _useKerning ),
					allowsVerticalLayout( _allowsVerticalLayout )
				{

				}
			};

			ShaperSettings shaperSettings[3] =
			{
				ShaperSettings( "en", "../Data/Fonts/DejaVuSerif.ttf", HB_SCRIPT_LATIN, true ),
				ShaperSettings( "ar", "../Data/Fonts/amiri-0.104/amiri-regular.ttf", HB_SCRIPT_ARABIC, false,
				Crystal::HorizReadingDir::RTL ),
				ShaperSettings( "ch", "../Data/Fonts/fireflysung-1.3.0/fireflysung.ttf", HB_SCRIPT_HAN, false,
				Crystal::HorizReadingDir::LTR, true )
			};

			crystalManager = new Crystal::CrystalManager( &g_crystalLogListener );
			Crystal::ShaperManager *shaperManager = crystalManager->getShaperManager();

			for( size_t i=0; i<sizeof( shaperSettings ) / sizeof( shaperSettings[0] ); ++i )
			{
				Crystal::Shaper *shaper;
				shaper = shaperManager->addShaper( shaperSettings[i].script, shaperSettings[i].fullpath,
												   shaperSettings[i].locale );
				if( shaperSettings[i].useKerning )
					shaper->addFeatures( Crystal::Shaper::KerningOn );
			}

			size_t defaultFont = 0; //"en"
			shaperManager->setDefaultShaper( defaultFont + 1u,
											 shaperSettings[defaultFont].horizReadingDir,
											 shaperSettings[defaultFont].allowsVerticalLayout );

			Ogre::CompositorPassCrystalGuiProvider *compoProvider =
					OGRE_NEW Ogre::CompositorPassCrystalGuiProvider( crystalManager );
			Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
			compositorManager->setCompositorPassProvider( compoProvider );

            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load(mResourcePath + "resources2.cfg");

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *(dataFolder.end() - 1) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, "FileSystem", "General" );
        }

    public:
        CrystalGuiGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
            mResourcePath = "../Data/";
			mAlwaysAskForConfig = false;

            //It's recommended that you set this path to:
            //	%APPDATA%/CrystalGui/ on Windows
            //	~/.config/CrystalGui/ on Linux
            //	macCachePath() + "/CrystalGui/" (NSCachesDirectory) on Apple -> Important because
            //	on iOS your app could be rejected from App Store when they see iCloud
            //	trying to backup your Ogre.log & ogre.cfg auto-generated without user
            //	intervention. Also convenient because these settings will be deleted
            //	if the user removes cached data from the app, so the settings will be
            //	reset.
            //  Obviously you can replace "CrystalGui" by your app's name.
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            mWriteAccessFolder =  + "/";
            TCHAR path[MAX_PATH];
            if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL,
                                            SHGFP_TYPE_CURRENT, path ) != S_OK ) )
            {
                //Need to convert to OEM codepage so that fstream can
                //use it properly on international systems.
        #if defined(_UNICODE) || defined(UNICODE)
                int size_needed = WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                                       NULL, 0, NULL, NULL );
                mWriteAccessFolder = std::string( size_needed, 0 );
                WideCharToMultiByte( CP_OEMCP, 0, path, (int)wcslen(path),
                                     &mWriteAccessFolder[0], size_needed, NULL, NULL );
        #else
                TCHAR oemPath[MAX_PATH];
                CharToOem( path, oemPath );
                mWriteAccessFolder = std::string( oemPath );
        #endif
                mWriteAccessFolder += "/CrystalGui/";

                //Attempt to create directory where config files go
                if( !CreateDirectoryA( mWriteAccessFolder.c_str(), NULL ) &&
                    GetLastError() != ERROR_ALREADY_EXISTS )
                {
                    //Couldn't create directory (no write access?),
                    //fall back to current working dir
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            const char *homeDir = getenv("HOME");
            if( homeDir == 0 )
                homeDir = getpwuid( getuid() )->pw_dir;
            mWriteAccessFolder = homeDir;
            mWriteAccessFolder += "/.config";
            int result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            int errorReason = errno;

            //Create "~/.config"
            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
            else
            {
                //Create "~/.config/CrystalGui"
                mWriteAccessFolder += "/CrystalGui/";
                result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
                errorReason = errno;

                if( result && errorReason != EEXIST )
                {
                    printf( "Error. Failing to create path '%s'. Do you have access rights?",
                            mWriteAccessFolder.c_str() );
                    mWriteAccessFolder = "";
                }
            }
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            mWriteAccessFolder = macCachePath() + "/CrystalGui/";
            //Create "pathToCache/CrystalGui"
            mWriteAccessFolder += "/CrystalGui/";
            result = mkdir( mWriteAccessFolder.c_str(), S_IRWXU|S_IRWXG );
            errorReason = errno;

            if( result && errorReason != EEXIST )
            {
                printf( "Error. Failing to create path '%s'. Do you have access rights?",
                        mWriteAccessFolder.c_str() );
                mWriteAccessFolder = "";
            }
#endif
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        CrystalGuiGameState *gfxGameState = new CrystalGuiGameState(
        "Empty Project Example" );

        GraphicsSystem *graphicsSystem = new CrystalGuiGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Empty Project Sample";
    }
}
