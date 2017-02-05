#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include "sfzparser.h"

int main() {
  /*
  sfz::sfz s;
  sfz::Value v = "fags";
  printf("%s\n", v.get_str());
  s.control["jm_vol"] = 12;
  s.control["jm_chan"] = 2;

  std::map<std::string, sfz::Value> region;
  region["sample"] = "test1.wav";
  region["volume"] = 3.5;
  s.regions.push_back(region);

  region["sample"] = "test2.wav";
  region["volume"] = 7.;
  s.regions.push_back(region);

  sfz::sfz s2 = s;
  sfz::write(s2, std::cout);
  */

  //sfz::sfz* s = sfz::Parser("/home/jdost/sounds/simple_grand/grand.sfz").parse();
  sfz::sfz* s = sfz::JMZParser("fake.jmz").parse();
  //sfz::sfz* s = sfz::JMZParser("fake.sfz").parse();
  sfz::write(s, std::cout);
  delete s;

  return 0;
}
