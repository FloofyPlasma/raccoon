#include "raccoon/LLVMBackend.hpp"
#include "raccoon/IR.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class LLVMBackend::Impl {
public:
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
};

LLVMBackend::LLVMBackend() : pimpl(std::make_unique<Impl>()) {}

LLVMBackend::~LLVMBackend() = default;

std::unique_ptr<llvm::Module> LLVMBackend::generate(IRModule *ir_module) {
  (void)ir_module;

  pimpl->module = std::make_unique<llvm::Module>("stub_module", pimpl->context);

  return std::move(pimpl->module);
}

bool LLVMBackend::emit_object_file(const std::string &filename) {
  (void)filename;

  return false;
}
