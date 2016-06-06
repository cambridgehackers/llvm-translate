


0)

    sudo apt-get install libffi-dev libblocksruntime-dev clang
    git clone git://github.com/cambridgehackers/llvm
    git clone git://github.com/cambridgehackers/clang
    cd clang; git checkout remotes/origin/release_37atomicc1 -b release_37atomicc1
    cd ../llvm; git checkout remotes/origin/release_37atomicc1 -b release_37atomicc1

1) cd llvm 

    mkdir build
    cd build
    source ../configure_atomicc
    make -j10

2) cd atomicc/examples/rulec

    make 
    make run

3) examine output in:

    ls *.generated.*

----------------

8) Klee

    git clone https://github.com/niklasso/minisat
    cmake -DBUILD_SHARED_LIBS:BOOL=OFF ..

    git clone https://github.com/stp/stp.git
    cmake -DBUILD_SHARED_LIBS:BOOL=OFF -DENABLE_PYTHON_INTERFACE:BOOL=OFF -DNO_BOOST:BOOL=ON ..

