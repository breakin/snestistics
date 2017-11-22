# squirrel
set(DISABLE_DYNAMIC 1)
set(SQ_DISABLE_INSTALLER 1)
add_subdirectory(squirrel)

set_target_properties(sq_static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
set_target_properties(sq_static PROPERTIES FOLDER deps-squirrel)
set_target_properties(sqstdlib_static PROPERTIES FOLDER deps-squirrel)
set_target_properties(squirrel_static PROPERTIES FOLDER deps-squirrel)

set(DISABLE_DYNAMIC)
set(SQ_DISABLE_INSTALLER)
