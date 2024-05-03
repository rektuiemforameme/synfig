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

#include <synfig/layers/layer_skeleton.h>
#include <synfig/layers/layer_skeletondeformation.h>
#include <synfig/valuenodes/valuenode_bone.h>
#include <synfig/valuenodes/valuenode_composite.h>
#include <synfig/valuenodes/valuenode_staticlist.h>
#include <synfigapp/canvasinterface.h>
#include <synfigapp/localization.h>
#include <iostream>
//#include <QDebug>

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
	if ((name == "value_desc" || name == "selected_value_desc") && param.get_type() == Param::TYPE_VALUEDESC
	 && param.get_value_desc().parent_is_value_node()
	 && ValueNode_Bone::Handle::cast_dynamic(param.get_value_desc().get_parent_desc().get_value_node()) )
	{
		selected_bone_descs.push_back(param.get_value_desc().get_parent_desc());
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
	if (!selected_bone_descs.size())
		return false;
	return Action::CanvasSpecific::is_ready();
}


void
Action::ValueDescBoneDuplicate::perform()
{
	//Map of bones that have been duplicated, with the original bone it was duped from as the key
	std::map<ValueNode_Bone::Handle,ValueNode_Bone::Handle> duplicate_bones;
	Layer::Handle layer = get_canvas_interface()->get_selection_manager()->get_selected_layer();
	Layer_Skeleton::Handle skel_layer = Layer_Skeleton::Handle::cast_dynamic(layer);
	Layer_SkeletonDeformation::Handle deform_layer = Layer_SkeletonDeformation::Handle::cast_dynamic(layer);
	ValueDesc list_desc(layer,"bones");
	// create a child bone
	ValueNode_StaticList::Handle list_node;
	list_node=ValueNode_StaticList::Handle::cast_dynamic(list_desc.get_value_node());


//	Sort bones from base of trees to leaves
//	Foreach
//		Duplicate Bone
//		Add Bone to map with original and duplicate
//		Find parent in map and reparent
	while(selected_bone_descs.size() > duplicate_bones.size())
	{
		for(auto it = selected_bone_descs.begin(); it!=selected_bone_descs.end(); ++it)
		{
			ValueDesc& current_bone_desc(*it);
			ValueNode_Bone::Handle current_bone_vn = ValueNode_Bone::Handle::cast_dynamic(current_bone_desc.get_value_node());
//			std::cout << (current_bone_desc.is_value_node()) << std::endl;
			if(current_bone_desc.is_value_node() && duplicate_bones.count(current_bone_vn) == 0)
			{
				ValueNode_Bone::Handle current_parent_bone_vn = ValueNode_Bone::Handle::cast_dynamic(current_bone_vn->get_link("parent")->operator()(time).get(ValueNode_Bone::Handle()));
				//ValueDesc current_parent_bone_desc = current_bone_desc.get_parent_desc();
				if(!list_contains_value_node(current_parent_bone_vn) || duplicate_bones.count(current_parent_bone_vn))	//If the bone's parent isn't in the list, it's ready to be duplicated
				{

////					ValueNode_Bone::Handle current_parent_bone_vn = ValueNode_Bone::Handle::cast_dynamic(current_parent_bone_desc.get_value_node());
//					Bone new_bone = Bone();
//					ValueNode_Bone::Handle new_parent_bone_vn;
//					if(duplicate_bones.count(current_parent_bone_vn) == 0){	//If the parent isn't in the dup map, then the parent should be the same as the duplicated bone's
//						new_parent_bone_vn = current_parent_bone_vn;
//					}else{													//If the parent is in the dup map, use its duplicaated counterpart
//						new_parent_bone_vn = duplicate_bones[current_parent_bone_vn];
//					}
//					Point new_bone_origin = current_bone_vn->get_link("origin")->operator()(time).get(Point());	//Flip the bone's pos across the y axis
//					new_bone_origin.multiply_coords(Point(-1,1));
//					new_bone.set_origin(new_bone_origin);
//					new_bone.set_scalelx(current_bone_vn->get_link("scalelx")->operator()(time).get(Real()));
//					new_bone.set_scalex(current_bone_vn->get_link("scalex")->operator()(time).get(Real()));
//					new_bone.set_width(current_bone_vn->get_link("width")->operator()(time).get(Real()));
//					new_bone.set_length(current_bone_vn->get_link("length")->operator()(time).get(Real()));
//					new_bone.set_tipwidth(current_bone_vn->get_link("tipwidth")->operator()(time).get(Real()));
//					new_bone.set_angle(current_bone_vn->get_link("angle")->operator()(time).get(Angle()));
//					new_bone.set_parent(new_parent_bone_vn.get());

//					ValueNode_Bone::Handle new_bone_node = ValueNode_Bone::create(new_bone,get_canvas());
//					new_bone_node->set_link("parent",ValueNode_Const::create(new_parent_bone_vn));

					if(active_bone && item_index >= 0 && !list_node->list.empty()){
						// if active bone is already set
						ValueNode_Bone::Handle bone_node = ValueNode_Bone::Handle::cast_dynamic(active_bone);
						if (deform_layer) {
							if (ValueNode_Composite::Handle comp = ValueNode_Composite::Handle::cast_dynamic(active_bone)) {
								value_desc = ValueDesc(comp,comp->get_link_index_from_name("first"),value_desc);
								bone_node = ValueNode_Bone::Handle::cast_dynamic(comp->get_link("first"));
							} else if ((comp = ValueNode_Composite::Handle::cast_dynamic(list_node->get_link(item_index)))) {
								value_desc = ValueDesc(comp, comp->get_link_index_from_name("first"),value_desc);
							} else {
								get_canvas_interface()->get_ui_interface()->error(_("Expected a ValueNode_Composite with a BonePair"));
								assert(0);
								return Smach::RESULT_ERROR;
							}
						}

						if (!bone_node)
						{
							error("expected a ValueNode_Bone");
							get_canvas_interface()->get_ui_interface()->error(_("Expected a ValueNode_Bone"));
							assert(0);
							return Smach::RESULT_ERROR;
						}
						ValueDesc v_d = ValueDesc(bone_node,bone_node->get_link_index_from_name("origin"),value_desc);
						Real sx = bone_node->get_link("scalelx")->operator()(get_canvas()->get_time()).get(Real());
						Matrix matrix = (*bone_node)(get_canvas()->get_time()).get(Bone()).get_animated_matrix();
						Real angle = atan2(matrix.axis(0)[1],matrix.axis(0)[0]);
						matrix = matrix.get_inverted();
						Point aOrigin = matrix.get_transformed(clickOrigin);
						aOrigin[0]/=sx;

						Action::Handle createChild(Action::Handle(Action::create("ValueDescCreateChildBone")));
						createChild->set_param("canvas",layer->get_canvas());
						createChild->set_param("canvas_interface",get_canvas_interface());
						createChild->set_param("highlight",true);

						createChild->set_param("value_desc",Action::Param(v_d));
						createChild->set_param("origin",Action::Param(ValueBase(aOrigin)));
						createChild->set_param("width",Action::Param(ValueBase(get_bone_width())));
						createChild->set_param("tipwidth",Action::Param(ValueBase(get_bone_width())));
						createChild->set_param("prev_active_bone_node",Action::Param(get_work_area()->get_active_bone_value_node()));
						if((clickOrigin-releaseOrigin).mag()>=0.01) {
							Real a = atan2((releaseOrigin-clickOrigin)[1],(releaseOrigin-clickOrigin)[0]);
							createChild->set_param("angle",Action::Param(ValueBase(Angle::rad(a-angle))));
							createChild->set_param("scalelx", Action::Param(ValueBase((releaseOrigin - clickOrigin).mag())));
						}else{
							Real a = default_angle;
							createChild->set_param("angle",Action::Param(ValueBase(Angle::rad(a-angle))));
							createChild->set_param("scalelx", Action::Param(ValueBase(1.0)));

						}

						if(createChild->is_ready()){
							try{
								get_canvas_interface()->get_instance()->perform_action(createChild);
							} catch (...) {
								info("Error performing action");
							}
						}


					duplicate_bones[current_bone_vn] = new_bone_node;
					std::cout << "Bone Duped" << std::endl;
					//selected_bone_descs.erase(it);		//Erase the bone from the list so that its children can go next
				}
			}else{
				std::cout << "Bummer dude" << std::endl;
			}
		}
	}
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

bool
Action::ValueDescBoneDuplicate::list_contains(ValueDesc vd) {
	for(auto it=selected_bone_descs.begin();it!=selected_bone_descs.end();++it)
	{
		const ValueDesc& current_value_desc(*it);
		if(vd == current_value_desc)
			return true;
	}
	return false;
}

bool
Action::ValueDescBoneDuplicate::list_contains_value_node(ValueNode_Bone::Handle vn) {
	for(auto it=selected_bone_descs.begin();it!=selected_bone_descs.end();++it)
	{
		const ValueDesc& current_value_desc(*it);

		if(vn == ValueNode_Bone::Handle::cast_dynamic(current_value_desc.get_value_node()))
			return true;
	}
	return false;
}

void
Action::ValueDescBoneDuplicate::set_active_bone(ValueNode::Handle bone_value_node){
	if (active_bone == bone_value_node)
		return;
	Action::Handle setActiveBone(Action::Handle(Action::create("ValueNodeSetActiveBone")));
	setActiveBone->set_param("canvas", get_canvas());
	setActiveBone->set_param("canvas_interface", get_canvas_interface());
	setActiveBone->set_param("prev_active_bone_node", get_work_area()->get_active_bone_value_node());
	setActiveBone->set_param("active_bone_node", bone_value_node);

	if(setActiveBone->is_ready()){
		try{
			get_canvas_interface()->get_instance()->perform_action(setActiveBone);
		} catch (...) {
			get_canvas_interface()->get_ui_interface()->error(_("Error setting the new active bone"));
		}
	}
}
