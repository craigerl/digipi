#!/usr/bin/env python3
# Copyright 2018 Red Hat, Inc.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.

"""Configuration Validator

Uses the sample config generator configuration file to retrieve a list of all
the available options in a project, then compares it to what is configured in
the provided file.  If there are any options set that are not defined in the
project then it returns those errors.
"""

import logging
import re
import sys

try:
    # For Python 3.8 and later
    import importlib.metadata as importlib_metadata
except ImportError:
    # For everyone else
    import importlib_metadata

import yaml

from oslo_config import cfg
from oslo_config import generator

VALIDATE_DEFAULTS_EXCLUSIONS = [
    '.*_ur(i|l)', '.*connection', 'password', 'username', 'my_ip',
    'host(name)?', 'glance_api_servers', 'osapi_volume_listen',
    'osapi_compute_listen',
]

_validator_opts = [
    cfg.MultiStrOpt(
        'namespace',
        help='Option namespace under "oslo.config.opts" in which to query '
             'for options.'),
    cfg.StrOpt(
        'input-file',
        required=True,
        help='Config file to validate.'),
    cfg.StrOpt(
        'opt-data',
        help='Path to a YAML file containing definitions of options, as '
             'output by the config generator.'),
    cfg.BoolOpt(
        'check-defaults',
        default=False,
        help='Report differences between the sample values and current '
             'values.'),
    cfg.ListOpt(
        'exclude-options',
        default=VALIDATE_DEFAULTS_EXCLUSIONS,
        help='Exclude options matching these patterns when comparing '
             'the current and sample configurations.'),
    cfg.BoolOpt(
        'fatal-warnings',
        default=False,
        help='Report failure if any warnings are found.'),
    cfg.MultiStrOpt(
        'exclude-group',
        default=[],
        help='Groups that should not be validated if they are present in the '
             'specified input-file. This may be necessary for dynamically '
             'named groups which do not appear in the sample config data.'),
]


KNOWN_BAD_GROUPS = ['keystone_authtoken']


def _register_cli_opts(conf):
    """Register the formatter's CLI options with a ConfigOpts instance.

    Note, this must be done before the ConfigOpts instance is called to parse
    the configuration.

    :param conf: a ConfigOpts instance
    :raises: DuplicateOptError, ArgsAlreadyParsedError
    """
    conf.register_cli_opts(_validator_opts)


def _validate_deprecated_opt(group, option, opt_data):
    if group not in opt_data['deprecated_options']:
        return False
    name_data = [o['name'] for o in opt_data['deprecated_options'][group]]
    name_data += [o.get('dest') for o in opt_data['deprecated_options'][group]]
    return option in name_data


def _validate_defaults(sections, opt_data, conf):
    """Compares the current and sample configuration and reports differences

    :param section: ConfigParser instance
    :param opt_data: machine readable data from the generator instance
    :param conf: ConfigOpts instance
    :returns: boolean wether or not warnings were reported
    """
    warnings = False
    # Generating regex objects from ListOpt
    exclusion_regexes = []
    for pattern in conf.exclude_options:
        exclusion_regexes.append(re.compile(pattern))
    for group, opts in opt_data['options'].items():
        if group in conf.exclude_group:
            continue
        if group not in sections:
            logging.warning(
                'Group %s from the sample config is not defined in '
                'input-file', group)
            continue
        for opt in opts['opts']:
            # We need to convert the defaults into a list to find
            # intersections. defaults are only a list if they can
            # be defined multiple times, but configparser only
            # returns list
            if not isinstance(opt['default'], list):
                defaults = [str(opt['default'])]
            else:
                defaults = opt['default']

            # Apparently, there's multiple naming conventions for
            # options, 'name' is mostly with hyphens, and 'dest'
            # is represented with underscores.
            opt_names = set([opt['name'], opt.get('dest')])
            if not opt_names.intersection(sections[group]):
                continue
            try:
                value = sections[group][opt['name']]
                keyname = opt['name']
            except KeyError:
                value = sections[group][opt.get('dest')]
                keyname = opt.get('dest')

            if any(rex.fullmatch(keyname) for rex in exclusion_regexes):
                logging.info(
                    '%s/%s Ignoring option because it is part of the excluded '
                    'patterns. This can be changed with the --exclude-options '
                    'argument', group, keyname)
                continue

            if len(value) > 1:
                logging.info(
                    '%s/%s defined %s times', group, keyname, len(value))
            if not opt['default']:
                logging.warning(
                    '%s/%s sample value is empty but input-file has %s',
                    group, keyname, ", ".join(value))
                warnings = True
            elif not frozenset(defaults).intersection(value):
                logging.warning(
                    '%s/%s sample value %s is not in %s',
                    group, keyname, defaults, value)
                warnings = True
    return warnings


def _validate_opt(group, option, opt_data):
    if group not in opt_data['options']:
        return False
    name_data = [o['name'] for o in opt_data['options'][group]['opts']]
    name_data += [o.get('dest') for o in opt_data['options'][group]['opts']]
    return option in name_data


def load_opt_data(conf):
    with open(conf.opt_data) as f:
        return yaml.safe_load(f)


def _validate(conf):
    conf.register_opts(_validator_opts)
    if conf.namespace:
        groups = generator._get_groups(generator._list_opts(conf.namespace))
        opt_data = generator._generate_machine_readable_data(groups, conf)
    elif conf.opt_data:
        opt_data = load_opt_data(conf)
    else:
        # TODO(bnemec): Implement this logic with group?
        raise RuntimeError('Neither namespace nor opt-data provided.')
    sections = {}
    parser = cfg.ConfigParser(conf.input_file, sections)
    parser.parse()
    warnings = False
    errors = False
    if conf.check_defaults:
        warnings = _validate_defaults(sections, opt_data, conf)
    for section, options in sections.items():
        if section in conf.exclude_group:
            continue
        for option in options:
            if _validate_deprecated_opt(section, option, opt_data):
                logging.warning('Deprecated opt %s/%s found', section, option)
                warnings = True
            elif not _validate_opt(section, option, opt_data):
                if section in KNOWN_BAD_GROUPS:
                    logging.info('Ignoring missing option "%s" from group '
                                 '"%s" because the group is known to have '
                                 'incomplete sample config data and thus '
                                 'cannot be validated properly.',
                                 option, section)
                    continue
                logging.error('%s/%s is not part of the sample config',
                              section, option)
                errors = True
    if errors or (warnings and conf.fatal_warnings):
        return 1
    return 0


def main():
    """The main function of oslo-config-validator."""
    version = importlib_metadata.version('oslo.config')
    logging.basicConfig(level=logging.INFO)
    conf = cfg.ConfigOpts()
    _register_cli_opts(conf)
    try:
        conf(sys.argv[1:], version=version)
    except cfg.RequiredOptError:
        conf.print_help()
        if not sys.argv[1:]:
            raise SystemExit
        raise
    return _validate(conf)


if __name__ == '__main__':
    sys.exit(main())
