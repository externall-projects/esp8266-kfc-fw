#
# Author: sascha_lammers@gmx.de
#

import sys
import os
import re
import argparse

parser = argparse.ArgumentParser(description="Build Number Tool")
parser.add_argument("buildfile", help="build.h", type=argparse.FileType("r+"))
parser.add_argument("--print-contents", help="print build instead of writing to file", action="store_true", default=False)
parser.add_argument("--print-number", help="print build number", action="store_true", default=False)
parser.add_argument("-v", "--verbose", help="verbose output", action="store_true", default=False)
args = parser.parse_args()

lines = args.buildfile.read().split("\n");
number = 0

output = ""
for line in lines:
    m = re.search("#define\s*__BUILD_NUMBER\s*\"(.*?)\"", line)
    try:
        number = int(m.group(1).strip());
        line = "#define __BUILD_NUMBER \"" + str(number + 1) + "\""
    except:
        pass
    if args.print_number:
        if number!=0:
            print(number)
            break
    elif args.print_contents:
        print(line)
    else:
        output += line + "\n"

if number==0:
    raise Exception("Cannot find build number")

if args.verbose:
    print("New build number: " + str(number + 1))

if output!="":
    args.buildfile.seek(0)
    args.buildfile.write(output.replace("\n\n\n", "\n\n"))
