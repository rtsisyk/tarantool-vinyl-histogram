add_library(bench SHARED bench.c nb_histogram.c)
set_target_properties(bench PROPERTIES PREFIX "" OUTPUT_NAME "bench")

# Install module
install(FILES init.lua DESTINATION ${TARANTOOL_INSTALL_LUADIR}/${PROJECT_NAME}/)
install(TARGETS bench LIBRARY DESTINATION ${TARANTOOL_INSTALL_LIBDIR}/${PROJECT_NAME}/)
