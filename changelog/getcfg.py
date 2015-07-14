#!/usr/bin/env python2

import ConfigParser
import argparse
import re
from sys import exit

config = ConfigParser.SafeConfigParser()
try:
  config.readfp(open('data.cfg'))
except:
  print "Could not read config!"
  exit(1)

VERSION = '321.312.321'

def apply_config(section):
  found = False
  output = ''
  regx = re.compile(section['regex'][1:-1])
  with open(section['name'], 'r') as f:
    for line in f:
      if not found:
        result = re.match(regx, line)
        if not result == None:
          line = line.replace(result.group(1), VERSION)
          found = True
      output += line
    f.close()

  print output
  with open(section['name'], 'w') as f:
    f.write(output)
    f.close()
      

def CreateSectionDict(section):
  config_section = {}
  section_options = config.options(section)

  for option in section_options:
    try:
      config_section[option] = config.get(section, option)
    except:
      print "Failure on %s" % option
      config_section[option] = None

  return config_section


def main():
  config_dict = {}
  for i in config.sections():
    config_dict[i] = CreateSectionDict(i)

#  for s in config_dict:
  apply_config(config_dict["Spec"])

main()
