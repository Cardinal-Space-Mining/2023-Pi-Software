width = 291
height = 149
port = 8080
smoothing = 1


release:
	clear
	g++ ./src/*.cpp -lz -lpthread -Wall -std=c++14 -Wextra -Wimplicit-fallthrough -flto -O3 -ffast-math -march=native  -Xlinker -o ./bin/server

debug:
	clear
	g++ ./src/*.cpp -lz -lpthread -std=c++14 -pg -Wall -Wextra -Wunused -g3 -O0 -o ./bin/server 

warn:
	clear
	g++ ./src/*.cpp -lz -lpthread -std=c++14 -Wall -Wextra -Wunused -g3 -Og -o ./bin/server -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Woverloaded-virtual \
	-Wredundant-decls -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default \
	-Wundef -Werror -Wno-unused

debug_run:
	gdb --args ./bin/server -p $(port) -h $(height) -w $(width) -v -t $(smoothing)

run:
	./bin/server -p $(port) -h $(height) -w $(width) -l -t $(smoothing)
test:
	clear
	python3 ../map_client/server_fuzzer.py
