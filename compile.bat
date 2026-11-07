:: Using 32-bit MinGW
g++ -c -fPIC -O2 dllmain.cpp
g++ -shared -static-libgcc -static-libstdc++ dllmain.o -o TS2AlwaysStayThings.asi
pause
