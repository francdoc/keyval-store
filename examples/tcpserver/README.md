#### Instructions to build & run.

1. Create in tcpserver a build folder. 
2. Enter it and run 'cmake ..' 
3. Run 'make' to generate the .bin file.
4. ./tcpserver.bin
5. valgrind --leak-check=full ./tcpserver.bin 
6. valgrind --leak-check=full --track-origins=yes ./tcpserver.bin