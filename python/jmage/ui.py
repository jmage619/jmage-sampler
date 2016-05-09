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

class Grid(wx.Panel):
  def __init__(self, *args, **kwargs):
    super(Grid, self).__init__(*args, **kwargs)
    self.cur_pos = 0
    self.panels = []
    self.hbox = wx.BoxSizer(wx.HORIZONTAL)
    #self.row_height = -1
    # just hardcode until we get it right
    self.row_height = 31
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
    self.hbox.Add(panel)
    # call layout here?

  def AddRow(self, data_row, header=False):
    if header:
      win = self.panels[0].CreateHeader(data_row)
      self.panels[0].Add(win)

      win = self.panels[1].CreateHeader(data_row)
      self.panels[1].Add(win)
    else:
      win = self.panels[0].CreateRow(data_row)
      self.panels[0].Add(win)

      win = self.panels[1].CreateRow(data_row)
      self.panels[1].Add(win)
    # correct for x scroll only for panel 1
    # can't use Move since it doesn't respect -1 coord
    size = win.GetSize()
    win.SetDimensions(- self.scroll_x, win.GetPosition()[1], size.width, size.height, wx.SIZE_ALLOW_MINUS_ONE)

    self.num_vis_rows += 1

  def RegisterData(self, data):
    self.data = data

    if len(data) == 0:
      return

    # first row is header
    self.AddRow(data[0], True)
 
    if len(data) == 1:
      return

    view_height = self.GetClientSize().height
    for data_row in data[1:]:
      # ceil ensures we count partially visible rows
      if self.num_vis_rows >= math.ceil(view_height / float(self.row_height)):
        return
      self.AddRow(data_row)

  def Append(self, data_row):
    self.data.append(data_row)

    # first row is header
    if self.num_vis_rows == 0:
      self.AddRow(data_row, True)
    else:
      view_height = self.GetClientSize().height
      # ceil ensures we count partially visible windows
      if self.num_vis_rows < math.ceil(view_height / float(self.row_height)):
        self.AddRow(data_row)

    # no matter what virtual size increases causing scroll pos to move up
    self.scroll_y_maxed = False
    self.UpdateScrollbars()

  # i is an index into data
  # for now it is also assumed i is also a visible row
  def Remove(self, i):
    self.data.pop(i)
    # corner case, if elements are still available from top, just rotate
    if self.cur_pos > 0 and self.num_vis_rows <= len(self.data):
      for p in self.panels:
        p.ScrollWindow(0, - self.row_height)
      # update removed el offset
      i -= 1
      self.cur_pos -= 1

    # corner case, if less data than visible windows, remove one
    elif self.cur_pos + self.num_vis_rows > len(self.data):
      for p in self.panels:
        p.RemoveLast()
      self.num_vis_rows -= 1

      self.scroll_y_maxed = True

    # re align visible elements >= i
    for j in range(self.num_vis_rows - (i - self.cur_pos)):
      for p in self.panels:
        p.UpdateRow(i - self.cur_pos + j, self.data[i + j])

    self.UpdateScrollbars()

  def GetMaxScrollX(self):
    # use first window since it's the header
    return self.panels[0].rows[0].GetSize().width + self.panels[1].rows[0].GetSize().width - self.GetClientSize().width

  # assumed to only ever handle vertical scrolling
  def ScrollToPos(self, pos):
    max_pos = len(self.data) - self.GetClientSize().height / self.row_height

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
            p.RemoveLast()
          self.num_vis_rows -= 1

        # leaving max scroll pos, re-add last element
        elif self.cur_pos == max_pos and num_steps < 0:
          self.AddRow(self.data[len(self.data) - 1])

      dy = self.row_height * num_steps
      for p in self.panels:
        p.ScrollWindow(0, dy)

      self.cur_pos = pos
    #self.scroll_y = new_y

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
    self.SetScrollbar(wx.VERTICAL, self.cur_pos, self.GetClientSize().height / self.row_height, len(self.data))

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
      for data_row in self.data[self.cur_pos + self.num_vis_rows:]:
        # ceil ensures we count partially visible windows
        if self.num_vis_rows >= math.ceil(new_size.height / float(self.row_height)):
          break

        self.AddRow(data_row)

    elif new_size.height < self.cur_size.height:
      # decreasing size always moves scrollbar from max pos
      self.scroll_y_maxed = False
      while self.num_vis_rows > math.ceil(new_size.height / float(self.row_height)):
        for p in self.panels:
          p.RemoveLast()

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

  def CreateHeader(self, data_row):
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
    #win = self.rows.pop()
    #win.Destroy()

  def Index(self, win):
    return self.GetParent().cur_pos + self.rows.index(win)

  def ScrollWindow(self, dx, dy, **kwargs):
    # scroll x first
    super(GridPanel, self).ScrollWindow(dx, 0)

    iterations = dy / self.GetParent().row_height
    if iterations == 0:
      return

    # skip first element it's the header
    for i in range(1, len(self.rows)):
      # parent cur_pos isn't updated yet, so include iterations
      self.UpdateRow(i, self.GetParent().data[self.GetParent().cur_pos + iterations + i])


'''class Grid(wx.ScrolledWindow):
  class RowPanel(ScrollListPane):
    def __init__(self, header, *args, **kwargs):
      self.header = header
      super(Grid.RowPanel, self).__init__(*args, **kwargs)

    def PopulateRow(self, row, data_row):
      pass

    def UpdateRow(self, row, data_row):
      pass

    def CreateWin(self, item):
      row = StretchRow(self)
      self.PopulateRow(row, item)

      # update sizes and positions to match header in case it was stretched
      row.Resize(self.header)

      return row

    def UpdateWin(self, win, item):
      self.UpdateRow(win, item)

  def __init__(self, HeadRowClass, MainRowClass, *args, **kwargs):
    super(Grid, self).__init__(*args, **kwargs)

    sizer = wx.BoxSizer(wx.HORIZONTAL)

    # contains first col header and row header
    col_sizer1 = wx.BoxSizer(wx.VERTICAL)
    self.col_header1 = StretchRow(self, stretch_callback=self.OnCol1Stretch)

    self.col_header1.EnableDrag()
    # will populate with ColHead1.Add

    col_sizer1.Add(self.col_header1)

    # row header (sticky columns on LHS)
    self.row_header = HeadRowClass(self.col_header1, self)
    col_sizer1.Add(self.row_header, proportion=1)

    # register row header with controlling header
    self.col_header1.AddPanel(self.row_header)

    sizer.Add(col_sizer1, flag=wx.EXPAND)

    # contains 2nd col header and main item grid
    col_sizer2 = wx.BoxSizer(wx.VERTICAL)

    # need an intermeidate panel so ScrollWindow call
    # doesn't mess with child element positions inside col_header
    # throwing off offset calculations on resize
    self.ch_panel2 = wx.Panel(self)
    self.col_header2 = StretchRow(self.ch_panel2, stretch_callback=self.OnCol2Stretch)
    # important to set position, we can't manage this with a sizer
    # and without, when stretching, row Fit call will mess up scroll offset
    self.col_header2.SetPosition((0,0))

    self.col_header2.EnableDrag()

    # will populate with ColHead2.Add
    col_sizer2.Add(self.ch_panel2)

    # add grid
    self.grid = MainRowClass(self.col_header2, self, scroll_win=self)

    col_sizer2.Add(self.grid, proportion=1, flag=wx.EXPAND)

    # register grid with controlling header
    self.col_header2.AddPanel(self.grid)
    self.SetTargetWindow(self.grid)

    sizer.Add(col_sizer2, proportion=1, flag=wx.EXPAND)
    self.SetSizer(sizer)
    #self.SetScrollRate(1,1)

    # for manually scrolling headers
    self.x = 0
    self.y = 0

    self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
    self.Bind(wx.EVT_CHILD_FOCUS, self.OnFocus)

  # hack to update scrollbars
  def UpdateScrollbars(self):
    self.SetSize(self.GetSize())
    new_y = self.row_header.windows[0].GetPosition()[1]
    # stupidly sometimes things move down unexpectedly
    # ensure first rows always begin at 0

  def RegisterData(self, data):
    self.row_header.RegisterData(data)
    self.grid.RegisterData(data)
    if self.grid.item_height >= 0:
      self.SetScrollRate(1, self.grid.item_height)

  def Remove(self, index):
    self.row_header.Remove(index)
    self.grid.Remove(index)
    # hack to update scrollbars
    #self.SetSize(self.GetSize())
    self.UpdateScrollbars()
    #self.GetSizer().Layout()

  def GetColHead1(self):
    return self.col_header1

  def GetColHead2(self):
    return self.col_header2

  def OnCol1Stretch(self, win):
    # hack to update scrollbars
    #self.SetSize(self.GetSize())
    self.UpdateScrollbars()
    self.GetSizer().Layout()

  def OnCol2Stretch(self, win):
    # must manually adjust, FitInside won't work!
    # when scrolled it seems to truncate to visible window size
    vsize = self.grid.GetVirtualSize()
    vsize.width = win.GetSize().width
    self.grid.SetVirtualSize(vsize)
    # hack to update scrollbars
    #self.SetSize(self.GetSize())
    self.UpdateScrollbars()
    self.GetSizer().Layout()

  def OnScroll(self, e):
    if (e.GetOrientation() == wx.HORIZONTAL):
      cur_pos = e.GetPosition()
      self.ch_panel2.ScrollWindow(self.x - cur_pos, 0)
      self.x = cur_pos
    elif (e.GetOrientation() == wx.VERTICAL):
      # convert scroll units to px
      cur_pos = self.row_header.item_height * e.GetPosition()
      self.row_header.ScrollWindow(0, self.y - cur_pos)
      self.y = cur_pos
    e.Skip()

  # this is to disable autoscrolling on clicking elements or tabbing
  def OnFocus(self, e):
    pass
'''
