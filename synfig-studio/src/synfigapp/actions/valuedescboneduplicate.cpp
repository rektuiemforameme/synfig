/* === S Y N F I G ========================================================= */
/*!	\file synfigapp/actions/valuedescboneduplicate.cpp
**	\brief Template File
**
**	\legal
**  Copyright (c) 2020 Aditya Abhiram J
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

#include "valuedescboneduplicate.h"

#include <synfig/valuenodes/valuenode_bone.h>
#include <synfig/valuenodes/valuenode_composite.h>
#include <synfigapp/canvasinterface.h>
#include <synfigapp/localization.h>

#endif

using namespace synfig;
using namespace synfigapp;
using namespace Action;

/* === M A C R O S ========================================================= */

ACTION_INIT(Action::ValueDescBoneDuplicate);
ACTION_SET_NAME(Action::ValueDescBoneDuplicate,"ValueDescBoneDuplicate");
ACTION_SET_LOCAL_NAME(Action::ValueDescBoneDuplicate,N_("Duplicate Bone(s)"));
ACTION_SET_TASK(Action::ValueDescBoneDuplicate,"duplicate_bone");
ACTION_SET_CATEGORY(Action::ValueDescBoneDuplicate,Action::CATEGORY_VALUEDESC);
ACTION_SET_PRIORITY(Action::ValueDescBoneDuplicate,0);
ACTION_SET_VERSION(Action::ValueDescBoneDuplicate,"0.0");

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Action::ValueDescBoneDuplicate::ValueDescBoneDuplicate()
{
}

Action::ParamVocab
Action::ValueDescBoneDuplicate::get_param_vocab()
{
	ParamVocab ret(Action::CanvasSpecific::get_param_vocab());

	ret.push_back(ParamDesc("value_desc",Param::TYPE_VALUEDESC)
		.set_local_name(_("ValueDesc of new parent Bone"))
	);
	ret.push_back(ParamDesc("time",Param::TYPE_TIME)
		.set_local_name(_("Time"))
		.set_optional()
	);
	ret.push_back(ParamDesc("child",Param::TYPE_VALUENODE)
		.set_local_name(_("ValueNode of Bone to be reparented"))
	);

	return ret;
}

bool
Action::ValueDescBoneDuplicate::is_candidate(const ParamList &x)
{
	ParamList::const_iterator i;

	i = x.find("value_desc");
	if (i == x.end()) return false;

	ValueDesc value_desc(i->second.get_value_desc());
	i=x.find("child");
	if(i==x.end()) return false;

	ValueNode::Handle child(i->second.get_value_node());
	if (!candidate_check(get_param_vocab(),x))
		return false;

	return value_desc.parent_is_value_node()
		&& ValueNode_Bone::Handle::cast_dynamic(value_desc.get_parent_value_node())
		&& child
		&& ValueNode_Bone::Handle::cast_dynamic(child);
}

bool
Action::ValueDescBoneDuplicate::set_param(const synfig::String& name, const Action::Param &param)
{
	ValueNode_DynamicList::Handle value_node;
	if(name=="value_desc" && param.get_type()==Param::TYPE_VALUEDESC)
	{
		ValueDesc value_desc(param.get_value_desc());
		if(!value_desc.parent_is_value_node())
			return false;
		value_node=ValueNode_DynamicList::Handle::cast_dynamic(value_desc.get_parent_value_node());
		if(!value_node)
		{
			ValueNode::Handle compo(ValueNode_Composite::Handle::cast_dynamic(value_desc.get_parent_value_node()));
			if(compo)
			{
				ValueNode_DynamicList::Handle parent_list=NULL;
				std::set<Node*>::iterator iter;
				// now check if the composite's parent is a dynamic list type
				for(iter=compo->parent_set.begin();iter!=compo->parent_set.end();++iter)
					{
						parent_list=ValueNode_DynamicList::Handle::cast_dynamic(*iter);
						if(parent_list)
						{
							value_node=parent_list;
							// Now we need to find the index of this composite item
							// on the dynamic list
							int i;
							for(i=0;i<value_node->link_count();i++)
								if(compo->get_guid()==value_node->get_link(i)->get_guid())
									break;
							if(i<value_node->link_count())
								value_desc=synfigapp::ValueDesc(value_node, i);
							else
								return false;
							break;
						}
					}
				if(!value_node)
					return false;
			}
			else
				return false;
			if(!value_node)
				return false;
		}
		ValueNodes::iterator it;
		// Try to find the current parent value node in our map
		it=value_nodes.find(value_node);
		if(it==value_nodes.end())
		{
			// Not found, then create a new one
			value_nodes[value_node].push_back(value_desc.get_index());
		}
		else
		{
			// found, then insert the index of the value desc.
			// Maybe the index is already inserted.
			// Later is ignored if that happen
			it->second.push_back(value_desc.get_index());
		}
		return true;
	}
	if(name=="time" && param.get_type()==Param::TYPE_TIME)
	{
		time=param.get_time();
		return true;
	}
	return Action::CanvasSpecific::set_param(name,param);
}

bool
Action::ValueDescBoneDuplicate::is_ready()const
{
	if (!value_nodes.size())
		return false;
	return Action::CanvasSpecific::is_ready();
}

void
Action::ValueDescBoneDuplicate::prepare()
{
	ValueNodes::iterator it;
	std::map<ValueDesc,ValueDesc> duplicate_bones;
//	Sort bones from base of trees to leaves
//	Foreach
//		Duplicate Bone
//		Add Bone to map with original and duplicate
//		Find parent in map and reparent
	int prev_index=-1;
	std::list<int>::iterator i;
	std::list<int> l(it->second.begin(), it->second.end());
	synfig::ValueNode_DynamicList::Handle value_node(it->first);
	while(value_nodes.size() > 0)
	{
		for(i=l.begin();i!=l.end();++i)
		{
			int index(*i);
			// This prevents duplicated index
			if(index==prev_index)
				continue;
			prev_index=index;
			ValueDesc bone_value_desc = ValueDesc(value_node,index);
			if(value_nodes.count(bone_value_desc.get_parent_desc().get_value_node()) == 0)	//If the bone's parent isn't in the list, it's ready to be duplicated
			{
				Bone bone = Bone();
				Bone parent_bone;
				if(duplicate_bones.count(parent_bone) == 0){
					parent_bone = ValueNode_Bone::Handle::cast_dynamic(bone_value_desc.get_parent_desc().get_value_node()).;
				}else{

				}
				bone.set_origin(bone_value_desc. .get(Point()));
				bone.set_scalelx(scalelx.get(Real()));
				bone.set_width(width.get(Real()));
				bone.set_tipwidth(tipwidth.get(Real()));
				bone.set_angle(angle.get(Angle()));

				ValueNode_Bone::Handle bone_node = ValueNode_Bone::create(bone,get_canvas());

			}

//				Bone bone = Bone();
//				if(c_parent){
//					bone.set_parent(ValueNode_Bone_Root::create(Bone()));
//				}else{
//					bone.set_parent(ValueNode_Bone::Handle::cast_dynamic(ValueNode::Handle(value_node->list[index])).get());
//				}
//				bone.set_origin(origin.get(Point()));
//				bone.set_scalelx(scalelx.get(Real()));
//				bone.set_width(width.get(Real()));
//				bone.set_tipwidth(tipwidth.get(Real()));
//				bone.set_angle(angle.get(Angle()));

//				ValueNode_Bone::Handle bone_node = ValueNode_Bone::create(bone,get_canvas());
		}
	}
	for(it=value_nodes.begin();it!=value_nodes.end();++it)
	{
		synfig::ValueNode_DynamicList::Handle value_node(it->first);
		std::list<int> l(it->second.begin(), it->second.end());
		std::list<int>::iterator i;
		// sort the indexes to perform the actions from higher to lower index
		l.sort();
		int prev_index=-1;
		for(i=l.begin();i!=l.end();++i)
		{
			int index(*i);
			// This prevents duplicated index
			if(index==prev_index)
				continue;
			prev_index=index;
			ValueDesc value_desc = ValueDesc(value_node,index);
//			Action::Handle action;
//			// If we are in animate editing mode
//			if(get_edit_mode()&MODE_ANIMATE)
//				action=Action::create("ActivepointSetOff");
//			else
//				action=Action::create("ValueNodeDynamicListRemove");
//			if(!action)
//				throw Error(_("Unable to find action (bug)"));
//			action->set_param("canvas",get_canvas());
//			action->set_param("canvas_interface",get_canvas_interface());
//			action->set_param("time",time);
//			action->set_param("origin",origin);
//			action->set_param("value_desc",ValueDesc(value_node,index));
//			if(!action->is_ready())
//				throw Error(Error::TYPE_NOTREADY);
//			add_action_front(action);
		}
	}
}


void
Action::ValueDescBoneDuplicate::perform()
{
//	if (!value_desc.parent_is_value_node()
//	 || !value_desc.is_parent_desc_declared()
//	 || !value_desc.get_parent_desc().is_value_node()
//	 || !child)
//			throw Error(Error::TYPE_NOTREADY);

//	ValueNode_Bone::Handle child_bone;
//	if((child_bone = ValueNode_Bone::Handle::cast_dynamic(child))){
//		if(ValueNode_Bone::Handle::cast_dynamic(value_desc.get_parent_value_node())){
//			ValueDesc new_parent_bone_desc = value_desc.get_parent_desc();

//			if(child_bone->set_link("parent",ValueNode_Const::create(ValueNode_Bone::Handle::cast_dynamic(new_parent_bone_desc.get_value_node())))){
//				Matrix new_parent_matrix = new_parent_bone_desc.get_value(time).get(Bone()).get_animated_matrix();
//				Angle new_parent_angle = Angle::rad(atan2(new_parent_matrix.axis(0)[1],new_parent_matrix.axis(0)[0]));
//				Real new_parent_scale = new_parent_bone_desc.get_value(time).get(Bone()).get_scalelx();
//				new_parent_matrix = new_parent_matrix.get_inverted();

//				ValueNode_Bone::Handle old_parent_bone = ValueNode_Const::Handle::cast_dynamic(prev_parent)->get_value().get(ValueNode_Bone::Handle());
//				Matrix old_parent_matrix = old_parent_bone->operator()(time).get(Bone()).get_animated_matrix();
//				Angle old_parent_angle = Angle::rad(atan2(old_parent_matrix.axis(0)[1],old_parent_matrix.axis(0)[0]));
//				Real old_parent_scale = old_parent_bone->get_link("scalelx")->operator()(time).get(Real());

//				Point origin = child_bone->get_link("origin")->operator()(time).get(Point());
//				Angle angle = child_bone->get_link("angle")->operator()(time).get(Angle());

//				angle+=old_parent_angle;
//				origin[0] *= old_parent_scale;
//				origin = old_parent_matrix.get_transformed(origin);
//				origin = new_parent_matrix.get_transformed(origin);
//				origin[0]/= new_parent_scale;
//				angle-=new_parent_angle;
//				child_bone->set_link("origin",ValueNode_Const::create(origin));
//				child_bone->set_link("angle",ValueNode_Const::create(angle));
//			}else{
//				get_canvas_interface()->get_ui_interface()->error(_("Can't make it the parent to the current active bone"));
//			}

//		}
//	}else{
//		get_canvas_interface()->get_ui_interface()->error(_("Please set an active bone"));
//	}

}

void
Action::ValueDescBoneDuplicate::undo() {
//	if(prev_parent){
//		ValueNode_Bone::Handle child_bone = ValueNode_Bone::Handle::cast_dynamic(child);
//		ValueDesc new_parent_bone_desc = value_desc.get_parent_desc();
//		Matrix new_parent_matrix = new_parent_bone_desc.get_value(time).get(Bone()).get_animated_matrix();
//		Angle new_parent_angle = Angle::rad(atan2(new_parent_matrix.axis(0)[1],new_parent_matrix.axis(0)[0]));
//		Real new_parent_scale = new_parent_bone_desc.get_value(time).get(Bone()).get_scalelx();

//		ValueNode_Bone::Handle old_parent_bone = ValueNode_Const::Handle::cast_dynamic(prev_parent)->get_value().get(ValueNode_Bone::Handle());
//		Matrix old_parent_matrix = old_parent_bone->operator()(time).get(Bone()).get_animated_matrix();
//		Angle old_parent_angle = Angle::rad(atan2(old_parent_matrix.axis(0)[1],old_parent_matrix.axis(0)[0]));
//		Real old_parent_scale = old_parent_bone->get_link("scalelx")->operator()(time).get(Real());
//		old_parent_matrix = old_parent_matrix.get_inverted();

//		Point origin = child_bone->get_link("origin")->operator()(time).get(Point());
//		Angle angle = child_bone->get_link("angle")->operator()(time).get(Angle());


//		angle+=new_parent_angle;
//		origin[0] *= new_parent_scale;
//		origin = new_parent_matrix.get_transformed(origin);
//		origin = old_parent_matrix.get_transformed(origin);
//		origin[0]/=old_parent_scale;
//		angle-=old_parent_angle;
//		if(child_bone->set_link("parent",ValueNode_Const::create(old_parent_bone))){
//			child_bone->set_link("origin",ValueNode_Const::create(origin));
//			child_bone->set_link("angle",ValueNode_Const::create(angle));
//		}
//	}else{
//		get_canvas_interface()->get_ui_interface()->error(_("Couldn't find parent to active bone"));
//	}
}
