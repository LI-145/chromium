# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

_no_value = object()


def Collect(futures):
  '''Creates a Future which returns a list of results from each Future in
  |futures|.
  '''
  return Future(delegate=Gettable(lambda: [f.Get() for f in futures]))


class Gettable(object):
  '''Allows a Future to accept a callable as a delegate. Wraps |f| in a .Get
  interface required by Future.
  '''
  def __init__(self, f, *args):
    self._g = lambda: f(*args)
  def Get(self):
    return self._g()


class Future(object):
  '''Stores a value, error, or delegate to be used later.
  '''
  def __init__(self, value=_no_value, delegate=None, exc_info=None):
    self._value = value
    self._delegate = delegate
    self._exc_info = exc_info
    if (self._value is _no_value and
        self._delegate is None and
        self._exc_info is None):
      raise ValueError('Must have either a value, error, or delegate.')

  def Get(self):
    '''Gets the stored value, error, or delegate contents.
    '''
    if self._value is not _no_value:
      return self._value
    if self._exc_info is not None:
      self._Raise()
    try:
      self._value = self._delegate.Get()
      return self._value
    except:
      self._exc_info = sys.exc_info()
      self._Raise()

  def _Raise(self):
    exc_info = self._exc_info
    raise exc_info[0], exc_info[1], exc_info[2]
