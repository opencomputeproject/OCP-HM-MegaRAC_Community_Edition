#!/usr/bin/env python

r"""
Create index files that can be displayed as web pages in a given directory
and all its sub-directories. There are options to exclude certain files and
sub-directories.
"""

import argparse
import os
import sys


def main(i_raw_args):
    l_args = parse_args(i_raw_args)
    create_index_file(l_args.logs_dir_path, '/', l_args.exclude)


def create_index_file(i_dir_path, i_pretty_dir_path, i_exclude_list):
    r"""
    Create HTML index files for a given directory and all its sub-directories.

    Description of argument(s):
    i_dir_path          The directory to generate an index file for.
    i_pretty_dir_path   A pretty version of i_dir_path that can be shown to
                        readers of the HTML page. For example, if i_dir_path
                        is set to '/home/johndoe/logs/`, the caller may wish
                        to only show '/logs/' in the HTML index pages.
    i_exclude_list      A Python list of files and directories to exclude from
    """

    l_index_file_path = os.path.join(i_dir_path, 'index.html')
    l_sub_dir_list = os.listdir(i_dir_path)

    # Created a sorted list of sub-directories in this directory
    l_dirs = sorted(
        [d for d
         in l_sub_dir_list
         if os.path.isdir(os.path.join(i_dir_path, d))
            and d not in i_exclude_list])

    # Create a sorted list of files in this directory
    l_files = sorted(
        [f for f
         in l_sub_dir_list
         if not os.path.isdir(os.path.join(i_dir_path, f))
            and f not in i_exclude_list])

    # Open up the index file we're going to write to.
    with open(l_index_file_path, 'w+') as l_index_file:
        l_index_file.write(
            '<html>\n'
            '<head><title>' + i_pretty_dir_path + '</title></head>\n'
            '<body>\n'
            '<h2>OpenBMC Logs</h2>\n'
            '<h3>' + i_pretty_dir_path + '</h3>\n')

        # Only show the link to go up a directory if this is not the root.
        if not i_pretty_dir_path == '/':
            l_index_file.write('<a href=".."><img src="/dir.png"> ..</a><br>\n')

        # List directories first.
        for l_dir in l_dirs:
            l_index_file.write(
                '<a href="%s"><img src="/dir.png"> %s</a><br>\n'
                % (l_dir, l_dir))
            create_index_file(
                os.path.join(i_dir_path, l_dir),
                i_pretty_dir_path + l_dir + '/',
                i_exclude_list)

        # List files second.
        for l_file in l_files:
            l_index_file.write('<a href="%s"><img src="/file.png"> %s</a><br>\n'
                               % (l_file, l_file))

        l_index_file.write('</body>\n</html>')


def parse_args(i_raw_args):
    r"""
    Parse the given list as command-line arguments and return an object with
    the argument values.

    Description of argument(s):
    i_raw_args  A list of command-line arguments, usually taken from
                sys.argv[1:].
    """

    parser = argparse.ArgumentParser(
        description="%(prog)s will create index files that can be displayed "
                    "as web pages in a given directory and all its "
                    "sub-directories.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        'logs_dir_path',
        help='Directory containing the logs that should be uploaded.')
    parser.add_argument(
        '--exclude',
        nargs='+',
        default=['.git', 'index.html'],
        help='A space-delimited list of files to exclude from the index.'
    )
    return parser.parse_args(i_raw_args)


if __name__ == '__main__':
    main(sys.argv[1:])