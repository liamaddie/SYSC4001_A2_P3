if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi
g++ -g -O0 -I . -o bin/interrupts interrupts.cpp
./bin/interrupts ./trace.txt vector_table.txt device_table.txt external_files.txt # easier to run the program