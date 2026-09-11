cmake_minimum_required(VERSION 3.10)
project(LostAndFound CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Run executables with the project root as the working directory,
# so they find/create the "data" (and, for the server, "public") folder
# in the right place.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# ---- console app (original project) ----
add_executable(lostfound
    main.cpp
    User.cpp
    Item.cpp
    Claim.cpp
    LostAndFoundSystem.cpp
)

# ---------------------------------------------------------------
# Web server target (backend for the browser front end in web/).
# Reuses the exact same model classes as the console app above --
# no logic is duplicated, only main.cpp is swapped for server_main.cpp.
# ---------------------------------------------------------------
find_package(Threads REQUIRED)

add_executable(lostfound_server
    server/server_main.cpp
    User.cpp
    Item.cpp
    Claim.cpp
    LostAndFoundSystem.cpp
)
target_link_libraries(lostfound_server PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(lostfound_server PRIVATE ws2_32 crypt32)
endif()
