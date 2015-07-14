#!/usr/bin/env python2

import ConfigParser
import argparse
import re
from sys import exit
from os import rename


VERSION = '321.312.321'

def apply_config(section):
  try:
    type = section['type']
  except:
    print "Invalid config"
    exit(1)

  if type == 'version':
    regx = re.compile(section['regex'][1:-1])
    update_verison(section['name'], regx)
  elif type == 'config':
    return

def update_version(file, regex):
  done = False
  output = ''
  with open(file, 'r'):
    for line in f:
      if not done:
        result = re.match(regex, line)
      if result:
        line = line.replace(result.group(1), VERSION)
        found = True
      output += line
     f.close()

  rename(file, file + '.old')

  with open(file,'w')
    f.write(output)
    f.close()

      

def CreateSectionDict(section, config):
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
  config = ConfigParser.SafeConfigParser()
  try:
    config.readfp(open('data.cfg'))
  except:
    print "Could not read config!"
    exit(1)

  config_dict = {}
  for i in config.sections():
    config_dict[i] = CreateSectionDict(i, config)

  apply_config(config_dict["Spec"])

main()
