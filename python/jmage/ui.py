import collections
import wx
import math

NOTES = []
for i in range(-1,10):
  NOTES.append("%iC" % i)
  NOTES.append("%iC#" % i)
  NOTES.append("%iD" % i)
  NOTES.append("%iD#" % i)
  NOTES.append("%iE" % i)
  NOTES.append("%iF" % i)
  NOTES.append("%iF#" % i)
  NOTES.append("%iG" % i)
  if i == 9:
    break
  NOTES.append("%iG#" % i)
  NOTES.append("%iA" % i)
  NOTES.append("%iA#" % i)
  NOTES.append("%iB" % i)

class NoteChoice(wx.Choice):
  def __init__(self, *args, **kwargs):

    super(NoteChoice, self).__init__(*args, choices=NOTES, **kwargs)
    self.Select(36)

class DragBox(wx.TextCtrl):
  def __init__(self, *args, **kwargs):
    # default max to min if not passed in
    self.min = kwargs.pop('min', 0.0)
    self.max = kwargs.pop('max', 100.0)
    self.steps = kwargs.pop('steps', 101)
    self.fmt = kwargs.pop('fmt', "%i")
    self.callback = kwargs.pop('callback', None)
    super(DragBox, self).__init__(*args, **kwargs)

    self.index = 0

    self.mouse_down = False
    self.down_y = None
    self.down_val = None

    self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
    self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    self.Bind(wx.EVT_MOTION, self.OnMouseMove)

  def GetValue(self):
    return (self.max - self.min) / (self.steps - 1) *  self.index + self.min

  def ChangeValue(self, value):
    try:
      if value < self.min:
        self.index = 0
      elif value > self.max:
        self.index = self.steps - 1
      # 0.05 is epsilon to deal with truncation errors
      else:
        self.index = int((self.steps - 1) * (value - self.min) / (self.max - self.min) + 0.05)
      super(DragBox, self).ChangeValue(self.fmt % self.GetValue())
    except:
      pass

  def OnMouseDown(self, e):
    self.mouse_down = True
    self.down_y = e.GetY()
    self.down_index = self.index
    e.Skip()

  def OnMouseUp(self, e):
    self.mouse_down = False
    e.Skip()

  def OnMouseMove(self, e):
    if self.mouse_down:
      index = self.down_index + self.down_y - e.GetY()
      if index < 0:
        self.index = 0
      elif index >= self.steps:
        self.index = self.steps - 1
      else:
        self.index = index
      super(DragBox, self).ChangeValue(self.fmt % self.GetValue())
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
    self.master = None
    self.border = 7
    self.is_down = False
    self.resize_el = -1
    self.delta = 0
    # we will resize all rows in a list of panels
    #self.panels = []
    self.rows = []
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
    # resize win to same width as master element
    if self.master is not None:
      width = self.master.GetWindows()[len(self.windows)].GetSize().width
      win.SetSize((width, -1))

    self.windows.append(win)
    # need to bind mouse listeners to sub windows for consistent mouse events
    win.Bind(wx.EVT_MOTION, self.OnMouseMove)
    win.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
    win.SetPosition((self.cur_pos, 0))
    self.cur_pos += win.GetSize().width + self.border
    self.Fit()

  def Index(self, win):
    return self.windows.index(win)

  def GetWindows(self):
    return self.windows

  #def AddPanel(self, panel):
  def AddRow(self, row):
    #self.panels.append(panel)
    self.rows.append(row)
    row.master = self

  def RemoveRow(self, i):
    self.rows.pop(i)

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
    e.Skip()

  def OnMouseMove(self, e):
    if self.is_down:
      if e.GetEventObject() == self:
        x = e.GetPosition()[0]
      # if not the same, this is a child element, translate x accordingly
      else:
        x = e.GetEventObject().GetPosition()[0] + e.GetPosition()[0]
      self.delta = x - self.down_x
    e.Skip()

  def OnMouseUp(self, e):
    if self.is_down:
      self.Stretch(self.resize_el, self.delta)
      #for p in self.panels:
      for row in self.rows:
        #for row in p.GetChildren():
        row.Stretch(self.resize_el, self.delta)

      if self.stretch_callback is not None:
        self.stretch_callback(self)

      self.is_down = False
      self.resize_el = -1
      self.delta = 0
    e.Skip()

class Grid(wx.Panel):
  def __init__(self, *args, **kwargs):
    # just default to something reasonable
    self.row_height = kwargs.pop('row_height', 31)
    style = kwargs.pop('style', 0)
    style = style | wx.HSCROLL | wx.VSCROLL
    super(Grid, self).__init__(*args, style=style, **kwargs)
    self.cur_pos = 0
    self.panels = []
    self.hbox = wx.BoxSizer(wx.HORIZONTAL)
    self.num_vis_rows = 0
    self.scroll_x = 0
    #self.scroll_y = 0
    self.scroll_x_maxed = False
    self.scroll_y_maxed = False
    self.cur_size = self.GetClientSize()
    self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
    self.Bind(wx.EVT_SIZE, self.OnResize)
    self.SetSizer(self.hbox)

  def Add(self, panel):
    self.panels.append(panel)
    self.hbox.Add(panel, flag=wx.EXPAND)
    # call layout here?

  def AddHeader(self):
    win = self.panels[0].CreateHeader()
    self.panels[0].Add(win)

    win = self.panels[1].CreateHeader()
    self.panels[1].Add(win)

    size = win.GetSize()
    # correct for x scroll only for panel 1
    # can't use Move since it doesn't respect -1 coord
    win.SetDimensions(- self.scroll_x, win.GetPosition()[1], size.width, size.height, wx.SIZE_ALLOW_MINUS_ONE)

    self.num_vis_rows += 1

    for i in range(16):
      self.AddRow()

  def AddRow(self):
    win = self.panels[0].CreateRow()
    win.Hide()
    self.panels[0].Add(win)

    win = self.panels[1].CreateRow()
    win.Hide()
    self.panels[1].Add(win)
    # correct for x scroll only for panel 1
    # can't use Move since it doesn't respect -1 coord
    size = win.GetSize()
    win.SetDimensions(- self.scroll_x, win.GetPosition()[1], size.width, size.height, wx.SIZE_ALLOW_MINUS_ONE)

  def RegisterData(self, data):
    self.data = data

    if len(data) == 0:
      return

    view_height = self.GetClientSize().height
    for i in range(len(data)):
      # ceil ensures we count partially visible rows
      if self.num_vis_rows >= math.ceil(view_height / float(self.row_height)):
        return
      for p in self.panels:
        # add 1 to skip header
        p.UpdateRow(i + 1, data[i])
        p.GetRow(i + 1).Show()
      self.num_vis_rows += 1

  def Append(self, data_row):
    self.data.append(data_row)

    view_height = self.GetClientSize().height
    # ceil ensures we count partially visible windows
    if self.num_vis_rows < math.ceil(view_height / float(self.row_height)):
      for p in self.panels:
        p.UpdateRow(self.num_vis_rows, data_row)
        p.GetRow(self.num_vis_rows).Show()
      self.num_vis_rows += 1

    # no matter what virtual size increases causing scroll pos to move up
    self.scroll_y_maxed = False
    # not needed?
    #self.hbox.Layout()
    self.UpdateScrollbars()

  # i is an index into data
  def Remove(self, i):
    self.data.pop(i)

    # case: item is before visible windows
    if i < self.cur_pos:
      self.cur_pos -= 1

    # case: item is visible
    elif i >= self.cur_pos and i < self.cur_pos + self.num_vis_rows - 1:
      # corner case, if elements are still available from top, just rotate
      if self.cur_pos > 0:
        for p in self.panels:
          p.ScrollWindow(0, - self.row_height)
        # update removed el offset
        i -= 1
        self.cur_pos -= 1

      # corner case, if less data than visible windows, remove one
      # subtract 1 since num_vis_rows includes header 
      elif self.cur_pos + self.num_vis_rows - 1 > len(self.data):
        for p in self.panels:
          p.GetRow(self.num_vis_rows - 1).Hide()
        self.num_vis_rows -= 1

        self.scroll_y_maxed = True

      # re align visible elements >= i
      # subtract 1 since num_vis_rows includes header 
      for j in range(self.num_vis_rows - 1 - (i - self.cur_pos)):
        for p in self.panels:
          # add 1 to row index since first visible row is header
          p.UpdateRow(i - self.cur_pos + 1 + j, self.data[i + j])

    #case: element after visible windows
    else:
      if self.cur_pos + self.num_vis_rows - 1 > len(self.data):
        self.scroll_y_maxed = True

    self.UpdateScrollbars()

  def GetMaxScrollX(self):
    # use first window since it's the header
    return self.panels[0].rows[0].GetSize().width + self.panels[1].rows[0].GetSize().width - self.GetClientSize().width

  # assumed to only ever handle vertical scrolling
  def ScrollToPos(self, pos):
    # subtract 1 to ignore header
    max_pos = len(self.data) - (self.GetClientSize().height / self.row_height - 1)

    if pos >= max_pos:
      self.scroll_y_maxed = True
    else:
      self.scroll_y_maxed = False

    num_steps = pos - self.cur_pos

    if num_steps != 0:
    # corner cases if view size is not divisible by item height
      if self.GetClientSize().height % self.row_height != 0:
        #  remove last element when hit max scroll
        if self.scroll_y_maxed:
          for p in self.panels:
            p.GetRow(self.num_vis_rows - 1).Hide()
          self.num_vis_rows -= 1

        # leaving max scroll pos, re-add last element
        elif self.cur_pos == max_pos and num_steps < 0:
          for p in self.panels:
            p.UpdateRow(self.num_vis_rows, self.data[len(self.data) - 1])
            p.GetRow(self.num_vis_rows).Show()
          self.num_vis_rows += 1

      dy = self.row_height * num_steps
      for p in self.panels:
        p.ScrollWindow(0, dy)

      self.cur_pos = pos

  def OnScroll(self, e):
    if e.GetOrientation() == wx.HORIZONTAL:
      new_pos = e.GetPosition()
      self.panels[1].ScrollWindow(self.scroll_x - new_pos, 0)
      self.scroll_x = new_pos

      if self.scroll_x >= self.GetMaxScrollX():
        self.scroll_x_maxed = True
      else:
        self.scroll_x_maxed = False

    else:
      self.ScrollToPos(e.GetPosition())
    e.Skip()

  def UpdateScrollbars(self):
    # use first window since it's the header
    self.SetScrollbar(wx.HORIZONTAL, self.scroll_x, self.GetClientSize().width, self.panels[0].rows[0].GetSize().width + self.panels[1].rows[0].GetSize().width)
    # subtract 1 to not count header
    self.SetScrollbar(wx.VERTICAL, self.cur_pos, self.GetClientSize().height / self.row_height - 1, len(self.data))

  def OnResize(self, e):
    self.UpdateScrollbars()

    # on resize position may be pushed up if at max scroll pos
    # correct for it by scrolling contents
    pos = self.GetScrollPos(wx.VERTICAL)
    if pos != self.cur_pos:
      self.ScrollToPos(pos)

    # important to update panel size after updating scroll
    # since below depends on cur_pos, and scrolling changes cur_pos
    new_size = self.GetClientSize()

    ### deal with virtical resize
    if new_size.height > self.cur_size.height:
      # subtract 1 to ignore header
      for i in range(self.cur_pos + self.num_vis_rows - 1, len(self.data)):
        # ceil ensures we count partially visible windows
        if self.num_vis_rows >= math.ceil(new_size.height / float(self.row_height)):
          break

        for p in self.panels:
          p.UpdateRow(self.num_vis_rows, self.data[i])
          p.GetRow(self.num_vis_rows).Show()
        self.num_vis_rows += 1

    elif new_size.height < self.cur_size.height:
      # decreasing size always moves scrollbar from max pos
      self.scroll_y_maxed = False
      while self.num_vis_rows > 1 and self.num_vis_rows > math.ceil(new_size.height / float(self.row_height)):
        for p in self.panels:
          p.GetRow(self.num_vis_rows - 1).Hide()

        self.num_vis_rows -= 1

    ### deal with horizontal resize

    if new_size.width > self.cur_size.width:
      if self.scroll_x >= self.GetMaxScrollX():
        self.scroll_x_maxed = True

      # if at max scroll, scroll in resize direction
      # to maintain max scroll
      if self.scroll_x_maxed and self.scroll_x > 0:
        del_width = new_size.width - self.cur_size.width

        if del_width > self.scroll_x:
          del_width = self.scroll_x

        self.panels[1].ScrollWindow(del_width, 0)
        self.scroll_x -= del_width 

    # negative resize always moves bar from max_scroll
    elif new_size.width < self.cur_size.width:
      self.scroll_x_maxed = False

    self.cur_size = new_size

    # will have to revisit when test with mult panels
    # self.panels[0].SetClientSize(self.GetClientSize())
    # call to layout instead? 
    e.Skip()

  def OnColStretch(self, win):
    self.UpdateScrollbars()

    max_scroll_pos = self.GetMaxScrollX()

    # col stretch always mods max scroll pos
    # so update max scroll
    if self.scroll_x >= max_scroll_pos:
      self.scroll_x_maxed = True
      if max_scroll_pos < 0:
        max_scroll_pos = 0
      # if at max scroll and virt size gets smaller, scroll dx
      # back down to  max scroll
      self.panels[1].ScrollWindow(self.scroll_x - max_scroll_pos, 0)
      self.scroll_x = max_scroll_pos
    else:
      self.scroll_x_maxed = False

    self.hbox.Layout()

class GridPanel(wx.Panel):
  def __init__(self, *args, **kwargs):
    super(GridPanel, self).__init__(*args, **kwargs)
    self.rows = []

  def CreateHeader(self):
    pass

  def CreateRow(self, data_row):
    pass

  def DestroyRow(self, i):
    # header row list is 1 less since it doesn't include itself
    self.header.RemoveRow(i - 1)
    win = self.rows.pop(i)
    win.Destroy()

  def UpdateRow(self, i, data_row):
    pass

  def GetRow(self, i):
    return self.rows[i]

  def NewHeader(self):
    self.header = StretchRow(self, stretch_callback=self.GetParent().OnColStretch)
    self.header.EnableDrag()
    return self.header

  def NewRow(self):
    row = StretchRow(self)
    self.header.AddRow(row)
    return row

  def Add(self, win):
    new_pos = (0, self.GetParent().row_height * len(self.rows))
    win.Move(new_pos)
    self.rows.append(win)

  def RemoveLast(self):
    self.DestroyRow(len(self.rows) - 1)

  def Index(self, win):
    # subtract 1 to ignore header
    return self.GetParent().cur_pos + self.rows.index(win) - 1

  def ScrollWindow(self, dx, dy, **kwargs):
    # scroll x first
    super(GridPanel, self).ScrollWindow(dx, 0)

    iterations = dy / self.GetParent().row_height
    if iterations == 0:
      return

    # skip first element it's the header
    for i in range(1, self.GetParent().num_vis_rows):
      # parent cur_pos isn't updated yet, so include iterations
      # subtract 1 since header makes ith window correspond to i - 1 data el
      self.UpdateRow(i, self.GetParent().data[self.GetParent().cur_pos - 1 + iterations + i])
