#!/usr/bin/python
#			g e t v e r s i o n l i s t . p y
# $Revision:  $
# $Author: gunn $
# $Id: getversionlist.py $
#
# Description:
#	RMS Gateway - get the Linux RMS Gateway version list to
#	determine the uptake after a new version release.
#
# RMS Gateway
#
# This program used getversionlist.py as a template.
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
import time
import platform
from xml.etree import ElementTree
from optparse import OptionParser
from pkg_resources import parse_version
from distutils.version import LooseVersion

#################################
# BEGIN CONFIGURATION SECTION
#################################

service_config_xml = '/etc/rmsgw/winlinkservice.xml'
version_info = '/etc/rmsgw/.version_info'
py_version_require='2.7.9'

#################################
# END CONFIGURATION SECTION
#################################

#
# parse command line args
#
cmdlineparser = OptionParser()
usage = "usage: %prog [options]"
cmdlineparser.add_option("-d", "--debug",
                         action="store_true", dest="DEBUG", default=False,
                         help="turn on debug output")
cmdlineparser.add_option("-c", "--callsign",
                         action="store", metavar="CALLSIGN",
                         dest="callsign", default=None,
                         help="get a specific callsign")
cmdlineparser.add_option("-l", "--list",
                         action="store_true", dest="DISPLAYLIST", default=False,
                         help="Display formatted list instead of json")
(options, args) = cmdlineparser.parse_args()

#
# check python version
#
python_version=platform.python_version()

if LooseVersion(python_version) >= LooseVersion(py_version_require):
    if options.DEBUG: print('Python Version Check: ' + str(python_version) + ' OK')
else:
    print('Need more current Python version, require version: ' + str(py_version_require) + ' or newer')
    print('Exiting ...')
    sys.exit(1)

#
# dictionaries for config info
#

ws_config = {}
svc_calls = {}
version = {}
param_roots = {}

#
# load version info
#
with open(version_info) as versionfile:
    for line in versionfile:
        if not line.strip().startswith('#'):
            name, val = line.partition("=")[::2]
            version[name.strip()] = val.strip()
versionfile.close()

if options.DEBUG: print('version_program = {}'.format(version['PROGRAM']))

#
# load service config from XML
#
winlink_service = ElementTree.parse(service_config_xml)
winlink_config = winlink_service.getroot()

for svc_config in winlink_config.iter('config'):
    # basic configuration data
    ws_config['WebServiceAccessCode'] =  svc_config.find('WebServiceAccessCode').text
    ws_config['svchost'] = svc_config.find('svchost').text
    ws_config['svcport'] = svc_config.find('svcport').text
    ws_config['namespace'] = svc_config.find('namespace').text

    # for the service operations, the tags are the operations,
    # element text is the service call detail
    for svc_ops in svc_config.findall('svcops'):
        for svc_call in svc_ops:
            svc_calls[svc_call.tag] = svc_call.text
            param_roots[svc_call.tag] = svc_call.attrib['paramRoot']

if options.DEBUG: print('ws_config =', ws_config)
if options.DEBUG: print('svc_calls =', svc_calls)
if options.DEBUG: print('param_roots =', param_roots)

#
# prepare and make webservice call
#
headers = {'Content-Type': 'application/xml'}

# V5 CMS web services url format
svc_url = 'https://' + ws_config['svchost'] + svc_calls['versionlist'] + '?' '&Program=' + format(version['PROGRAM']) + '&HistoryHours=48' + '&Key=' + format(ws_config['WebServiceAccessCode'] + '&format=json')
if options.DEBUG: print('svc_url =', svc_url)

#
# prepare xml parameters for call
#
verlist = ElementTree.Element(param_roots['versionlist'])
verlist.set('xmlns:i', 'http://www.w3.org/2001/XMLSchema-instance')
verlist.set('xmlns', ws_config['namespace'])

child = ElementTree.SubElement(verlist, 'Key')
child.text = ws_config['WebServiceAccessCode']

child = ElementTree.SubElement(verlist, 'Program')
child.text = format(version['PROGRAM'])

child = ElementTree.SubElement(verlist, 'HistoryHours')
child.text = "48"


if options.DEBUG: print('verlist XML =', ElementTree.tostring(verlist))

# Post the request
try:
    response = requests.post(svc_url, data=ElementTree.tostring(verlist), headers=headers)
except requests.ConnectionError as e:
    print("Error: Internet connection failure:")
    print('svc_url = ', svc_url)
    print(e)
    sys.exit(1)

# print the return code of this request, should be 200 which is "OK"
if options.DEBUG: print("Request status code: " + str(response.status_code))
if options.DEBUG: print('Debug: Response =', response.content)
if options.DEBUG: print("Debug: Content type: " + response.headers['content-type'])
# print('ResponseStatus : ', response.json().get('ResponseStatus'))

json_data = response.json()
if options.DEBUG: print(json.dumps(json_data, indent=2))
json_dict = json.loads(response.text)
#num_chan = len(json_data['Channels'])

#
# Verify request status code
#
if response.ok:
    if options.DEBUG: print("Debug: Good Request status code")
else:
    print('*** Get for', options.callsign, 'failed, ErrorCode =',  str(response.status_code))
    print('*** Error code:    ' + json_dict['ResponseStatus']['ErrorCode'])
    print('*** Error message: ' + json_dict['ResponseStatus']['Message'])
    sys.exit(1)

#
# check for errors coming back
#
if json_dict['ResponseStatus']:
    print('ResponseStatus not NULL: ', json_dict['ResponseStatus'])
    sys.exit(1)
else:
    if options.DEBUG: print('ResponseStatus is NULL: ', json_dict['ResponseStatus'])

#
# display the returned data
#

# print('Number of records returned: ' + str(len(json_dict)))
# print('Number of items in VersionList returned: ' + str(len(json_data['VersionList'])))

if options.DISPLAYLIST:
    # Display formatted list
    for i in range(len(json_data['VersionList'])):
        print((json_dict['VersionList'][i]['Callsign'] + "  " + json_dict['VersionList'][i]['Version']))
else:
    # Display json
    print((json.dumps(json_data, indent=2)))


sys.exit(0)
