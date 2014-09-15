


0) 
    git clone git://github.com/cambridgehackers/llvm -b release_34atomicc1
    sudo apt-get install libffi-dev

1) cd llvm 
    mkdir build
    cd build
    source ../../llvm-translate/configure_llvm
    make -j10

2) cd llvm-translate
    make

3) cd atomicc/examples/echo
    make 

4) cd llvm-translate
    ./run
