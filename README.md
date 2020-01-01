# libuv-app

https://gist.github.com/SKempin/b7857a6ff6bddb05717cc17a44091202

This contains the app + libuv as a git subtree

for updating
    git fetch libuv master

pulling in:
    git subtree pull --prefix libuv/ https://github.com/baberuth/libuv.git master --squash

main.c
    Show the basic usage of libuv (timers + keyhandling + signals)

main-server.c
    Shows a tcp server that can accept multiple clients and create a basic chat room

main-client.c
    Shows a tcp client to connect to the server application to send some characters from the user input
