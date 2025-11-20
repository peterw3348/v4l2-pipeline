CAPTURE="/dev/video0"
OUTPUT="/dev/video10"

clang-format -i *.c *.h
gcc my_pipeline.c v4l2_helper.c -g -o test.exe

echo "./test.exe $CAPTURE $OUTPUT"
./test.exe $CAPTURE $OUTPUT


echo "set args $CAPTURE $OUTPUT
show args" > .gdbinit
