/* === S Y N F I G ========================================================= */
/*!	\file synfig/rendering/task.h
**	\brief Task Header
**
**	$Id$
**
**	\legal
**	......... ... 2015-2018 Ivan Mahonin
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_RENDERING_TASK_H
#define __SYNFIG_RENDERING_TASK_H

/* === H E A D E R S ======================================================= */

#include <vector>
#include <set>
#include <map>

#include <synfig/rect.h>
#include <synfig/vector.h>

#include "surface.h"

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig
{
namespace rendering
{

class Renderer;


// Helpers


template<typename T>
class Holder
{
public:
	typedef T Type;
	typedef etl::handle<Type> Handle;

private:
	Handle value;

public:
	Type& get()
		{ return *value; }
	const Type& get() const
		{ return *value; }
	const Handle& handle() const
		{ return value; }

	Holder():
		value(new Type()) { }
	Holder(const Holder &other):
		value(new Type(other.get())) { }
	Holder& operator=(const Holder &other)
		{ *value = other.get(); return *this; }
	Type& operator*()
		{ return get(); }
	const Type& operator*() const
		{ return get(); }
	Type* operator->()
		{ return &get(); }
	const Type* operator->() const
		{ return &get(); }
};


// Mode


class Mode {
public:
	static synfig::Token mode_token;
	virtual ~Mode() { }
	virtual Surface::Token::Handle get_mode_target_token() const
		{ return Surface::Token::Handle(); }
	virtual bool get_mode_allow_multithreading() const
		{ return true; }
	virtual bool get_mode_allow_source_as_target() const
		{ return false; }
	virtual bool get_mode_allow_simultaneous_write() const //!< allow simultaneous write to the same target
		{ return false; }
};


class ModeToken: public synfig::Token
{
public:
	typedef ConstRef<ModeToken> Handle;
	const String name;
	ModeToken(const String &name):
		synfig::Token(Mode::mode_token.handle()),
		name(name) { }
	inline Handle handle() const
		{ return Handle(*this); }
};

typedef std::vector<ModeToken::Handle> ModeList;

// Interfaces


class TaskInterfaceConstant
{
public:
	virtual bool is_constant()
		{ return true; }
	virtual ~TaskInterfaceConstant() { }
};


class TaskInterfaceSplit
{
public:
	virtual bool is_splittable()
		{ return true; }
	virtual ~TaskInterfaceSplit() { }
};


// Task


class Task: public etl::shared_object
{
public:
	typedef etl::handle<Task> Handle;
	typedef std::vector<Handle> List;
	typedef std::set<Handle> Set;

	typedef Task* (*Fabric)();
	typedef Task* (*CloneFabric)(const Task&);

	class Token;

	class DescBase {
	protected:
		template<typename Type>
		static Task* create_func()
			{ return new Type(); }

		template<typename Type, typename TypeAbstract = Type>
		static Task* convert_func(const Task &other)
		{
			const TypeAbstract *orig = dynamic_cast<const TypeAbstract*>(&other);
			if (!orig) return 0;
			Type *task = new Type();
			*(TypeAbstract*)task = *orig;
			return task;
		}

	public:
		const synfig::Token::Handle parent_token;
		const String name;
		const ModeToken::Handle mode;
		const ConstRef<Token> abstract_task;
		const Fabric create;
		const CloneFabric clone;
		const CloneFabric convert;

		DescBase(
			const synfig::Token::Handle &parent_token,
			const String &name,
			const ModeToken::Handle &mode,
			const ConstRef<Token> &abstract_task,
			const Fabric create,
			const CloneFabric clone,
			const CloneFabric convert
		):
			parent_token(parent_token),
			name(name),
			mode(mode),
			abstract_task(abstract_task),
			create(create),
			clone(clone),
			convert(convert)
		{ }
	};

	class Token: public synfig::Token, public DescBase
	{
	public:
		typedef ConstRef<Token> Handle;
		typedef std::map<ModeToken::Handle, Token::Handle> Map;

	private:
		Map alternatives_;

		inline Token& cast_const() const
			{ return *const_cast<Token*>(this); }

	protected:
		virtual void unprepare_vfunc();
		virtual void prepare_vfunc();

	public:
		Token(const DescBase &desc):
			synfig::Token(desc.parent_token),
			DescBase(desc)
		{ }

		const Map& alternatives() const
			{ return alternatives_; }
		inline bool is_abstract() const
			{ return !mode || !abstract_task || !convert; }
		inline Handle handle() const
			{ return Handle(*this); }
	};

	//! token for special task
	template<typename Type>
	class DescSpecial: public DescBase {
	public:
		explicit DescSpecial(const String &name):
			DescBase(
				token.handle(),
				name,
				ModeToken::Handle(),
				Token::Handle(),
				&DescBase::create_func<Type>,
				&DescBase::convert_func<Type>,
				0 ) { }
	};

	//! token for abstract task
	template<typename Type, typename TypeParent = Task>
	class DescAbstract: public DescBase {
	public:
		explicit DescAbstract(const String &name):
			DescBase(
				TypeParent::token.handle(),
				name,
				ModeToken::Handle(),
				Token::Handle(),
				&DescBase::create_func<Type>,
				&DescBase::convert_func<Type>,
				0 ) { }
	};

	//! token for real task
	template<typename Type, typename TypeParent, typename TypeAbstract = TypeParent>
	class DescReal: public DescBase {
	public:
		explicit DescReal(const String &name):
			DescBase(
				TypeParent::token.handle(),
				name,
				Type::mode_token.handle(),
				TypeAbstract::token.handle(),
				&DescBase::create_func<Type>,
				&DescBase::convert_func<Type>,
				&DescBase::convert_func<Type, TypeAbstract> ) { }
	};

	struct RunParams {
		etl::handle<etl::shared_object> rendererHolder;
		Renderer *renderer;
		mutable Task::List sub_queue;
		RunParams(): renderer() { }
		explicit RunParams(const etl::handle<Renderer> &renderer);
	};

	struct RendererData
	{
		int batch_index;
		int index;
		int deps_count;
		Set deps; // always empty (deps_count is enough), used inside of Renderer::find_deps only
		Set back_deps;

		Set tmp_deps;
		Set tmp_back_deps;

		RunParams params;
		bool success;

		RendererData(): batch_index(), index(), deps_count(), success() { }
	};

	class LockReadBase: public SurfaceResource::LockReadBase
	{
	public:
		explicit LockReadBase(const Task *task):
			SurfaceResource::LockReadBase(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt() ) { }
		explicit LockReadBase(const Task::Handle &task):
			SurfaceResource::LockReadBase(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt() ) { }
	};

	class LockWriteBase: public SurfaceResource::SemiLockWriteBase
	{
	public:
		explicit LockWriteBase(const Task *task):
			SurfaceResource::SemiLockWriteBase(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt(),
				task ? task->get_target_token() : Surface::Token::Handle() ) { }
		explicit LockWriteBase(const Task::Handle &task):
			SurfaceResource::SemiLockWriteBase(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt(),
				task ? task->get_target_token() : Surface::Token::Handle() ) { }
	};

	template<typename T>
	class LockReadGeneric: public SurfaceResource::LockRead<T>
	{
	public:
		explicit LockReadGeneric(const Task *task):
			SurfaceResource::LockRead<T>(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt() ) { }
		explicit LockReadGeneric(const Task::Handle &task):
			SurfaceResource::LockRead<T>(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt() ) { }
	};

	template<typename T>
	class LockWriteGeneric: public SurfaceResource::SemiLockWrite<T>
	{
	public:
		explicit LockWriteGeneric(const Task *task):
			SurfaceResource::SemiLockWrite<T>(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt(),
				task ? task->get_target_token() : Surface::Token::Handle() ) { }
		explicit LockWriteGeneric(const Task::Handle &task):
			SurfaceResource::SemiLockWrite<T>(
				task ? task->target_surface : SurfaceResource::Handle(),
				task ? task->target_rect : RectInt(),
				task ? task->get_target_token() : Surface::Token::Handle() ) { }
	};


	static synfig::Token token;
	virtual Token::Handle get_token() const = 0;

private:
	static const etl::handle<Task> blank;

	mutable bool bounds_calculated;
	mutable Rect bounds;

public:
	Rect source_rect;
	RectInt target_rect;
	SurfaceResource::Handle target_surface;
	List sub_tasks;

	mutable RendererData renderer_data;

	Task();
	virtual ~Task();

	// mode options
	virtual Surface::Token::Handle get_target_token() const {
		if (const Mode *mode = dynamic_cast<const Mode*>(this))
			return mode->get_mode_target_token();
		return Surface::Token::Handle();
	}
	virtual bool get_allow_multithreading() const {
		if (const Mode *mode = dynamic_cast<const Mode*>(this))
			return mode->get_mode_allow_multithreading();
		return true;
	}
	virtual bool get_mode_allow_source_as_target() const {
		if (const Mode *mode = dynamic_cast<const Mode*>(this))
			return mode->get_mode_allow_source_as_target();
		return false;
	}
	virtual bool get_mode_allow_simultaneous_write() const { //!< allow simultaneous write to the same target
		if (const Mode *mode = dynamic_cast<const Mode*>(this))
			return mode->get_mode_allow_simultaneous_write();
		return true;
	}

	Task::Handle& sub_task(int index) {
		assert(index >= 0);
		if (index >= (int)sub_tasks.size())
			sub_tasks.resize(index + 1);
		return sub_tasks[index];
	}

	const Task::Handle& sub_task(int index) const {
		assert(index >= 0);
		return index < (int)sub_tasks.size() ? sub_tasks[index] : blank;
	}

	bool can_convert_to(ModeToken::Handle mode) const;
	ModeToken::Handle get_mode() const;
	Task::Handle convert_to(ModeToken::Handle mode) const;
	Task::Handle convert_to_any() const;
	Task::Handle clone() const;
	Task::Handle clone_recursive() const;

	virtual Rect calc_bounds() const;
	void reset_bounds()
		{ bounds_calculated = false; }
	const Rect& get_bounds() const {
		if (!bounds_calculated) { bounds = calc_bounds(); bounds_calculated = true; }
		return bounds;
	}

	Vector get_pixels_per_unit() const;
	Vector get_units_per_pixel() const;

	void trunc_to_zero();
	void trunc_source_rect(const Rect &rect);
	void trunc_target_rect(const RectInt &rect);
	void trunc_by_bounds();
	void move_target_rect(const VectorInt &offset);
	void set_target_origin(const VectorInt &origin);

	bool is_valid_coords_source() const
		{ return !source_rect.is_nan_or_inf() && source_rect.is_valid(); }
	bool is_valid_coords_target() const
		{ return target_rect.is_valid(); }
	bool is_valid_coords() const
		{ return is_valid_coords_source() && is_valid_coords_target(); }
	bool is_valid_surface_size() const
		{ return target_surface
			  && target_surface->is_exists()
			  && etl::contains(RectInt(VectorInt::zero(), target_surface->get_size()), target_rect); }
	bool is_valid() const
		{ return is_valid_coords() && is_valid_surface_size(); }

	bool allow_simultaneous_run_with(Task &other) const {
		if (!is_valid() || !other.is_valid())
			return true;
		if (!get_allow_multithreading() && !other.get_allow_multithreading())
			return false;
		if (target_surface != other.target_surface)
			return true;
		if ( !get_mode_allow_simultaneous_write()
		  || !other.get_mode_allow_simultaneous_write()
		  || get_target_token() != other.get_target_token() )
			return false;
		return etl::intersect(target_rect, other.target_rect);
	}

	void touch_coords();
	void set_coords(const Rect &source_rect, const VectorInt &target_size);
	void set_coords_zero();
	virtual void set_coords_sub_tasks();
	virtual bool run(RunParams &params) const;
};


// Special tasks


class TaskSurface: public Task
{
public:
	typedef etl::handle<TaskSurface> Handle;
	static Token token;
	virtual Token::Handle get_token() const { return token.handle(); }
};


class TaskLockSurface: public TaskSurface
{
public:
	typedef etl::handle<TaskLockSurface> Handle;
	static Token token;
	virtual Token::Handle get_token() const { return token.handle(); }

private:
	SurfaceResource::LockReadBase *lock_;

public:
	TaskLockSurface():
		lock_() { }
	explicit TaskLockSurface(const SurfaceResource::Handle &surface):
		lock_() { set_surface(surface); }
	TaskLockSurface(const TaskLockSurface& other):
		lock_() { *this = other; }
	~TaskLockSurface() { unlock(); }

	TaskLockSurface& operator=(const TaskLockSurface& other);

	void set_surface(const SurfaceResource::Handle &surface);
	void lock();
	void unlock();
};


class TaskNone: public Task, public TaskInterfaceConstant
{
public:
	typedef etl::handle<TaskNone> Handle;
	static Token token;
	virtual Token::Handle get_token() const { return token.handle(); }
	virtual Rect calc_bounds() const { return Rect(); }
	virtual bool run(RunParams&) const
		{ return true; }
};


class TaskList: public Task
{
public:
	typedef etl::handle<TaskList> Handle;
	static Token token;
	virtual Token::Handle get_token() const { return token.handle(); }
	virtual bool run(RunParams&) const
		{ return true; }
};


class TaskCallbackCond: public Task
{
public:
	typedef etl::handle<TaskCallbackCond> Handle;
	static Token token;
	virtual Token::Handle get_token() const { return token.handle(); }

	Glib::Threads::Cond *cond;
	Glib::Threads::Mutex *mutex;

	TaskCallbackCond(): cond(), mutex() { }

	virtual bool run(RunParams & /* params */) const {
		if (!cond || !mutex) return false;
		Glib::Threads::Mutex::Lock lock(*mutex);
		cond->signal();
		return true;
	}
};


} /* end namespace rendering */
} /* end namespace synfig */

/* -- E N D ----------------------------------------------------------------- */

#endif
