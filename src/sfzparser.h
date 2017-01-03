#ifndef JM_SFZ_PARSER_H
#define JM_SFZ_PARSER_H

#include <string>
#include <vector>
#include <iostream>

#include "jmage/sampler.h"

class SFZControl {
  protected:
    virtual void write_fields(std::ostream& out) {}

  public:
    virtual ~SFZControl() {}
    void write(std::ostream& out) {out << "<control>";write_fields(out);}
};

class SFZRegion {
  protected:
    virtual void write_fields(std::ostream& out);

  public:
    double volume;
    int pitch_keycenter;
    int lokey;
    int hikey;
    int lovel;
    int hivel;
    int tune;
    int offset;
    int loop_start;
    int loop_end;
    loop_mode mode;
    float loop_crossfade;
    int group;
    int off_group;
    double ampeg_attack;
    double ampeg_hold;
    double ampeg_decay;
    double ampeg_sustain;
    double ampeg_release;
    std::string sample;

    SFZRegion():
      volume(0.),
      pitch_keycenter(32),
      lokey(0),
      hikey(127),
      lovel(0),
      hivel(127),
      tune(0),
      offset(0),
      loop_start(0),
      loop_end(0),
      mode(LOOP_OFF),
      loop_crossfade(0.),
      group(0),
      off_group(0),
      ampeg_attack(0.),
      ampeg_hold(0.),
      ampeg_decay(0.),
      ampeg_sustain(100.),
      ampeg_release(0.)
    {}
    virtual ~SFZRegion(){}

    void write(std::ostream& out) {out << "<region>";write_fields(out);}
};

class SFZ {
  private:
    std::vector<SFZRegion*> regions;
    SFZControl* control;

  public:
    SFZ(): control(NULL) {}
    ~SFZ();
    void add_region(SFZRegion* r) {regions.push_back(r);}
    void add_control(SFZControl* c) {control = c;}
    std::vector<SFZRegion*>::iterator regions_begin() {return regions.begin();}
    std::vector<SFZRegion*>::iterator regions_end() {return regions.end();}

    void write(std::ostream& out);
};

class SFZParser {
  private:
    enum State {
      CONTROL,
      GROUP,
      REGION
    } state;

    SFZControl* control;
    SFZRegion* cur_group;
    SFZRegion* cur_region;
    std::istream* in;

    void save_prev();

  protected:
    std::string data;
    std::string cur_op;
    virtual SFZControl* new_control() {return new SFZControl;}
    virtual SFZRegion* new_region() {return new SFZRegion;}
    virtual SFZRegion* new_region(const SFZRegion* region) {return new SFZRegion(*region);}
    virtual void update_control(SFZControl* control) {}
    virtual void update_region(SFZRegion* region);
  public:
    SFZParser(std::istream* in): cur_group(NULL), in(in) {}
    ~SFZParser() {if (cur_group != NULL) delete cur_group;}
    SFZ* parse();
};

class JMZControl: public SFZControl {
  protected:
    virtual void write_fields(std::ostream& out);

  public:
    int jm_vol;
    int jm_chan;

    JMZControl(): jm_vol(16), jm_chan(1) {}
};

class JMZRegion: public SFZRegion {
  protected:
    virtual void write_fields(std::ostream& out);

  public:
    std::string jm_name;
};

class JMZParser: public SFZParser {
  protected:
    virtual SFZControl* new_control() {return new JMZControl;}
    virtual SFZRegion* new_region() {return new JMZRegion;}
    virtual SFZRegion* new_region(const SFZRegion* region) {return new JMZRegion(*static_cast<const JMZRegion*>(region));}
    virtual void update_control(SFZControl* control);
    virtual void update_region(SFZRegion* region);
  public:
    JMZParser(std::istream* in): SFZParser(in) {}
};

#endif
