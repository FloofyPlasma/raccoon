#include "raccoon/LLVMBackend.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

LLVMBackend::LLVMBackend() : builder(context) {}

LLVMBackend::~LLVMBackend() = default;

std::expected<void, std::vector<LLVMError>>
LLVMBackend::generate(const IRModule &ir_module) {
  errors.clear();
  value_map.clear();
  label_map.clear();

  module = std::make_unique<llvm::Module>(ir_module.name, context);

  module->setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));

  for (const auto &func : ir_module.functions) {
    llvm::Type *ret_type = convert_type(func->return_type);
    if (!ret_type) {
      continue;
    }

    std::vector<llvm::Type *> param_types;
    for (const auto &param : func->parameters) {
      llvm::Type *param_type = convert_type(param.type);
      if (!param_type) {
        continue;
      }
      param_types.push_back(param_type);
    }

    llvm::FunctionType *func_type =
        llvm::FunctionType::get(ret_type, param_types, false);

    llvm::Function *llvm_func = llvm::Function::Create(
        func_type, llvm::Function::ExternalLinkage, func->name, module.get());

    value_map[func->name] = llvm_func;
  }

  for (const auto &func : ir_module.functions) {
    generate_function(*func);
    if (!errors.empty()) {
      break;
    }
  }

  if (!errors.empty()) {
    return std::unexpected(errors);
  }

  std::string error_str;
  llvm::raw_string_ostream error_stream(error_str);
  if (llvm::verifyModule(*module, &error_stream)) {
    error(std::format("LLVM module verification failed: {}", error_str));
    return std::unexpected(errors);
  }

  return {};
}

void LLVMBackend::generate_function(const IRFunction &func) {
  llvm::Function *llvm_func =
      static_cast<llvm::Function *>(value_map[func.name]);

  if (!llvm_func) {
    error(std::format("Internal error: Function not found in value map: {}",
                      func.name));
    return;
  }

  std::unordered_map<std::string, llvm::Value *> saved_funcs;
  for (const auto &f : value_map) {
    if (auto *fn = llvm::dyn_cast<llvm::Function>(f.second)) {
      saved_funcs[f.first] = fn;
    }
  }
  value_map = saved_funcs;
  label_map.clear();

  for (const auto &block : func.basic_blocks) {
    llvm::BasicBlock *llvm_block =
        llvm::BasicBlock::Create(context, block->label, llvm_func);
    label_map[block->label] = llvm_block;
  }

  for (const auto &block : func.basic_blocks) {
    generate_basic_block(*block);
  }

  std::string error_str;
  llvm::raw_string_ostream error_stream(error_str);
  if (llvm::verifyFunction(*llvm_func, &error_stream)) {
    error(std::format("Function verification failed for '{}': {}", func.name,
                      error_str));
    return;
  }
}

void LLVMBackend::generate_basic_block(const IRBasicBlock &block) {
  llvm::BasicBlock *llvm_block = label_map[block.label];
  builder.SetInsertPoint(llvm_block);

  for (const auto &instr : block.instructions) {
    generate_instruction(*instr);
    if (!errors.empty()) {
      return;
    }
  }
}

void LLVMBackend::generate_instruction(const IRInstruction &instr) {
  switch (instr.opcode) {

  case IRInstruction::OpCode::ALLOCA: {
    llvm::Type *alloc_type = convert_type(instr.result.type);
    if (!alloc_type) {
      return;
    }

    llvm::AllocaInst *alloca =
        builder.CreateAlloca(alloc_type, nullptr, instr.result.name);

    value_map[instr.result.name] = alloca;
    break;
  }

  case IRInstruction::OpCode::LOAD: {
    llvm::Value *ptr = get_value(instr.operands[0]);
    if (!ptr) {
      return;
    }

    llvm::Type *load_type = convert_type(instr.result.type);
    if (!load_type) {
      return;
    }

    llvm::Value *loaded = builder.CreateLoad(load_type, ptr, instr.result.name);
    value_map[instr.result.name] = loaded;
    break;
  }

  case IRInstruction::OpCode::STORE: {
    llvm::Value *val = get_value(instr.operands[0]);
    llvm::Value *ptr = get_value(instr.operands[1]);
    if (!val || !ptr) {
      return;
    }

    builder.CreateStore(val, ptr);
    break;
  }

  case IRInstruction::OpCode::ADD: {
    llvm::Value *left = get_value(instr.operands[0]);
    llvm::Value *right = get_value(instr.operands[1]);
    if (!left || !right) {
      return;
    }

    llvm::Value *result = builder.CreateAdd(left, right, instr.result.name);
    value_map[instr.result.name] = result;
    break;
  }

  case IRInstruction::OpCode::SUB: {
    llvm::Value *left = get_value(instr.operands[0]);
    llvm::Value *right = get_value(instr.operands[1]);
    if (!left || !right) {
      return;
    }

    llvm::Value *result = builder.CreateSub(left, right, instr.result.name);
    value_map[instr.result.name] = result;
    break;
  }

  case IRInstruction::OpCode::MUL: {
    llvm::Value *left = get_value(instr.operands[0]);
    llvm::Value *right = get_value(instr.operands[1]);
    if (!left || !right) {
      return;
    }

    llvm::Value *result = builder.CreateMul(left, right, instr.result.name);
    value_map[instr.result.name] = result;
    break;
  }

  case IRInstruction::OpCode::SDIV: {
    llvm::Value *left = get_value(instr.operands[0]);
    llvm::Value *right = get_value(instr.operands[1]);
    if (!left || !right) {
      return;
    }

    llvm::Value *result = builder.CreateSDiv(left, right, instr.result.name);
    value_map[instr.result.name] = result;
    break;
  }

  case IRInstruction::OpCode::RET: {
    if (instr.operands.empty()) {
      builder.CreateRetVoid();
    } else {
      llvm::Value *ret_val = get_value(instr.operands[0]);
      if (!ret_val) {
        return;
      }
      builder.CreateRet(ret_val);
    }
    break;
  }

  case IRInstruction::OpCode::BR: {
    // Unconditional branch
    if (instr.operands.empty()) {
      error("BR instruction missing target lable");
      return;
    }
    llvm::BasicBlock *target = label_map[instr.operands[0].name];
    if (!target) {
      error(std::format("Unknown branch target: {}", instr.operands[0].name));
      return;
    }
    builder.CreateBr(target);
    break;
  }

  case IRInstruction::OpCode::BR_COND: {
    if (instr.operands.size() < 3) {
      error("BR_COND instruction missing operands");
      return;
    }
    llvm::Value *cond = get_value(instr.operands[0]);
    llvm::BasicBlock *true_block = label_map[instr.operands[1].name];
    llvm::BasicBlock *false_block = label_map[instr.operands[2].name];
    if (!cond || !true_block || !false_block) {
      error("BR_COND: invalid operands");
      return;
    }
    builder.CreateCondBr(cond, true_block, false_block);
    break;
  }

  default: {
    error(std::format("Unimplemented IR instruction: {}",
                      instr.opcode_to_string()));
    break;
  }
  }
}

llvm::Type *LLVMBackend::convert_type(const IRType &ir_type) {
  switch (ir_type.kind) {
  case IRType::Kind::I32: {
    return llvm::Type::getInt32Ty(context);
  }

  case IRType::Kind::VOID: {
    return llvm::Type::getVoidTy(context);
  }

  default: {
    error("Unknown IR type");
    return nullptr;
  }
  }
}

llvm::Value *LLVMBackend::get_value(const IRValue &ir_value) {
  switch (ir_value.kind) {
  case IRValue::Kind::CONSTANT: {
    return get_constant(ir_value);
  }

  case IRValue::Kind::REGISTER: {
    if (value_map.count(ir_value.name)) {
      return value_map[ir_value.name];
    }

    error(std::format("Undefined register: {}", ir_value.name));
    return nullptr;
  }

  default: {
    error("Unsupported IR value kind");
    return nullptr;
  }
  }
}

llvm::Constant *LLVMBackend::get_constant(const IRValue &ir_value) {
  switch (ir_value.type.kind) {
  case IRType::Kind::I32: {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                  ir_value.int_const);
  }

  default: {
    error("Unsupported constant type");
    return nullptr;
  }
  }
}

void LLVMBackend::error(const std::string &message) {
  errors.push_back(LLVMError{SourceLocation{}, message});
}

bool LLVMBackend::emit_object_file(const std::string &filename) {
  if (!module) {
    error("No module to emit (call generate() first");
    return false;
  }

  static bool targets_initialized = false;
  if (!targets_initialized) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    targets_initialized = true;
  }

  std::string target_triple = llvm::sys::getDefaultTargetTriple();

  std::string error_str;
  const auto *target =
      llvm::TargetRegistry::lookupTarget(target_triple, error_str);
  if (!target) {
    error(std::format("Failed to lookup target: {}", error_str));
    return false;
  }

  llvm::TargetOptions opt;
  auto target_machine =
      std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
          llvm::Triple(target_triple), "generic", "", opt, llvm::Reloc::PIC_));
  if (!target_machine) {
    error("Failed to create target machine");
    return false;
  }

  module->setDataLayout(target_machine->createDataLayout());

  std::error_code ec;
  llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
  if (ec) {
    error(std::format("Could not open file '{}': {}", filename, ec.message()));
    return false;
  }

  llvm::legacy::PassManager pass;
  if (target_machine->addPassesToEmitFile(pass, dest, nullptr,
                                          llvm::CodeGenFileType::ObjectFile)) {
    error("Target machine cannot emit object file");
    return false;
  }

  pass.run(*module);
  dest.flush();

  return true;
}

bool LLVMBackend::emit_llvm_ir(const std::string &filename) {
  if (!module) {
    error("No module to emit (call generate() first");
    return false;
  }

  std::error_code ec;
  llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
  if (ec) {
    error(std::format("Could not open file '{}': {}", filename, ec.message()));
    return false;
  }

  module->print(dest, nullptr);
  dest.flush();

  return true;
}