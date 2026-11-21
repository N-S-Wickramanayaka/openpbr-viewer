# Test models from the Khronos glTF-Sample-Assets repo, downloaded at configure time.
# They are not committed: DamagedHelmet is partly CC BY-NC 4.0. Pinned to one commit so the files never change.
set(GLTF_SAMPLE_ASSETS
    https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Assets/e124e8d8d7330a5372bc2b4d00d39a4a9d7997ea/Models)

function(download_model file url_path sha256)
    set(dest "${PROJECT_SOURCE_DIR}/assets/models/${file}")
    if(EXISTS "${dest}")
        return()
    endif()
    message(STATUS "Downloading test model ${file}")
    file(DOWNLOAD "${GLTF_SAMPLE_ASSETS}/${url_path}" "${dest}.part"
         EXPECTED_HASH SHA256=${sha256} STATUS result)
    list(GET result 0 code)
    if(NOT code EQUAL 0)
        file(REMOVE "${dest}.part")
        message(FATAL_ERROR "Downloading ${file} failed: ${result}")
    endif()
    file(RENAME "${dest}.part" "${dest}")
endfunction()

download_model(DamagedHelmet.glb
    DamagedHelmet/glTF-Binary/DamagedHelmet.glb
    a1e3b04de97b11de564ce6e53b95f02954a297f0008183ac63a4f5974f6b32d8)
download_model(MetalRoughSpheres.glb
    MetalRoughSpheres/glTF-Binary/MetalRoughSpheres.glb
    450c05557b0823a10835e32c05caf77a7b175eba94516555c1c9bb084f563f01)