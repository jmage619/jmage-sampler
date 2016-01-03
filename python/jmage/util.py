import sys

VALID_SECTIONS = set(['group', 'region'])

DEFAULT_VALUES = {
  'lokey': 0,
  'hikey': 127,
  'pitch_keycenter': 32,
  'ampeg_attack': 0.0, 
  'ampeg_hold': 0.0,
  'ampeg_decay': 0.0,
  'ampeg_sustain': 100.0,
  'ampeg_release': 0.0
}

REQUIRED_KEYS = set(['sample'])

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

def convert_and_validate(key, value):
  if key == 'lokey' or key == 'hikey' or key == 'pitch_keycenter': 
    conv_val = int(value)
    if conv_val < 0 or conv_val > 127:
      raise RuntimeError('%s must be between 0 and 127' % key)
    return conv_val
  if key == 'loop_start' or key == 'loop_end':
    return int(value)
  if key == 'ampeg_decay' or key == 'ampeg_sustain' or key == 'ampeg_release':
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
  sfz = SFZParser(open(path))
  sfz.parse()
  return sfz
