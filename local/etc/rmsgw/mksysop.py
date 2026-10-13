#!/usr/bin/python
#			m k s y s o p . p y
# $Revision: 165 $
# $Author: eckertb $
# $Id: mksysop.py 165 2014-06-05 11:28:26Z eckertb $
#
# Description:
#	RMS Gateway - generate sysop XML file to maintain
#	the sysop record in the winlink system
#
# RMS Gateway
#
# Copyright (c) 2004-2013 Hans-J. Barthen - DL5DI
# Copyright (c) 2008-2013 Brian R. Eckert - W3SG
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

import os
import sys
import re
import requests
import json
import platform
from xml.etree import ElementTree
from optparse import OptionParser
from distutils.version import LooseVersion

#################################
# BEGIN CONFIGURATION SECTION
#################################

gateway_config = '/etc/rmsgw/gateway.conf'
service_config_xml = '/etc/rmsgw/winlinkservice.xml'
channel_config_xml = '/etc/rmsgw/channels.xml'
version_info = '/etc/rmsgw/.version_info'
sysop_template_xml = '/etc/rmsgw/sysop-template.xml'
sysop_config_xml = '/etc/rmsgw/sysop.xml'

new_sysop_config_xml = 'new-sysop.xml'

py_version_require='2.7.9'

#################################
# END CONFIGURATION SECTION
#################################

### Local Functions #############

def NotFoundHelp():
    print '*' * 75
    print 'If you need to create your initial sysop record,'
    print 'copy', sysop_template_xml, 'to', sysop_config_xml
    print 'and enter the appropriate text within each of the'
    print 'sysop element tags.'
    print '*' * 75

#################################

cmdlineparser = OptionParser()
cmdlineparser.add_option("-d", "--debug",
                         action="store_true", dest="DEBUG", default=False,
                         help="turn on debug output")
cmdlineparser.add_option("-c", "--callsign",
                         action="store", metavar="CALLSIGN",
                         dest="callsign", default=None,
                         help="use a specific callsign")
(options, args) = cmdlineparser.parse_args()

#
# dictionaries for config info
#
rms_chans = {}
gw_config = {}
ws_config = {}
svc_calls = {}
version = {}
param_roots = {}

#
# Check running as root
#
if os.geteuid() != 0:
    print("Must be root, exiting ...")
    sys.exit(1)

#
# check python version
#
python_version=platform.python_version()

# This does not work with release candidates (Python 2.7.15rc1)
# if StrictVersion(python_version) >= StrictVersion(py_version_require):
# if parse_version(python_version) >= parse_version(py_version_require):

if LooseVersion(python_version) >= LooseVersion(py_version_require):
    if options.DEBUG: print('Python Version Check: ' + str(python_version) + ' OK')
else:
    print('Need more current Python version than: ' + str(python_version) + ' require version: ' + str(py_version_require) + ' or newer')
    print('Exiting ...')
    sys.exit(1)

#
# load gateway config
#
with open(gateway_config) as gwfile:
    for line in gwfile:
        if not line.strip().startswith('#'):
            name, val = line.partition("=")[::2]
            gw_config[name.strip()] = val.strip()
gwfile.close()

if options.DEBUG: print('Gateway config =', gw_config)

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

#
# load channel config from XML - need password
#

document = ElementTree.parse(channel_config_xml)
rmschannels = document.getroot()

ns = '{http://www.namespace.org}'
for channel in rmschannels.findall("%schannel" % (ns)):
#    if options.DEBUG: print('channel xml = {}'.format(ElementTree.tostring(channel)))

    callsign = channel.find("%scallsign" % (ns)).text
    rms_chans['callsign'] = callsign

    password = channel.find("%spassword" % (ns)).text
    rms_chans['password'] = password

if options.DEBUG: print('ws_config =', ws_config)
if options.DEBUG: print('svc_calls =', svc_calls)
if options.DEBUG: print('param_roots =', param_roots)

#
# need a callsign to try to get things setup
#
# if there is existing sysop info stored in
# the winlink system, we'll grab that and create
# the initial local XML file of values so that
# it can be easily maintained and updated going
# forward
#
if options.callsign == None: # no callsign given on cmd line?
    #
    # first check for gateway callsign and extract
    # the basecall
    #
    if 'GWCALL' in gw_config:
        options.callsign = gw_config['GWCALL'].split('-', 1)[0] # first item of list
    else:
        # ask for callsign if none given on command line
        # and none in the gateway config
        options.callsign = raw_input('Enter gateway callsign (without SSID): ')

options.callsign = options.callsign.upper()

if options.DEBUG: print('callsign =', options.callsign)

#
# prepare and make webservice call
#
headers = {'Content-Type': 'application/xml'}

# Old winlink services url format
# svc_url = 'http://' + ws_config['svchost'] + ':' + ws_config['svcport'] + svc_calls['sysopget']
#svc_url = 'https://' + ws_config['svchost'] + svc_calls['sysop2get'] + '?' + 'Callsign=' + format(options.callsign) + '&Program=' + format(version['PROGRAM']) + '&Password=' + format(rms_chans['password']) + '&Key=' + format(ws_config['WebServiceAccessCode'] + '&format=json')
svc_url = 'https://' + ws_config['svchost'] + svc_calls['sysop2get'] + '?' + 'Callsign=' + format(options.callsign) + '&Program=' + format(version['PROGRAM']) + '&Password=' + format(rms_chans['password']) + '&Key=' + format(ws_config['WebServiceAccessCode'] + '&format=json')

if options.DEBUG: print 'svc_url =', svc_url

#
# prepare xml parameters for call
#
sysop_get = ElementTree.Element(param_roots['sysop2get'])
sysop_get.set('xmlns:i', 'http://www.w3.org/2001/XMLSchema-instance')
sysop_get.set('xmlns', ws_config['namespace'])

child = ElementTree.SubElement(sysop_get, 'Key')
child.text = ws_config['WebServiceAccessCode']

child = ElementTree.SubElement(sysop_get, 'Callsign')
child.text = options.callsign

if options.DEBUG: print('sysop_get XML =', ElementTree.tostring(sysop_get))

# Post the request
try:
    response = requests.post(svc_url, data=ElementTree.tostring(sysop_get), headers=headers)
except requests.ConnectionError as e:
    print("Error: Internet connection failure:")
    print('svc_url = ', svc_url)
    print(e)
    sys.exit(1)

if options.DEBUG: print 'Response =', response.content

# we'll load the returned xml into this dictionary
return_data = {}

json_data = response.json()
if options.DEBUG: print((json.dumps(json_data, indent=2)))
json_dict = json.loads(response.text)

# print the return code of this request, should be 200 which is "OK"
if options.DEBUG: print("Request status code:" + str(response.status_code))

#
# Verify request status code"
#
if response.ok:
    if options.DEBUG: print("Debug: Good Request status code")
else:
    print('*** Get for', options.callsign, 'failed, ErrorCode =', str(response.status_code))
    print('*** Error code:    ' + json_dict['ResponseStatus']['ErrorCode'])
    print('*** Error message: ' + json_dict['ResponseStatus']['Message'])
#    sys.exit(1)

#
# check for errors coming back
#
if json_dict['ResponseStatus']:
    print('ResponseStatus not NULL: ', json_dict['ResponseStatus'])
    sys.exit(1)
else:
    if options.DEBUG: print(('ResponseStatus is NULL: ', json_dict['ResponseStatus']))


#
# build xml element tree from json response while
#

#
# check that we got something useful back
#
returned_callsign=json_dict['Sysop']['Callsign']

if not returned_callsign or not returned_callsign.strip():
    print('*** No record found for', options.callsign)
    sys.exit(2)

#
# load the sysop XML doc (this is an unpopulated template
# for our purposes here)
#
if not os.path.isfile(sysop_template_xml):
    print("File: " + sysop_template_xml + " does not exist")
    sys.exit(2)

document = ElementTree.parse(sysop_template_xml)
sysops = document.getroot()

#
# update the XML document with the values returned from
# our webservice call...
#
for sysop in sysops.findall('sysop'):
    sysop.find('Callsign').text = returned_callsign
    sysop.find('Password').text = rms_chans['password']
    sysop.find('GridSquare').text = json_dict['Sysop']['GridSquare']
    sysop.find('SysopName').text = json_dict['Sysop']['SysopName']
    sysop.find('StreetAddress1').text = json_dict['Sysop']['StreetAddress1']
    sysop.find('StreetAddress2').text = json_dict['Sysop']['StreetAddress2']
    sysop.find('City').text = json_dict['Sysop']['City']
    sysop.find('State').text = json_dict['Sysop']['State']
    sysop.find('Country').text = json_dict['Sysop']['Country']
    sysop.find('PostalCode').text = json_dict['Sysop']['PostalCode']
    sysop.find('Email').text = json_dict['Sysop']['Email']
    sysop.find('Phones').text = json_dict['Sysop']['Phones']
    sysop.find('Website').text = json_dict['Sysop']['Website']
    sysop.find('Comments').text = json_dict['Sysop']['Comments']

#
# ... and write the updated document to a new XML file
# for the sysop to inspect and move into place
# as appropriate
document.write(new_sysop_config_xml)

print 'New sysop XML data written to', new_sysop_config_xml
print 'Please inspect the results and copy to', sysop_config_xml
print 'if it is correct.'

sys.exit(0)

