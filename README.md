0) branch llvm: mdk@sj9:~/workspace/llvm$ git checkout remotes/origin/release_34atomicc1 -b release_34atomicc1

1) cd ../llvm ; mkdir build; cd build; source ../../llvm-translate/configure_llvm; make

2) cd llvm-translate; make

3) cd atomicc/examples/echo; make 

4) cd llvm-translate; ./run
