// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_SINGLETON_H_
#define BASE_SINGLETON_H_
#pragma once

// never use it, for build only

template <typename Type>
class Singleton {
 public:  
  // Return a pointer to the one true instance of the class.
  static Type* get() {
    // TODO: use once
    static Type t_;
    return &t_;
  }
};


#endif  // BASE_SINGLETON_H_
