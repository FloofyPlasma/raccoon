#include "raccoon/Linker.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <lld/Common/Driver.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)

bool Linker::link(const std::string &object_file,
                  const std::string &output_file) {
  return link(std::vector<std::string>{object_file}, output_file);
}

bool Linker::link(const std::vector<std::string> &object_files,
                  const std::string &output_file) {
  errors.clear();

  if (object_files.empty()) {
    error("No object files to link");
    return false;
  }

  if (link_with_lld_library(object_files, output_file)) {
    return true;
  }

  return link_with_system_command(object_files, output_file);
}

void Linker::error(const std::string &message) {
  errors.push_back(LinkerError{message});
}

std::vector<std::string> Linker::find_crt_files() {
  if (paths_cached && !cached_crt_files.empty()) {
    return cached_crt_files;
  }

  auto files = try_common_crt_paths();
  if (!files.empty()) {
    cached_crt_files = files;
    paths_cached = true;
    return files;
  }

  files = query_crt_files_from_compiler();
  if (!files.empty()) {
    cached_crt_files = files;
    paths_cached = true;
    return files;
  }

  return {};
}

std::vector<std::string> Linker::try_common_crt_paths() {
  std::vector<std::string> search_paths = {
      "/usr/lib/x86_64-linux-gnu",  "/usr/lib64",
      "/lib/x86_64-linux-gnu",      "/lib64",
      "/usr/lib/aarch64-linux-gnu", "/usr/lib/arm-linux-gnueabihf"};

  for (const auto &path : search_paths) {
    std::filesystem::path crt1 = std::filesystem::path(path) / "crt1.o";
    std::filesystem::path crti = std::filesystem::path(path) / "crti.o";
    std::filesystem::path crtn = std::filesystem::path(path) / "crtn.o";

    if (std::filesystem::exists(crt1) && std::filesystem::exists(crti) &&
        std::filesystem::exists(crtn)) {
      return {crt1.string(), crti.string(), crtn.string()};
    }
  }

  return {};
}

std::vector<std::string> Linker::query_crt_files_from_compiler() {
  if (has_command("clang")) {
    std::string crt1 = run_command("clang -print-file-name=crt1.o");
    std::string crti = run_command("clang -print-file-name=crti.o");
    std::string crtn = run_command("clang -print-file-name=crtn.o");

    if (crt1 != "crt1.o" && std::filesystem::exists(crt1) && crti != "crti.o" &&
        std::filesystem::exists(crti) && crtn != "crtn.o" &&
        std::filesystem::exists(crtn)) {
      return {crt1, crti, crtn};
    }
  }

  if (has_command("gcc")) {
    std::string crt1 = run_command("gcc -print-file-name=crt1.o");
    std::string crti = run_command("gcc -print-file-name=crti.o");
    std::string crtn = run_command("gcc -print-file-name=crtn.o");

    if (crt1 != "crt1.o" && std::filesystem::exists(crt1) && crti != "crti.o" &&
        std::filesystem::exists(crti) && crtn != "crtn.o" &&
        std::filesystem::exists(crtn)) {
      return {crt1, crti, crtn};
    }
  }

  return {};
}

std::vector<std::string> Linker::find_library_paths() {
  if (paths_cached && !cached_library_paths.empty()) {
    return cached_library_paths;
  }

  std::vector<std::string> paths;

  std::vector<std::string> search_paths = {"/usr/lib/x86_64-linux-gnu",
                                           "/usr/lib64",
                                           "/lib/x86_64-linux-gnu",
                                           "/lib64",

                                           "/usr/lib/aarch64-linux-gnu",
                                           "/usr/lib/arm-linux-gnueabihf",

                                           "/usr/lib",
                                           "/usr/local/lib"};

  for (const auto &path : search_paths) {
    if (std::filesystem::exists(path)) {
      paths.push_back(path);
    }
  }

  cached_library_paths = paths;
  return paths;
}

std::string Linker::find_dynamic_linker() {
  if (paths_cached && !cached_dynamic_linker.empty()) {
    return cached_dynamic_linker;
  }

  std::vector<std::string> dynamic_linkers = {
      "/lib64/ld-linux-x86-64.so.2",
      "/lib/ld-linux-x86-64.so.2",
      "/lib/ld-linux-aarch64.so.1",
      "/lib/ld-linux-armhf.so.3",
  };

  for (const auto &linker : dynamic_linkers) {
    if (std::filesystem::exists(linker)) {
      cached_dynamic_linker = linker;
      return linker;
    }
  }

  return "";
}

bool Linker::link_with_lld_library(const std::vector<std::string> &object_files,
                                   const std::string &output_file) {
  std::vector<std::string> arg_storage;
  std::vector<const char *> args;

  args.push_back("ld.lld");

  for (const auto &obj : object_files) {
    args.push_back(obj.c_str());
  }

  args.push_back("-o");
  args.push_back(output_file.c_str());

  auto crt_files = find_crt_files();
  if (crt_files.empty()) {
    error("Could not find C runtime files (crt1.o, crti.o, crtn.o).\n"
          "You can link manually with:\n"
          "  clang " +
          object_files[0] + " -o " + output_file);
    return false;
  }

  for (const auto &crt : crt_files) {
    args.push_back(crt.c_str());
  }

  for (const auto &path : find_library_paths()) {
    arg_storage.push_back("-L" + path);
    args.push_back(arg_storage.back().c_str());
  }

  args.push_back("-lc");

  std::string dynamic_linker = find_dynamic_linker();
  if (!dynamic_linker.empty()) {
    args.push_back("-dynamic-linker");
    args.push_back(dynamic_linker.c_str());
  }

  std::string stdout_str, stderr_str;
  llvm::raw_string_ostream stdout_os(stdout_str);
  llvm::raw_string_ostream stderr_os(stderr_str);

  lld::Result res = lld::lldMain(llvm::ArrayRef<const char*>(args.data(), args.size()), stdout_os, stderr_os, LLD_ALL_DRIVERS);

  if (res.retCode != 0) {
    error("LLD linking failed: " + stderr_str);
    return false;
  }

  if (!res.canRunAgain) {
    error("LLD entered an unrecoverable state; process should exit.");
    return false;
  }

  return true;
}

bool Linker::link_with_system_command(
    const std::vector<std::string> &object_files,
    const std::string &output_file) {

  std::vector<std::string> linkers = {"clang", "gcc"};

  for (const auto &linker : linkers) {
    if (!has_command(linker)) {
      continue;
    }

    std::ostringstream cmd;
    cmd << linker;

    for (const auto &obj : object_files) {
      cmd << " " << obj;
    }

    cmd << " -o " << output_file;

    int result = std::system(cmd.str().c_str());

    if (result == 0) {
      return true;
    }
  }

  error("No suitable linker found (tried: clang, gcc)");
  return false;
}

bool Linker::has_command(const std::string &command) {
  std::string check_cmd = "command -v " + command + " >/dev/null 2>&1";
  return std::system(check_cmd.c_str()) == 0;
}

std::string Linker::run_command(const std::string &command) {
  FILE *pipe = popen((command + " 2>/dev/null").c_str(), "r");
  if (!pipe)
    return "";

  char buffer[256];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
  }
  pclose(pipe);

  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }

  return result;
}