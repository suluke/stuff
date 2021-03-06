#ifndef JNSN_STRING_TABLE_H
#define JNSN_STRING_TABLE_H
#include <set> // don't use unordered_set because elements might relocate
#include <string>
#include <string_view>

namespace jnsn {

class string_table;
class string_table_entry {
  friend class string_table;
  std::string_view text;
  // Having this constructor private is the whole point why we don't
  // just use an alias declaration for string_table_entry:
  // You can't just go and create one from a string literal, which helps
  // avoiding bugs where a programmer assumes a method works by-value
  // but the method relies on the pointer identity of the string, for
  // example.
  // FIXME find and document a real example where this matters
  string_table_entry(std::string_view text) : text(std::move(text)) {}

public:
  string_table_entry() = default;
  string_table_entry(const string_table_entry &) = default;
  string_table_entry(string_table_entry &&) = default;
  string_table_entry &operator=(const string_table_entry &) = default;
  string_table_entry &operator=(string_table_entry &&) = default;
  operator std::string_view() const { return text; }
  bool empty() const { return text.empty(); }
  const char *data() const { return text.data(); }
  std::string str() const { return std::string{data(), size()}; }
  std::string_view::size_type size() const noexcept { return text.size(); }
  std::string_view::iterator begin() const { return text.begin(); }
  std::string_view::iterator end() const { return text.end(); }
  bool operator==(const string_table_entry &o) const { return text == o.text; }
  bool operator<(const string_table_entry &o) const { return text < o.text; }
  const std::string_view *operator->() const { return &text; }
  friend std::ostream &operator<<(std::ostream &stream,
                                  const string_table_entry &entry) {
    return stream << entry.text;
  }
  friend bool operator==(const string_table_entry &entry,
                         const std::string_view &view) {
    return entry.text == view;
  }
  friend bool operator==(const std::string_view &view,
                         const string_table_entry &entry) {
    return entry.text == view;
  }
};

class string_table {
private:
  using container = std::set<std::string>;
  container table;

public:
  using entry = string_table_entry;
  using iterator = container::iterator;
  using const_iterator = container::const_iterator;
  entry get_handle(std::string s) {
    auto it_ins = table.insert(std::move(s));
    auto it = it_ins.first;
    return std::string_view{it->data(), it->size()};
  }
  iterator begin() { return table.begin(); }
  const_iterator begin() const { return table.begin(); }
  iterator end() { return table.end(); }
  const_iterator end() const { return table.end(); }
};

} // namespace jnsn
#endif // JNSN_STRING_TABLE_H
