cmake_minimum_required(VERSION 3.0)

project(EasyCppOGL_TUTO LANGUAGES CXX)

macro(compile)
  add_executable(${ARGV0} ${ARGV0}.cpp)
  target_link_libraries(${ARGV0} easycppogl)
endmacro()

set( tuto_list
tuto2_ex1
tuto2_ex2
tuto2_ex3
tuto2_ex4
tuto3_ex1
tuto3_ex2
tuto3_ex3
tuto4_ex1
tuto4_ex2
)

foreach( exec_tuto ${tuto_list})
	compile(${exec_tuto})
endforeach()

# nice vs solution
set_target_properties(${tuto_list} PROPERTIES FOLDER Tuto)

