project(test_gui_context)

add_executable(${PROJECT_NAME}
        main.cpp
        training.hpp
        training.cpp)
target_link_libraries(${PROJECT_NAME} om)