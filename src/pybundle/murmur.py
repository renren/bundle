#!/usr/bin/env python
# coding=utf-8

def MurmurHash2(data, seed=0):
  """
  c implement http://sites.google.com/site/murmurhash/
  
  >>> MurmurHash2('a')
  2456313694L
  >>> MurmurHash2('ab')
  446775395L
  >>> MurmurHash2('abc')
  324500635L
  >>> MurmurHash2('abcd')
  646393889L
  """
  
  m = long(0x5bd1e995)
  r = 24

  data_len = len(data)
  h = long(seed ^ data_len)

  i = 0
  while data_len >= 4:
    k = ord(data[i+3]) << 24 | ord(data[i+2]) << 16 | ord(data[i+1]) << 8 | ord(data[i])
    k *= m
    k &= 0xffffffff
    k ^= k >> r
    
    k *= m
    k &= 0xffffffff

    h *= m
    h ^= k
    h &= 0xffffffff

    i += 4
    data_len -= 4
    
  tail = i

  if 3 == data_len:
    h ^= (ord(data[tail+2]) << 16)
    h ^= ord(data[tail+1]) << 8
    h ^= ord(data[tail])
    h *= m
    h &= 0xffffffff
  elif 2 == data_len:
    h ^= ord(data[tail+1]) << 8
    h ^= ord(data[tail])
    h *= m
    h &= 0xffffffff
  elif 1 == data_len:
    h ^= ord(data[tail])
    h *= m
    h &= 0xffffffff

  h ^= h >> 13
  h *= m
  h &= 0xffffffff
  
  h ^= (h >> 15)
  return h
  
if __name__ == "__main__":
    import sys
    if len(sys.argv) > 2:
      f = open(sys.argv[2], 'rb')
      print MurmurHash2(f.read())
    else:
      print MurmurHash2(str(sys.argv[1]))
