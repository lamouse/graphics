set(sources
    app.hpp
    app.cpp
    gui.hpp
    gui.cpp
    main.cpp
)

if(USE_QT)
    list(APPEND sources
        QT_common.hpp
        QT_common.cpp
        QT_window.hpp
        QT_window.cpp
        QT_render_widget.hpp
        QT_render_widget.cpp
        imgui_qt.hpp
        imgui_qt.cpp
    )
else()
    list(APPEND sources
        SDL_common.hpp
        SDL_common.cpp
        sdl_window.hpp
        sdl_window.cpp
    )
endif()
