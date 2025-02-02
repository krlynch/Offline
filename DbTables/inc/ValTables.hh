#ifndef DbTables_ValTables_hh
#define DbTables_ValTables_hh

#include "Offline/DbTables/inc/DbTable.hh"
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

namespace mu2e {

class ValTables : public DbTable {
 public:
  class Row {
   public:
    Row(int tid, std::string name, std::string dbname, std::string create_time,
        std::string create_user) :
        _tid(tid),
        _name(name), _dbname(dbname), _create_time(create_time),
        _create_user(create_user) {}
    int tid() const { return _tid; }
    std::string const& name() const { return _name; }
    std::string const& dbname() const { return _dbname; }
    std::string const& create_time() const { return _create_time; }
    std::string const& create_user() const { return _create_user; }

   private:
    int _tid;
    std::string _name;
    std::string _dbname;
    std::string _create_time;
    std::string _create_user;
  };

  constexpr static const char* cxname = "ValTables";

  ValTables() :
      DbTable(cxname, "val.tables", "tid,name,dbname,create_time,create_user") {
  }

  const Row& rowAt(const std::size_t index) const { return _rows.at(index); }
  const Row& row(const int tid) const { return _rows.at(_index.at(tid)); }
  std::vector<Row> const& rows() const { return _rows; }
  std::size_t nrow() const override { return _rows.size(); };
  size_t size() const override {
    size_t b = baseSize() + sizeof(this) + nrow() * 44 + nrow() * nrow() / 2;
    for (auto const& r : _rows)
      b += r.name().capacity() + r.dbname().capacity();
    return b;
  };
  const std::string orderBy() const { return std::string("tid"); }

  void addRow(const std::vector<std::string>& columns) override {
    _rows.emplace_back(std::stoi(columns[0]), columns[1], columns[2],
                       columns[3], columns[4]);
    _index[_rows.back().tid()] = _rows.size() - 1;
  }

  void rowToCsv(std::ostringstream& sstream, std::size_t irow) const override {
    Row const& r = _rows.at(irow);
    sstream << r.tid() << ",";
    sstream << r.name() << ",";
    sstream << r.dbname() << ",";
    sstream << r.create_time() << ",";
    sstream << r.create_user();
  }

  virtual void clear() override {
    baseClear();
    _rows.clear();
  }

 private:
  std::vector<Row> _rows;
  std::map<int, std::size_t> _index;
};

};  // namespace mu2e
#endif
