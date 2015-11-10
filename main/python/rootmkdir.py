#!/usr/bin/env python

# ROOT command line tools: rootmkdir
# Author: Julien Ripoche
# Mail: julien.ripoche@u-psud.fr
# Date: 20/08/15

"""Command line to add directories in ROOT files"""

import cmdLineUtils
import sys

# Help strings
COMMAND_HELP = "Add directories in ROOT files"

PARENT_HELP = "make parent directories as needed, no error if existing."

EPILOG="""Examples:
- rootmkdir example.root:dir
  Add the directory 'dir' to the ROOT file 'example.root'

- rootmkdir example.root:dir1/dir2
  Add the directory 'dir2' in 'dir1' which is into the ROOT file 'example.root'

- rootmkdir -p example.root:dir1/dir2/dir3
  Make parent directories of 'dir3' as needed, no error if existing

- rootmkdir example.root
  Create an empty ROOT file named 'example.root'
"""

def execute():
    # Collect arguments with the module argparse
    parser = cmdLineUtils.getParserFile(COMMAND_HELP, EPILOG)
    parser.add_argument("-p", "--parents", help=PARENT_HELP, action="store_true")

    # Put arguments in shape
    sourceList, optDict = cmdLineUtils.getSourceListOptDict(parser, wildcards = False)

    # Process rootMkdir
    return cmdLineUtils.rootMkdir(sourceList, parents=optDict["parents"])

sys.exit(execute())
