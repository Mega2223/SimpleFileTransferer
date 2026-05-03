w=src/net.c src/utils.c src/main.c src/file.c

build:
	rm -r bin  >> /dev/null || true
	mkdir bin >> /dev/null
	gcc $w -o ./bin/main.out

debug:
	mkdir bin  >> /dev/null || true
	gcc $w -g3 -o ./bin/debug.out