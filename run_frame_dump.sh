CAPTURE="/dev/video0"
OUTPUT=""

clang-format -i *.c *.h
gcc my_pipeline.c v4l2_helper.c conversion.c -g -D_GNU_SOURCE -lturbojpeg -o pipeline

echo "./test.exe $CAPTURE $OUTPUT"
./pipeline $CAPTURE $OUTPUT


echo "set args $CAPTURE $OUTPUT
show args" > .gdbinit
