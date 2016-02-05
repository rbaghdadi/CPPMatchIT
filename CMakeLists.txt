cmake_minimum_required(VERSION 3.3)
project(CPPMatchIT)

set(MD5_FLAGS "-I/usr/local/Cellar/openssl/1.0.2d_1/include/ -L/usr/local/Cellar/openssl/1.0.2d_1/lib/ -lssl -lcrypto")
set(LLVM_FLAGS "-I/Users/JRay/llvm-build/llvm-3.7.0.src/include -I/Users/JRay/llvm-build/llvm-3.7.0.src/build/include  -fPIC -fvisibility-inlines-hidden -Wall -W -Wno-unused-parameter -Wwrite-strings -Wcast-qual -Wmissing-field-initializers -pedantic -Wno-long-long -Wcovered-switch-default -Wnon-virtual-dtor -std=c++11 -O3  -fno-exceptions -fno-rtti  -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -L/Users/JRay/llvm-build/llvm-3.7.0.src/build/lib -Wl,-search_paths_first -Wl,-headerpad_max_install_names -lLLVMBitWriter -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMCodeGen -lLLVMScalarOpts -lLLVMProfileData -lLLVMInstCombine -lLLVMInstrumentation -lLLVMTransformUtils -lLLVMipa -lLLVMX86Desc -lLLVMMCDisassembler -lLLVMX86Info -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMMCJIT -lLLVMExecutionEngine -lLLVMTarget -lLLVMAnalysis -lLLVMRuntimeDyld -lLLVMObject -lLLVMMCParser -lLLVMBitReader -lLLVMMC -lLLVMCore -lLLVMSupport -lpthread -lz -lm -I/Users/JRay/llvm-build/llvm-3.7.0.src/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -v -std=c++11 ${MD5_FLAGS} ${LLVM_FLAGS} -lLLVMOrcJIT")

set(SOURCE_FILES apps/correlation/Stages.cpp src/MergeStages.h src/ComparisonStage.cpp src/FilterStage.h apps/file_dups/main.cpp src/TransformStage.cpp src/TransformStage.h src/Pipeline.cpp src/FilterStage.cpp src/Stage.cpp src/Stage.h src/MFunc.cpp src/MFunc.h src/JIT.cpp src/LLVM.cpp src/LLVM.h src/MType.h src/MType.cpp src/CodegenUtils.h src/CodegenUtils.cpp src/ForLoop.cpp src/ForLoop.h src/InstructionBlock.cpp src/InstructionBlock.h src/Utils.h src/Utils.cpp apps/file_dups/Stages.h apps/file_dups/Stages.cpp src/CompositeTypes.cpp src/SegmentationStage.cpp src/SegmentationStage.h)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY /Users/JRay/Documents/Research/CPPMatchIT/bin)
add_executable(CPPMatchIT ${SOURCE_FILES})
