#pragma once

#include <string>
#include <vector>

struct LinkerError {
  std::string message;
};

class Linker {
public:
  Linker() = default;
  ~Linker() = default;

  bool link(const std::vector<std::string>& object_files, const std::string &output_file);

  bool link(const std::string &object_file, const std::string &output_file);

  bool has_errors() const {return !errors.empty();}
  const std::vector<LinkerError> &get_errors() const {return errors;}

private:
  std::vector<LinkerError> errors;

  std::vector<std::string> cached_crt_files;
  std::vector<std::string> cached_library_paths;
  std::string cached_dynamic_linker;
  bool paths_cached = false;

  void error(const std::string &message);

  std::vector<std::string> find_crt_files();
  std::vector<std::string> try_common_crt_paths();
  std::vector<std::string> query_crt_files_from_compiler();

  std::vector<std::string> find_library_paths();
  std::string find_dynamic_linker();

  bool link_with_lld_library(const std::vector<std::string> &object_files, const std::string &output_file);

  bool link_with_system_command(const std::vector<std::string> &object_files, const std::string& output_file);
  bool has_command(const std::string &command);

  std::string run_command(const std::string& command);
};