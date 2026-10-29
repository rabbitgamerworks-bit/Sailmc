project(sailmc)

# Native Sailfish OS frontend for the MultiMC launcher core (built with -DLauncher_SAILFISH=ON).

find_package(PkgConfig REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
pkg_check_modules(SAILFISHAPP REQUIRED IMPORTED_TARGET sailfishapp)

set(SAILMC_SOURCES
    src/main.cpp
    src/SailLauncher.h
    src/SailLauncher.cpp
    src/JavaRuntimeManager.h
    src/JavaRuntimeManager.cpp
    src/RendererConfig.h
    src/RendererConfig.cpp
    src/ImageProvider.h
    src/ImageProvider.cpp
    src/models/InstanceModel.h
    src/models/InstanceModel.cpp
    src/models/AccountModel.h
    src/models/AccountModel.cpp
    src/models/VersionModel.h
    src/models/VersionModel.cpp
    src/models/ModsModel.h
    src/models/ModsModel.cpp
    src/models/NewsModel.h
    src/models/NewsModel.cpp
    src/models/LogProxyModel.h
)

add_executable(${Launcher_Name} ${SAILMC_SOURCES})
target_include_directories(${Launcher_Name} PRIVATE src src/core)
target_link_libraries(${Launcher_Name}
    Launcher_logic
    Qt5::Quick
    Qt5::Qml
    PkgConfig::SAILFISHAPP
)
if(DEFINED Launcher_BINARY_RPATH)
    set_target_properties(${Launcher_Name} PROPERTIES INSTALL_RPATH "${Launcher_BINARY_RPATH}")
endif()

install(TARGETS ${Launcher_Name} RUNTIME DESTINATION ${BINARY_DEST_DIR})

# QML, desktop file and icons follow the Sailfish OS application layout
install(DIRECTORY qml DESTINATION share/${Launcher_Name})
install(FILES ${Launcher_Name}.desktop DESTINATION share/applications)
foreach(size 86x86 108x108 128x128 172x172)
    install(FILES icons/${size}/${Launcher_Name}.png DESTINATION share/icons/hicolor/${size}/apps)
endforeach()

# Helper jars of the launcher (NewLaunch.jar, JavaCheck.jar), built on the host when the
# build engine has no JDK - see build-jars.sh
set(Launcher_SAILFISH_PREBUILT_JARS "${CMAKE_CURRENT_SOURCE_DIR}/jars" CACHE PATH "Folder with prebuilt NewLaunch.jar and JavaCheck.jar")
if(NOT Java_JAVAC_EXECUTABLE)
    foreach(jar NewLaunch.jar JavaCheck.jar)
        if(EXISTS "${Launcher_SAILFISH_PREBUILT_JARS}/${jar}")
            install(FILES "${Launcher_SAILFISH_PREBUILT_JARS}/${jar}" DESTINATION ${JARS_DEST_DIR})
        else()
            message(WARNING "${jar} not found in ${Launcher_SAILFISH_PREBUILT_JARS}; run sailfish/build-jars.sh first")
        endif()
    endforeach()
endif()

# Natives for the game built by native/build-natives.sh (GL4ES, GLFW for Wayland, LWJGL, OpenAL)
set(SAILMC_NATIVES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/native/prebuilt/${CMAKE_SYSTEM_PROCESSOR}" CACHE PATH "Prebuilt game natives for this architecture")
if(EXISTS "${SAILMC_NATIVES_DIR}")
    message(STATUS "Bundling game natives from ${SAILMC_NATIVES_DIR}")
    install(DIRECTORY "${SAILMC_NATIVES_DIR}/" DESTINATION share/${Launcher_Name}/renderers USE_SOURCE_PERMISSIONS)
else()
    message(STATUS "No game natives in ${SAILMC_NATIVES_DIR}, run native/build-natives.sh to bundle them")
endif()
