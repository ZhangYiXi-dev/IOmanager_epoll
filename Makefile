fiber:main.cpp common.cpp
	g++ -rdynamic -g main.cpp mutex.cpp common.cpp fiber.cpp log.cpp zyx_thread.cpp scheduler.cpp IOmanager.cpp -I ./include -lpthread -o io 