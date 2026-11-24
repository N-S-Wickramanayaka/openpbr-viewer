# Test assets, downloaded at configure time and pinned to one commit so the files never change.
# They are not committed: some licences (DamagedHelmet: CC BY-NC 4.0) do not allow it.
set(GLTF_SAMPLE_ASSETS
    https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/e124e8d8d7330a5372bc2b4d00d39a4a9d7997ea/Models)
set(GLTF_SAMPLE_ENVIRONMENTS
    https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Environments/3a030b54c2e4f0f4d469eec2f98f47398c9a2a30)

function(download_asset dest url sha256)
    if(EXISTS "${dest}")
        return()
    endif()
    get_filename_component(name "${dest}" NAME)
    message(STATUS "Downloading ${name}")
    file(DOWNLOAD "${url}" "${dest}.part" EXPECTED_HASH SHA256=${sha256} STATUS result)
    list(GET result 0 code)
    if(NOT code EQUAL 0)
        file(REMOVE "${dest}.part")
        message(FATAL_ERROR "Downloading ${name} failed: ${result}")
    endif()
    file(RENAME "${dest}.part" "${dest}")
endfunction()

set(MODELS ${PROJECT_SOURCE_DIR}/assets/models)
download_asset(${MODELS}/DamagedHelmet.glb ${GLTF_SAMPLE_ASSETS}/DamagedHelmet/glTF-Binary/DamagedHelmet.glb
    a1e3b04de97b11de564ce6e53b95f02954a297f0008183ac63a4f5974f6b32d8)
download_asset(${MODELS}/MetalRoughSpheres.glb ${GLTF_SAMPLE_ASSETS}/MetalRoughSpheres/glTF-Binary/MetalRoughSpheres.glb
    450c05557b0823a10835e32c05caf77a7b175eba94516555c1c9bb084f563f01)

# Environments (1024 x 512): Cannon_Exterior is the Khronos Sample Viewer's default (CC0, Greg Zaal);
# neutral is a studio setup (CC BY 4.0, Amazon, distributed by Khronos).
set(ENVIRONMENTS ${PROJECT_SOURCE_DIR}/assets/environments)
download_asset(${ENVIRONMENTS}/Cannon_Exterior.hdr ${GLTF_SAMPLE_ENVIRONMENTS}/Cannon_Exterior.hdr
    7d65926ed2fa41c121738af2addf1fd5f1ae42d12ff77255e2679777f06c984b)
download_asset(${ENVIRONMENTS}/neutral.hdr ${GLTF_SAMPLE_ENVIRONMENTS}/neutral.hdr
    dad135da12b0bc55c488574bd56bf011407c9ed3040493f240528cdfb51782e8)