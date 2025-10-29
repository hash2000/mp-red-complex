message(STATUS "> Build Launcher")

set(Launcher-Sources
	${RedComplex_SOURCE_DIR}/src/Launcher
)

file(GLOB_RECURSE SRCS ${Launcher-Sources}/*.cpp)
add_executable(Launcher
	${SRCS}
)

target_link_libraries(Launcher
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)

target_include_directories(Launcher
	PUBLIC
		${Launcher-Sources}
)

# add_executable(Launcher main.cpp)

# target_link_libraries(Launcher PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# set_target_properties(Launcher PROPERTIES
#     CXX_STANDARD 20
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
# )
