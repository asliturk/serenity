compile_ipc(DialogServer.ipc DialogServerEndpoint.h)
compile_ipc(DialogClient.ipc DialogClientEndpoint.h)

set(SOURCES
    ClientConnection.cpp
    main.cpp
    DialogClientEndpoint.h
    DialogServerEndpoint.h
)

serenity_bin(DialogServer)
target_link_libraries(DialogServer LibCore LibGfx LibGUI LibIPC)
