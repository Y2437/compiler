#include "symtable.hpp"

#include "error.hpp"

namespace midend::symbol {
std::ostream& operator<<(std::ostream& os, SymbolPrintType type) {
  constexpr std::string_view kind_strings[] = {
#define X(enum, content) #content,
      TYPE_PRINT_TABLE
#undef X
  };
  auto index = static_cast<size_t>(type);
  os << ((index < static_cast<size_t>(SymbolPrintType::COUNT))
             ? kind_strings[index]
             : "?");
  return os;
}

SymbolPrintType VarSymbol::get_symbol_print_type() const {
  if (_is_const) {
    return SymbolPrintType::CONST_INT;
  }
  if (_is_static) {
    return SymbolPrintType::STATIC_INT;
  }
  return SymbolPrintType::INT;
}

SymbolPrintType ArraySymbol::get_symbol_print_type() const {
  if (_is_const) {
    return SymbolPrintType::CONST_INT_ARR;
  }
  if (_is_static) {
    return SymbolPrintType::STATIC_INT_ARR;
  }
  return SymbolPrintType::INT_ARR;
}

SymbolPrintType FuncSymbol::get_symbol_print_type() const {
  return returns_value_ ? SymbolPrintType::INT_FUNC
                        : SymbolPrintType::VOID_FUNC;
}
}  // namespace midend::symbol

namespace midend::symbol {
SymbolTable::SymbolTable() {
  // declare built-in functions and avoid index 0
  enter_scope();

  auto getint_symbol = std::make_shared<FuncSymbol>(true);
  insert("getint", std::move(getint_symbol));

  // global scope
  enter_scope();
}

void SymbolTable::enter_scope() {
  scopes.emplace_back(std::make_unique<Scope>(
      scope_level, std::unordered_map<std::string, SymbolPtr>()));
  scope_level++;
}

void SymbolTable::exit_scope() {
  ENSURE(scopes.size() > 1, "Cannot exit global scope");
  popped_scopes.emplace_back(std::move(scopes.back()));
  scopes.pop_back();
}

bool SymbolTable::current_scope_contains(const std::string& name) const {
  const auto& current_scope = scopes.back()->second;
  return current_scope.find(name) != current_scope.end();
}

void SymbolTable::insert(const std::string& name, SymbolPtr symbol) {
  symbol->set_present_index(++present_counter);
  symbol->set_name(name);
  auto& current_scope = scopes.back()->second;
  auto [_, inserted] = current_scope.emplace(name, std::move(symbol));
  ENSURE(inserted, "Failed to insert symbol");
}

auto SymbolTable::get(const std::string& name, int offset) const
    -> std::optional<SymbolPtr> {
  for (auto it = scopes.rbegin() + offset; it != scopes.rend(); ++it) {
    auto found = (*it)->second.find(name);
    if (found != (*it)->second.end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

void SymbolTable::insert_main() {
  // insert into built-in functions scope
  // since we skip the first scope when outputting symbol table
  auto main_symbol = std::make_shared<FuncSymbol>(true);
  main_symbol->set_name("main");
  auto [_, inserted] =
      scopes[0]->second.emplace("main", std::move(main_symbol));
  ENSURE(inserted, "Failed to insert main symbol");
}

bool SymbolTable::is_global() const { return scopes.size() == 2; }

void SymbolTable::output_symbol_table(std::ostream& os) const {
  std::vector<const Scope*> ordered_scopes;
  ordered_scopes.reserve(scopes.size() - 1 + popped_scopes.size());
  // skip first scope (built-in functions)
  for (size_t i = 1; i < scopes.size(); ++i) {
    ordered_scopes.push_back(scopes[i].get());
  }
  for (const auto& scope : popped_scopes) {
    ordered_scopes.push_back(scope.get());
  }
  std::sort(ordered_scopes.begin(), ordered_scopes.end(),
            [](const Scope* a, const Scope* b) { return a->first < b->first; });

  for (const auto* scope_ptr : ordered_scopes) {
    struct Entry {
      size_t present_index;
      std::string name;
      SymbolPrintType print_type;
    };
    std::vector<Entry> entries;
    entries.reserve(scope_ptr->second.size());
    for (const auto& [name, symbol] : scope_ptr->second) {
      entries.push_back(Entry{symbol->get_present_index(), name,
                              symbol->get_symbol_print_type()});
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) {
                return a.present_index < b.present_index;
              });
    for (const auto& entry : entries) {
      os << scope_ptr->first << ' ' << entry.name << ' ' << entry.print_type
         << '\n';
    }
    // os << '\n';
  }
}

void SymbolTable::prepare_replay() {
  std::sort(
      popped_scopes.begin(), popped_scopes.end(),
      [](const std::unique_ptr<Scope>& a, const std::unique_ptr<Scope>& b) {
        return a->first > b->first;
      });
}

void SymbolTable::replay_enter_scope() {
  ENSURE(!popped_scopes.empty(), "No scopes to replay");
  scopes.emplace_back(std::move(popped_scopes.back()));
  popped_scopes.pop_back();
}

void SymbolTable::replay_exit_scope() {
  ENSURE(scopes.size() > 1, "Cannot exit global scope");
  scopes.pop_back();
}
}  // namespace midend::symbol
