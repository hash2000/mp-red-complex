message(STATUS "> Build Launcher")

set(Launcher-Sources
	${RedComplex_SOURCE_DIR}/src/Launcher
)

file(GLOB_RECURSE SRCS ${Launcher-Sources}/*.cpp)

qt_add_executable(Launcher
	${SRCS}
)

set_target_properties(Launcher PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(Launcher PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(Launcher PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(Launcher
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets
		Qt6::Network
		Qt6::Xml

		RedComplexBase
		RedComplexBaseWidgets
		RedComplexGraphics		
		RedComplexEngine

		RedComplexCodeEditorWidget

		RedComplexCharactersModule
		RedComplexConsoleModule
		RedComplexDatabaseModule
		RedComplexFetchApiModule
		RedComplexInventoriesModule
		RedComplexMaterialsModule
		RedComplexMessagerModule
		RedComplexShadersModule
		RedComplexTexturesModule
		RedComplexUsersModule

)

if(WIN32)
	set(OPENSSL_DLL_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/openssl")
	set(OPENSSL_CRYPTO_DLL "${OPENSSL_DLL_DIR}/libcrypto-3-${RedComplex_TARGET_ARCHITECTURE}.dll")
	set(OPENSSL_SSL_DLL "${OPENSSL_DLL_DIR}/libssl-3-${RedComplex_TARGET_ARCHITECTURE}.dll")

	message (status "OPENSSL_CRYPTO_DLL ${OPENSSL_CRYPTO_DLL}")
	message (status "OPENSSL_SSL_DLL ${OPENSSL_SSL_DLL}")

	add_custom_command(TARGET Launcher POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
					"${OPENSSL_CRYPTO_DLL}"
					"${OPENSSL_SSL_DLL}"
					$<TARGET_FILE_DIR:Launcher>
			COMMENT "Copying OpenSSL DLLs from vcpkg"
	)
endif()
