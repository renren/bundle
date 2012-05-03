#!/usr/bin/env python
# coding=utf-8

def ToSixty(number):
  """
  >>> ToSixty(12324234)
  7DY4
  """
  assert type(number) in (int, long)
  if number < 0:
    return ""

  result = []
  base = 60
  SixtyChar = "ABCDEFGHJKLMNOPQRSTUVWXYZabcdefghjklmnopqrstuvwxyz0123456789"

  while True:
    m = number % base
    result.append(SixtyChar[m])
    number /= base
    if number == 0:
      break
  return "".join(result[::-1])

_CharTable = [
  50, 51, 52, 53, 54, 55, 56, 57, 58, 59, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2
  , 3, 4, 5, 6, 7, -1, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
  , 22, 23, 24, -1, -1, -1, -1, -1, -1, 25, 26, 27, 28, 29, 30, 31, 32, -1
  , 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49
]
_PowTable = [
  1, 60, 3600, 216000, 12960000, 777600000, 46656000000, 2799360000000,
  167961600000000, 10077696000000000, 604661760000000000
]

def FromSixty(sixty):
  """
  >>> FromSixty("MJNhPBlxMM2")
  6734006701145392312
  """
  assert isinstance(sixty, str)
  size = len(sixty)
  ten = 0

  if size > 11:
    return -1;

  for j in range(0, size):
    c = sixty[j]
    if c < '0' or c > 'z':
      return -1

    index = _CharTable[ord(c) - ord('0')]

    ten += index * _PowTable[size - j - 1]

  return ten


def encode(s):
    pass

def FilePath(index):
  """
  >>> FilePath(0)
  /mnt/mfs/p/00/00000000
  """
  filepath = []
  filepath.append("/mnt/mfs/p/")
  filepath.append("%02d/%08d" % (index % 50, index))
  return "".join(filepath)

if __name__ == "__main__":
    # import doctest
    # doctest.testmod()

    import sys
    if len(sys.argv) > 3:
      print "the argvs is too more"
    elif int(sys.argv[1]) == 1:
      print ToSixty(int(sys.argv[2]))
    elif int(sys.argv[1]) == 2:
      print FromSixty(str(sys.argv[2]))
    elif int(sys.argv[1]) == 3:
      print FilePath(int(sys.argv[2]))
