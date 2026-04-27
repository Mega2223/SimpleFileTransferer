i=0;

while pidof main.out; do
    export i=$($i + 1)
    echo $i
    pkill -$i main.out
    pidof main.out
    sleep 1
done
