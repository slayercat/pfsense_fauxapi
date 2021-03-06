# FauxAPI - v1.3
A REST API interface for pfSense 2.3.x and 2.4.x to facilitate devops:-
 - https://github.com/ndejong/pfsense_fauxapi

Additionally available are a set of [client libraries](#client-libraries) 
that hence make programmatic access and management of pfSense hosts for devops 
tasks feasible.


## API Action Summary
 - [alias_update_urltables](#user-content-alias_update_urltables) - Causes the pfSense host to immediately update any urltable alias entries from their (remote) source URLs.
 - [config_backup](#user-content-config_backup) - Causes the system to take a configuration backup and add it to the regular set of system change backups.
 - [config_backup_list](#user-content-config_backup_list) - Returns a list of the currently available system configuration backups.
 - [config_get](#user-content-config_get) - Returns the full system configuration as a JSON formatted string.
 - [config_patch](#user-content-config_patch) - Patch the system config with a granular piece of new configuration.
 - [config_reload](#user-content-config_reload) - Causes the pfSense system to perform an internal reload of the `config.xml` file.
 - [config_restore](#user-content-config_restore) - Restores the pfSense system to the named backup configuration.
 - [config_set](#user-content-config_set) - Sets a full system configuration and (by default) reloads once successfully written and tested.
 - [function_call](#user-content-function_call) - Call directly a pfSense PHP function with API user supplied parameters.
 - [gateway_status](#user-content-gateway_status) - Returns gateway status data.
 - [interface_stats](#user-content-interface_stats) - Returns statistics and information about an interface.
 - [rule_get](#user-content-rule_get) - Returns the numbered list of loaded pf rules from a `pfctl -sr -vv` command on the pfSense host.
 - [send_event](#user-content-send_event) - Performs a pfSense "send_event" command to cause various pfSense system actions.
 - [system_reboot](#user-content-system_reboot) - Reboots the pfSense system.
 - [system_stats](#user-content-system_stats) - Returns various useful system stats.


## Approach
At its core FauxAPI simply reads the core pfSense `config.xml` file, converts it 
to JSON and returns to the API caller.  Similarly it can take a JSON formatted 
configuration and write it to the pfSense `config.xml` and handles the required 
reload operations.  The ability to programmatically interface with a running 
pfSense host(s) is enormously useful however it should also be obvious that this 
provides the API user the ability to create configurations that can break your 
pfSense system.

FauxAPI provides easy backup and restore API interfaces that by default store
configuration backups on all configuration write operations thus it is very easy 
to roll-back even if the API user manages to deploy a "very broken" configuration.

Multiple sanity checks take place to make sure a user provided JSON config will 
correctly convert into the (slightly quirky) pfSense XML `config.xml` format and 
then reload as expected in the same way.  However, because it is not a real 
per-action application-layer interface it is still possible for the API caller 
to create configuration changes that make no sense and can potentially disrupt 
your pfSense system - as the package name states, it is a "Faux" API to pfSense
filling a gap in functionality with the current pfSense product.

Because FauxAPI is a utility that interfaces with the pfSense `config.xml` there
are some cases where reloading the configuration file is not enough and you 
may need to "tickle" pfSense a little more to do what you want.  This is not 
common however a good example is getting newly defined network interfaces or 
VLANs to be recognized.  These situations are easily handled by calling the
**send_event** action with the payload **interface reload all** - see the example 
included below and refer to a the resolution to [Issue #10](https://github.com/ndejong/pfsense_fauxapi/issues/10)

__NB:__ *As at FauxAPI v1.2 the **function_call** action has been introduced that 
now provides the ability to issue function calls directly into pfSense.*


## Installation
Until the FauxAPI is added to the pfSense FreeBSD-ports tree you will need to 
install manually from **root** as shown:-
```bash
set fauxapi_base_package_url='https://raw.githubusercontent.com/ndejong/pfsense_fauxapi_packages/master'
set fauxapi_latest=`fetch -qo - ${fauxapi_base_package_url}/LATEST`
fetch ${fauxapi_base_package_url}/${fauxapi_latest}
pkg-static install ${fauxapi_latest}
```

Installation and de-installation is quite straight forward, further examples can 
be found in the `README.md` located [here](https://github.com/ndejong/pfsense_fauxapi_packages).

Refer to the published package [`SHA256SUMS`](https://github.com/ndejong/pfsense_fauxapi_packages/blob/master/SHA256SUMS)

**Hint:** if not already, consider installing the `jq` tool on your local machine (not 
pfSense host) to pipe and manage JSON outputs from FauxAPI - https://stedolan.github.io/jq/

**NB:** you MUST at least setup your `/etc/fauxapi/credentials.ini` file on the 
pfSense host before you continue, see the API Authentication section below.

## Client libraries

#### Python
A [Python interface](https://github.com/ndejong/pfsense_fauxapi_client_python) 
to pfSense was perhaps the most desired end-goal at the onset of the FauxAPI 
package project.  Anyone that has tried to parse the pfSense `config.xml` files 
using a Python based library will understand that things don't quite work out as 
expected or desired.

The Python client-library can be easily installed from PyPi as such
```bash
pip3 install pfsense-fauxapi
```

Package Status: [![PyPi](https://img.shields.io/pypi/v/pfsense-fauxapi.svg)](https://pypi.org/project/pfsense-fauxapi/) [![Build Status](https://travis-ci.org/ndejong/pfsense_fauxapi_client_python.svg?branch=master)](https://travis-ci.org/ndejong/pfsense_fauxapi_client_python)

Use of the package should be easy enough as shown
```python
import pprint, sys
from PfsenseFauxapi.PfsenseFauxapi import PfsenseFauxapi
PfsenseFauxapi = PfsenseFauxapi('<host-address>', '<fauxapi-key>', '<fauxapi-secret>')

aliases = PfsenseFauxapi.config_get('aliases')
## perform some kind of manipulation to `aliases` here ##
pprint.pprint(PfsenseFauxapi.config_set(aliases, 'aliases'))
```

It is recommended to review the [Python code examples](https://github.com/ndejong/pfsense_fauxapi_client_python/tree/master/examples)
to observe worked examples with the client library.  Of small note is that the 
Python library supports the ability to get and set single sections of the pfSense 
system, not just the entire system configuration as with the Bash library.

**Python examples**
 - `usergroup-management.py` - example code that provides the ability to `get_users`, 
   `add_user`, `manage_user`, `remove_user` and perform the same functions on groups.
 - `update-aws-aliases.py` - example code that pulls in the latest AWS `ip-ranges.json` 
   data, parses it and injects them into the pfSense aliases section if required.
 - `function-iterate.py` - iterates (almost) all the FauxAPI functions to 
   confirm operation.  

#### Command Line
As distinct from the Bash library as described below the Python pip also introduces
a command-line tool to interact with the API, which makes a wide range of actions
possible directly from the command line, for example
```bash
fauxapi --host 192.168.1.200 gateway_status | jq .
```

#### Bash
The [Bash client library](https://github.com/ndejong/pfsense_fauxapi_client_bash) 
makes it possible to add a line with `source pfsense-fauxapi.sh` to your bash script 
and then access a pfSense host configuration directly as a JSON string
```bash
source pfsense-fauxapi.sh
export fauxapi_auth=$(fauxapi_auth <fauxapi-key> <fauxapi-secret>)

fauxapi_config_get <host-address> | jq .data.config > /tmp/config.json
## perform some kind of manipulation to `/tmp/config.json` here ##
fauxapi_config_set <host-address> /tmp/config.json
```

It is recommended to review the commented out samples in the provided 
`fauxapi-sample.sh` file that cover all possible FauxAPI calls to gain a better
idea on usage.

#### NodeJS/TypeScript
A NodeJS client has been developed by a third party and is available here
 - NPMJS: [npmjs.com/package/faux-api-client](https://www.npmjs.com/package/faux-api-client)
 - Github: [github.com/Elucidia/faux-api-client](https://github.com/Elucidia/faux-api-client)

#### PHP
A PHP client has been developed by a third party and is available here
 - Packagist: [packagist.org/packages/travisghansen/pfsense_fauxapi_php_client](https://packagist.org/packages/travisghansen/pfsense_fauxapi_php_client)
 - Github: [github.com/travisghansen/pfsense_fauxapi_php_client](https://github.com/travisghansen/pfsense_fauxapi_php_client)


## API Authentication
A deliberate design decision to decouple FauxAPI authentication from both the 
pfSense user authentication and the pfSense `config.xml` system.  This was done 
to limit the possibility of an accidental API change that removes access to the 
host.  It also seems more prudent to only establish API user(s) manually via the 
FauxAPI `/etc/fauxapi/credentials.ini` file - happy to receive feedback about 
this approach.

The two sample FauxAPI keys (PFFAexample01 and PFFAexample02) and their 
associated secrets in the sample `credentials.sample.ini` file are hard-coded to 
be inoperative, you must create entirely new values before your client scripts
will be able to issue commands to FauxAPI.

You can start your own `/etc/fauxapi/credentials.ini` file by copying the sample
file provided in `credentials.sample.ini`

API authentication itself is performed on a per-call basis with the auth value 
inserted as an additional **fauxapi-auth** HTTP request header, it can be 
calculated as such:-
```
fauxapi-auth: <apikey>:<timestamp>:<nonce>:<hash>

For example:-
fauxapi-auth: PFFA4797d073:20161119Z144328:833a45d8:9c4f96ab042f5140386178618be1ae40adc68dd9fd6b158fb82c99f3aaa2bb55
```

Where the &lt;hash&gt; value is calculated like so:-
```
<hash> = sha256(<apisecret><timestamp><nonce>)
```

NB: that the timestamp value is internally passed to the PHP `strtotime` function 
which can interpret a wide variety of timestamp formats together with a timezone.
A nice tidy timestamp format that the `strtotime` PHP function is able to process
can be obtained using bash command `date --utc +%Y%m%dZ%H%M%S` where the `Z` 
date-time seperator hence also specifies the UTC timezone.  

This is all handled in the [client libraries](#client-libraries) 
provided, but as can be seen it is relatively easy to implement even in a Bash 
shell script.

Getting the API credentials right seems to be a common source of confusion in
getting started with FauxAPI because the rules about valid API keys and secret 
values are pedantic to help make ensure poor choices are not made.

The API key + API secret values that you will need to create in `/etc/fauxapi/credentials.ini`
have the following rules:-
 - <apikey_value> and <apisecret_value> may have alphanumeric chars ONLY!
 - <apikey_value> MUST start with the prefix PFFA (pfSense Faux API)
 - <apikey_value> MUST be >= 12 chars AND <= 40 chars in total length
 - <apisecret_value> MUST be >= 40 chars AND <= 128 chars in length
 - you must not use the sample key/secret in the `credentials.ini` since they
   are hard coded to fail.

To make things easier consider using the following shell commands to generate 
valid values:-

#### apikey_value
```bash
echo PFFA`head /dev/urandom | base64 -w0 | tr -d /+= | head -c 20`
```

#### apisecret_value
```bash
echo `head /dev/urandom | base64 -w0 | tr -d /+= | head -c 60`
```

NB: Make sure the client side clock is within 60 seconds of the pfSense host 
clock else the auth token values calculated by the client will not be valid - 60 
seconds seems tight, however, provided you are using NTP to look after your 
system time it's quite unlikely to cause issues - happy to receive feedback 
about this.

__Shout Out:__ *Seeking feedback on the API authentication, many developers 
seem to stumble here - if you feel something could be improved without compromising
security then submit an Issue ticket via Github.*


## API Authorization
The file `/etc/fauxapi/credentials.ini` additionally provides a method to restrict
the API actions available to the API key using the **permit** configuration 
parameter.  Permits are comma delimited and may contain * wildcards to match more
than one rule as shown in the example below.

```
[PFFAexample01]
secret = abcdefghijklmnopqrstuvwxyz0123456789abcd
permit = alias_*, config_*, gateway_*, rule_*, send_*, system_*, function_*
comment = example key PFFAexample01 - hardcoded to be inoperative
```

## Debugging
FauxAPI comes with awesome debug logging capability, simply insert `__debug=true` 
as a URL request parameter and the response data will contain rich debugging log 
data about the flow of the request.

If you are looking for more debugging at various points feel free to submit a 
pull request or lodge an issue describing your requirement and I'll see what
can be done to accommodate.


## Logging
FauxAPI actions are sent to the system syslog via a call to the PHP `syslog()` 
function thus causing all FauxAPI actions to be logged and auditable on a per 
action (callid) basis which provide the full basis for the call, for example:-
```text
Jul  3 04:37:59 pfSense php-fpm[55897]: {"INFO":"20180703Z043759 :: fauxapi\\v1\\fauxApi::__call","DATA":{"user_action":"alias_update_urltables","callid":"5b3afda73e7c9","client_ip":"192.168.1.5"},"source":"fauxapi"}
Jul  3 04:37:59 pfSense php-fpm[55897]: {"INFO":"20180703Z043759 :: valid auth for call","DATA":{"apikey":"PFFAdevtrash","callid":"5b3afda73e7c9","client_ip":"192.168.1.5"},"source":"fauxapi"}
```
Enabling debugging yields considerably more logging data to assist with tracking 
down issues if you encounter them - you may review the logs via the pfSense GUI
as usual unser Status->System Logs->General or via the console using the `clog` tool
```bash
$ clog /var/log/system.log | grep fauxapi
``` 

## Configuration Backups
All configuration edits through FauxAPI create configuration backups in the 
same way as pfSense does with the webapp GUI.  

These backups are available in the same way as edits through the pfSense 
GUI and are thus able to be reviewed and diff'd in the same way under 
Diagnostics->Backup & Restore->Config History.

Changes made through the FauxAPI carry configuration change descriptions that
name the unique `callid` which can then be tied to logs if required for full 
usage audit and change tracking.

FauxAPI functions that cause write operations to the system config `config.xml` 
return reference to a backup file of the configuration immediately previous
to the change.


## API REST Actions
The following REST based API actions are provided, example cURL call request 
examples are provided for each.  The API user is perhaps more likely to interface 
with the [client libraries](#client-libraries) as documented above 
rather than directly with these REST end-points.

The framework around the FauxAPI has been put together with the idea of being
able to easily add more actions at a later time, if you have ideas for actions 
that might be useful be sure to get in contact.

NB: the cURL requests below use the '--insecure' switch because many pfSense
deployments do not deploy certificate chain signed SSL certificates.  A reasonable 
improvement in this regard might be to implement certificate pinning at the 
client side to hence remove scope for man-in-middle concerns.

---
### alias_update_urltables
 - Causes the pfSense host to immediately update any urltable alias entries from their (remote) 
   source URLs.  Optionally update just one table by specifying the table name, else all 
   tables are updated.
 - HTTP: **GET**
 - Params:
    - **table** (optional, default = null)

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=alias_update_urltables"
```

*Example Response*
```javascript
{
  "callid": "598ec756b4d09",
  "action": "alias_update_urltables",
  "message": "ok",
  "data": {
    "updates": {
      "bruteforceblocker": {
        "url": "https://raw.githubusercontent.com/firehol/blocklist-ipsets/master/bruteforceblocker.ipset",
        "status": [
          "no changes."
        ]
      }
    }
  }
}
```

---
### config_backup
 - Causes the system to take a configuration backup and add it to the regular 
   set of pfSense system backups at `/cf/conf/backup/`
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=config_backup"
```

*Example Response*
```javascript
{
  "callid": "583012fea254f",
  "action": "config_backup",
  "message": "ok",
  "data": {
    "backup_config_file": "/cf/conf/backup/config-1479545598.xml"
  }
}
```

---
### config_backup_list
 - Returns a list of the currently available pfSense system configuration backups.
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=config_backup_list"
```

*Example Response*
```javascript
{
  "callid": "583065cb670db",
  "action": "config_backup_list",
  "message": "ok",
  "data": {
    "backup_files": [
      {
        "filename": "/cf/conf/backup/config-1479545598.xml",
        "timestamp": "20161119Z144635",
        "description": "fauxapi-PFFA4797d073@192.168.10.10: update via fauxapi for callid: 583012fea254f",
        "version": "15.5",
        "filesize": 18535
      },
      ....
```

---
### config_get
 - Returns the system configuration as a JSON formatted string.  Additionally, 
   using the optional **config_file** parameter it is possible to retrieve backup
   configurations by providing the full path to it under the `/cf/conf/backup` 
   path.
 - HTTP: **GET**
 - Params:
    - **config_file** (optional, default=`/cf/config/config.xml`)

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=config_get"
```

*Example Response*
```javascript
{
    "callid": "583012fe39f79",
    "action": "config_get",
    "message": "ok",
    "data": {
      "config_file": "/cf/conf/config.xml",
      "config": {
        "version": "15.5",
        "staticroutes": "",
        "snmpd": {
          "syscontact": "",
          "rocommunity": "public",
          "syslocation": ""
        },
        "shaper": "",
        "installedpackages": {
          "pfblockerngsouthamerica": {
            "config": [
             ....
```

Hint: use `jq` to parse the response JSON and obtain the config only, as such:-
```bash
cat /tmp/faux-config-get-output-from-curl.json | jq .data.config > /tmp/config.json
```

---
### config_patch
 - Allows the API user to patch the system configuration with the existing system config
 - A **config_patch** call allows the API user to supply the partial configuration to be updated 
   which is quite different to the **config_set** function that requires the full configuration
   to be posted.
 - HTTP: **POST**
 - Params:
    - **do_backup** (optional, default = true)
    - **do_reload** (optional, default = true)

*Example Request*
```bash
cat > /tmp/config_patch.json <<EOF
{
  "system": {
    "dnsserver": [
      "8.8.8.8",
      "8.8.4.4"
    ],
    "hostname": "newhostname"
  }
}
EOF

curl \
    -X POST \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    --header "Content-Type: application/json" \
    --data @/tmp/config_patch.json \
    "https://<host-address>/fauxapi/v1/?action=config_patch"
```

*Example Response*
```javascript
{
  "callid": "5b3b506f72670",
  "action": "config_patch",
  "message": "ok",
  "data": {
    "do_backup": true,
    "do_reload": true,
    "previous_config_file": "/cf/conf/backup/config-1530613871.xml"
  }
```

---
### config_reload
 - Causes the pfSense system to perform a reload action of the `config.xml` file, by 
   default this happens when the **config_set** action occurs hence there is 
   normally no need to explicitly call this after a **config_set** action.
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=config_reload"
```

*Example Response*
```javascript
{
  "callid": "5831226e18326",
  "action": "config_reload",
  "message": "ok"
}
```

---
### config_restore
 - Restores the pfSense system to the named backup configuration.
 - HTTP: **GET**
 - Params:
    - **config_file** (required, full path to the backup file to restore)

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=config_restore&config_file=/cf/conf/backup/config-1479545598.xml"
```

*Example Response*
```javascript
{
  "callid": "583126192a789",
  "action": "config_restore",
  "message": "ok",
  "data": {
    "config_file": "/cf/conf/backup/config-1479545598.xml"
  }
}
```

---
### config_set
 - Sets a full system configuration and (by default) takes a system config
   backup and (by default) causes the system config to be reloaded once 
   successfully written and tested.
 - NB1: be sure to pass the *FULL* system configuration here, not just the piece you 
   wish to adjust!  Consider the **config_patch** or **config_item_set** functions if 
   you wish to adjust the configuration in more granular ways.
 - NB2: if you are pulling down the result of a `config_get` call, be sure to parse that
   response data to obtain the config data only under the key `.data.config`
 - HTTP: **POST**
 - Params:
    - **do_backup** (optional, default = true)
    - **do_reload** (optional, default = true)

*Example Request*
```bash
curl \
    -X POST \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    --header "Content-Type: application/json" \
    --data @/tmp/config.json \
    "https://<host-address>/fauxapi/v1/?action=config_set"
```

*Example Response*
```javascript
{
  "callid": "5b3b50e8b1bc6",
  "action": "config_set",
  "message": "ok",
  "data": {
    "do_backup": true,
    "do_reload": true,
    "previous_config_file": "/cf/conf/backup/config-1530613992.xml"
  }
}
```

---
### function_call
 - Call directly a pfSense PHP function with API user supplied parameters.  Note
   that is action is a *VERY* raw interface into the inner workings of pfSense 
   and it is not recommended for API users that do not have a solid understanding 
   of PHP and pfSense.  Additionally, not all pfSense functions are appropriate 
   to be called through the FauxAPI and only very limited testing has been 
   performed against the possible outcomes and responses.  It is possible to 
   harm your pfSense system if you do not 100% understand what is going on.
 - Functions to be called via this interface *MUST* be defined in the file 
   `/etc/pfsense_function_calls.txt` only a handful very basic and 
   read-only pfSense functions are enabled by default.
 - You can start your own `/etc/fauxapi/pfsense_function_calls.txt` file by 
   copying the sample file provided in `pfsense_function_calls.sample.txt`
 - HTTP: **POST**
 - Params: none

*Example Request*
```bash
curl \
    -X POST \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    --header "Content-Type: application/json" \
    --data "{\"function\": \"get_services\"}" \
    "https://<host-address>/fauxapi/v1/?action=function_call"
```

*Example Response*
```javascript
{
  "callid": "59a29e5017905",
  "action": "function_call",
  "message": "ok",
  "data": {
    "return": [
      {
        "name": "unbound",
        "description": "DNS Resolver"
      },
      {
        "name": "ntpd",
        "description": "NTP clock sync"
      },
      ....

```

---
### gateway_status
 - Returns gateway status data.
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=gateway_status"
```

*Example Response*
```javascript
{
  "callid": "598ecf3e7011e",
  "action": "gateway_status",
  "message": "ok",
  "data": {
    "gateway_status": {
      "10.22.33.1": {
        "monitorip": "8.8.8.8",
        "srcip": "10.22.33.100",
        "name": "GW_WAN",
        "delay": "4.415ms",
        "stddev": "3.239ms",
        "loss": "0.0%",
        "status": "none"
      }
    }
  }
}
```

---
### interface_stats
 - Returns interface statistics data and information - the real interface name must be provided 
   not an alias of the interface such as "WAN" or "LAN"
 - HTTP: **GET**
 - Params:
    - **interface** (required)

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=interface_stats&interface=em0"
```

*Example Response*
```javascript
{
  "callid": "5b3a5bce65d01",
  "action": "interface_stats",
  "message": "ok",
  "data": {
    "stats": {
      "inpkts": 267017,
      "inbytes": 21133408,
      "outpkts": 205860,
      "outbytes": 8923046,
      "inerrs": 0,
      "outerrs": 0,
      "collisions": 0,
      "inmcasts": 61618,
      "outmcasts": 73,
      "unsuppproto": 0,
      "mtu": 1500
    }
  }
}
```

---
### rule_get
 - Returns the numbered list of loaded pf rules from a `pfctl -sr -vv` command 
   on the pfSense host.  An empty rule_number parameter causes all rules to be
   returned.
 - HTTP: **GET**
 - Params:
    - **rule_number** (optional, default = null)

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=rule_get&rule_number=5"
```

*Example Response*
```javascript
{
  "callid": "583c279b56958",
  "action": "rule_get",
  "message": "ok",
  "data": {
    "rules": [
      {
        "number": 5,
        "rule": "anchor \"openvpn/*\" all",
        "evaluations": "14134",
        "packets": "0",
        "bytes": "0",
        "states": "0",
        "inserted": "21188",
        "statecreations": "0"
      }
    ]
  }
}
```

---
### send_event
 - Performs a pfSense "send_event" command to cause various pfSense system 
   actions as is also available through the pfSense console interface.  The 
   following standard pfSense send_event combinations are permitted:-
    - filter: reload, sync
    - interface: all, newip, reconfigure
    - service: reload, restart, sync
 - HTTP: **POST**
 - Params: none

*Example Request*
```bash
curl \
    -X POST \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    --header "Content-Type: application/json" \
    --data "[\"interface reload all\"]" \
    "https://<host-address>/fauxapi/v1/?action=send_event"
```

*Example Response*
```javascript
{
  "callid": "58312bb3398bc",
  "action": "send_event",
  "message": "ok"
}
```

---
### system_reboot
 - Just as it says, reboots the system.
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=system_reboot"
```

*Example Response*
```javascript
{
  "callid": "58312bb3487ac",
  "action": "system_reboot",
  "message": "ok"
}
```

---
### system_stats
 - Returns various useful system stats.
 - HTTP: **GET**
 - Params: none

*Example Request*
```bash
curl \
    -X GET \
    --silent \
    --insecure \
    --header "fauxapi-auth: <auth-value>" \
    "https://<host-address>/fauxapi/v1/?action=system_stats"
```

*Example Response*
```javascript
{
  "callid": "5b3b511655589",
  "action": "system_stats",
  "message": "ok",
  "data": {
    "stats": {
      "cpu": "20770421|20494981",
      "mem": "20",
      "uptime": "1 Day 21 Hours 25 Minutes 48 Seconds",
      "pfstate": "62/98000",
      "pfstatepercent": "0",
      "temp": "",
      "datetime": "20180703Z103358",
      "cpufreq": "",
      "load_average": [
        "0.01",
        "0.04",
        "0.01"
      ],
      "mbuf": "1016/61600",
      "mbufpercent": "2"
    }
  }
}
```
---

## Versions and Testing
The FauxAPI has been developed against pfSense 2.3.2, 2.3.3, 2.3.4, 2.3.5, 2.4.3, 2.4.4 it has 
not (yet) been tested against 2.3.0 or 2.3.1.  Further, it is apparent that the pfSense 
packaging technique changed significantly prior to 2.3.x so it is unlikely that it will be 
backported to anything prior to 2.3.0.

Testing is reasonable but does not achieve 100% code coverage within the FauxAPI 
codebase.  Two client side test scripts (1x Bash, 1x Python) that both 
demonstrate and test all possible server side actions are provided.  Under the 
hood FauxAPI, performs real-time sanity checks and tests to make sure the user 
supplied configurations will save, load and reload as expected.

__Shout Out:__ *Anyone that happens to know of _any_ test harness or test code 
for pfSense please get in touch - I'd very much prefer to integrate with existing 
pfSense test infrastructure if it already exists.*


## Releases
#### v1.0 - 2016-11-20
 - initial release

#### v1.1 - 2017-08-12
 - 2x new API actions **alias_update_urltables** and **gateway_status**
 - update documentation to address common points of confusion, especially the 
   requirement to provide the _full_ config file not just the portion to be 
   updated.
 - testing against pfSense 2.3.2 and 2.3.3

#### v1.2 - 2017-08-27
 - new API action **function_call** allowing the user to reach deep into the inner 
   code infrastructure of pfSense, this feature is intended for people with a 
   solid understanding of PHP and pfSense.
 - the `credentials.ini` file now provides a way to control the permitted API 
   actions.
 - various update documentation updates.
 - testing against pfSense 2.3.4

#### v1.3 - 2018-07-02
 - add the **config_patch** function providing the ability to patch the system config, 
   thus allowing API users to make granular configuration changes.  
 - added a "previous_config_file" response attribute to functions that cause write 
   operations to the running `config.xml`
 - add the **interface_stats** function to help in determining the usage of an 
   interface to (partly) address [Issue #20](https://github.com/ndejong/pfsense_fauxapi/issues/20)
 - added a "number" attibute to the "rules" output making the actual rule number more 
   explict as described in [Issue #13](https://github.com/ndejong/pfsense_fauxapi/issues/13)
 - addressed a bug with the **system_stats** function that was preventing it from 
   returning, caused by an upstream change(s) in the pfSense code.
 - rename the confusing "owner" field in `credentials.ini` to "comment", legacy 
   configuration files using "owner" are still supported. 
 - added a "source" attribute to the logs making it easier to grep fauxapi events, 
   for example `clog /var/log/system.log | grep fauxapi`
 - plenty of dcoumentation fixes and updates
 - added documentation highlighting features and capabilities that existed without 
   them being obvious
 - added the [`extras`](https://github.com/ndejong/pfsense_fauxapi/tree/master/extras) path 
   in the project repo as a better place to keep non-package files, `client-libs`, `examples`,
   `build-tools` etc
 - testing against pfSense 2.3.5
 - testing against pfSense 2.4.3


## FauxAPI License
```
Copyright 2016-2019 Nicholas de Jong

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
