set(sources
    app.hpp
    app.cpp
    graphic.hpp
    graphic.cpp
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
        ui/render_config.hpp
        ui/render_config.cpp
    )
else()
    list(APPEND sources
        SDL_common.hpp
        SDL_common.cpp
        sdl_window.hpp
        sdl_window.cpp
    )
endif()
