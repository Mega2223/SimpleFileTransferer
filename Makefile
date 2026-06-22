w=src/net.c src/main.c src/file.c

build:
	rm -r bin  >> /dev/null || true
	mkdir bin >> /dev/null
	gcc -Wall $w -o ./bin/main.out

debug:
	mkdir bin  >> /dev/null || true
	gcc -Wall $w -g3 -o ./bin/debug.out

install:
	sudo install ./bin/main.out /bin/sft

