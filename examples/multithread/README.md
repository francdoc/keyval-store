#### Instructions to build & run.

1. Create in multithread a build folder. 
2. Enter it and run 'cmake ..' 
3. Run 'make' to generate the .bin file.
4. ./multithread.bin
5. valgrind --leak-check=full ./multithread.bin 
6. valgrind --leak-check=full --track-origins=yes ./multithread.bin
7. valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./multithread.bin