
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

#include "CrystalRenderable.inl"

#define TODO_should_flag_transforms_dirty

namespace Crystal
{
	Window::Window( CrystalManager *manager ) :
		Renderable( manager ),
		m_currentScroll( Ogre::Vector2::ZERO ),
		m_nextScroll( Ogre::Vector2::ZERO ),
		m_maxScroll( Ogre::Vector2::ZERO ),
		m_allowsFocusWithChildren( true ),
		m_defaultChildWidget( 0 ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false )
	{
	}
	//-------------------------------------------------------------------------
	Window::~Window()
	{
		CRYSTAL_ASSERT( m_childWindows.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	Window* Window::getParentAsWindow() const
	{
		CRYSTAL_ASSERT( dynamic_cast<Window*>( m_parent ) );
		Window *parentWindow = static_cast<Window*>( m_parent );
		return parentWindow;
	}
	//-------------------------------------------------------------------------
	void Window::_destroy()
	{
		m_destructionStarted = true;
		setWindowNavigationDirty();

		if( m_parent )
		{
			//Remove ourselves from being our Window parent's child
			Window *parentWindow = getParentAsWindow();
			WindowVec::iterator itor = std::find( parentWindow->m_childWindows.begin(),
												  parentWindow->m_childWindows.end(),
												  this );
			parentWindow->m_childWindows.erase( itor );
		}

		{
			WindowVec::const_iterator itor = m_childWindows.begin();
			WindowVec::const_iterator end  = m_childWindows.end();

			while( itor != end )
				m_manager->destroyWindow( *itor++ );

			m_childWindows.clear();
		}

		Renderable::_destroy();
	}
	//-------------------------------------------------------------------------
	void Window::setScrollAnimated( const Ogre::Vector2 &nextScroll, bool animateOutOfRange )
	{
		m_nextScroll = nextScroll;
		if( !animateOutOfRange )
		{
			m_nextScroll.makeFloor( m_maxScroll );
			m_nextScroll.makeCeil( Ogre::Vector2::ZERO );
		}
	}
	//-------------------------------------------------------------------------
	void Window::setScrollImmediate( const Ogre::Vector2 &scroll )
	{
		m_currentScroll = scroll;
		m_currentScroll.makeFloor( m_maxScroll );
		m_currentScroll.makeCeil( Ogre::Vector2::ZERO );
		m_nextScroll = m_currentScroll;
	}
	//-------------------------------------------------------------------------
	void Window::setMaxScroll( const Ogre::Vector2 &maxScroll )
	{
		CRYSTAL_ASSERT_LOW( maxScroll.x >= 0 && maxScroll.y >= 0 );
		m_maxScroll = maxScroll;
	}
	//-------------------------------------------------------------------------
	void Window::calculateMaxScrollFromScrollableArea( const Ogre::Vector2 &scrollableArea )
	{
		Ogre::Vector2 maxScroll = scrollableArea - m_size + m_clipBorderTL + m_clipBorderBR;
		maxScroll.makeCeil( Ogre::Vector2::ZERO );
		setMaxScroll( maxScroll );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Window::getScrollableArea() const
	{
		return m_maxScroll + m_size - m_clipBorderTL - m_clipBorderBR;
	}
	//-------------------------------------------------------------------------
	void Window::sizeScrollToFit()
	{
		Ogre::Vector2 scrollableArea( Ogre::Vector2::ZERO );

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			Widget *widget = *itor;
			scrollableArea.makeCeil( widget->getLocalTopLeft() + widget->getSize() );
			++itor;
		}

		calculateMaxScrollFromScrollableArea( scrollableArea );
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Window::getCurrentScroll() const
	{
		return m_currentScroll;
	}
	//-------------------------------------------------------------------------
	void Window::update( float timeSinceLast )
	{
		TODO_should_flag_transforms_dirty; //??? should we?
		const Ogre::Vector2 pixelSize = m_manager->getPixelSize();

		if( m_nextScroll.y < 0.0f )
		{
			m_nextScroll.y = Ogre::Math::lerp( 0.0f, m_nextScroll.y,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.y > m_maxScroll.y )
		{
			m_nextScroll.y = Ogre::Math::lerp( m_maxScroll.y, m_nextScroll.y,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x < 0.0f )
		{
			m_nextScroll.x = Ogre::Math::lerp( 0.0f, m_nextScroll.x,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x > m_maxScroll.x )
		{
			m_nextScroll.x = Ogre::Math::lerp( m_maxScroll.x, m_nextScroll.x,
											   exp2f( -15.0f * timeSinceLast ) );
		}

		if( fabs( m_currentScroll.x - m_nextScroll.x ) >= pixelSize.x ||
			fabs( m_currentScroll.y - m_nextScroll.y ) >= pixelSize.y )
		{
			m_currentScroll = Ogre::Math::lerp( m_nextScroll, m_currentScroll,
												exp2f( -15.0f * timeSinceLast ) );
		}
		else
		{
			m_currentScroll = m_nextScroll;
		}

		WindowVec::const_iterator itor = m_childWindows.begin();
		WindowVec::const_iterator end  = m_childWindows.end();

		while( itor != end )
		{
			(*itor)->update( timeSinceLast );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	size_t Window::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		const size_t idx = Widget::notifyParentChildIsDestroyed( childWidgetBeingRemoved );

		if( m_defaultChildWidget >= idx )
			--m_defaultChildWidget;

		return idx;
	}
	//-------------------------------------------------------------------------
	void Window::notifyChildWindowIsDirty()
	{
		m_childrenNavigationDirty = true;

		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->notifyChildWindowIsDirty();
		}
		else
		{
			m_manager->_setWindowNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::setWidgetNavigationDirty()
	{
		m_widgetNavigationDirty = true;

		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->notifyChildWindowIsDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::setWindowNavigationDirty()
	{
		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->m_windowNavigationDirty = true;
			parentWindow->notifyChildWindowIsDirty();
		}
		else
		{
			m_manager->_setWindowNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::attachChild( Window *window )
	{
		window->detachFromParent();
		m_childWindows.push_back( window );
		window->_setParent( this );
	}
	//-------------------------------------------------------------------------
	void Window::detachChild( Window *window )
	{
		WindowVec::iterator itor = std::find( m_childWindows.begin(), m_childWindows.end(), window );

		if( itor == m_childWindows.end() )
		{
			LogListener	*log = m_manager->getLogListener();
			log->log( "Window::removeChild could not find the window. It's not our child.",
					  LogSeverity::Fatal );
		}
		else
		{
			m_childWindows.erase( itor );
			window->m_parent = 0;

			WidgetVec::iterator itWidget = std::find( m_children.begin() + m_numWidgets,
													  m_children.end(), window );
			if( itWidget == m_children.end() )
			{
				LogListener	*log = m_manager->getLogListener();
				log->log( "Window::removeChild could not find the window in "
						  "m_children but it was in m_childWindows!",
						  LogSeverity::Fatal );
			}
			else
			{
				m_children.erase( itWidget );
			}

			m_manager->_setAsParentlessWindow( window );
		}
	}
	//-------------------------------------------------------------------------
	void Window::detachFromParent()
	{
		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->detachChild( this );
		}
	}
	//-------------------------------------------------------------------------
	void Window::setDefault( Widget *widget )
	{
		CRYSTAL_ASSERT( !widget->isWindow() );
		WidgetVec::const_iterator itor = std::find( m_children.begin(),
													m_children.end(), widget );
		m_defaultChildWidget = itor - m_children.begin();
		CRYSTAL_ASSERT( m_defaultChildWidget < m_numWidgets );
	}
	//-------------------------------------------------------------------------
	Widget* crystalgui_nullable Window::getDefaultWidget() const
	{
		Widget *retVal = 0;

		size_t numChildren = m_numWidgets;

		size_t defaultChild = m_defaultChildWidget;
		for( size_t i=0; i<numChildren && !retVal; ++i )
		{
			Widget *child = m_children[defaultChild];
			if( child->getCurrentState() != States::Disabled )
				retVal = child;

			defaultChild = (defaultChild + 1u) % numChildren;
		}

		if( !retVal && m_numWidgets > 0 )
			retVal = m_children.front();

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Window::fillBuffersAndCommands( UiVertex **vertexBuffer,
										 GlyphVertex **textVertBuffer,
										 const Ogre::Vector2 &parentPos,
										 const Ogre::Vector2 &parentCurrentScrollPos,
										 const Ogre::Matrix3 &parentRot )
	{
		Renderable::fillBuffersAndCommands( vertexBuffer, textVertBuffer, parentPos,
											parentCurrentScrollPos, parentRot, m_currentScroll, true );
	}
	//-------------------------------------------------------------------------
	FocusPair Window::setIdleCursorMoved( const Ogre::Vector2 &newPosInCanvas )
	{
		FocusPair retVal;

		//The first window that our button is touching wins. We go in LIFO order.
		WindowVec::const_reverse_iterator ritor = m_childWindows.rbegin();
		WindowVec::const_reverse_iterator rend  = m_childWindows.rend();

		while( ritor != rend && !retVal.widget )
		{
			retVal = (*ritor)->setIdleCursorMoved( newPosInCanvas );
			++ritor;
		}

		//One of the child windows is being touched by the cursor. We're done.
		if( retVal.widget )
			return retVal;

		if( !this->intersects( newPosInCanvas ) || !m_allowsFocusWithChildren )
			return FocusPair();

		WidgetVec::const_iterator itor = m_children.begin() + m_numNonRenderables;
		WidgetVec::const_iterator end  = m_children.begin() + m_numWidgets;

		while( itor != end && !retVal.widget )
		{
			Widget *widget = *itor;
			if( widget->isNavigable() &&
				this->intersectsChild( widget, m_currentScroll ) &&
				widget->intersects( newPosInCanvas ) )
			{
				retVal.widget = widget;
			}
			++itor;
		}

		retVal.window = this;

		return retVal;
	}
}
