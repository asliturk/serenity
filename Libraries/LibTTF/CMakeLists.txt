set(SOURCES
    Cmap.cpp
    Font.cpp
    Glyf.cpp
)

serenity_lib(LibTTF ttf)
target_link_libraries(LibTTF LibGfx LibM LibCore)

