if (TARGET nlohmann_json::nlohmann_json)
    return()
endif()

include_guard()
include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git
    GIT_TAG v3.11.2
    GIT_SHALLOW TRUE
    )
FetchContent_MakeAvailable(json)
