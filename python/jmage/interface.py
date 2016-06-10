import collections
import ctypes

from ctypes import byref
from ctypes import POINTER
from ctypes import c_int
from ctypes import c_char
from ctypes import c_char_p
from ctypes import c_float
from ctypes import c_double
from ctypes import c_size_t
from ctypes import Structure
from ctypes import Union

PATH_MAX = 4096

LOOP_OFF = 0
LOOP_CONTINUOUS = 1
LOOP_ONE_SHOT = 2

class key_zone(Structure):
  _fields_ = [('wave', POINTER(c_float)),
    ('num_channels', c_int),
    ('wave_length', c_int),
    ('start', c_int),
    ('left', c_int),
    ('right', c_int),
    ('low_key', c_int),
    ('high_key', c_int),
    ('origin', c_int),
    ('low_vel', c_int),
    ('high_vel', c_int),
    ('amp', c_float),
    ('attack', c_int),
    ('hold', c_int),
    ('decay', c_int),
    ('sustain', c_float),
    ('release', c_int),
    ('pitch_corr', c_double),
    ('mode', c_int),
    ('crossfade', c_int),
    ('name', c_char * PATH_MAX),
    ('path', c_char * PATH_MAX)
  ]

lib = ctypes.cdll.LoadLibrary('libjm_sampler.so')

lib.jm_init_key_zone.argtypes = [POINTER(key_zone)]
lib.jm_zone_set_name.argtypes = [POINTER(key_zone), c_char_p]
lib.jm_zone_set_path.argtypes = [POINTER(key_zone), c_char_p]

def init_key_zone(zone):
  lib.jm_init_key_zone(byref(zone))

def zone_set_name(zone, name):
  lib.jm_zone_set_name(byref(zone), name)

def zone_set_path(zone, path):
  lib.jm_zone_set_path(byref(zone), path)

class _ZoneList(Structure):
  pass

lib.jm_new_zonelist.restype = POINTER(_ZoneList)
lib.jm_destroy_zonelist.argtypes = [POINTER(_ZoneList)]
lib.jm_zonelist_insert.argtypes = [POINTER(_ZoneList), c_int, POINTER(key_zone)]
lib.jm_zonelist_get.argtypes = [POINTER(_ZoneList), c_int, POINTER(key_zone)]
lib.jm_zonelist_get.restype = c_int
lib.jm_zonelist_set.argtypes = [POINTER(_ZoneList), c_int, POINTER(key_zone)]
lib.jm_zonelist_set.restype = c_int
lib.jm_zonelist_erase.argtypes = [POINTER(_ZoneList), c_int]
lib.jm_zonelist_erase.restype = c_int
lib.jm_zonelist_size.argtypes = [POINTER(_ZoneList)]
lib.jm_zonelist_size.restype = c_size_t
lib.jm_zonelist_lock.argtypes = [POINTER(_ZoneList)]
lib.jm_zonelist_unlock.argtypes = [POINTER(_ZoneList)]

class ZoneList(collections.MutableSequence):
  def __init__(self):
    self.list = lib.jm_new_zonelist()

  def destroy(self):
    lib.jm_destroy_zonelist(self.list)

  def insert(self, index, zone):
    lib.jm_zonelist_lock(self.list)
    lib.jm_zonelist_insert(self.list, index, byref(zone))
    lib.jm_zonelist_unlock(self.list)

  def __getitem__(self, index):
    zone = key_zone()
    ret = lib.jm_zonelist_get(self.list, index, byref(zone))
    if ret != 0:
      raise IndexError
    return zone

  def __setitem__(self, index, zone):
    lib.jm_zonelist_lock(self.list)
    ret = lib.jm_zonelist_set(self.list, index, byref(zone))
    lib.jm_zonelist_unlock(self.list)
    if ret != 0:
      raise IndexError

  def __delitem__(self, index):
    lib.jm_zonelist_lock(self.list)
    ret = lib.jm_zonelist_erase(self.list, index)
    lib.jm_zonelist_unlock(self.list)
    if ret != 0:
      raise IndexError

  def __len__(self):
    return lib.jm_zonelist_size(self.list)

class _Sampler(Structure):
  pass

class msg_data(Union):
  _fields_ = [('i', c_int)]

MT_VOLUME = 0

class msg(Structure):
  _fields_ = [('type', c_int), ('data', msg_data)]

lib.jm_new_sampler.argtypes = [POINTER(_ZoneList)]
lib.jm_new_sampler.restype = POINTER(_Sampler)
lib.jm_destroy_sampler.argtypes = [POINTER(_Sampler)]
lib.jm_send_msg.argtypes = [POINTER(_Sampler), POINTER(msg)]
lib.jm_receive_msg.argtypes = [POINTER(_Sampler), POINTER(msg)]
lib.jm_receive_msg.restype = c_int

class Sampler(object):
  def __init__(self, zone_list):
    self.sampler = lib.jm_new_sampler(zone_list.list)

  def destroy(self):
    lib.jm_destroy_sampler(self.sampler)

  def send_msg(self, msg):
    lib.jm_send_msg(self.sampler, byref(msg))

  def receive_msg(self, msg):
    lib.jm_receive_msg(self.sampler, byref(msg))
