cmake_minimum_required(VERSION 3.0)

project(TP_Prog_Graphique_3D LANGUAGES CXX)

set (TP_SHADERS_PATH $ENV{HOME} CACHE PATH "Path where to put GLSL shaders ...")

add_definitions("-DDEF_SHADERS_PATH=${TP_SHADERS_PATH}")

macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
endmacro()

set( tp_list

)

foreach( exec_tp ${tp_list})
	compile(${exec_tp})
endforeach()

# nice vs solution
set_target_properties(${tp_list} PROPERTIES FOLDER TP_Prog_Graphique_3D)