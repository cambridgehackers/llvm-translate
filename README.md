


0) 
    sudo apt-get install libffi-dev
    git clone git://github.com/cambridgehackers/llvm
    git clone git://github.com/cambridgehackers/clang
    cd llvm; git checkout release_37
    cd ../clang; git checkout release_37

1) cd llvm 
    mkdir build
    cd build
    source ../../llvm-translate/configure_llvm
    make -j10

2) cd llvm-translate
    ./configure

3) cd llvm-translate
    make

4) cd atomicc/examples/echo
    make 

5) cd llvm-translate
    ./run

6) examine output in:
    output.tmp foo.tmp.xc foo.tmp.h
