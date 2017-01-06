#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include "sfzparser.h"

int main() {
  SFZ* sfz;
  /*sfz_region s;
  s.volume = 3;
  sfz.add_region(s);
  s = sfz_region();
  sfz.add_region(s);
  */
  /*s.volume = 7;
  sfz.add_region(s);
  s.volume = 2;
  sfz.add_region(s);
  */

  /*std::vector<sfz_region>::iterator it;
  for (it = sfz.regions_begin(); it != sfz.regions_end(); ++it) {
    printf("region vol: %f\n", it->volume);
  }
  */

  JMZParser parser("fake.jmz");
  //JMZParser parser("fake.sfz");
  //SFZParser parser("/home/jdost/sounds/simple_grand/grand.sfz");
  sfz = parser.parse();

  JMZRegion jmz;
  jmz.jm_name = "bob";
  SFZRegion& jmz2 = jmz;
  printf("jmz name: %s\n", static_cast<JMZRegion&>(jmz2).jm_name.c_str());
  //jmz2.write(std::cout);

  sfz->write(std::cout);

  delete sfz;

  return 0;
}
