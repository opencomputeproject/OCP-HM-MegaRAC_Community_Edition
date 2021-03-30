# -*- coding: utf-8 -*-
# Tests for fru.py
# Copyright (c) 2017 Dell Technologies
# Copyright (c) 2018 Kurt McKee <contactme@kurtmckee.org>
#
# https://github.com/genotrance/fru-tool/
#
# Licensed under the terms of the MIT License:
# https://opensource.org/licenses/MIT

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os

# noinspection PyUnresolvedReferences,PyPackageRequirements
import pytest

import fru


sections = [
    'all', 'empty', 'board', 'chassis', 'internal-data', 'internal-file',
    'product',
]


@pytest.mark.parametrize('name', sections)
def test_basic_ini_sections(name):
    path = os.path.join(os.path.dirname(__file__), 'basic-{}.ini'.format(name))
    config = fru.read_config(path)
    actual = fru.dump(config)

    path = os.path.join(os.path.dirname(__file__), 'basic-{}.bin'.format(name))
    with open(path, 'rb') as f:
        expected = f.read()

    assert actual == expected


@pytest.mark.parametrize('name', sections)
def test_identical_loading(name):
    path = os.path.join(os.path.dirname(__file__), 'basic-{}.ini'.format(name))
    ini_data = fru.read_config(path)

    path = os.path.join(os.path.dirname(__file__), 'basic-{}.bin'.format(name))
    bin_data = fru.load(path=path)

    assert ini_data == bin_data


def test_too_much_data():
    config = {
        "common": {"version": 1, "size": 20},
        "chassis": {"part": "a" * 250},
    }
    with pytest.raises(ValueError):
        fru.dump(config)


def test_empty_everything():
    config = {
        "common": {
            "version": 1,
            "size": 256,
        },
        "internal": {}, "chassis": {}, "board": {}, "product": {},
    }
    fru.dump(config)


def test_missing_required_elements():
    with pytest.raises(ValueError):
        fru.dump({})
    with pytest.raises(ValueError):
        fru.dump({"common": {"size": 512}})
    with pytest.raises(ValueError):
        fru.dump({"common": {"version": 1}})


def test_skipped_section():
    path = os.path.join(os.path.dirname(__file__), 'skip-section.ini')
    config = fru.read_config(path)
    assert "internal" not in config


def test_load_bad_calls():
    with pytest.raises(ValueError):
        fru.load()
    with pytest.raises(ValueError):
        fru.load(path='a', blob='a'.encode('ascii'))


def test_bad_header_checksum():
    blob = b"\x01\x00\x00\x00\x00\x00\x00\x00"
    with pytest.raises(ValueError):
        fru.load(blob=blob)


def test_internal_fru_file_not_found():
    path = os.path.join(
        os.path.dirname(__file__),
        'internal-fru-file-not-found.ini'
    )
    with pytest.raises(ValueError) as error:
        fru.read_config(path)
        assert 'not found' in error.msg


def test_internal_fru_requested_but_empty():
    path = os.path.join(os.path.dirname(__file__), 'internal-empty.ini')
    data = fru.read_config(path)
    assert 'internal' not in data


def test_checksum_of_zero():
    path = os.path.join(
        os.path.dirname(__file__),
        'checksum-zero.bin'
    )
    fru.load(path)


@pytest.mark.parametrize('section', ['board', 'chassis', 'product'])
def test_extras(section):
    for i in range(1, 10):
        key = 'extra{}'.format(i)
        data = {
            'common': {'size': 64, 'version': 1},
            section: {key: '---'}
        }
        symmetric_data = fru.load(blob=fru.dump(data))
        assert data[section][key] == symmetric_data[section][key]
