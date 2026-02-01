#pragma once

#include <memory>
#include <string>

namespace llvm {
class Module;
class LLVMContext;
} // namespace llvm

struct IRModule;

class LLVMBackend {
public:
  LLVMBackend();
  ~LLVMBackend();

  std::unique_ptr<llvm::Module> generate(IRModule *ir_module);
  bool emit_object_file(const std::string &filename);

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};
