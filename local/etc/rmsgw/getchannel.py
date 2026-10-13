#!/usr/bin/python
#			g e t c h a n n e l . p y
# $Revision:  $
# $Author: gunn $
# $Id: getchannel.py $
#
# Description:
#	RMS Gateway - get the channel info currently stored
#	by the winlink system
#
# RMS Gateway
#
# This program used getsysop.py as a template.
# Copyright (c) 2004-2013 Hans-J. Barthen - DL5DI
# Copyright (c) 2008-2013 Brian R. Eckert - W3SG
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
from distutils.version import LooseVersion

#################################
# BEGIN CONFIGURATION SECTION
#################################

gateway_config = '/etc/rmsgw/gateway.conf'
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
cmdlineparser.add_option("-d", "--debug",
                         action="store_true", dest="DEBUG", default=False,
                         help="turn on debug output")
cmdlineparser.add_option("-c", "--callsign",
                         action="store", metavar="CALLSIGN",
                         dest="callsign", default=None,
                         help="get a specific callsign")
(options, args) = cmdlineparser.parse_args()

#
# check python version
#
python_version=platform.python_version()

if LooseVersion(python_version) >= LooseVersion(py_version_require):
    if options.DEBUG: print( 'Python Version Check: ' + str(python_version) + ' OK')
else:
    print(sys.argv[0] + ': Error, need more current Python version, require version: ' + str(py_version_require) + ' or newer')
    print(sys.argv[0] + ': Exiting ...')
    sys.exit(1)

#
# dictionaries for config info
#
gw_config = {}
ws_config = {}
svc_calls = {}
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

if options.DEBUG: print('ws_config =', ws_config)
if options.DEBUG: print('svc_calls =', svc_calls)
if options.DEBUG: print('param_roots =', param_roots)

#
# need a callsign - unless we have a command line parameter
# first try the gateway config then ask the user for a
# callsign
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
        options.callsign = input('Enter gateway callsign (without SSID): ')

options.callsign = options.callsign.upper()

if options.DEBUG: print('callsign =', options.callsign)

#
# prepare and make webservice call
#
headers = {'Content-Type': 'application/xml'}

# Old winlink services url format
#svc_url = 'http://' + ws_config['svchost'] + ':' + ws_config['svcport'] + svc_calls['sysopget']

# V5 CMS web services url format
svc_url = 'https://' + ws_config['svchost'] + svc_calls['channelget'] + '?' + 'Callsign=' + format(options.callsign) + '&Program=' + format(version['PROGRAM']) + '&Key=' + format(ws_config['WebServiceAccessCode'] + '&format=json')
if options.DEBUG: print('svc_url =', svc_url)

#
# prepare xml parameters for call
#
channel_get = ElementTree.Element(param_roots['channelget'])
channel_get.set('xmlns:i', 'http://www.w3.org/2001/XMLSchema-instance')
channel_get.set('xmlns', ws_config['namespace'])

child = ElementTree.SubElement(channel_get, 'Key')
child.text = ws_config['WebServiceAccessCode']

child = ElementTree.SubElement(channel_get, 'Callsign')
child.text = options.callsign

if options.DEBUG: print('channel_get XML =', ElementTree.tostring(channel_get))

# Post the request
try:
    response = requests.post(svc_url, data=ElementTree.tostring(channel_get), headers=headers)
except requests.ConnectionError as e:
    print(sys.argv[0] + ": Error: Internet connection failure:")
    print(sys.argv[0] + ': svc_url = ', svc_url)
    print(e)
    sys.exit(1)

# print the return code of this request, should be 200 which is "OK"
if options.DEBUG: print("Request status code: " + str(response.status_code))
if options.DEBUG: print('Debug: Response =', response.content)
if options.DEBUG: print("Debug: Content type: " + response.headers['content-type'])
# print 'ResponseStatus : ', response.json().get('ResponseStatus')

json_data = response.json()
if options.DEBUG: print((json.dumps(json_data, indent=2)))
json_dict = json.loads(response.text)
num_chan = len(json_data['Channels'])

#
# Verify request status code
#
if response.ok:
    if options.DEBUG: print("Getchannel for ", options.callsign,  " Good Request status code")
else:
    print(sys.argv[0] + ' *** Getchannel for ', options.callsign, 'failed, ErrorCode =',  str(response.status_code))
    print(sys.argv[0] + ' *** Error code:    ' + json_dict['ResponseStatus']['ErrorCode'])
    print(sys.argv[0] + ' *** Error message: ' + json_dict['ResponseStatus']['Message'])
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

print('=== Record for ' + str(num_chan) + ' Channel(s) In Winlink System ===')

for i in range(num_chan):
    print('== data for record ' + str(i+1) + ' ==')
    print

    # Convert non standard UTC date string to a local time
    date_str = json_dict['Channels'][int(i)]['Timestamp']
    date_str = date_str.split("(")[1]
    date_str = date_str.split(")")[0]
    date_str = date_str[0:10]
    print('Date:            ' + time.strftime("%Z - %Y/%m/%d, %H:%M:%S", time.localtime(float(date_str))))
    print('Call sign:       ' + json_dict['Channels'][int(i)]['Callsign'])
    print('Base Call sign:  ' + json_dict['Channels'][int(i)]['BaseCallsign'])
    print('Grid Square:     ' + json_dict['Channels'][int(i)]['GridSquare'])
    print('Frequency:       ' + str(json_dict['Channels'][int(i)]['Frequency']))
    print('Baud:            ' + str(json_dict['Channels'][int(i)]['Baud']))
    print('Power:           ' + str(json_dict['Channels'][int(i)]['Power']))
    print('Height:          ' + str(json_dict['Channels'][int(i)]['Height']))
    print('Gain:            ' + str(json_dict['Channels'][int(i)]['Gain']))
    print('Direction:       ' + str(json_dict['Channels'][int(i)]['Direction']))
    print('Operating Hours: ' + str(json_dict['Channels'][int(i)]['OperatingHours']))
    print('Service Code:    ' + json_dict['Channels'][int(i)]['ServiceCode'])


       # it looks like this will be caught by the winlink system
       # and give an appropriate error in the response, so this
       # check may not be necessary any longer, but it hurts nothing
#       if returned_callsign == None:
#           print '*** No record found for', options.callsign
#           sys.exit(2)


sys.exit(0)
