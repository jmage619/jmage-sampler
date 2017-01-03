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

  //std::ifstream fin("fake.sfz");
  std::ifstream fin("fake.jmz");
  //std::ifstream fin("/home/jdost/sounds/simple_grand/grand.sfz");
  //SFZParser parser(&fin);
  JMZParser parser(&fin);
  sfz = parser.parse();

  /*const SFZRegion* jmz = new JMZRegion;
  SFZRegion* jmz2 = new JMZRegion(*static_cast<const JMZRegion*>(jmz));
  jmz2->write(std::cout);
  delete jmz2;
  delete jmz;
  */
  sfz->write(std::cout);
  fin.close();

  delete sfz;

  return 0;
}
