import sys

VALID_SECTIONS = set(['group', 'region'])

DEFAULT_VALUES = {
  'lokey': 0,
  'hikey': 127,
  'lovel': 0,
  'hivel': 127,
  'pitch_keycenter': 32,
  'tune': 0,
  'offset': 0,
  # important to NOT ever have defaults for loop_mode, start, and end
  # we do this since they may be set inside the wav file
  # ideal prio would be: sfz default -> wav -> sfz override
  # current sampler zone creation doesn't allow this
  'loop_crossfade': 0.0,
  'ampeg_attack': 0.0, 
  'ampeg_hold': 0.0,
  'ampeg_decay': 0.0,
  'ampeg_sustain': 100.0,
  'ampeg_release': 0.0
}

REQUIRED_KEYS = set(['sample'])

WRITE_ORDER = ['pitch_keycenter', 'lokey', 'hikey', 'lovel', 'hivel', 'tune', 'offset', 'loop_start', 'loop_end', 'loop_mode', 'loop_crossfade', 'ampeg_attack', 'ampeg_hold', 'ampeg_decay', 'ampeg_sustain', 'ampeg_release', 'sample']

class SFZ(object):
  def __init__(self, regions=[]):
    self.regions = regions

  def add_region(self, region):
    reg = dict(DEFAULT_VALUES)
    reg.update(region)
    self.regions.append(reg)

  def write(self, out):
    for reg in self.regions:
      out.write("<region>")
      for k in WRITE_ORDER:
        out.write(" %s=%s" % (k, reg[k]))
      out.write('\n')
    
class SFZParser(object):
  def __init__(self, file):
    self.file = file
    self.in_section = False
    self.data = []
    self.cur_group = {}
    self.cur_section = None
    self.regions = []

  def close(self):
    self.file.close()

  def parse_fields(self):
    field_dict = {}
    fields = ''.join(self.data).split()
    for field in fields:
      key,val = field.split('=')
      field_dict[key] = convert_and_validate(key,val)

    return field_dict

  # somehwere this has to be called if we hit end of file
  def handle_data(self):
    #print "".join(data).split()
    # exiting group section, update current group
    if self.cur_section == 'group':
      self.cur_group = dict(DEFAULT_VALUES)
      self.cur_group.update(self.parse_fields())
      #print cur_group
    elif self.cur_section == 'region':
      # start with current default group
      region = dict(self.cur_group)
      # parse the region and update relevant values
      region.update(self.parse_fields())
      check_missing(region)
      #print "region: %s" % region
      self.regions.append(region)

  def parse(self):
    for line in self.file:
      # strip comment
      line = line.split('//')[0] 

      for ch in line:
        # parsing data
        if not self.in_section:
          # beginning a new section, handle data first
          if ch == '<':
            self.handle_data()
            # clear out data, now entering next section tag
            self.data = []
            self.in_section = True
          else:
            self.data.append(ch)
        # parsing section tag
        else:
          # reached end of section tag
          if ch == '>':
            self.cur_section = ''.join(self.data)
            if self.cur_section not in VALID_SECTIONS:
              print "unhandled section: %s" % self.cur_section
            self.data = []
            self.in_section = False
          else:
            self.data.append(ch)

    # handle data in last section of file
    if not self.in_section and len(self.data) > 0:
      self.handle_data()

    return SFZ(self.regions)

def convert_and_validate(key, value):
  if (key == 'lokey' or key == 'hikey' or key == 'pitch_keycenter'
      or key == 'lovel' or key == 'hivel'): 
    conv_val = int(value)
    if conv_val < 0 or conv_val > 127:
      raise RuntimeError('%s must be between 0 and 127' % key)
    return conv_val
  if key == 'tune':
    conv_val = int(value)
    if conv_val < -100 or conv_val > 100:
      raise RuntimeError('%s must be between -100 and 100' % key)
    return conv_val
  if key == 'offset' or key == 'loop_start' or key == 'loop_end':
    return int(value)
  if key == 'loop_crossfade' or key == 'ampeg_attack' or key == 'ampeg_hold' or key == 'ampeg_decay' or key == 'ampeg_sustain' or key == 'ampeg_release':
    return float(value)
  if key == 'loop_mode':
    if value != 'no_loop' and value != 'loop_continuous':
      raise RuntimeError('loop_mode must be either "no_loop" or "loop_continuous"')
    return value

  # just return it as-is if we don't know what it is
  return value

def check_missing(region):
  for key in REQUIRED_KEYS:
    if key not in region:
      raise RuntimeError('region missing required key "%s": %s' % (key, region))

def parse_sfz(path):
  sfzp = SFZParser(open(path))
  sfz = sfzp.parse()
  sfzp.close()
  return sfz
