/**
 * limbo_task_db.cpp
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "limbo_task_db.h"

#include "../compat/limbo_compat.h"
#include "../compat/project_settings.h"

#ifdef LIMBOAI_MODULE
#include "core/io/dir_access.h"
#endif // LIMBOAI_MODULE

#ifdef LIMBOAI_GDEXTENSION
#include <godot_cpp/classes/dir_access.hpp>
using namespace godot;
#endif // LIMBOAI_GDEXTENSION

LimboTaskDB *LimboTaskDB::singleton = nullptr;

HashMap<String, List<String>> LimboTaskDB::core_tasks;
HashMap<String, List<String>> LimboTaskDB::tasks_cache;


LimboTaskDB *LimboTaskDB::get_singleton() {
	return singleton;
}

void LimboTaskDB::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_task_api", "p_class_name", "p_task_category"), &LimboTaskDB::register_task_api);
}


_FORCE_INLINE_ void _populate_scripted_tasks_from_dir(String p_path, List<String> *p_task_classes) {
	if (p_path.is_empty()) {
		return;
	}

	Ref<DirAccess> dir = DIR_ACCESS_CREATE();

	if (dir->change_dir(p_path) == OK) {
		dir->list_dir_begin();
		String fn = dir->get_next();
		while (!fn.is_empty()) {
			if (fn.ends_with(".gd") || fn.ends_with(".cs")) {
				String full_path = p_path.path_join(fn);
				p_task_classes->push_back(full_path);
			}
			fn = dir->get_next();
		}
		dir->list_dir_end();
	} else {
		ERR_FAIL_MSG(vformat("Failed to list \"%s\" directory.", p_path));
	}
}

_FORCE_INLINE_ void _populate_from_user_dir(String p_path, HashMap<String, List<String>> *p_categories) {
	if (p_path.is_empty()) {
		return;
	}

	Ref<DirAccess> dir = DIR_ACCESS_CREATE();
	if (dir->change_dir(p_path) == OK) {
		dir->list_dir_begin();
		String fn = dir->get_next();
		while (!fn.is_empty()) {
			if (dir->current_is_dir() && !fn.begins_with(".")) {
				String full_path;
				String category;
				if (fn == ".") {
					full_path = p_path;
					category = LimboTaskDB::get_misc_category();
				} else {
					full_path = p_path.path_join(fn);
					category = fn.capitalize();
				}

				if (!p_categories->has(category)) {
					p_categories->insert(category, List<String>());
				}

				_populate_scripted_tasks_from_dir(full_path, &p_categories->get(category));
			}
			fn = dir->get_next();
		}
		dir->list_dir_end();

		_populate_scripted_tasks_from_dir(p_path, &p_categories->get(LimboTaskDB::get_misc_category()));

	} else {
		ERR_FAIL_MSG(vformat("Failed to list \"%s\" directory.", p_path));
	}
}

void LimboTaskDB::scan_user_tasks() {
	tasks_cache = HashMap<String, List<String>>(core_tasks);

	if (!tasks_cache.has(LimboTaskDB::get_misc_category())) {
		tasks_cache[LimboTaskDB::get_misc_category()] = List<String>();
	}

	PackedStringArray user_task_directories = GLOBAL_GET("limbo_ai/behavior_tree/user_task_dirs");
	for (const String &user_task_dir : user_task_directories) {
		_populate_from_user_dir(user_task_dir, &tasks_cache);
	}

	for (KeyValue<String, List<String>> &E : tasks_cache) {
		E.value.sort_custom<ComparatorByTaskName>();
	}
}

List<String> LimboTaskDB::get_categories() {
	List<String> r_cat;
	for (const KeyValue<String, List<String>> &E : tasks_cache) {
		r_cat.push_back(E.key);
	}
	r_cat.sort();
	return r_cat;
}

List<String> LimboTaskDB::get_tasks_in_category(const String &p_category) {
	return List<String>(tasks_cache[p_category]);
}

void LimboTaskDB::register_task_api(String p_class_name, String p_task_category)
{
	HashMap<String, List<String>>::Iterator E = core_tasks.find(p_task_category);
	if (E) {
		E->value.push_back(p_class_name);
	} else {
		List<String> tasks;
		tasks.push_back(p_class_name);
		core_tasks.insert(p_task_category, tasks);
	}
}		

LimboTaskDB::LimboTaskDB() {
	singleton = this;
}

LimboTaskDB::~LimboTaskDB() {
	singleton = nullptr;
}
