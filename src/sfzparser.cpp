#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "sfzparser.h"

namespace {
  void validate_int(const std::string& op, long val, long min, long max) {
    if (val < min || val > max) {
      std::ostringstream sout;
      sout << op << " must be between " << min << " and " << max << ": " << val;
      throw std::runtime_error(sout.str());
    }
  }
};

void sfz::Value::write(std::ostream& out) const {
  switch (type) {
    case STRING:
      out << str;
      break;
    case INT:
      out << i;
      break;
    case DOUBLE:
      out << d;
      break;
  }
}

void sfz::write(const sfz* s, std::ostream& out) {
  out << "<control>";

  std::map<std::string, Value>::const_iterator it;

  for (it = s->control.begin(); it != s->control.end(); ++it) {
    out << " " << it->first << "=";
    it->second.write(out);
  }

  out << std::endl;

  std::vector<std::map<std::string, Value>>::const_iterator v_it;
  for (v_it = s->regions.begin(); v_it != s->regions.end(); ++v_it) {
    out << "<region>";
    for (it = v_it->begin(); it != v_it->end(); ++it) {
      out << " " << it->first << "=";
      it->second.write(out);
    }
    out << std::endl;
  }
}

void sfz::Parser::save_prev() {
  switch (state) {
    case CONTROL:
      update_control(*cur_control, cur_op, data);
      break;
    case GROUP:
      update_region(*cur_group, cur_op, data);
      break;
    case REGION:
      update_region(*cur_region, cur_op, data);
      break;
    default:
      break;
  }

  data.erase();
}

void sfz::Parser::set_region_defaults(std::map<std::string, Value>& region) {
  region["volume"] = 0.;
  region["pitch_keycenter"] = 32;
  region["lokey"] = 0;
  region["hikey"] = 127;
  region["lovel"] = 0;
  region["hivel"] = 127;
  region["tune"] = 0;
  region["offset"] = 0;
  // -1 to say not defined since may be defined in wav
  region["loop_start"] = -1;
  region["loop_end"] = -1;
  region["mode"] = LOOP_UNSET;
  region["loop_crossfade"] = 0.;
  region["group"] = 0;
  region["off_group"] = 0;
  region["ampeg_attack"] = 0.;
  region["ampeg_hold"] = 0.;
  region["ampeg_decay"] = 0.;
  region["ampeg_sustain"] = 100.;
  region["ampeg_release"] = 0.;
}

// missing some validation checks here
void sfz::Parser::update_region(std::map<std::string, Value>& region, const std::string& field, const std::string& data) {
  // generic double
  if (field == "volume" || field == "loop_crossfade" || field == "ampeg_attack" ||
      field == "ampeg_hold" || field == "ampeg_decay" || field == "ampeg_sustain" ||
      field == "ampeg_release")
    region[field] = strtod(data.c_str(), NULL);
  // int range 0-127
  else if (field == "pitch_keycenter" || field == "lokey" || field == "hikey" ||
      field == "lovel" || field == "hivel") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 127);
    region[field] = (int) val;
  }
  // int range -100-100
  else if (field == "tune") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, -100, 100);
    region[field] = (int) val;
  }
  // generic int
  else if (field == "offset" || field == "loop_start" || field == "loop_end" ||
      field == "group" || field == "off_group")
    region[field] = (int) strtol(data.c_str(), NULL, 10);
  // loop mode
  else if (field == "loop_mode") {
    if (data == "no_loop")
      region[field] = LOOP_OFF;
    else if (data == "loop_continuous")
      region[field] = LOOP_CONTINUOUS;
    else if (data == "one_shot")
      region[field] = LOOP_ONE_SHOT;
    else
      throw std::runtime_error("loop_mode must be \"no_loop\", \"loop_continuous\", or \"one_shot\"");
  }
  // sample path
  else if (field == "sample") {
    struct stat sb;
    std::string sample_path(dir_path);
    sample_path += data;

    // bail if stat fails or if you don't own file and others not allowed to read
    if (stat(sample_path.c_str(), &sb) || (sb.st_uid != getuid() && !(sb.st_mode & S_IROTH)))
      throw std::runtime_error("unable to access file: " + sample_path);

    if (!S_ISREG(sb.st_mode))
      throw std::runtime_error("not regular file: " + sample_path);

    region[field] = sample_path.c_str();
  }
  // don't know what it is; just pass through as string
  else {
    region[field] = data.c_str();
  }
}

sfz::sfz* sfz::Parser::parse() {
  char tmp_str[256];
  strcpy(tmp_str, path.c_str());
  dir_path += dirname(tmp_str);
  dir_path += "/";

  std::ifstream fin(path);
  sfz* s = new sfz;
  std::map<std::string, Value> cur_control;
  set_control_defaults(cur_control);
  std::map<std::string, Value> cur_group;
  set_region_defaults(cur_group);
  std::map<std::string, Value> cur_region = cur_group;
  this->cur_control = &cur_control;
  this->cur_group = &cur_group;
  this->cur_region = &cur_region;

  std::string line;
  while (std::getline(fin, line)) {
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
            s->regions.push_back(cur_region);
          else if (state == CONTROL)
            s->control = cur_control;
        }
        if (field == "<control>") {
          // reset cur_control
          cur_control = std::map<std::string, Value>();
          set_control_defaults(cur_control);
          state = CONTROL;
        }
        else if (field == "<group>") {
          // reset cur_group
          cur_group = std::map<std::string, Value>();
          set_region_defaults(cur_group);
          state = GROUP;
        }
        else if (field == "<region>") {
          // reset cur_region
          cur_region = cur_group;
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
      s->regions.push_back(cur_region);
    else if (state == CONTROL)
      s->control = cur_control;
  }

  fin.close();

  return s;
}

void sfz::JMZParser::set_control_defaults(std::map<std::string, Value>& control) {
  Parser::set_control_defaults(control);
  control["jm_vol"] = 16;
  control["jm_chan"] = 1;
}

void sfz::JMZParser::set_region_defaults(std::map<std::string, Value>& region) {
  Parser::set_region_defaults(region);
  region["jm_name"] = "";
}

void sfz::JMZParser::update_control(std::map<std::string, Value>& control, const std::string& field, const std::string& data) {
  if (field == "jm_vol") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 0, 16);
    control["jm_vol"] = (int) val;
  }
  else if (field == "jm_chan") {
    long val = strtol(data.c_str(), NULL, 10);
    validate_int(field, val, 1, 16);
    control["jm_chan"] = (int) val;
  }
  // don't know what it is; let parent handle it
  else
    Parser::update_control(control, field, data);
}

void sfz::JMZParser::update_region(std::map<std::string, Value>& region, const std::string& field, const std::string& data) {
  if (field == "jm_name")
    region["jm_name"] = data.c_str();
  // don't know what it is; let parent handle it
  else
    Parser::update_region(region, field, data);
}

