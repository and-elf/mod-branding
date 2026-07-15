#
# mod-branding -- per-module CMake hook (included inline by modules/CMakeLists.txt during module
# configuration, with SOURCE_MODULE set to this module).
#
# mod-branding consumes the shared brand taxonomy (Branding::BrandId) and the injected IRng/IClock
# interfaces from the header-only mod-common module instead of defining them privately. Link the
# `mod-common` INTERFACE target so the include path is declared explicitly and resolves under DYNAMIC
# linkage, where each module only gets its own include dirs.
#
# STATIC linkage: this module is compiled into the aggregate `modules` target, which the core CMake
# wiring already links to mod-common -- nothing to do here.
# DYNAMIC linkage (e.g. -DMODULE_MOD-BRANDING=dynamic): this module has its own project target that
# needs the include path, so link mod-common to it here.
#
if(TARGET mod-common)
    GetProjectNameOfModuleName("${SOURCE_MODULE}" _mb_project)
    if(TARGET ${_mb_project})
        target_link_libraries(${_mb_project} PUBLIC mod-common)
    endif()
else()
    message(WARNING
        "mod-branding requires mod-common (modules/mod-common) for the shared brand taxonomy "
        "(Branding::BrandId, IRng, IClock); add it via install-modules.sh.")
endif()
