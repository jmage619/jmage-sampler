#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <sys/stat.h>
#include <unistd.h>

#include "sfzparser.h"

//    self.reg_write_order = ['volume', 'pitch_keycenter', 'lokey', 'hikey', 'lovel', 'hivel', 'tune', 'offset', 'loop_start', 'loop_end', 'loop_mode', 'loop_crossfade', 'group', 'off_group', 'ampeg_attack', 'ampeg_hold', 'ampeg_decay', 'ampeg_sustain', 'ampeg_release', 'sample']

void SFZRegion::write_fields(std::ostream& out) {
  out << " volume=" << volume;
  out << " pitch_keycenter=" << pitch_keycenter;
  out << " lokey=" << lokey;
  out << " hikey=" << hikey;
  out << " lovel=" << lovel;
  out << " hivel=" << hivel;
  out << " tune=" << tune;
  out << " offset=" << offset;
  out << " loop_start=" << loop_start;
  out << " loop_end=" << loop_end;
  out << " loop_mode=";
  switch (mode) {
    case LOOP_OFF:
      out << "no_loop";
      break;
    case LOOP_CONTINUOUS:
      out << "loop_continuous";
      break;
    case LOOP_ONE_SHOT:
      out << "one_shot";
      break;
  }
  out << " loop_crossfade=" << loop_crossfade;
  out << " group=" << group;
  out << " off_group=" << off_group;
  out << " ampeg_attack=" << ampeg_attack;
  out << " ampeg_hold=" << ampeg_hold;
  out << " ampeg_decay=" << ampeg_decay;
  out << " ampeg_sustain=" << ampeg_sustain;
  out << " ampeg_release=" << ampeg_release;
  out << " sample=" << sample;
}

SFZ::~SFZ() {
  if (control != NULL)
    delete control;

  std::vector<SFZRegion*>::iterator it;
  for (it = regions.begin(); it != regions.end(); ++it) {
    delete *it;
  }
}

void SFZ::write(std::ostream& out) {
  if (control != NULL) {
    control->write(out);
    out << std::endl;
  }

  std::vector<SFZRegion*>::iterator it;
  for (it = regions.begin(); it != regions.end(); ++it) {
    (*it)->write(out);
    out << std::endl;
  }
}

namespace {
  void validate_int(const std::string& op, long val, long min, long max) {
    if (val < min || val > max) {
      std::ostringstream sout;
      sout << op << " must be between " << min << " and " << max << ": " << val;
      throw std::runtime_error(sout.str());
    }
  }
};

// missing some validation checks here
void SFZParser::update_region(SFZRegion* region, const std::string& field, const std::string& data) {
  if (field == "volume")
    region->volume = strtod(data.c_str(), NULL);
  else if (field == "pitch_keycenter") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region->pitch_keycenter = val;
  }
  else if (field == "lokey") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region->lokey = val;
  }
  else if (field == "hikey") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region->hikey = val;
  }
  else if (field == "lovel") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region->lovel = val;
  }
  else if (field == "hivel") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region->hivel = val;
  }
  else if (field == "tune") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, -100, 100);
    region->tune = val;
  }
  else if (field == "offset")
    region->offset = strtol(data.c_str(), NULL, 10);
  else if (field == "loop_start")
    region->loop_start = strtol(data.c_str(), NULL, 10);
  else if (field == "loop_end")
    region->loop_end = strtol(data.c_str(), NULL, 10);
  else if (field == "loop_mode") {
    if (data == "no_loop")
      region->mode = LOOP_OFF;
    else if (data == "loop_continuous")
      region->mode = LOOP_CONTINUOUS;
    else if (data == "one_shot")
      region->mode = LOOP_ONE_SHOT;
    else
      throw std::runtime_error("loop_mode must be \"no_loop\", \"loop_continuous\", or \"one_shot\"");
  }
  else if (field == "loop_crossfade")
    region->loop_crossfade = strtod(data.c_str(), NULL);
  else if (field == "group")
    region->group = strtol(data.c_str(), NULL, 10);
  else if (field == "off_group")
    region->off_group = strtol(data.c_str(), NULL, 10);
  else if (field == "ampeg_attack")
    region->ampeg_attack = strtod(data.c_str(), NULL);
  else if (field == "ampeg_hold")
    region->ampeg_hold = strtod(data.c_str(), NULL);
  else if (field == "ampeg_decay")
    region->ampeg_decay = strtod(data.c_str(), NULL);
  else if (field == "ampeg_sustain")
    region->ampeg_sustain = strtod(data.c_str(), NULL);
  else if (field == "ampeg_release")
    region->ampeg_release = strtod(data.c_str(), NULL);
  else if (field == "sample") {
    struct stat sb;

    // bail if stat fails or if you don't own file and others not allowed to read
    if (stat(data.c_str(), &sb) || (sb.st_uid != getuid() && !(sb.st_mode & S_IROTH)))
      throw std::runtime_error("unable to access file: " + data);

    if (!S_ISREG(sb.st_mode))
      throw std::runtime_error("not regular file: " + data);

    region->sample = data;
  }
}

void SFZParser::save_prev() {
  switch (state) {
    case CONTROL:
      update_control(control, cur_op, data);
      break;
    case GROUP:
      update_region(cur_group, cur_op, data);
      break;
    case REGION:
      update_region(cur_region, cur_op, data);
      break;
    default:
      break;
  }

  data.erase();
}

// i am too lazy to consider a SFZ copy constructor / assignment op
// so we just return a pointer rather than an object copy
SFZ* SFZParser::parse() {
  SFZ* sfz = new SFZ;
  control = new_control();
  cur_group = new_region();
  cur_region = new_region(cur_group);

  std::string line;
  while (std::getline(*in, line)) {
    // strip comment
    size_t pos = line.find("//");
    if (pos != std::string::npos)
      line.erase(pos);

    // split line by space
    std::istringstream sin(line);
    std::string field;
    while (sin >> field) {
      // either a new tag
      if (field[0] == '<' && field[field.length() - 1] == '>') {
        if (data.length() > 0) {
          save_prev();
          if (state == REGION)
            // create copy of region for sfz to prevent ownership conflict
            sfz->add_region(new_region(cur_region));
          else if (state == CONTROL)
            // create copy of control for sfz to prevent ownership conflict
            sfz->add_control(new_control(control));
        }
        if (field == "<control>") {
          // reset control
          delete control;
          control = new_control();
          state = CONTROL;
        }
        else if (field == "<group>") {
          // reset cur_group
          delete cur_group;
          cur_group = new_region();
          state = GROUP;
        }
        else if (field == "<region>") {
          // reset cur_region
          delete cur_region;
          cur_region = new_region(cur_group);
          state = REGION;
        }
      }
      // or new op code
      else if ((pos = field.find('=')) != std::string::npos) {
        if (data.length() > 0)
          save_prev();

        cur_op = field.substr(0, pos);
        data += field.substr(pos + 1);
      }
      // or continuing space separated data for prev op code
      else {
        data += " ";
        data += field;
      }
    }
  }
  // save last region or control if data left over
  if (data.length() > 0) {
    save_prev();
    if (state == REGION)
      sfz->add_region(new_region(cur_region));
    else if (state == CONTROL)
      sfz->add_control(new_control(control));
  }

  delete control;
  delete cur_group;
  delete cur_region;

  return sfz;
}

void JMZControl::write_fields(std::ostream& out) {
  out << " jm_vol=" << jm_vol;
  out << " jm_chan=" << jm_chan;
  SFZControl::write_fields(out);
}

void JMZRegion::write_fields(std::ostream& out) {
  out << " jm_name=" << jm_name;
  SFZRegion::write_fields(out);
}

void JMZParser::update_control(SFZControl* control, const std::string& field, const std::string& data) {
  if (field == "jm_vol") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 16);
    static_cast<JMZControl*>(control)->jm_vol = val;
  }
  else if (field == "jm_chan") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 1, 16);
    static_cast<JMZControl*>(control)->jm_chan = val;
  }
  else
    SFZParser::update_control(control, field, data);
}

void JMZParser::update_region(SFZRegion* region, const std::string& field, const std::string& data) {
  if (field == "jm_name")
    static_cast<JMZRegion*>(region)->jm_name = data;
  else
    SFZParser::update_region(region, field, data);
}
