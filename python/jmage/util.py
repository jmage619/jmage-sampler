import sys

valid_sections = set(['group', 'region'])

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
      field_dict[key] = val

    return field_dict

  # somehwere this has to be called if we hit end of file
  def handle_data(self):
    #print "".join(data).split()
    # exiting group section, update current group
    if self.cur_section == 'group':
      self.cur_group = self.parse_fields()
      #print cur_group
    elif self.cur_section == 'region':
      # start with current default group
      region = dict(self.cur_group)
      # parse the region and update relevant values
      region.update(self.parse_fields())
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
            if self.cur_section not in valid_sections:
              print "unhandled section: %s" % self.cur_section
            self.data = []
            self.in_section = False
          else:
            self.data.append(ch)

    # handle data in last section of file
    if not self.in_section and len(self.data) > 0:
      self.handle_data()

def parse_sfz(path):
  sfz = SFZParser(open(path))
  sfz.parse()
  return sfz
