PRESENTATION

Video de pr�sentation : https://www.youtube.com/watch?v=vJRDc06tM80
Github : https://github.com/Toffanim/Patchwork

COMPILATION

Windows

Les binary compil�s sont donn�s pour SDL et Boost, il faut juste utiliser VS2013, lancer le .sln et c'est bon.
Pour utiliser une autre version, le lecteur doit compiler boost lui-m�me

Linux

Test� sous ubuntu
Installer SDL2 (sudo apt-get install libsdl2-dev)
Installer boost (sudo apt-get install libboost-all-dev )
Ensuite lancer le make, les fichiers build devrais �tre dans le dossier Debug

WHAT IS WHERE ?

/Client
|____/Client.cpp
/Server
|____/Server.cpp
/Include
|____/SDL2
|____/Message.hpp
/Shapes
|____/Asserts.h
|____/Maths.h
|____/Shape.h
/ShapesTests
|____/ShapesTests.cpp    