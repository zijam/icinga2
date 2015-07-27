#!/usr/bin/env python
#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
# *                                                                            *
# * This program is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU General Public License                *
# * as published by the Free Software Foundation; either version 2             *
# * of the License, or (at your option) any later version.                     *
# *                                                                            *
# * This program is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
# * GNU General Public License for more details.                               *
# *                                                                            *
# * You should have received a copy of the GNU General Public License          *
# * along with this program; if not, write to the Free Software Foundation     *
# * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
# ******************************************************************************/

import urllib2, json, sys, string
from argparse import ArgumentParser

DESCRIPTION = "update release changes"
VERSION="1.1.0"
ISSUE_URL="https://dev.icinga.org/issues/"

def format_header(text, lvl, ftype):
   if ftype == "html":
       return "<h%s>%s</h%s>" % (lvl, text, lvl)
   if ftype == "md":
       return "#" * lvl + " " + text

def format_logentry(log_entry, args, issue_url=ISSUE_URL):
   if args.links:
       if args.html:
           return "<li> {0} <a href=\"{3}{1}\">{1}</a>: {2}</li>".format(log_entry[0], log_entry[1], log_entry[2], issue_url)
       else:
           return "* {0} [{1}]({3}{1} \"{0} {1}\"): {2}".format(log_entry[0], log_entry[1], log_entry[2], issue_url)
   else:
       if args.html:
           return "<li>%s %d: %s</li>" % log_entry
       else:
           return "* %s %d: %s" % log_entry

def main():
  arg_parser = ArgumentParser(description= "%s (Version: %s)" % (DESCRIPTION, VERSION))
  arg_parser.add_argument('-V', '--version', required=True, type=str, help="define version to query")
  arg_parser.add_argument('-p', '--project', type=str, help="project name (default = i2)")
  arg_parser.add_argument('-l', '--links', action='store_true', help="add urls to issues")
  arg_parser.add_argument('-H', '--html', action='store_true', help="print html output (defaults to markdown)")
  arg_parser.add_argument('--config', type=str, help='path to changelog config (default changelog.cfg)')
  arg_parser.add_argument('--print', dest='verbose', action='store_true', help="also print changelog (old behaviour)")

  args = arg_parser.parse_args(sys.argv[1:])

  project = "i2" if not args.project else args.project
  ftype = "md" if not args.html else "html"

  version_name = args.version

  rsp = urllib2.urlopen("https://dev.icinga.org/projects/%s/versions.json" % (project))
  versions_data = json.loads(rsp.read())

  version_id = None

  for version in versions_data["versions"]:
    if version["name"] == version_name:
      version_id = version["id"]
      break

  if version_id == None:
    print "Version '%s' not found." % (version_name)
    return 1

  changes = ""

  if "custom_fields" in version:
    for field in version["custom_fields"]:
      if field["id"] == 14:
        changes = field["value"]
        break

    changes = string.join(string.split(changes, "\r\n"), "\n")
  
  changelog = format_header("What's New in Version {0}".format(version_name, args), 3, ftype) + "\n\n"
    
  if changes:
    changelog += "{0}\n\n{1}\n\n".format(format_header("Changes", 4, ftype), changes)

  offset = 0

  log_entries = []

  while True:
    # We could filter using &cf_13=1, however this doesn't currently work because the custom field isn't set
    # for some of the older tickets:
    rsp = urllib2.urlopen("https://dev.icinga.org/projects/%s/issues.json?offset=%d&status_id=closed&fixed_version_id=%d" % (project, offset, version_id))
    issues_data = json.loads(rsp.read())
    issues_count = len(issues_data["issues"])
    offset = offset + issues_count

    if issues_count == 0:
      break

    for issue in issues_data["issues"]:
      ignore_issue = False

      if "custom_fields" in issue:
        for field in issue["custom_fields"]:
          if field["id"] == 13 and "value" in field and field["value"] == "0":
            ignore_issue = True
            break

          if ignore_issue:
            continue

      log_entries.append((issue["tracker"]["name"], issue["id"], issue["subject"].strip()))

  for p in range(2):
    not_empty = False

    for log_entry in log_entries:
      if (p == 0 and log_entry[0] == "Feature") or (p == 1 and log_entry[0] != "Feature"):
        not_empty = True

    if not_empty:
      changelog += \
        (format_header("Features", 4, ftype) if p == 0 else format_header("Bugfixes", 4, ftype)) \
        + "\n\n"
    if args.html:
      changelog += "<ul>\n"

    for log_entry in sorted(log_entries):
      if (p == 0 and log_entry[0] == "Feature") or (p == 1 and log_entry[0] != "Feature"):
        changelog += format_logentry(log_entry, args) + '\n'

    if not_empty:
      if args.html:
        changelog += "</ul>\n"
    changelog += '\n'

  if args.verbose:
    print changelog

  return 0

if __name__ == "__main__":
  main()
