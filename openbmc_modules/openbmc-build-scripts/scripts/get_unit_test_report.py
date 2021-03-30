#!/usr/bin/python

# This script generates the unit test coverage report for openbmc project.
#
# Usage:
# get_unit_test_report.py target_dir [url_file]
#
# Positional arguments:
# target_dir  Target directory in pwd to place all cloned repos and logs.
# url_file    Text file containing url of repositories. Optional.
#             By using this argument, the user can get a report only for
#             specific repositories given in the file.
#             Refer ./scripts/repositories.txt
#
# Examples:
#     get_unit_test_report.py target_dir
#     get_unit_test_report.py target_dir repositories.txt
#
# Output format:
#
# ***********************************OUTPUT***********************************
# https://github.com/openbmc/phosphor-dbus-monitor.git               NO
# https://github.com/openbmc/phosphor-sel-logger.git;protocol=git    NO
# ***********************************OUTPUT***********************************
#
# Other outputs and errors are redirected to output.log and debug.log in target_dir.

import argparse
import logging
import os
import re
import requests
import shutil
import sys
import subprocess

# Repo list not expected to contain UT. Will be moved to a file in future.
skip_list = ["openbmc-tools", "inarp", "openbmc", "openbmc.github.io",
             "phosphor-ecc", "phosphor-pcie-presence", "phosphor-u-boot-env-mgr",
             "rrd-ipmi-blob", "librrdplus", "openpower-inventory-upload",
             "openpower-logging", "openpower-power-control", "docs",
             "openbmc-test-automation", "openbmc-build-scripts", "skeleton",
             "linux",
             # Not active, expected to be archived soon.
             "ibm-pldm-oem"]


# Create parser.
text = '''%(prog)s target_dir [url_file]

Example usages:
get_unit_test_report.py target_dir
get_unit_test_report.py target_dir repositories.txt'''

parser = argparse.ArgumentParser(usage=text,
                                 description="Script generates the unit test coverage report")
parser.add_argument("target_dir", type=str,
                    help='''Name of a non-existing directory in pwd to store all
                            cloned repos, logs and UT reports''')
parser.add_argument("url_file", type=str, nargs='?',
                    help='''Text file containing url of repositories.
                            By using this argument, the user can get a report only for
                            specific repositories given in the file.
                            Refer ./scripts/repositories.txt''')
args = parser.parse_args()

input_urls = []
if args.url_file:
    try:
        # Get URLs from the file.
        with open(args.url_file) as reader:
            file_content = reader.read().splitlines()
            input_urls = list(filter(lambda x:x, file_content))
        if not(input_urls):
            print("Input file {} is empty. Quitting...".format(args.url_file))
            quit()
    except IOError as e:
        print("Issue in reading file '{}'. Reason: {}".format(args.url_file,
                                                                  str(e)))
        quit()


# Create target working directory.
pwd = os.getcwd()
working_dir = os.path.join(pwd, args.target_dir)
try:
    os.mkdir(working_dir)
except OSError as e:
    answer = raw_input("Target directory " + working_dir + " already exists. "
                       + "Do you want to delete [Y/N]: ")
    if answer == "Y":
        try:
            shutil.rmtree(working_dir)
            os.mkdir(working_dir)
        except OSError as e:
            print(str(e))
            quit()
    else:
        print("Exiting....")
        quit()

# Create log directory.
log_dir = os.path.join(working_dir, "logs")
try:
    os.mkdir(log_dir)
except OSError as e:
    print("Unable to create log directory: " + log_dir)
    print(str(e))
    quit()


# Log files
debug_file = os.path.join(log_dir, "debug.log")
output_file = os.path.join(log_dir, "output.log")
logging.basicConfig(format='%(levelname)s - %(message)s', level=logging.DEBUG,
                    filename=debug_file)
logger = logging.getLogger(__name__)

# Create handlers
console_handler = logging.StreamHandler()
file_handler = logging.FileHandler(output_file)
console_handler.setLevel(logging.INFO)
file_handler.setLevel(logging.INFO)

# Create formatters and add it to handlers
log_format = logging.Formatter('%(message)s')
console_handler.setFormatter(log_format)
file_handler.setFormatter(log_format)

# Add handlers to the logger
logger.addHandler(console_handler)
logger.addHandler(file_handler)


# Create report directory.
report_dir = os.path.join(working_dir, "reports")
try:
    os.mkdir(report_dir)
except OSError as e:
    logger.error("Unable to create report directory: " + report_dir)
    logger.error(str(e))
    quit()

# Clone OpenBmc build scripts.
try:
    output = subprocess.check_output("git clone https://github.com/openbmc/openbmc-build-scripts.git",
                                     shell=True, cwd=working_dir, stderr=subprocess.STDOUT)
    logger.debug(output)
except subprocess.CalledProcessError as e:
    logger.error(e.output)
    logger.error(e.cmd)
    logger.error("Unable to clone openbmc-build-scripts")
    quit()

repo_data = []
if input_urls:
    api_url = "https://api.github.com/repos/openbmc/"
    for url in input_urls:
        try:
            repo_name = url.strip().split('/')[-1].split(";")[0].split(".")[0]
        except IndexError as e:
            logger.error("ERROR: Unable to get sandbox name for url " + url)
            logger.error("Reason: " + str(e))
            continue

        try:
            resp = requests.get(api_url + repo_name)
            if resp.status_code != 200:
                logger.info(api_url + repo_name + " ==> " + resp.reason)
                continue
            repo_data.extend([resp.json()])
        except ValueError as e:
            logger.error("ERROR: Failed to get response for " + repo_name)
            logger.error(resp)
            continue

else:
    # Get number of pages.
    resp = requests.head('https://api.github.com/users/openbmc/repos')
    if resp.status_code != 200:
        logger.error("Error! Unable to get repositories")
        logger.error(resp.status_code)
        logger.error(resp.reason)
        quit()
    num_of_pages = int(resp.links['last']['url'].split('page=')[-1])
    logger.debug("No. of pages: " + str(num_of_pages))

    # Fetch data from all pages.
    for page in range(1, num_of_pages+1):
        resp = requests.get('https://api.github.com/users/openbmc/repos?page='
                            + str(page))
        data = resp.json()
        repo_data.extend(data)


# Get URLs and their archive status from response.
url_info = {}
for repo in repo_data:
    try:
        url_info[repo["clone_url"]] = repo["archived"]
    except KeyError as e:
        logger.error("Failed to get archived status of {}".format(repo))
        url_info[repo["clone_url"]] = False
        continue
logger.debug(url_info)
repo_count = len(url_info)
logger.info("Number of repositories (Including archived): " + str(repo_count))

# Clone repository and run unit test.
coverage_report = []
counter = 0
tested_report_count = 0
coverage_count = 0
unit_test_count = 0
no_report_count = 0
error_count = 0
skip_count = 0
archive_count = 0
url_list = sorted(url_info)
for url in url_list:
    ut_status = "NO"
    skip = False
    if url_info[url]:
        ut_status = "ARCHIVED"
        skip = True
    else:
        try:
            # Eg: url = "https://github.com/openbmc/u-boot.git"
            #     sandbox_name = "u-boot"
            sandbox_name = url.strip().split('/')[-1].split(";")[0].split(".")[0]
        except IndexError as e:
            logger.error("ERROR: Unable to get sandbox name for url " + url)
            logger.error("Reason: " + str(e))
            continue

        if (sandbox_name in skip_list or
            re.match(r'meta-', sandbox_name)):
            logger.debug("SKIPPING: " + sandbox_name)
            skip = True
            ut_status = "SKIPPED"
        else:
            checkout_cmd = "rm -rf " + sandbox_name + ";git clone " + url
            try:
                subprocess.check_output(checkout_cmd, shell=True, cwd=working_dir,
                                                 stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                logger.debug(e.output)
                logger.debug(e.cmd)
                logger.debug("Failed to clone " + sandbox_name)
                ut_status = "ERROR"
                skip = True
    if not(skip):
        docker_cmd = "WORKSPACE=$(pwd) UNIT_TEST_PKG=" + sandbox_name + " " + \
                     "./openbmc-build-scripts/run-unit-test-docker.sh"
        try:
            result = subprocess.check_output(docker_cmd, cwd=working_dir, shell=True,
                                             stderr=subprocess.STDOUT)
            logger.debug(result)
            logger.debug("UT BUILD COMPLETED FOR: " + sandbox_name)

        except subprocess.CalledProcessError as e:
            logger.debug(e.output)
            logger.debug(e.cmd)
            logger.debug("UT BUILD EXITED FOR: " + sandbox_name)
            ut_status = "ERROR"

        folder_name = os.path.join(working_dir, sandbox_name)
        repo_report_dir = os.path.join(report_dir, sandbox_name)

        report_names = ("coveragereport", "test-suite.log", "LastTest.log")
        find_cmd = "".join("find " + folder_name + " -name " + report + ";"
                           for report in report_names)
        try:
            result = subprocess.check_output(find_cmd, shell=True)
        except subprocess.CalledProcessError as e:
            logger.debug(e.output)
            logger.debug(e.cmd)
            logger.debug("CMD TO FIND REPORT FAILED FOR: " + sandbox_name)
            ut_status = "ERROR"

        if ut_status != "ERROR":
            if result:
                if result.__contains__("coveragereport"):
                    ut_status = "YES, COVERAGE"
                    coverage_count += 1
                elif "test-suite.log" in result:
                    ut_status = "YES, UNIT TEST"
                    unit_test_count += 1
                elif "LastTest.log" in result:
                    file_names = result.splitlines()
                    for file in file_names:
                        pattern_count_cmd = "sed -n '/Start testing/,/End testing/p;' " + \
                              file + "|wc -l"
                        try:
                            num_of_lines = subprocess.check_output(pattern_count_cmd,
                                                                   shell=True)
                        except subprocess.CalledProcessError as e:
                            logger.debug(e.output)
                            logger.debug(e.cmd)
                            logger.debug("CONTENT CHECK FAILED FOR: " + sandbox_name)
                            ut_status = "ERROR"

                        if int(num_of_lines.strip()) > 5:
                            ut_status = "YES, UNIT TEST"
                            unit_test_count += 1

        if "YES" in ut_status:
            tested_report_count += 1
            result = result.splitlines()
            for file_path in result:
                destination = os.path.dirname(os.path.join(report_dir,
                                                           os.path.relpath(file_path,
                                                                           working_dir)))
                copy_cmd = "mkdir -p " + destination + ";cp -rf " + \
                           file_path.strip() + " " + destination
                try:
                    subprocess.check_output(copy_cmd, shell=True)
                except subprocess.CalledProcessError as e:
                    logger.debug(e.output)
                    logger.debug(e.cmd)
                    logger.info("FAILED TO COPY REPORTS FOR: " + sandbox_name)

    if ut_status == "ERROR":
        error_count += 1
    elif ut_status == "NO":
        no_report_count += 1
    elif ut_status == "SKIPPED":
        skip_count += 1
    elif ut_status == "ARCHIVED":
        archive_count += 1

    coverage_report.append("{:<65}{:<10}".format(url.strip(), ut_status))
    counter += 1
    logger.info(str(counter) + " in " + str(repo_count) + " completed")

logger.info("*" * 30 + "UNIT TEST COVERAGE REPORT" + "*" * 30)
for res in coverage_report:
    logger.info(res)
logger.info("*" * 30 + "UNIT TEST COVERAGE REPORT" + "*" * 30)

logger.info("REPORTS: " + report_dir)
logger.info("LOGS: " + log_dir)
logger.info("*" * 85)
logger.info("SUMMARY: ")
logger.info("TOTAL REPOSITORIES     : " + str(repo_count))
logger.info("TESTED REPOSITORIES    : " + str(tested_report_count))
logger.info("ERROR                  : " + str(error_count))
logger.info("COVERAGE REPORT        : " + str(coverage_count))
logger.info("UNIT TEST REPORT       : " + str(unit_test_count))
logger.info("NO REPORT              : " + str(no_report_count))
logger.info("SKIPPED                : " + str(skip_count))
logger.info("ARCHIVED               : " + str(archive_count))
logger.info("*" * 85)
