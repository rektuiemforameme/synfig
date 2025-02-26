/* === S Y N F I G ========================================================= */
/*!	\file dialog_tooloptions.cpp
**	\brief Template File
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2009 Nikita Kitaev
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "docks/dialog_tooloptions.h"
#include <gui/localization.h>

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Dialog_ToolOptions::Dialog_ToolOptions():
	Dockable("tool_options",_("Tool Options"),Gtk::StockID("synfig-about")),
	empty_label(_("This tool has no options"))
{
	add(sub_vbox_);

	sub_vbox_.set_margin_end(10);
	sub_vbox_.set_margin_bottom(10);
	
	set_widget(empty_label);
	empty_label.show();
}

Dialog_ToolOptions::~Dialog_ToolOptions()
{
}

void
Dialog_ToolOptions::clear()
{
	Dockable::clear();
	set_local_name(_("Tool Options"));
	add(sub_vbox_);
	set_widget(empty_label);
	sub_vbox_.set_valign(Gtk::Align::ALIGN_CENTER);
	empty_label.show();

	set_stock_id(Gtk::StockID("synfig-about"));
}

void
Dialog_ToolOptions::set_widget(Gtk::Widget&x)
{
	std::vector<Gtk::Widget*> children = sub_vbox_.get_children();
	for(std::vector<Gtk::Widget*>::iterator i = children.begin(); i != children.end(); ++i)
		sub_vbox_.remove(**i);
	sub_vbox_.show();
	sub_vbox_.pack_start(x,false,false);
	sub_vbox_.set_valign(Gtk::Align::ALIGN_FILL);
	x.show();
}

void
Dialog_ToolOptions::set_name(const synfig::String& name)
{
	set_stock_id(Gtk::StockID("synfig-"+name));
}
