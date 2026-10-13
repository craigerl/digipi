#!/usr/bin/python
#			u p d a t e v e r s i o n . p y
# $Revision: 171 $
# $Author: eckertb $
# $Id: updateversion.py 171 2014-10-19 10:00:22Z eckertb $
#
# Description:
#	RMS Gateway - update the version info currently stored in
#	the winlink system
#
# RMS Gateway
#
# Copyright (c) 2004-2014 Hans-J. Barthen - DL5DI
# Copyright (c) 2008-2014 Brian R. Eckert - W3SG
#
# Questions or problems regarding this program can be emailed
# to linux-rmsgw@w3sg.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

import sys
import re
import requests
import json
import platform
from xml.etree import ElementTree
from optparse import OptionParser
from distutils.version import LooseVersion
import syslog

#################################
# BEGIN CONFIGURATION SECTION
#################################

service_config_xml = '/etc/rmsgw/winlinkservice.xml'
gateway_config = '/etc/rmsgw/gateway.conf'
version_info = '/etc/rmsgw/.version_info'
py_version_require='2.7.9'

#################################
# END CONFIGURATION SECTION
#################################
cmdlineparser = OptionParser()
cmdlineparser.add_option("-d", "--debug",
                         action="store_true", dest="DEBUG", default=False,
                         help="turn on debug output")
(options, args) = cmdlineparser.parse_args()

errors = 0

ws_config = {}
svc_calls = {}
gw_config = {}
version = {}
param_roots = {}

#
# load gateway config
#
with open(gateway_config) as gwfile:
    for line in gwfile:
        if not line.strip().startswith('#'):
            name, val = line.partition("=")[::2]
            gw_config[name.strip()] = val.strip()
gwfile.close()

#
# setup syslog
#
fac = eval('syslog.LOG_' + gw_config['LOGFACILITY'].upper())
mask = eval('syslog.LOG_' + gw_config['LOGMASK'].upper())
syslog.openlog(logoption=syslog.LOG_PID, facility=fac)
syslog.setlogmask(syslog.LOG_UPTO(mask))

#
# check python version
#
python_version=platform.python_version()

if LooseVersion(python_version) >= LooseVersion(py_version_require):
    if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'Python Version Check: ' + str(python_version) + ' OK')
else:
    syslog.syslog(syslog.LOG_ERR, 'Need more current Python version, require version: ' + str(py_version_require) + ' or newer')
    print('Exiting ...')
    syslog.closelog()
    sys.exit(1)

#
# load service config from XML
#
winlink_service = ElementTree.parse(service_config_xml)
winlink_config = winlink_service.getroot()

for svc_config in winlink_config.iter('config'):
    ws_config['WebServiceAccessCode'] =  svc_config.find('WebServiceAccessCode').text
    ws_config['svchost'] = svc_config.find('svchost').text
    ws_config['svcport'] = svc_config.find('svcport').text
    ws_config['namespace'] = svc_config.find('namespace').text

    for svc_ops in svc_config.findall('svcops'):
        for svc_call in svc_ops:
            svc_calls[svc_call.tag] = svc_call.text
            param_roots[svc_call.tag] = svc_call.attrib['paramRoot']

#
# load version info
#
with open(version_info) as versionfile:
    for line in versionfile:
        if not line.strip().startswith('#'):
            name, val = line.partition("=")[::2]
            version[name.strip()] = val.strip()
versionfile.close()

if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'ws_config = {}'.format(ws_config))
if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'svc_calls = {}'.format(svc_calls))
if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'param_roots = {}'.format(param_roots))
if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'version_program = {}'.format(version['PROGRAM']))

#
# get gateway callsign from config
# the basecall
#
if 'GWCALL' in gw_config:
    callsign = gw_config['GWCALL'].upper()

basecall=callsign.split("-", 1)[0]
if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'basecallsign = {}'.format(basecall))

headers = {'Content-Type': 'application/xml'}

# svc_url = 'http://' + ws_config['svchost'] + ':' + ws_config['svcport'] + svc_calls['versionadd']
# Needs to look like this:
svc_url = 'https://' + ws_config['svchost'] + svc_calls['versionadd'] + '?' + 'Callsign=' + format(callsign) + '&Program=' + format(version['PROGRAM']) + '&Version=' + format(version['LABEL']) + '&Key=' + format(ws_config['WebServiceAccessCode'] + '&format=json')
if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'svc_url = {}'.format(svc_url))

#
# create xml tree for call parameters
#
version_add = ElementTree.Element(param_roots['versionadd'])
version_add.set('xmlns:i', 'http://www.w3.org/2001/XMLSchema-instance')
version_add.set('xmlns', ws_config['namespace'])

ElementTree.SubElement(version_add, "Key")
ElementTree.SubElement(version_add, "Callsign")
ElementTree.SubElement(version_add, "Comments")
ElementTree.SubElement(version_add, "Program")
ElementTree.SubElement(version_add, "Version")

if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'bare version_add XML = {}'.format(ElementTree.tostring(version_add)))

#
# prepare xml parameters for call
#
version_add.find('Key').text = ws_config['WebServiceAccessCode']
version_add.find('Callsign').text = callsign
version_add.find('Comments').text = version['PACKAGE']
version_add.find('Program').text = version['PROGRAM']
version_add.find('Version').text = version['LABEL']

if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'version_add XML = {}'.format(ElementTree.tostring(version_add)))

# Post the request
try:
    response = requests.post(svc_url, data=ElementTree.tostring(version_add), headers=headers)
except requests.ConnectionError as e:
    syslog.syslog(syslog.LOG_DEBUG, "Error: Internet connection failure:")
    syslog.syslog(syslog.LOG_DEBUG, 'svc_url = {}'.format(svc_url))
    syslog.syslog(syslog.LOG_DEBUG, str(e))
    syslog.closelog()
    sys.exit(1)

if options.DEBUG: syslog.syslog(syslog.LOG_DEBUG, 'Response = {}'.format(response.content))

json_data = response.json()
if options.DEBUG: print(json.dumps(json_data, indent=2))
json_dict = json.loads(response.text)

# print the return code of this request, should be 200 which is "OK"
if options.DEBUG: print("Response status code: " + str(response.status_code))
if options.DEBUG: print('Debug: Response =', response.content)
if options.DEBUG: print("Debug: Content type: " + response.headers['content-type'])

#
# Verify request status code
#
if response.ok:
    if options.DEBUG: print("Debug: Good Response status code")
else:
    print('*** Version update for ', callsign, 'failed, ErrorCode =',  str(response.status_code))
    errors += 1

#
# get status response (if there is one) and confirm success
#
if json_dict['ResponseStatus']:
    print('ResponseStatus not NULL: ', json_dict['ResponseStatus'])
    print('*** Channel Update for', callsign, 'failed')
    print('*** Error code:    ' + json_dict['ResponseStatus']['ErrorCode'])
    print('*** Error message: ' + json_dict['ResponseStatus']['Message'])

    errors += 1
else:
    if options.DEBUG: print('ResponseStatus is NULL: ', json_dict['ResponseStatus'])
    syslog.syslog(syslog.LOG_INFO, 'Version update for {} to version {} successful.'.format(callsign, version['LABEL']))

syslog.closelog()

if errors > 0:
    sys.exit(1)
#else
sys.exit(0)
