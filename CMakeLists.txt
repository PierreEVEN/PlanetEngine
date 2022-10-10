cmake_minimum_required(VERSION 3.13)

# Set project constants
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}) # Project dir
set(BINARIES_DIR ${PROJECT_ROOT}/bin) # Binaries dir
set(INTERMEDIATES_DIR ${CMAKE_BINARY_DIR}) # Intermediates dir

#Global cmake features
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#Set startup project
set_property(DIRECTORY ${PROJECT_ROOT} PROPERTY VS_STARTUP_PROJECT PlanetEngine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

project(ProjetProgrammationGraphique)

# Set compiler options
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

	# Enable Hot Reload
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /ZI /Ob0 /Od /RTC1")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /ZI /Ob0 /Od /RTC1")
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /debug /INCREMENTAL /LTCG:OFF")
	set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /debug /INCREMENTAL /LTCG:OFF")
	set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /debug /INCREMENTAL /LTCG:OFF")
	
	# MP build for MSVC
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /EHsc")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BINARIES_DIR}/Lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARIES_DIR}/Bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BINARIES_DIR}/Bin")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_ROOT}")

function(configure_project PROJECT_NAME SOURCES)
	set_target_properties(${PROJECT_NAME} PROPERTIES 
		ARCHIVE_OUTPUT_DIRECTORY "${BINARIES_DIR}/Lib"
		RUNTIME_OUTPUT_DIRECTORY "${BINARIES_DIR}/Bin"
		LIBRARY_OUTPUT_DIRECTORY "${BINARIES_DIR}/Bin"
		VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_ROOT}")
	target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
	set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	source_group(TREE ${PROJECT_ROOT} FILES ${SOURCES})
endfunction()

# Ensure dependencies are available
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

# Find or build easycppogl
find_package(easycppogl QUIET)
if (easycppogl_FOUND)
	message("-- easycppogl is already installed")
else()
	message("-- easycppogl was not found on the current device. Using local version instead")
	set(EASY_CPP_OGL_INSTALL_DIR "${INTERMEDIATES_DIR}/easycppogl/easycppogl_src/easycppogl")
	get_filename_component(_fullpath ${EASY_CPP_OGL_INSTALL_DIR} REALPATH)
	if(NOT EXISTS "${_fullpath}")
		message(WARNING "easycppogl is not built !")

		include(ProcessorCount)
		ProcessorCount(N)

		message("--------------------------building easycppogl : Release using ${N} workers")
		execute_process (COMMAND cmake -S "${PROJECT_ROOT}/third_party/easycppogl" -B "${INTERMEDIATES_DIR}/easycppogl" -DBUILD_TYPE=Release RESULT_VARIABLE output)
		execute_process (COMMAND cmake --build "${INTERMEDIATES_DIR}/easycppogl" --parallel ${N} --target easycppogl --config=Debug RESULT_VARIABLE output)
		message("done")

		message("--------------------------building easycppogl : Debug using ${N} workers")
		execute_process (COMMAND cmake -S "${PROJECT_ROOT}/third_party/easycppogl" -B "${INTERMEDIATES_DIR}/easycppogl" -DBUILD_TYPE=Debug RESULT_VARIABLE output)
		execute_process (COMMAND cmake --build "${INTERMEDIATES_DIR}/easycppogl" --parallel ${N} --target easycppogl --config=Release RESULT_VARIABLE output)
		message("done")

		message("--------------------------installing easycppogl")
		execute_process (COMMAND cmake install "${INTERMEDIATES_DIR}/easycppogl" RESULT_VARIABLE output)
		message("done")
	endif()
endif()

set(easycppogl_DIR ${EASY_CPP_OGL_INSTALL_DIR})

find_package(easycppogl CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h resources/**)
add_executable(PlanetEngine ${SOURCES})
configure_project(PlanetEngine ${SOURCES})
target_link_libraries(PlanetEngine PUBLIC easycppogl)
target_include_directories(PlanetEngine PRIVATE src)
source_group(TREE ${PROJECT_ROOT} FILES  ${SOURCES})
