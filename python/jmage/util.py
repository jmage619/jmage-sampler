import sys


SFZ_DEFAULTS = {
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

JMZ_DEFAULTS = dict(SFZ_DEFAULTS)
JMZ_DEFAULTS['jm_amp'] = 1.0

class SFZ(object):
  def __init__(self, regions=[]):
    self.defaults = SFZ_DEFAULTS
    self.write_order = ['pitch_keycenter', 'lokey', 'hikey', 'lovel', 'hivel', 'tune', 'offset', 'loop_start', 'loop_end', 'loop_mode', 'loop_crossfade', 'ampeg_attack', 'ampeg_hold', 'ampeg_decay', 'ampeg_sustain', 'ampeg_release', 'sample']
    self.regions = regions

  def add_region(self, region):
    reg = dict(self.defaults)
    reg.update(region)
    self.regions.append(reg)

  def write(self, out):
    for reg in self.regions:
      out.write("<region>")
      for k in self.write_order:
        out.write(" %s=%s" % (k, reg[k]))
      out.write('\n')
    
class JMZ(SFZ):
  def __init__(self, regions=[]):
    super(JMZ, self).__init__(regions)
    self.defaults = JMZ_DEFAULTS
    self.write_order = ['jm_name', 'jm_amp'] + self.write_order

class SFZParser(object):
  def __init__(self, file):
    self.defaults = SFZ_DEFAULTS
    self.valid_sections = set(['group', 'region'])
    self.required_keys = set(['sample'])
    self.file = file
    self.in_section = False
    self.data = []
    self.cur_group = {}
    self.cur_section = None
    self.regions = []
    self.cur_op = None
    self.cur_group = None
    self.cur_region = None
    self.state = None

  def close(self):
    self.file.close()

  # return a tuple of success and value
  # rather than raising exception
  # so we can just return unknown types rather than bailing out
  def convert_and_validate(self, key, value):
    if (key == 'lokey' or key == 'hikey' or key == 'pitch_keycenter'
        or key == 'lovel' or key == 'hivel'): 
      conv_val = int(value)
      if conv_val < 0 or conv_val > 127:
        raise RuntimeError('%s must be between 0 and 127' % key)
      return (True, conv_val)
    if key == 'tune':
      conv_val = int(value)
      if conv_val < -100 or conv_val > 100:
        raise RuntimeError('%s must be between -100 and 100' % key)
      return (True, conv_val)
    if key == 'offset' or key == 'loop_start' or key == 'loop_end':
      return (True, int(value))
    if key == 'loop_crossfade' or key == 'ampeg_attack' or key == 'ampeg_hold' or key == 'ampeg_decay' or key == 'ampeg_sustain' or key == 'ampeg_release':
      return (True, float(value))
    if key == 'loop_mode':
      if value != 'no_loop' and value != 'loop_continuous' and value != 'one_shot':
        raise RuntimeError('loop_mode must be "no_loop", "loop_continuous", or "one_shot"')
      return (True, value)

    # just return it as-is if we don't know what it is
    return (False, value)

  def check_missing(self, region):
    for key in self.required_keys:
      if key not in region:
        raise RuntimeError('region missing required key "%s": %s' % (key, region))

  def save_prev(self):
    if self.state == 'group':
      self.cur_group[self.cur_op] = self.convert_and_validate(self.cur_op, "".join(self.data))[1]
      self.data = []
    elif self.state == 'region':
      self.cur_region[self.cur_op] = self.convert_and_validate(self.cur_op, "".join(self.data))[1]
      self.data = []

  def parse(self):
    self.cur_group = dict(self.defaults)
    for line in self.file:
      # strip comment
      line = line.split('//')[0] 

      for field in line.split():
        # either a new tag
        if field[0] == '<' and field[-1] == '>':
          if len(self.data) > 0:
            self.save_prev()
            if self.state == 'region':
              self.check_missing(self.cur_region)
              self.regions.append(self.cur_region)
          if field[1:-1] == 'group':
            self.cur_group = dict(self.defaults)
            self.state = 'group'
          elif field[1:-1] == 'region':
            self.cur_region = dict(self.cur_group)
            self.state = 'region'
        # or new op code
        elif '=' in field:
          if len(self.data) > 0:
            self.save_prev()
          fields = field.split('=')
          self.cur_op = fields[0]
          self.data.append(fields[1])
        # or continuing space separated data for prev op code
        else:
          self.data.append(" ")
          self.data.append(field)

    # save last region if data left over
    if len(self.data) > 0 and self.state == 'region':
      self.save_prev()
      self.check_missing(self.cur_region)
      self.regions.append(self.cur_region)
      
    return SFZ(self.regions)

class JMZParser(SFZParser):
  def __init__(self, file):
    super(JMZParser, self).__init__(file)
    self.defaults = JMZ_DEFAULTS

  def convert_and_validate(self, key, value):
    results = super(JMZParser, self).convert_and_validate(key, value)
    # if success it was a valid SFZ attr
    if results[0]:
      return results
    # now try JMZ tests
    if key == 'jm_amp':
      return (True, float(value))

    # just return it as-is if we don't know what it is
    return (False, value)

def parse_sfz(path):
  sp = SFZParser(open(path))
  sfz = sp.parse()
  sp.close()
  return sfz

def parse_jmz(path):
  jp = JMZParser(open(path))
  jmz = jp.parse()
  jp.close()
  return jmz
