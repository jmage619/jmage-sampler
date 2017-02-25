#ifndef SFZPARSER_H
#define SFZPARSER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <stdexcept>

class SFZValue;

namespace sfz {
  struct sfz {
    std::map<std::string, SFZValue> control;
    std::vector<std::map<std::string, SFZValue>> regions;
  };

  void write(const sfz* s, std::ostream& out);
};

class SFZValue {
  private:
    enum {
      STRING,
      INT,
      DOUBLE
    } type;

    std::string str;
    int i;
    double d;

  public:
    SFZValue(): type(STRING) {}
    SFZValue(const char* str): type(STRING), str(str) {}
    SFZValue(int i): type(INT), i(i) {}
    SFZValue(double d): type(DOUBLE), d(d) {};
    void write(std::ostream& out) const;
    const std::string& get_str() const {if (type != STRING)throw std::runtime_error("Value is not of type STRING");return str;}
    int get_int() const {if (type != INT)throw std::runtime_error("Value is not of type INT");return i;}
    double get_double() const {if (type != DOUBLE)throw std::runtime_error("Value is not of type DOUBLE");return d;}
};

class SFZParser {
  private:
    enum State {
      CONTROL,
      GROUP,
      REGION
    } state;

    std::string data;
    std::string cur_op;
    std::map<std::string, SFZValue>* cur_control;
    std::map<std::string, SFZValue>* cur_group;
    std::map<std::string, SFZValue>* cur_region;
    std::string path;
    std::string dir_path;

    void save_prev();

  protected:
    virtual void set_control_defaults(std::map<std::string, SFZValue>& /*control*/) {}
    virtual void set_region_defaults(std::map<std::string, SFZValue>& region);
    virtual void update_control(std::map<std::string, SFZValue>& /*control*/, const std::string& /*field*/, const std::string& /*data*/) {}
    virtual void update_region(std::map<std::string, SFZValue>& region, const std::string& field, const std::string& data);
  public:
    SFZParser(const std::string& path);
    virtual ~SFZParser(){}
    sfz::sfz* parse();
};

class JMZParser: public SFZParser {
  protected:
    virtual void set_control_defaults(std::map<std::string, SFZValue>& control);
    virtual void set_region_defaults(std::map<std::string, SFZValue>& region);
    virtual void update_control(std::map<std::string, SFZValue>& control, const std::string& field, const std::string& data);
    virtual void update_region(std::map<std::string, SFZValue>& region, const std::string& field, const std::string& data);
  public:
    JMZParser(const std::string& path): SFZParser(path) {}
};

#endif