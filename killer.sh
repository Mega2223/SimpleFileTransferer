i=1; while pidof main.out; do export i=3; echo 2; pkill -2 main.out; pidof main.out; sleep 1; done
