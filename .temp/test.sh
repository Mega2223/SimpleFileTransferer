ep 0.1
done
#!/bin/bash

shell=kitty

make build debug 
./bin/debug.out --port 25564 --receiver --file .temp/t --once &
sleep 3
./bin/debug.out --port 25564 --sender --file . --net-de