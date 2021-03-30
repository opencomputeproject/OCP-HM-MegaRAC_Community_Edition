#!/usr/bin/python3
#
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018 IBM Corp.

import argparse
import sys
from collections import namedtuple, OrderedDict
from enum import Enum, unique
from typing import (Dict, NamedTuple, Iterator, Sequence, Union, Optional,
                    List, cast, IO)
from pprint import pprint

@unique
class LineType(Enum):
    REPO = 1
    MAINTAINER = 2
    REVIEWER = 3
    FORKED = 4
    COMMENT = 5

@unique
class ParseState(Enum):
    BEGIN = 1
    BLOCK = 2

Email = NamedTuple("Email", [("name", str), ("address", str)])
Identity = NamedTuple("Identity", [("email", Email), ("irc", Optional[str])])
Entry = NamedTuple("Entry", [("type", LineType), ("content", str)])

def parse_line(line: str) -> Optional[Entry]:
    sline = line.strip()
    if not sline:
        return None

    if sline == "MAINTAINERS":
        return Entry(LineType.REPO, sline)

    tag = line[:2]
    if '@' in tag:
        return Entry(LineType.REPO, sline[1:].split(":")[0].strip())
    elif tag == 'M:':
        return Entry(LineType.MAINTAINER, sline.split(":")[1].strip())
    elif tag == 'R:':
        return Entry(LineType.REVIEWER, sline.split(":")[1].strip())
    elif tag == 'F:':
        return Entry(LineType.FORKED, sline[2:].strip())
    elif '#' in tag:
        return Entry(LineType.COMMENT, line)

    return None

D = Union[str, List[Identity], List[str]]

def parse_repo(content: str) -> str:
    return content

def parse_forked(content: str) -> str:
    return content

def parse_irc(src: Iterator[str]) -> Optional[str]:
    irc = ""
    for c in src:
        if c == '#':
            return None
        if c == '<':
            break
    else:
        return None

    for c in src:
        if c in '!#':
            return irc.strip()
        irc += c

    raise ValueError("Unterminated IRC handle")

def parse_address(src: Iterator[str]) -> str:
    addr = ""
    for c in src:
        if c in '>#':
            return addr.strip()
        addr += c
    raise ValueError("Unterminated email address")

def parse_name(src: Iterator[str]) -> str:
    name = ""
    for c in src:
        if c in '<#':
            return name.strip()
        name += c
    raise ValueError("Unterminated name")

def parse_email(src: Iterator[str]) -> Email:
    name = parse_name(src)
    address = parse_address(src)
    return Email(name, address)

def parse_identity(content: str) -> Identity:
    ci = iter(content)
    email = parse_email(ci)
    irc = parse_irc(ci)
    return Identity(email, irc)

B = Dict[LineType, D]

def parse_block(src: Iterator[str]) -> Optional[B]:
    state = ParseState.BEGIN
    repo = cast(B, OrderedDict())
    for line in src:
        try:
            entry = parse_line(line)
            if state == ParseState.BEGIN and not entry:
                continue
            elif state == ParseState.BEGIN and entry:
                state = ParseState.BLOCK
            elif state == ParseState.BLOCK and not entry:
                return repo

            assert entry

            if entry.type == LineType.REPO:
                repo[entry.type] = parse_repo(entry.content)
            elif entry.type in { LineType.MAINTAINER, LineType.REVIEWER }:
                if not entry.type in repo:
                    repo[entry.type] = cast(List[Identity], list())
                cast(list, repo[entry.type]).append(parse_identity(entry.content))
            elif entry.type == LineType.FORKED:
                repo[entry.type] = parse_forked(entry.content)
            elif entry.type == LineType.COMMENT:
                if not entry.type in repo:
                    repo[entry.type] = cast(List[str], list())
                cast(list, repo[entry.type]).append(entry.content)
        except ValueError as e:
            print("Failed to parse line '{}': {}".format(line.strip(), e))

    if not repo:
        return None

    return repo

def trash_preamble(src: Iterator[str]) -> None:
    s = 0
    for line in src:
        sline = line.strip()
        if "START OF MAINTAINERS LIST" == sline:
            s = 1
        if s == 1 and sline == "-------------------------":
            break

def parse_maintainers(src: Iterator[str]) -> Dict[D, B]:
    maintainers = cast(Dict[D, B], OrderedDict())
    trash_preamble(src)
    while True:
        repo = cast(B, parse_block(src))
        if not repo:
            break
        maintainers[repo[LineType.REPO]] = repo
    return maintainers

def assemble_name(name: str, dst: IO[str]) -> None:
    dst.write(name)

def assemble_address(address: str, dst: IO[str]) -> None:
    dst.write("<")
    dst.write(address)
    dst.write(">")

def assemble_email(email: Email, dst: IO[str]) -> None:
    assemble_name(email.name, dst)
    dst.write(" ")
    assemble_address(email.address, dst)

def assemble_irc(irc: Optional[str], dst: IO[str]) -> None:
    if irc:
        dst.write(" ")
        dst.write("<")
        dst.write(irc)
        dst.write("!>")

def assemble_identity(identity: Identity, dst: IO[str]) -> None:
    assemble_email(identity.email, dst)
    assemble_irc(identity.irc, dst)

def assemble_maintainers(identities: List[Identity], dst: IO[str]) -> None:
    for i in identities:
        dst.write("M:  ")
        assemble_identity(i, dst)
        dst.write("\n")

def assemble_reviewers(identities: List[Identity], dst: IO[str]) -> None:
    for i in identities:
        dst.write("R:  ")
        assemble_identity(i, dst)
        dst.write("\n")

def assemble_forked(content: str, dst: IO[str]) -> None:
    if content:
        dst.write("F:  ")
        dst.write(content)
        dst.write("\n")

def assemble_comment(content: List[str], dst: IO[str]) -> None:
    dst.write("".join(content))

def assemble_block(block: B, default: B, dst: IO[str]) -> None:
    if LineType.COMMENT in block:
        assemble_comment(cast(List[str], block[LineType.COMMENT]), dst)
    if LineType.MAINTAINER in block:
        maintainers = block[LineType.MAINTAINER]
    else:
        maintainers = default[LineType.MAINTAINER]
    assemble_maintainers(cast(List[Identity], maintainers), dst)
    if LineType.REVIEWER in block:
        assemble_reviewers(cast(List[Identity], block[LineType.REVIEWER]), dst)
    if LineType.FORKED in block:
        assemble_forked(cast(str, block[LineType.FORKED]), dst)

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("maintainers", type=argparse.FileType('r'),
                        default=sys.stdin)
    parser.add_argument("output", type=argparse.FileType('w'),
                        default=sys.stdout)
    args = parser.parse_args()
    blocks = parse_maintainers(args.maintainers)
    for block in blocks.values():
        print(block[LineType.REPO])
        assemble_block(block, blocks['MAINTAINERS'], args.output)
        print()

if __name__ == "__main__":
    main()
