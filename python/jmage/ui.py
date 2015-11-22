import collections
import wx

class NoteChoice(wx.Choice):
  def __init__(self, *args, **kwargs):
    notes = []
    for i in range(-2,9):
      notes.append("%iC" % i)
      notes.append("%iC#" % i)
      notes.append("%iD" % i)
      notes.append("%iD#" % i)
      notes.append("%iE" % i)
      notes.append("%iF" % i)
      notes.append("%iF#" % i)
      notes.append("%iG" % i)
      if i == 8:
        break
      notes.append("%iG#" % i)
      notes.append("%iA" % i)
      notes.append("%iA#" % i)
      notes.append("%iB" % i)

    super(NoteChoice, self).__init__(*args, choices=notes, **kwargs)
    self.Select(36)


class DragBox(wx.TextCtrl):
  def __init__(self, step, min, max, *args, **kwargs):
    self.fmt = kwargs.pop('fmt', "%i")
    self.callback = kwargs.pop('callback', None)
    super(DragBox, self).__init__(*args, **kwargs)

    self.step = step
    self.min = min
    self.max = max
    cur_step = min

    self.values = []
    # epsilon here if we go over due to float err
    # no good reason yet for picking this value
    # other than music controls don't usually go beyond
    # milli range
    while cur_step <= max + 0.00001:
      self.values.append(cur_step)
      cur_step += step
    # put in the exact max
    # better to have near duplicates and the exact max
    # near top than never being able to reach it
    if self.values[len(self.values) - 1] < max:
      self.values.append(max)

    self.index = 0

    self.mouse_down = False
    self.down_y = None
    self.down_val = None

    self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
    self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    self.Bind(wx.EVT_MOTION, self.OnMouseMove)

  def GetValue(self):
    return self.values[self.index]

  def ChangeValue(self, value):
    try:
      if value < self.min:
        value = self.min
      elif value > self.max:
        value = self.max
      self.index = int((value - self.min) * 1 / self.step)
      super(DragBox, self).ChangeValue(self.fmt % self.values[self.index])
    except:
      pass

  def OnMouseDown(self, e):
    self.mouse_down = True
    self.down_y = e.GetY()
    self.down_index = self.index
    e.Skip()

  def OnMouseUp(self, e):
    self.mouse_down = False
    #print e.GetPosition()
    e.Skip()

  def OnMouseMove(self, e):
    if self.mouse_down:
      index = self.down_index + self.down_y - e.GetY()
      if index < 0:
        self.index = 0
      elif index >= len(self.values):
        self.index = len(self.values) - 1
      else:
        self.index = index
      super(DragBox, self).ChangeValue(self.fmt % self.values[self.index])
      if self.callback is not None:
        self.callback(self)
    #e.Skip()

class StretchColGrid(wx.ScrolledWindow):
  def __init__(self, n_cols, *args, **kwargs):
    super(StretchColGrid, self).__init__(*args, **kwargs)
    self.n_cols = n_cols
    # hard code border for now
    self.border = 7
    self.windows = []
    self.index = 0
    # these are relative to virtual space, NOT current scroll pos!
    self.cur_x = 0
    self.cur_y = 0
    # to figure out how far down next row should be
    self.max_height = 0
    self.down_x = None
    self.to_update = {'resize': [], 'move': []}
    self.virt_size = wx.Size(0, 0)
    #self.sizer = wx.BoxSizer(wx.VERTICAL)
    self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
    self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    self.Bind(wx.EVT_MOTION, self.OnMouseMove)
    self.Bind(wx.EVT_CHILD_FOCUS, self.OnChildFocus)
    self.SetScrollRate(1,1)

  def Add(self, win):
    # also bind to elements so dragging and stopping still happens if
    # mouse pointer is moved over an element during resizing
    win.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    win.Bind(wx.EVT_MOTION, self.OnMouseMove)

    # reached a new row
    if self.index % self.n_cols == 0:
      self.windows.append([])     
      self.cur_x = 0
      # don't update cur_y if its the first row
      if len(self.windows) > 1:
        self.cur_y += self.max_height + self.border

    # if have prev row, set to column size in case user changed it
    if len(self.windows) > 1:
      prev_row = self.index / self.n_cols - 1
      cur_col = self.index % self.n_cols
      win.SetSize((
        self.windows[prev_row][cur_col].GetSize().width, -1))    

    # need to correct for current scroll position
    scroll_pos = self.GetViewStart()
    win.Move((self.cur_x - scroll_pos[0], self.cur_y - scroll_pos[1]))
    self.windows[self.index / self.n_cols].append(win)
    size = win.GetSize()
    self.cur_x += size.width + self.border
    self.max_height = max(self.max_height, size.height)
    if len(self.windows) == 1:
      self.virt_size.width = self.cur_x
    self.virt_size.height = self.cur_y + self.max_height
    self.SetVirtualSize(self.virt_size)
    self.index += 1

  def GetIndex(self, win):
    for i in range(len(self.windows)):
      for j in range(self.n_cols):
        if self.windows[i][j] == win:
          return (i,j)

    return None

  def RemoveRow(self, index):
    for win in self.windows[index]:
      win.Destroy()
    self.windows.pop(index)
    # shift the rows below up
    if index < len(self.windows):
      for row in self.windows[index:]:
        for win in row:
          pos = win.GetPosition()
          win.Move((pos[0], pos[1] - self.max_height - self.border))

    # rewind some state vars and resize virtual space
    self.index -= self.n_cols
    self.cur_y -= self.max_height + self.border
    self.virt_size.height -= self.max_height + self.border
    self.SetVirtualSize(self.virt_size)

  def OnChildFocus(self, e):
    pass

  def OnMouseDown(self, e):
    # use first row to set grid column widths
    # only loop over the elements we have, current grid may be < n_cols
    row_size = len(self.windows[0])
    n = min(self.n_cols, row_size)

    self.to_update['resize'] = []
    self.to_update['move'] = []

    for i in range(n - 1):
      win = self.windows[0][i]
      next_win = self.windows[0][i + 1]
      if (e.GetX() >= win.GetPosition()[0] + win.GetSize().width and
          e.GetX() <= next_win.GetPosition()[0]):
      
        #print "clicked x: %i; pos: %i" % (e.GetX(), win.GetPosition()[0])
        for row in self.windows:
          self.to_update['resize'].append(row[i])
          for j in range(i + 1, len(row)):
            self.to_update['move'].append(row[j])
            
        self.down_x = e.GetX()
        e.Skip()
        return

    # last column requires special case
    i = n - 1
    win = self.windows[0][i]
    if e.GetX() >= win.GetPosition()[0] + win.GetSize().width:
      #print "clicked x: %i; pos: %i" % (e.GetX(), win.GetPosition()[0])
      for row in self.windows:
        self.to_update['resize'].append(row[i])
      self.down_x = e.GetX()

    e.Skip()

  def OnMouseUp(self, e):
    self.to_update['resize'] = []
    self.to_update['move'] = []
    self.SetVirtualSize(self.virt_size)
    e.Skip()

  def OnMouseMove(self, e):
    if len(self.to_update['resize']) > 0:
      eo = e.GetEventObject()

      if eo.GetId() == self.GetId():
        abs_x = e.GetX()
        #print "outside get x: %i; get pos: %i" % (e.GetX(), eo.GetPosition()[0])
      # if sub element, GetX needs to be translated to absolute x
      else:
        # GetX goes negative when scrolled right, add by GetViewStart to correct
        abs_x = e.GetX() + eo.GetPosition()[0] + self.GetViewStart()[0]
        #print "inside get x: %i; get pos: %i" % (e.GetX(), eo.GetPosition()[0])
      for win in self.to_update['resize']:
        win.SetSize((win.GetSize().width + abs_x - self.down_x, -1))
      for win in self.to_update['move']:
        win.Move((win.GetPosition()[0] + abs_x - self.down_x, -1))
      self.virt_size.width = self.virt_size.x + abs_x - self.down_x
      self.down_x = abs_x
    e.Skip()
