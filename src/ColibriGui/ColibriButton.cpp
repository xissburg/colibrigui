
#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriLabel.h"

namespace Colibri
{
	Button::Button( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 )
	{
		m_clickable = true;
		m_keyboardNavigable = true;
	}
	//-------------------------------------------------------------------------
	void Button::_initialize()
	{
		_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Button ) );
		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Button::_destroy()
	{
		Renderable::_destroy();

		//m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	Label* Button::getLabel()
	{
		if( !m_label )
		{
			m_label = m_manager->createWidget<Label>( this );
			m_label->setSize( getSizeAfterClipping() );
			m_label->setTextHorizAlignment( TextHorizAlignment::Center );
			m_label->setTextVertAlignment( TextVertAlignment::Center );
		}

		return m_label;
	}
	//-------------------------------------------------------------------------
	void Button::setTransformDirty()
	{
		if( m_label )
		{
			if( m_label->getSize() != m_size )
				m_label->setSize( getSizeAfterClipping() );
		}
		Renderable::setTransformDirty();
	}
}