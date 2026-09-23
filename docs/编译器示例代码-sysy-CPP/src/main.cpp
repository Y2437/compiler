#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "backend/mips/codegen/codegen.hpp"
#include "backend/mips/opt/optimizer.hpp"
#include "backend/mips/regalloc/regalloc.hpp"
#include "error.hpp"
#include "frontend/ast.hpp"
#include "frontend/lexer.hpp"
#include "midend/irbuilder.hpp"
#include "midend/opt/engine.hpp"
#include "midend/opt/pass.hpp"
#include "midend/symbuilder.hpp"

ErrorCollector ec;

namespace {
constexpr std::string_view kDefaultInputFile = "testfile.txt";
constexpr std::string_view kTokensOutputFile = "lexer.txt";
constexpr std::string_view kAstOutputFile = "parser.txt";
constexpr std::string_view kSymbolOutputFile = "symbol.txt";
constexpr std::string_view kLlvmOutputFile = "llvm_ir.txt";
constexpr std::string_view kOptimizedLlvmOutputFile = "llvm_ir_opt.txt";
constexpr std::string_view kLoweredLlvmOutputFile = "llvm_ir_remove_phi.txt";
constexpr std::string_view kMipsOutputFile = "mips.txt";
constexpr std::string_view kErrorOutputFile = "error.txt";

struct DriverOptions {
  bool emit_tokens = false;
  bool emit_ast = false;
  bool emit_symbol = false;
  bool emit_llvm = false;
  bool emit_mips = false;
  bool optimize = false;
  bool show_help = false;

  std::string input_file = std::string(kDefaultInputFile);
  std::filesystem::path output_dir = ".";
};

void enable_all_outputs(DriverOptions& options) {
  options.emit_tokens = true;
  options.emit_ast = true;
  options.emit_symbol = true;
  options.emit_llvm = true;
  options.emit_mips = true;
  options.optimize = false;  // change it to enable opt
}

void print_usage(std::ostream& os) {
  os << "Usage: Compiler [options] [input-file]\n"
        "\n"
        "Options:\n"
        "  -T, --tokens       Output tokens\n"
        "  -A, --ast          Output AST\n"
        "  -S, --symbol       Output symbol table\n"
        "  -L, --llvm         Output LLVM IR\n"
        "  -M, --mips         Output MIPS\n"
        "  -O, --optimize     Enable middle-end and backend optimizations\n"
        "  -d, --dir <dir>    Output directory (default: current directory)\n"
        "  -h, --help         Show this help\n";
}

auto parse_args(int argc, char* argv[]) -> DriverOptions {
  DriverOptions options;
  if (argc == 1) {
    enable_all_outputs(options);
    return options;
  }

  bool has_input_file = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "-T" || arg == "--tokens") {
      options.emit_tokens = true;
      continue;
    }

    if (arg == "-A" || arg == "--ast") {
      options.emit_ast = true;
      continue;
    }

    if (arg == "-S" || arg == "--symbol") {
      options.emit_symbol = true;
      continue;
    }

    if (arg == "-L" || arg == "--llvm") {
      options.emit_llvm = true;
      continue;
    }

    if (arg == "-M" || arg == "--mips") {
      options.emit_mips = true;
      continue;
    }

    if (arg == "-O" || arg == "--optimize") {
      options.optimize = true;
      continue;
    }

    if (arg == "-d" || arg == "--dir") {
      ENSURE(i + 1 < argc, "Missing value for -d/--dir");
      options.output_dir = argv[++i];
      continue;
    }

    if (arg == "-h" || arg == "--help") {
      options.show_help = true;
      continue;
    }

    ENSURE(!arg.empty(), "Invalid empty argument");
    ENSURE(arg[0] != '-', "Unknown option: " + std::string(arg));
    ENSURE(!has_input_file, "Only one input file is supported");
    options.input_file = std::string(arg);
    has_input_file = true;
  }

  return options;
}

auto read_file(const std::string& path) -> std::string {
  std::ifstream input(path, std::ios::binary);
  ENSURE(input.is_open(), "Failed to open file: " + path);

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void write_file(const std::filesystem::path& path, std::string_view content) {
  std::ofstream output(path, std::ios::binary);
  ENSURE(output.is_open(), "Failed to open output file: " + path.string());
  output.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void write_output(const std::filesystem::path& output_dir,
                  std::string_view filename, std::string_view content) {
  write_file(output_dir / filename, content);
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    const DriverOptions options = parse_args(argc, argv);
    if (options.show_help) {
      print_usage(std::cout);
      return 0;
    }

    std::filesystem::create_directories(options.output_dir);
    const std::string source_code = read_file(options.input_file);

    frontend::lexer::Lexer lexer(source_code);
    if (options.emit_tokens) {
      std::ostringstream os;
      lexer.output_tokens(os);
      // write only if no error occurs during lexing
      if (!ec.has_error()) {
        write_output(options.output_dir, kTokensOutputFile, os.str());
      }
    }

    frontend::ast::CompUnit comp_unit;
    comp_unit.parse(lexer);

    if (options.emit_ast && !ec.has_error()) {
      std::ostringstream os;
      comp_unit.output_ast(os);
      write_output(options.output_dir, kAstOutputFile, os.str());
    }

    midend::visitor::SymbolBuilder symbol_builder;
    symbol_builder.build(comp_unit);

    if (options.emit_symbol && !ec.has_error()) {
      std::ostringstream os;
      symbol_builder.get_symbol_table().output_symbol_table(os);
      write_output(options.output_dir, kSymbolOutputFile, os.str());
    }

    if (ec.has_error()) {  // should be no error to generate llvm ir
      std::ostringstream os;
      os << ec;
      write_output(options.output_dir, kErrorOutputFile, os.str());
      // Source diagnostics are a successful compiler result for the judge;
      // non-zero exits are reserved for compiler/driver failures.
      return 0;
    }

    midend::visitor::IrBuilder ir_builder(
        std::move(symbol_builder).get_symbol_table());
    ir_builder.build(comp_unit);

    auto& module = ir_builder.get_module();
    if (options.emit_llvm) {
      std::ostringstream os;
      os << module.to_string();
      write_output(options.output_dir, kLlvmOutputFile, os.str());
    }

    if (options.optimize) {
      midend::opt::optimize(module);
      if (options.emit_llvm) {
        write_output(options.output_dir, kOptimizedLlvmOutputFile,
                     module.to_string());
      }
    }

    if (options.emit_mips) {
      if (options.optimize) {
        midend::opt::pass::ControlFlowGraphAnalysis cfg;
        cfg.run(module);
        midend::opt::pass::RemovePhi remove_phi;
        remove_phi.run(module);
        if (options.emit_llvm) {
          write_output(options.output_dir, kLoweredLlvmOutputFile,
                       module.to_string());
        }
      }
      backend::mips::CodeGenerator codegen;
      auto mips_module = codegen.generate(module);
      if (options.optimize) {
        backend::mips::MipsOptimizer optimizer(mips_module);
        optimizer.run_before_ra();
        backend::mips::GraphColoringRegisterAllocator reg_alloc;
        reg_alloc.allocate(mips_module);
      } else {
        backend::mips::SimpleRegisterAllocator reg_alloc;
        reg_alloc.allocate(mips_module);
      }

      write_output(options.output_dir, kMipsOutputFile,
                   mips_module.to_string());
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}
