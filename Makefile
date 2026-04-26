
build:
	rm -r bin || true
	mkdir bin
	gcc src/net.c src/utils.c src/main.c -o ./bin/main.out