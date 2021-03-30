#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018 IBM Corp.

import argparse
import sh
import os
import maintainers
from pprint import pprint
import requests
import json
from typing import List, Dict, Union, cast, Iterator
import sys
import itertools

git = sh.git.bake()

mailmap = {
    'andrewg@us.ibm.com' : 'geissonator@yahoo.com',
}

def gerrit_url(name: str, user: str) -> str:
    return "ssh://{}@gerrit.openbmc-project.xyz:29418/openbmc/{}".format(user, name)

def gerrit_push_args(reviewers: Iterator[maintainers.Identity]) -> str:
    addrs = (i.email.address for i in reviewers)
    maddrs = (mailmap[a] if a in mailmap else a for a in addrs)
    return ','.join("r={}".format(ma) for ma in maddrs)

def gerrit_push(name: str, user: str, reviewers: Iterator[maintainers.Identity]) -> None:
    refspec = 'HEAD:refs/for/master/maintainers%{}'.format(gerrit_push_args(reviewers))
    git.push(gerrit_url(name, user), refspec)

def org_repos_url(name) -> str:
    return "https://api.github.com/users/{}/repos?per_page=100".format(name)

V = Union[Dict[str, str], str]
E = Dict[str, V]
R = List[E]

def org_repos(name: str) -> R:
    r = requests.get(org_repos_url(name))
    if not r.ok:
        raise ValueError("Bad organisation name")
    return json.loads(r.text or r.content)

def git_reset_upstream(name: str) -> None:
    cwd = os.getcwd()
    os.chdir(name)
    git.fetch("origin")
    git.reset("--hard", "origin/master")
    os.chdir(cwd)

def ensure_org_repo(name: str, user: str) -> str:
    if os.path.exists(os.path.join(name, ".git")):
        # git_reset_upstream(name)
        pass
    else:
        git.clone(gerrit_url(name, user), name)
    scp_src = "{}@gerrit.openbmc-project.xyz:hooks/commit-msg".format(user)
    scp_dst = "{}/.git/hooks/".format(name)
    sh.scp("-p", "-P", 29418, scp_src, scp_dst)
    return name

def repo_url(name: str) -> str:
    return "https://github.com/openbmc/{}.git".format(name)

def ensure_repo(name: str) -> str:
    if os.path.exists(os.path.join(name, ".git")):
        # git_reset_upstream(name)
        pass
    else:
        git.clone(repo_url(name), name)
    return name

preamble_text = """\
How to use this list:
    Find the most specific section entry (described below) that matches where
    your change lives and add the reviewers (R) and maintainers (M) as
    reviewers. You can use the same method to track down who knows a particular
    code base best.

    Your change/query may span multiple entries; that is okay.

    If you do not find an entry that describes your request at all, someone
    forgot to update this list; please at least file an issue or send an email
    to a maintainer, but preferably you should just update this document.

Description of section entries:

    Section entries are structured according to the following scheme:

    X:  NAME <EMAIL_USERNAME@DOMAIN> <IRC_USERNAME!>
    X:  ...
    .
    .
    .

    Where REPO_NAME is the name of the repository within the OpenBMC GitHub
    organization; FILE_PATH is a file path within the repository, possibly with
    wildcards; X is a tag of one of the following types:

    M:  Denotes maintainer; has fields NAME <EMAIL_USERNAME@DOMAIN> <IRC_USERNAME!>;
        if omitted from an entry, assume one of the maintainers from the
        MAINTAINERS entry.
    R:  Denotes reviewer; has fields NAME <EMAIL_USERNAME@DOMAIN> <IRC_USERNAME!>;
        these people are to be added as reviewers for a change matching the repo
        path.
    F:  Denotes forked from an external repository; has fields URL.

    Line comments are to be denoted "# SOME COMMENT" (typical shell style
    comment); it is important to follow the correct syntax and semantics as we
    may want to use automated tools with this file in the future.

    A change cannot be added to an OpenBMC repository without a MAINTAINER's
    approval; thus, a MAINTAINER should always be listed as a reviewer.

START OF MAINTAINERS LIST
-------------------------

"""

def generate_maintainers_change(name: str, block: maintainers.B,
        default: maintainers.B, user: str) -> None:
    cwd = os.getcwd()
    os.chdir(name)
    mpath = "MAINTAINERS"
    try:
        if os.path.exists(mpath):
            print("{} already exists, skipping".format(mpath))
            return
        with open(mpath, 'w') as m:
            m.write(preamble_text)
            maintainers.assemble_block(block, default, m)
        git.add(mpath)
        git.commit("-s", "-m", "Add {} file".format(mpath), _out=sys.stdout)
        with open(mpath, 'r') as m:
            maintainers.trash_preamble(m)
            block = maintainers.parse_block(m)
            pprint(block)
            audience = cast(List[maintainers.Identity],
                    block[maintainers.LineType.MAINTAINER][:])
            if maintainers.LineType.REVIEWER in block:
                reviewers = cast(List[maintainers.Identity],
                        block[maintainers.LineType.REVIEWER])
                audience.extend(reviewers)
            gerrit_push(name, user, iter(audience))
    finally:
        os.chdir(cwd)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--organisation", type=str, default="openbmc")
    parser.add_argument("--user", type=str, default="amboar")
    args = parser.parse_args()
    ensure_repo("docs")
    with open('docs/MAINTAINERS', 'r') as mfile:
        mast = maintainers.parse_maintainers(mfile)

    # Don't leak the generic comment into the repo-specific MAINTAINERS file
    del mast['MAINTAINERS'][maintainers.LineType.COMMENT]

    for e in org_repos(args.organisation):
        print("Ensuring MAINTAINERS for {}".format(e['name']))
        name = cast(str, e['name'])
        try:
            ensure_org_repo(name, args.user)
            default = mast['MAINTAINERS']
            block = mast[name] if name in mast else default
            if not maintainers.LineType.FORKED in block:
                generate_maintainers_change(name, block, default, args.user)
        except sh.ErrorReturnCode_128:
            print("{} has not been imported into Gerrit, skipping".format(name))
        print()

if __name__ == "__main__":
    main()
