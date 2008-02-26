# Produce tools/plugins/plugins.h
# These variables need to be passed in on the command line:
#   plugins_h
#   inc_files (space separated list of plugin .inc files)
# 

if("${plugins_h}" MATCHES "^$")
    message(FATAL_ERROR "** Internal error: plugins_h variable not set **")
endif("${plugins_h}" MATCHES "^$")

separate_arguments(inc_files)
file(WRITE ${plugins_h} "/* automatically generated */\n")
foreach(inc ${inc_files})
    # It's too easy to miss when something goes wrong without messages.
    message("Appending " ${inc})
    file(READ ${inc} content)
    file(APPEND "plugins/plugins.h" "${content}")
endforeach(inc)
