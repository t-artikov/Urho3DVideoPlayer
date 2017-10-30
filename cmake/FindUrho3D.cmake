# output:
# Urho3D target
# install_urho3d()

set(URHO3D_DIR "c:/Program Files/Urho3D" CACHE PATH "Path to Urho3D SDK")

if(NOT EXISTS "${URHO3D_DIR}/include/Urho3D/Urho3D.h")
	message(FATAL_ERROR "Urho3D SDK not found. Set 'URHO3D_DIR' variable.")
endif()

include_directories()

add_library(Urho3D STATIC IMPORTED)
set_target_properties(Urho3D PROPERTIES
    IMPORTED_LOCATION "${URHO3D_DIR}/lib/Urho3D.lib"
    IMPORTED_LOCATION_DEBUG "${URHO3D_DIR}/lib/Urho3D_d.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${URHO3D_DIR}/include;${URHO3D_DIR}/include/Urho3D/ThirdParty"
)

function(install_urho3d)
	install(FILES "${URHO3D_DIR}/bin/Urho3D.dll" CONFIGURATIONS Release RelWithDebInfo MinSizeRel DESTINATION ".")
 	install(FILES "${URHO3D_DIR}/bin/Urho3D_d.dll" CONFIGURATIONS Debug DESTINATION ".")
    install(DIRECTORY "${URHO3D_DIR}/share/Resources/CoreData" DESTINATION ".")
endfunction()
