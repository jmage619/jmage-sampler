#ifndef JM_SFZ_PARSER_H
#define JM_SFZ_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <exception>

#include "jmage/sampler.h"

namespace sfz {
  class Value {
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
      Value(): type(STRING) {}
      Value(const char* str): type(STRING), str(str) {}
      Value(int i): type(INT), i(i) {}
      Value(double d): type(DOUBLE), d(d) {};
      void write(std::ostream& out) const;
      const char* get_str() {if (type != STRING)throw std::runtime_error("Value is not of type STRING");return str.c_str();}
      int get_int() {if (type != INT)throw std::runtime_error("Value is not of type INT");return i;}
      double get_double() {if (type != DOUBLE)throw std::runtime_error("Value is not of type DOUBLE");return d;}
  };

  struct sfz {
    std::map<std::string, Value> control;
    std::vector<std::map<std::string, Value>> regions;
  };

  void write(const sfz* s, std::ostream& out);

  class Parser {
    private:
      enum State {
        CONTROL,
        GROUP,
        REGION
      } state;

      std::string data;
      std::string cur_op;
      std::map<std::string, Value>* cur_control;
      std::map<std::string, Value>* cur_group;
      std::map<std::string, Value>* cur_region;
      std::string path;
      std::string dir_path;

      void save_prev();

    protected:
      virtual void set_control_defaults(std::map<std::string, Value>& control) {}
      virtual void set_region_defaults(std::map<std::string, Value>& region);
      virtual void update_control(std::map<std::string, Value>& control, const std::string& field, const std::string& data) {}
      virtual void update_region(std::map<std::string, Value>& region, const std::string& field, const std::string& data);
    public:
      Parser(const std::string& path): path(path) {}
      virtual ~Parser(){}
      sfz* parse();
  };

  class JMZParser: public Parser {
    protected:
      virtual void set_control_defaults(std::map<std::string, Value>& control);
      virtual void set_region_defaults(std::map<std::string, Value>& region);
      virtual void update_control(std::map<std::string, Value>& control, const std::string& field, const std::string& data);
      virtual void update_region(std::map<std::string, Value>& region, const std::string& field, const std::string& data);
    public:
      JMZParser(const std::string& path): Parser(path) {}
  };
};


#endif
