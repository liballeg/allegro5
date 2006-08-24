# Produce tools/plugins/plugins.h
# These variables need to be passed in on the command line:
#   plugins_h
#   inc_files
# 

if("${plugins_h}" MATCHES "^$")
	message(FATAL_ERROR "** Internal error: plugins_h variable not set **")
endif("${plugins_h}" MATCHES "^$")

FILE(WRITE ${plugins_h} "/* automatically generated */\n")
FOREACH(inc ${inc_files})
	FILE(READ ${inc} content)
	FILE(APPEND "plugins/plugins.h" "${content}")
ENDFOREACH(inc)
