cmake_minimum_required(VERSION 3.16)

project(ITit C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(COMMON_SOURCES
	src/core/connection_manager.c
	src/core/message_router.c
	src/core/session_manager.c
	src/network/client_handler.c
	src/network/event_handler.c
	src/network/event_loop.c
	src/network/tcp_client.c
	src/network/tcp_server.c
	src/protocol/builder.c
	src/protocol/command_dandler.c
	src/protocol/parser.c
	src/storage/history_manager.c
	src/storage/storage.c
	src/storage/user_store.c
	src/utils/logger.c
	src/utils/safe_utils.c
	src/utils/time_utils.c
)

set(CLIENT_SUPPORT_SOURCES
	src/network/tcp_client.c
	src/protocol/builder.c
	src/protocol/parser.c
	src/utils/logger.c
	src/utils/safe_utils.c
	src/utils/time_utils.c
)

add_executable(server src/server/server.c ${COMMON_SOURCES})
add_executable(client_app
	src/client/client.c
	src/client/client_commands.c
	src/client/main.c
	src/client/ui.c
	${CLIENT_SUPPORT_SOURCES}
)

if(WIN32)
	set(TUI_SOURCE src/tui/tui_pdcurses.c)
	set(PDCURSES_DIR "${CMAKE_SOURCE_DIR}/third_party/PDCurses")
	set(PDCURSES_LIB "${PDCURSES_DIR}/wincon/pdcurses.a")

	add_custom_command(
		OUTPUT "${PDCURSES_LIB}"
		COMMAND ${CMAKE_MAKE_PROGRAM} -C "${PDCURSES_DIR}/wincon" -f Makefile
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		COMMENT "Building vendored PDCurses"
		VERBATIM
	)
	add_custom_target(vendored_pdcurses DEPENDS "${PDCURSES_LIB}")

	add_library(pdcurses_vendor STATIC IMPORTED GLOBAL)
	set_target_properties(pdcurses_vendor PROPERTIES
		IMPORTED_LOCATION "${PDCURSES_LIB}"
		INTERFACE_INCLUDE_DIRECTORIES "${PDCURSES_DIR}"
	)
	add_dependencies(pdcurses_vendor vendored_pdcurses)
else()
	set(TUI_SOURCE src/tui/tui_ncurses.c)
	set(NCURSES_DIR "${CMAKE_SOURCE_DIR}/third_party/ncurses")
	set(NCURSES_PREFIX "${NCURSES_DIR}/_install")
	set(NCURSES_LIB "${NCURSES_PREFIX}/lib/libncursesw.a")

	add_custom_command(
		OUTPUT "${NCURSES_LIB}"
		COMMAND "${NCURSES_DIR}/configure"
			--prefix="${NCURSES_PREFIX}"
			--with-shared=no
			--without-debug
			--without-tests
			--without-ada
			--without-cxx
			--without-cxx-binding
			--enable-widec
			--with-default-terminfo-dir="${NCURSES_PREFIX}/share/terminfo"
			--with-terminfo-dirs="${NCURSES_PREFIX}/share/terminfo:/etc/terminfo:/lib/terminfo:/usr/share/terminfo"
		COMMAND ${CMAKE_MAKE_PROGRAM}
		COMMAND ${CMAKE_MAKE_PROGRAM} install
		WORKING_DIRECTORY "${NCURSES_DIR}"
		COMMENT "Building vendored ncurses"
		VERBATIM
	)
	add_custom_target(vendored_ncurses DEPENDS "${NCURSES_LIB}")

	add_library(ncurses_vendor STATIC IMPORTED GLOBAL)
	set_target_properties(ncurses_vendor PROPERTIES
		IMPORTED_LOCATION "${NCURSES_LIB}"
		INTERFACE_INCLUDE_DIRECTORIES "${NCURSES_PREFIX}/include/ncursesw;${NCURSES_PREFIX}/include"
	)
	add_dependencies(ncurses_vendor vendored_ncurses)
endif()

add_executable(client_tui
	src/client/client.c
	src/client/client_commands.c
	src/client/tui_main.c
	${TUI_SOURCE}
	${CLIENT_SUPPORT_SOURCES}
)

add_executable(test_utils tests/test_utils.c
	src/utils/logger.c
	src/utils/safe_utils.c
	src/utils/time_utils.c
)

add_executable(test_protocol tests/test_protocol.c ${COMMON_SOURCES})
add_executable(test_builder tests/test_builder.c ${COMMON_SOURCES})
add_executable(test_connection tests/test_connection.c
	src/core/connection_manager.c
	src/utils/logger.c
	src/utils/safe_utils.c
	src/utils/time_utils.c
)
add_executable(test_session tests/test_session.c ${COMMON_SOURCES})

set(ITIT_TARGETS
	server
	client_app
	client_tui
	test_utils
	test_protocol
	test_builder
	test_connection
	test_session
)

if(WIN32)
	foreach(target ${ITIT_TARGETS})
		target_link_libraries(${target} PRIVATE ws2_32)
	endforeach()
	target_link_libraries(client_tui PRIVATE pdcurses_vendor)
else()
	find_package(Threads REQUIRED)
	foreach(target ${ITIT_TARGETS})
		target_link_libraries(${target} PRIVATE Threads::Threads)
	endforeach()
	target_link_libraries(client_tui PRIVATE ncurses_vendor)
endif()
