cmake_minimum_required(VERSION 3.3)
project(CPPMatchIT)

set(MD5_FLAGS "-I/usr/local/Cellar/openssl/1.0.2d_1/include/ -L/usr/local/Cellar/openssl/1.0.2d_1/lib/ -lssl -lcrypto")
set(FFTW_FLAGS "-lfftw3f")
set(LLVM_FLAGS "-I/Users/JRay/llvm-build/llvm-3.7.0.src/include -I/Users/JRay/llvm-build/llvm-3.7.0.src/build/include  -fPIC -fvisibility-inlines-hidden -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wmissing-field-initializers -pedantic -Wno-long-long -Wcovered-switch-default -Wnon-virtual-dtor -std=c++11 -fno-exceptions -fno-rtti  -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -L/Users/JRay/llvm-build/llvm-3.7.0.src/build/lib -Wl,-search_paths_first -Wl,-headerpad_max_install_names -lLLVMOrcJIT -lLLVMBitWriter -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMCodeGen -lLLVMScalarOpts -lLLVMProfileData -lLLVMInstCombine -lLLVMInstrumentation -lLLVMTransformUtils -lLLVMipa -lLLVMX86Desc -lLLVMMCDisassembler -lLLVMX86Info -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMMCJIT -lLLVMExecutionEngine -lLLVMTarget -lLLVMAnalysis -lLLVMRuntimeDyld -lLLVMObject -lLLVMMCParser -lLLVMBitReader -lLLVMMC -lLLVMCore -lLLVMSupport -lpthread -lz -lm -I/Users/JRay/llvm-build/llvm-3.7.0.src/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -v -std=c++11 ${MD5_FLAGS} ${LLVM_FLAGS} ${FFTW_FLAGS} -DPRINT_MALLOC")

set(SOURCE_FILES src/Preallocator.cpp src/MType.cpp src/JIT.cpp src/Utils.cpp src/LLVM.cpp src/TransformStage.cpp src/FilterStage.cpp src/SegmentationStage.cpp src/InstructionBlock.cpp src/MFunc.cpp src/Stage.cpp src/CodegenUtils.cpp src/ForLoop.cpp src/Pipeline.cpp src/ComparisonStage.cpp src/StageFactory.cpp src/Field.cpp)
set(FILEDUPS_SOURCE apps/FileDups/Driver.cpp)
set(NW_SOURCE apps/NeedlemanWunsch/NW.cpp)
set(NWMatchIT_SOURCE apps/NeedlemanWunsch/NWMatchIT.cpp)
set(MF_SOURCE apps/MatchedFilter/MatchedFilter.cpp)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /Users/JRay/Desktop/CPPMatchIT/bin/)
add_executable(FileDups ${SOURCE_FILES} ${FILEDUPS_SOURCE})
add_executable(NW ${SOURCE_FILES} ${NW_SOURCE})
add_executable(NWMatchIT ${SOURCE_FILES} ${NWMatchIT_SOURCE})
add_executable(MF ${SOURCE_FILES} ${MF_SOURCE})
