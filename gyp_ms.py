#!/usr/bin/env python

import os
import platform
import sys

CC = os.environ.get('CC', 'cc')
script_dir = os.path.dirname(__file__)
ms_root = os.path.normpath(script_dir)
sys.path.insert(0, os.path.join(ms_root, 'build', 'gyp', 'pylib'))
try:
  import gyp
except ImportError:
  print('You need to install gyp in build/gyp first. See the README.')
  sys.exit(42)

def host_arch():
  machine = platform.machine()
  if machine == 'i386': return 'ia32'
  if machine == 'AMD64': return 'x64'
  if machine == 'x86_64': return 'x64'
  if machine.startswith('arm'): return 'arm'
  if machine.startswith('mips'): return 'mips'
  return machine  # Return as-is and hope for the best.

def run_gyp(args):
  rc = gyp.main(args)
  if rc != 0:
    print('Error running GYP')
    sys.exit(rc)

def main():
  args = sys.argv[1:]
  args.append('--depth=' + ms_root)

  if '-f' not in args:
      args.extend('-f make'.split())

  if not any(a.startswith('-Dhost_arch=') for a in args):
    args.append('-Dhost_arch=%s' % host_arch())

  if not any(a.startswith('-Dtarget_arch=') for a in args):
    args.append('-Dtarget_arch=%s' % host_arch())

  run_gyp(args)

if __name__ == "__main__":
  main()