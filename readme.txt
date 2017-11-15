multi client chatroom server:
author:Ali Reza Aboofazeli


run the server:

compile serverproject.c:

to compile run this command:
gcc -pthread -o serverproject serverproject.c

now serverproject is the executable file.

the executable file should run like this:
./serverproject <portID>
it needs an portID,
example: ./serverproject 5000

now we can telnet the server on same machine or different machine by knowing the ip address of server machine.

example telnet 127.0.0.1 5000
now we connected to server.


by connecting more clients, each client has its own unique id.
and every can see each others msgs with their id.

there is also possible options:
menu options:

\HELP
\QUIT
\ACTIVE
\PING
\NAME <nickname>
\PRIVATE <client id> <msg>


for example if a client type \NAME ALI from now he will have this name instead od id.

or if a client type \PRIVATE 2 hello  this command will send private message "hello" to client number 2.


\ACTIVE will show active users.
