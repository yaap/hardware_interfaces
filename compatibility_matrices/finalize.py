#!/usr/bin/env python3
#
# Copyright (C) 2025 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Finalizes the current compatibility matrix and allows `next` targets to

use the new FCM.
"""

import argparse
import os
import pathlib
import re
import subprocess
import textwrap


def check_call(*args, **kwargs):
  print(args)
  subprocess.check_call(*args, **kwargs)


def check_output(*args, **kwargs):
  print(args)
  return subprocess.check_output(*args, **kwargs)


class Bump(object):

  def __init__(self, cmdline_args):
    self.top = pathlib.Path(os.environ["ANDROID_BUILD_TOP"])
    self.interfaces_dir = self.top / "hardware/interfaces"

    self.current_level = cmdline_args.current_level
    self.current_module_name = (
        f"framework_compatibility_matrix.{self.current_level}.xml"
    )
    self.device_module_name = "framework_compatibility_matrix.device.xml"

  def run(self):
    self.edit_android_bp()

  def edit_android_bp(self):
    android_bp = self.interfaces_dir / "compatibility_matrices/Android.bp"

    # update the SYSTEM_MATRIX_DEPS_<NEXT> variable to include the
    # latests FCM. This adds the file to `next` configs so releasing devices
    # can use the latest interfaces.
    # Only add this to the second SYSTEM_MATRIX_DEPS* soong variable which is
    # the older version used for `next`
    lines = []
    num_device_modules_seen = 0
    with open(android_bp) as f:
      for line in f:
        if f'    "{self.device_module_name}",\n' in line:
          if num_device_modules_seen == 1:
            lines.append(f'    "{self.current_module_name}",\n')
          num_device_modules_seen += 1
        lines.append(line)

    if num_device_modules_seen < 2:
      raise ValueError("Unexpected format of SYSTEM_MATRIX_DEPS* modules")


    with open(android_bp, "w") as f:
      f.write("".join(lines))


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      "current_level",
      type=str,
      help="VINTF level of the current version (e.g. 202404)",
  )
  cmdline_args = parser.parse_args()

  Bump(cmdline_args).run()


if __name__ == "__main__":
  main()
