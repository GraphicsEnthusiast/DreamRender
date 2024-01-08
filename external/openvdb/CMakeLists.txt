ADD_LIBRARY(openvdb INTERFACE)

SET(OPENVDB_INCLUDE_DIRS 
${CMAKE_CURRENT_SOURCE_DIR}/nanovdb/include
${CMAKE_CURRENT_SOURCE_DIR}/openvdb/include
${CMAKE_CURRENT_SOURCE_DIR}/boost/include
${CMAKE_CURRENT_SOURCE_DIR}/tbb/include
${CMAKE_CURRENT_SOURCE_DIR}/openexr/include
)
SET(OPENVDB_INCLUDE_DIRS ${OPENVDB_INCLUDE_DIRS} PARENT_SCOPE)

target_include_directories(openvdb INTERFACE ${OPENVDB_INCLUDE_DIRS})

# target_compile_definitions(openvdb INTERFACE NOMINMAX)
set(OPENEXR_BUILD_SHARED ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ilmbase)

target_link_directories(openvdb INTERFACE 
${CMAKE_CURRENT_SOURCE_DIR}/openvdb/lib
${CMAKE_CURRENT_SOURCE_DIR}/boost/lib
${CMAKE_CURRENT_SOURCE_DIR}/tbb/lib/$<$<CONFIG:Debug>:debug>
)

target_link_libraries(openvdb INTERFACE 
	"$<IF:$<CONFIG:Debug>,openvdb_d,openvdb>"
	"$<IF:$<CONFIG:Debug>,tbb_debug,tbb>"
	libboost_system-vc141-mt-x64-1_70
	libboost_iostreams-vc141-mt-x64-1_70
	IlmBase::Half
)
