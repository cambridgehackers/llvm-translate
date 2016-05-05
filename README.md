


0)

    sudo apt-get install libffi-dev libblocksruntime-dev
    git clone git://github.com/cambridgehackers/llvm
    git clone git://github.com/cambridgehackers/clang
    cd llvm; git checkout remotes/origin/release_37atomicc1 -b release_37atomicc1
    cd ../clang; git checkout remotes/origin/release_37atomicc1 -b release_37atomicc1

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

----------------

8) Klee

    git clone https://github.com/niklasso/minisat
    git clone https://github.com/stp/stp.git

    cmake -DBUILD_SHARED_LIBS:BOOL=OFF -DENABLE_PYTHON_INTERFACE:BOOL=OFF ..
    cmake -DBUILD_SHARED_LIBS:BOOL=OFF ..
