import collections
import wx
import math

class NoteChoice(wx.Choice):
  def __init__(self, *args, **kwargs):
    notes = []
    for i in range(-1,10):
      notes.append("%iC" % i)
      notes.append("%iC#" % i)
      notes.append("%iD" % i)
      notes.append("%iD#" % i)
      notes.append("%iE" % i)
      notes.append("%iF" % i)
      notes.append("%iF#" % i)
      notes.append("%iG" % i)
      if i == 9:
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

class StretchRow(wx.PyPanel):
  def __init__(self, *args, **kwargs):
    self.stretch_callback = kwargs.pop('stretch_callback', None)
    super(StretchRow, self).__init__(*args, **kwargs)
    #self.sizer = wx.BoxSizer(wx.HORIZONTAL)
    #self.SetSizer(self.sizer)
    # hardcode border for now
    self.windows = []
    self.border = 7
    self.is_down = False
    self.resize_el = -1
    self.delta = 0
    # we will resize all rows in a list of panels
    self.panels = []
    self.cur_pos = 0
    # add our own pane
    #self.panels.append(self.GetParent())
    self.Bind(wx.EVT_MOTION, self.OnMouseMove)
    self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)

  # override to account for our border on the RHS
  def DoGetBestSize(self):
    size = super(StretchRow, self).GetBestSize()
    size.width += self.border
    return size

  def Add(self, win):
    self.windows.append(win)
    # need to bind mouse listeners to sub windows for consistent mouse events
    win.Bind(wx.EVT_MOTION, self.OnMouseMove)
    win.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    win.SetPosition((self.cur_pos, 0))
    self.cur_pos += win.GetSize().width + self.border
    self.Fit()

  def AddPanel(self, panel):
    self.panels.append(panel)

  def Resize(self, other):
    for i in range(len(other.windows)):
      self.windows[i].SetSize(other.windows[i].GetSize())
      first_x = other.windows[i].GetPosition()[0]
      pos = self.windows[i].GetPosition()
      pos[0] = first_x
      self.windows[i].SetPosition(pos)
    self.Fit()

  def EnableDrag(self, enabled=True):
    if enabled:
      self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
    else:
      self.Unbind(wx.EVT_LEFT_DOWN)

  def Stretch(self, index, delta):
    # first resize the one we care about
    cur_el = self.windows[index]
    size = cur_el.GetSize()
    size.width += delta
    cur_el.SetSize(size)
    # then move over the rest
    if len(self.windows) - 1 > index:
      for i in range(index + 1, len(self.windows)):
        pos = self.windows[i].GetPosition()
        pos[0] += delta
        self.windows[i].SetPosition(pos)
    self.Fit()

  def OnMouseDown(self, e):
    self.is_down = True
    self.down_x = e.GetPosition()[0]

    # last el is corner case
    for i in range(len(self.windows) - 1):
      cur_child = self.windows[i]
      next_child = self.windows[i + 1]
      el_x = cur_child.GetPosition()[0]
      el_width = cur_child.GetSize().width
      next_el_x = next_child.GetPosition()[0]      

      if self.down_x > el_x + el_width and self.down_x < next_el_x:
        self.resize_el = i
        break

    if  self.resize_el == -1:
      self.resize_el = len(self.windows) - 1

    self.down_size = self.windows[self.resize_el].GetSize()

  def OnMouseMove(self, e):
    if self.is_down:
      if e.GetEventObject() == self:
        x = e.GetPosition()[0]
      # if not the same, this is a child element, translate x accordingly
      else:
        x = e.GetEventObject().GetPosition()[0] + e.GetPosition()[0]
      self.delta = x - self.down_x

  def OnMouseUp(self, e):
    if self.is_down:
      self.Stretch(self.resize_el, self.delta)
      for p in self.panels:
        for row in p.GetChildren():
          row.Stretch(self.resize_el, self.delta)

      if self.stretch_callback is not None:
        self.stretch_callback(self)

      self.is_down = False
      self.resize_el = -1
      self.delta = 0

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

class BigScrollList(wx.ScrolledWindow):
  def __init__(self, data, *args, **kwargs):
    # default scrollwin to myself
    # but allow overriding if this panel is to be controlled
    # by an external scroll win
    self.scroll_win = kwargs.pop('scroll_win', self)
    super(BigScrollList, self).__init__(*args, **kwargs)
    self.windows = []
    self.item_height = -1
    self.first_index = 0
    # we track scroll positions ourselves since 
    # we use a combination of ScrolledWindow and ScrollWindow
    self.scroll_x = 0
    self.scroll_y = 0
    self.max_scroll = False
    self.cur_size = self.GetClientSize()
    self.scroll_win.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
    self.Bind(wx.EVT_SIZE, self.OnResize)
    self.RegisterData(data)

  def CreateWin(self, item):
    pass

  def UpdateWin(self, win, item):
    pass

  def Add(self, win):
    #new_pos = self.scroll_win.CalcScrolledPosition(0, self.item_height * (self.first_index + len(self.windows)))
    new_pos = (0 - self.scroll_x, self.item_height * (self.first_index + len(self.windows)) - self.scroll_y)
    win.Move(new_pos)
    self.windows.append(win)
    #print "win added, count: %i, val: %s, virt y: %i" % (len(self.windows), win.GetValue(), self.item_height * (self.first_index + len(self.windows) - 1))

  def InsertFront(self, win):
    self.first_index -= 1
    #new_pos = self.scroll_win.CalcScrolledPosition(0, self.item_height * self.first_index)
    new_pos = (0 - self.scroll_x, self.item_height * self.first_index - self.scroll_y)
    win.Move(new_pos)
    self.windows.insert(0, win)
    #print "win inserted, count: %i, virt y: %i" % (len(self.windows), self.item_height * (self.first_index))

  def RegisterData(self, data):
    self.data = data

    if len(data) == 0:
      return

    win = self.CreateWin(data[0])
    size = win.GetSize()
    self.item_height = size.height
    self.SetVirtualSize((size.width, self.item_height * len(data)))
    self.Add(win)
 
    if len(data) == 1:
      return

    view_height = self.GetClientSize().height
    for item in data[1:]:
      # we should always have 1 more than visible windows
      # ceil ensures we count partially visible windows
      if len(self.windows) >= math.ceil(view_height / float(self.item_height)) + 1:
        return
      win = self.CreateWin(item)
      self.Add(win)

  def Append(self, item):
    self.data.append(item)

    if len(self.windows) == 0:
      win = self.CreateWin(item)
      size = win.GetSize()
      self.item_height = size.height
      self.SetVirtualSize((size.height, self.item_height * len(self.data)))
      self.Add(win)
    else:
      # cases below my not create new window so reuse last virt width
      # -1 is never ok, it disables virt size in that direction
      self.SetVirtualSize((self.GetVirtualSize().width, self.item_height * len(self.data)))
      view_height = self.GetClientSize().height
      # we should always have 1 more than visible windows
      # ceil ensures we count partially visible windows
      if len(self.windows) < math.ceil(view_height / float(self.item_height)) + 1:
        win = self.CreateWin(item)
        self.Add(win)
      # need to update value if its the last hidden one just outside view
      elif self.max_scroll == True:
        self.UpdateWin(self.windows[len(self.windows) - 1], item)
    # no matter what virtual size increases causing scroll pos to move up
    self.max_scroll = False
        
  def Rotate(self, iterations):
    # rotate by 1 is special case, only requires updating 1 cell 
    if iterations == -1:
      win = self.windows.pop()
      self.windows.insert(0, win)
      self.first_index -= 1
      self.UpdateWin(win, self.data[self.first_index])
      #win.Move(self.scroll_win.CalcScrolledPosition(0, self.item_height * self.first_index))
      win.Move((0 - self.scroll_x, self.item_height * self.first_index - self.scroll_y))
    elif iterations == 1:
      new_index = self.first_index + len(self.windows)
      win = self.windows.pop(0)
      self.windows.append(win)
      if new_index < len(self.data):
        self.UpdateWin(win, self.data[new_index])
      #win.Move(self.scroll_win.CalcScrolledPosition(0, self.item_height * new_index))
      win.Move((0 - self.scroll_x, self.item_height * new_index - self.scroll_y))
      self.first_index += 1
    else:
      self.first_index += iterations
      for i in range(len(self.windows)):
        if self.first_index + i < len(self.data):
          self.UpdateWin(self.windows[i], self.data[self.first_index + i])
        # must use instead of Move to allow for -1 coords
        width = self.windows[i].GetSize().width
        #new_pos = self.scroll_win.CalcScrolledPosition((0, self.item_height * (self.first_index + i)))
        new_pos = (0 - self.scroll_x, self.item_height * (self.first_index + i) - self.scroll_y)
        self.windows[i].SetDimensions(new_pos[0], new_pos[1], width, self.item_height, wx.SIZE_ALLOW_MINUS_ONE)
    '''print "rotate %i; new coords:" % (iterations)
    for i in range(len(self.windows)):
      pos = self.windows[i].GetPosition()
      print "  %i: val: %s; virt y: %i" % (i, self.windows[i].GetValue(), self.CalcUnscrolledPosition(pos)[1])
    '''

  def ScrollToPos(self, pos):
    if (pos >= self.GetVirtualSize().height - self.GetClientSize().height):
      self.max_scroll = True
    else:
      self.max_scroll = False

    first_y = self.item_height * self.first_index
    rotations = (pos - first_y) / self.item_height
    # prevent negative rotations on resize if scrollbar maxed out
    if rotations != 0 and not (self.max_scroll and rotations < 0):
      #print "rotating; old view start y: %i; new view start y: %i" % (e.GetEventObject().GetViewStart()[1], self.scroll_y)
      self.Rotate(rotations)

  def ScrollWindow(self, dx, dy, **kwargs):
    if dx != 0:
      self.scroll_x -= dx
    if dy != 0:
      new_pos = self.scroll_y - dy
      self.ScrollToPos(new_pos)
      self.scroll_y = new_pos
    super(BigScrollList, self).ScrollWindow(dx, dy, **kwargs)

    #print "ScrollWindow called"

  def OnScroll(self, e):
    if (e.GetOrientation() == wx.HORIZONTAL):
      self.scroll_x = e.GetPosition()
    elif (e.GetOrientation() == wx.VERTICAL):
      new_pos = e.GetPosition()
      self.ScrollToPos(new_pos)
      '''if (new_pos >= self.GetVirtualSize().height - self.GetClientSize().height):
        self.max_scroll = True
      else:
        self.max_scroll = False

      first_y = self.item_height * self.first_index
      rotations = (new_pos - first_y) / self.item_height
      # prevent negative rotations on resize if scrollbar maxed out
      if rotations != 0 and not (self.max_scroll and rotations < 0):
        #print "rotating; old view start y: %i; new view start y: %i" % (e.GetEventObject().GetViewStart()[1], self.scroll_y)
        self.Rotate(rotations)
      '''

      # important to update after rotating to prevent quantization errors in row placement
      self.scroll_y = new_pos
    e.Skip()

  def OnResize(self, e):
    new_size = self.GetClientSize()
    if new_size.height > self.cur_size.height:
      # special case, if scrollbar maxed out, add to beginning of window list rather than end
      if self.max_scroll:
        while len(self.windows) < math.ceil(new_size.height / float(self.item_height)) + 1 and self.first_index > 0:
          win = self.CreateWin(self.data[self.first_index - 1])
          self.InsertFront(win)
      else:
        for item in self.data[self.first_index + len(self.windows):]:
          # we should always have 1 more than visible windows
          # ceil ensures we count partially visible windows
          if len(self.windows) >= math.ceil(new_size.height / float(self.item_height)) + 1:
            break
          win = self.CreateWin(item)
          self.Add(win)

    elif new_size.height < self.cur_size.height:
      # decreasing size always moves scrollbar from max pos
      self.max_scroll = False
      while (len(self.windows) > math.ceil(new_size.height / float(self.item_height)) + 1):
        win = self.windows.pop()
        #print "win removed, count: %i, virt y: %i" % (len(self.windows), self.CalcUnscrolledPosition(win.GetPosition())[1])
        win.Destroy()

    self.cur_size = new_size
    e.Skip()
