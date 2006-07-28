# THIS WILL COMPILE SOME CODE AND RETURN SUCCESS OR FAILURE AND SET THE VAR TO THAT
MACRO(COMPILE_TEST VAR FLAGS LIBS SOURCE_TXT)
	FILE(WRITE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c "${SOURCE_TXT}\n")
	IF(NOT ${FLAGS})
		SET(${FLAGS} " ")
	ENDIF(NOT ${FLAGS})
	IF(NOT ${LIBS})
		SET(${LIBS} " ")
	ENDIF(NOT ${LIBS})
	TRY_COMPILE(${VAR}
      		    ${CMAKE_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c
                    COMPILE_DEFINITIONS ${FLAGS}
      		    "-DLINK_LIBRARIES:STRING=${LIBS}"
                    OUTPUT_VARIABLE OUTPUT)
                    IF(${VAR})
                    	MESSAGE("-- Compilation test for ${VAR} successfull!")
                    	SET(${VAR} ${OUTPUT_VARIABLE})
                    ELSE(${VAR})
                    	MESSAGE("-- Compilation test for ${VAR} failed output to ${CMAKE_BINARY_DIR}/CMakeError.log!")
                    	FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
        		"Performing Compilation Test ${VAR} failed with the following output:\n"
        		"${OUTPUT}\n")
                    ENDIF(${VAR})
	FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c)
ENDMACRO(COMPILE_TEST VAR FLAGS LIBS SOURCE_TXT)

# SAME AS THE ABOVE BUT WILL RUN THE COMPILED EXECUTABLE AND STORE IT'S RETURN VALUE IN VAR_1 and the COMPILE DATA IN VAR_2
MACRO(RUN_TEST VAR_1 VAR_2 FLAGS LIBS SOURCE_TXT)
	FILE(WRITE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c "${SOURCE_TXT}\n")
	IF(NOT ${FLAGS})
		SET(${FLAGS} " ")
	ENDIF(NOT ${FLAGS})
	IF(NOT ${LIBS})
		SET(${LIBS} " ")
	ENDIF(NOT ${LIBS})
	TRY_RUN(${VAR_1} ${VAR_2}
      		    ${CMAKE_BINARY_DIR}
                    ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c
                    COMPILE_DEFINITIONS ${FLAGS}
                    "-DLINK_LIBRARIES:STRING=${LIBS}"
                    OUTPUT_VARIABLE OUTPUT)
                    IF(${VAR_1})
                    	MESSAGE("-- Test for ${VAR_1} successfull!")
                    	SET(${VAR_1} ${OUTPUT_VARIABLE})
                    ELSE(${VAR_1})
                    	MESSAGE("-- Run test for ${VAR_1} failed output to ${CMAKE_BINARY_DIR}/CMakeError.log!")
                    	FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
        		"Performing Compilation & Run Test ${VAR_1} failed with the following output:\n"
        		"${OUTPUT}\n")
                    ENDIF(${VAR_1})
	FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.c)
ENDMACRO(RUN_TEST VAR_1 VAR_2 FLAGS LIBS SOURCE_TXT)

# RUNS ASM CODE WITH as STORES SUCCESS OR NOT IN VAR
MACRO(ASM_TEST VAR SOURCE_TXT)
	FILE(WRITE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.s "${SOURCE_TXT}\n")
	EXECUTE_PROCESS(COMMAND "as" "tmp.s"
		     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp"
		     RESULT_VARIABLE OUTPUT
		     ERROR_VARIABLE  ERROR)
		    IF(EXISTS ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/a.out)
                    	MESSAGE("-- Compilation test for ${VAR} successfull!")
                    	FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/a.out)
                    	SET(${VAR} 1)
                    ELSE(EXISTS ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/a.out)
                    	MESSAGE("-- Compilation test for ${VAR} failed output to ${CMAKE_BINARY_DIR}/CMakeError.log!")
                    	FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeError.log
        		"Performing Compilation Test ${VAR} failed with the following output:\n"
        		"${OUTPUT} ${ERROR}\n")
                    ENDIF(EXISTS ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/a.out)
	FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/tmp.s)
ENDMACRO(ASM_TEST VAR SOURCE_TXT)