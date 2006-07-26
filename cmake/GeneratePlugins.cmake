# Note: these lines are duplicated in tools/CMakeLists.cmake
SET(plugins_h "plugins/plugins.h")
FILE(GLOB inc_files RELATIVE "" plugins/*.inc)

# Produce tools/plugins/plugins.h
FILE(WRITE ${plugins_h} "/* automatically generated */\n")
FOREACH(inc ${inc_files})
	FILE(READ ${inc} content)
	FILE(APPEND "plugins/plugins.h" "${content}")
ENDFOREACH(inc)
