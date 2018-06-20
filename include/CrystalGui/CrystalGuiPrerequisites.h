/// @file CrystalGuiPrerequisites.h

#pragma once

#if __clang__
	#define crystalgui_nullable _Nullable
	#define crystalgui_nonnull _Nonnull
	#define CRYSTALGUI_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
	#define CRYSTALGUI_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")
	#define crystalgui_likely(x)	__builtin_expect((x),1)
	#define crystalgui_unlikely(x)	__builtin_expect((x),0)
#else
	#define crystalgui_nullable
	#define crystalgui_nonnull
	#define CRYSTALGUI_ASSUME_NONNULL_BEGIN
	#define CRYSTALGUI_ASSUME_NONNULL_END
	#if __GNUC__
		#define crystalgui_likely(x)	__builtin_expect((x),1)
		#define crystalgui_unlikely(x)	__builtin_expect((x),0)
	#else
		#define crystalgui_likely(x)	(x)
		#define crystalgui_unlikely(x)	(x)
	#endif
#endif

#include <stdint.h>
#include "math_round.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

/// @defgroup Controls
/// @defgroup Api_Backend
namespace Crystal
{
	class Button;
	struct CachedGlyph;
	class Checkbox;
	class CrystalManager;
	class Editbox;
	class Label;
	class LogListener;
	class Renderable;
	struct ShapedGlyph;
	class Shaper;
	class ShaperManager;
	struct SkinInfo;
	class SkinManager;
	class Widget;
	class Window;

	namespace Action
	{
		enum Action
		{
			/// Any previous action on this widget was cancelled
			Cancel,
			/// Widget is highlighted by user (e.g. cursor is on top of a button)
			Highlighted,
			/// User is holding enter/main button, but hasn't released it yet
			Hold,
			/// User released the main button and the main action should be performed
			PrimaryActionPerform,
			/// User released the secondary button and that action should be performed
			SecondaryActionPerform,
			/// The value the Widget was holding has changed. For example, Spinners
			ValueChanged
		};
	}
	namespace ActionMask
	{
		enum ActionMask
		{
			Cancel					= 1u << Action::Cancel,
			Highlighted				= 1u << Action::Highlighted,
			Hold					= 1u << Action::Hold,
			PrimaryActionPerform	= 1u << Action::PrimaryActionPerform,
			SecondaryActionPerform	= 1u << Action::SecondaryActionPerform,
			ValueChanged			= 1u << Action::ValueChanged
		};
	}

	namespace States
	{
		enum States
		{
			Disabled,
			Idle,
			HighlightedCursor,
			HighlightedButton,
			HighlightedButtonAndCursor,
			Pressed,
			NumStates
		};
	}
	namespace Borders
	{
		enum Borders
		{
			Top,
			Left,
			Right,
			Bottom,
			NumBorders
		};
	}
	namespace GridLocations
	{
		enum GridLocations
		{
			TopLeft,
			Top,
			TopRight,
			CenterLeft,
			Center,
			CenterRight,
			BottomLeft,
			Bottom,
			BottomRight,
			NumGridLocations
		};
	}
	/*namespace Corners
	{
		enum NumCorners
		{
			TopLeft,
			BottomLeft,
			BottomRight,
			TopRight,
			NumCorners
		};
	}*/

	struct FocusPair
	{
		Window * crystalgui_nullable window;
		Widget * crystalgui_nullable widget;

		FocusPair() : window( 0 ), widget( 0 ) {}
	};

	namespace HorizReadingDir
	{
		enum HorizReadingDir
		{
			/// Use same setting as ShaperManager
			Default,
			/// The base direction depends on the first strong directional character in the
			/// text according to the Unicode Bidirectional Algorithm. If no strong directional
			/// character is present, then set the paragraph level to 1 (LTR or RTL respectively)
			///
			/// For example the text (which 1st strong directional char. is LTR from the h in hello):
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	hello CIBARA person ISRAF
			///
			/// While the text (which 1st strong directional char. is RTL from A in ARABIC):
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	person ISRAF hello CIBARA
			AutoLTR,
			AutoRTL,
			/// Text is strongly LTR
			/// The text:
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	hello CIBARA person ISRAF
			///
			/// While the text
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	CIBARA hello ISRAF person
			LTR,
			/// Text is strongly RTL
			/// The text:
			///		hello ARABIC person FARSI
			/// Will be displayed as:
			/// 	ISRAF person CIBARA hello
			///
			/// While the text
			///		ARABIC hello FARSI person
			/// Will be displayed as:
			/// 	person ISRAF hello CIBARA
			RTL
		};
	}
	namespace TextHorizAlignment
	{
		enum TextHorizAlignment
		{
			/// When default langauge is LTR, it will display text aligned Left unless the string
			/// is fully RTL.
			/// If the string is mixed, text is aligned to the Left but the reading directions
			/// will be respected for RTL words according to HorizReadingDir.
			/// CJK languages are Left unless overriden by VertReadingDir
			///
			/// When default langauge is RTL, it will display text aligned Right unless the string
			/// is fully LTR.
			/// If the string is mixed, text is aligned to the Right but the reading directions
			/// will be respected for LTR words according to HorizReadingDir.
			Natural,
			Mixed = Natural,
			/// Text always starts from the left. Can be overriden by VertReadingDir
			Left,
			/// Text is always centered
			Center,
			/// Text always starts from the right. Can be overriden by VertReadingDir
			Right
		};
	}
	namespace VertReadingDir
	{
		enum VertReadingDir
		{
			/// Always obey HorizReadingDir & TextHorizAlignment (Default)
			Disabled,
			/// When default language is not CJK, same as Disabled
			/// When default language is CJK, same aligns top to bottom, newlines right to left
			IfNeededTTB,
			/// Aligns top to bottom, newlines right to left, regardless of language
			ForceTTB,
			/// Aligns top to bottom, newlines left to right, regardless of language
			ForceTTBLTR,
			/// Not used
			//IfNeededBTT,
			/// Not used
			//ForceBTT,
		};
	}
	namespace TextVertAlignment
	{
		enum TextVertAlignment
		{
			/// Always equal to Top. It's there just in case there's ever a bottom to top language
			/// Also useful for "use the default" in Label::sizeToFit
			Natural,
			/// Note: Anything other than 'Top' is SLIGHTLY slower for text rendering, as we need
			/// to first calculate the height of the entire text before iterating through
			/// each character to upload position data to GPU.
			Top,
			Center,
			Bottom
		};
	}

	namespace HorizWidgetDir
	{
		enum HorizWidgetDir
		{
			/// Use LTR unless CrystalManager::swapRTLControls returns true
			AutoLTR,
			/// Use RTL unless CrystalManager::swapRTLControls returns true
			AutoRTL,
			/// Always use LTR
			LTR,
			/// Always use RTL
			RTL
		};
	}

	struct RichText
	{
		uint32_t ptSize;
		uint32_t offset;
		uint32_t length;
		HorizReadingDir::HorizReadingDir readingDir;
		uint32_t rgba32;			//Not part of operator ==
		uint32_t backgroundRgba32;	//Not part of operator ==
		uint16_t font;
		bool noBackground;		//Not part of operator ==

		uint32_t glyphStart;	//Not part of operator ==
		uint32_t glyphEnd;		//Not part of operator ==

		bool operator == ( const RichText &other ) const;
	};

	namespace LinebreakMode
	{
		enum LinebreakMode
		{
			/// Words will be broken into the next newline.
			/// Our word-wrap algorithm is quite simple: spaces, tabs, commas and
			/// similar characters are used to identify separation of words. This works
			/// well for popular languages such as western ones (English, Spanish, German, etc)
			/// and some eastern scripts.
			/// However it does not properly perform word breaking for complex languages
			/// such as Thai, Chinese or Japanese; as these would require us to include very
			/// big unicode dictionaries, which goes against CrystalGui's philosophy of
			/// being small.
			WordWrap,
			/// Text outside bounds will disapear
			Clip
		};
	}

	namespace SkinWidgetTypes
	{
		/// KEEP THIS IN SYNC WITH SkinManager::loadDefaultSkinPacks !!!
		enum SkinWidgetTypes
		{
			Window,
			Button,
			Spinner,
			SpinnerBtnDecrement,
			SpinnerBtnIncrement,
			Checkbox,
			CheckboxTickmarkUnchecked,
			CheckboxTickmarkChecked,
			CheckboxTickmarkThirdState,
			Editbox,
			NumSkinWidgetTypes
		};
	}
}

namespace Ogre
{
	class CrystalOgreRenderable;
}

CRYSTALGUI_ASSUME_NONNULL_END

// No checks done at all
#define CRYSTALGUI_DEBUG_NONE		0
// We perform basic assert checks and other non-performance intensive checks
#define CRYSTALGUI_DEBUG_LOW		1
// We alter classes to add some debug variables for a bit more thorough checking
// and also perform checks that may cause some performance hit
#define CRYSTALGUI_DEBUG_MEDIUM		2
// We perform intensive validation without concerns for performance
#define CRYSTALGUI_DEBUG_HIGH		3

#define CRYSTALGUI_DEBUG CRYSTALGUI_DEBUG_HIGH

#include "CrystalGui/CrystalAssert.h"

#define CRYSTAL_ASSERT_LOW CRYSTAL_ASSERT

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
	#define CRYSTAL_ASSERT_MEDIUM CRYSTAL_ASSERT
#else
	#define CRYSTAL_ASSERT_MEDIUM do { CRYSTAL_UNUSED(condition); } while(0)
#endif

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_HIGH
	#define CRYSTAL_ASSERT_HIGH CRYSTAL_ASSERT
#else
	#define CRYSTAL_ASSERT_HIGH do { CRYSTAL_UNUSED(condition); } while(0)
#endif
