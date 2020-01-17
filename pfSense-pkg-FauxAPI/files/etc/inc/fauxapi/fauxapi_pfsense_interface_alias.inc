<?php

namespace fauxapi\v1;

// write_config requires functions from this
include '/etc/inc/phpsessionmanager.inc';
include '/etc/inc/auth.inc';
trait network_address_aliases
{

    /*
        From PFSENSE. copyed for old version.
        If $return_message is true then
            returns a text message about the reason that the name is invalid.
            the text includes the type of "thing" that is being checked, passed in $object. (e.g. "alias", "gateway group", "schedule")
        else
            returns true if $name is a valid name for an alias
            returns false if $name is not a valid name for an alias

        Aliases cannot be:
            bad chars: anything except a-z 0-9 and underscore
            bad names: empty string, pure numeric, pure underscore
            reserved words: pre-defined service/protocol/port names which should not be ambiguous, and the words "port" and  "pass" */

    private function is_valid_network_address_alias_name($name)
    {
        global $config;
        global $pf_reserved_keywords;
        $reserved_table_names = array(
            "bogons",
            "bogonsv6",
            "negate_networks",
            "snort2c",
            "sshguard",
            "tonatsubnets",
            "virusprot",
            "vpn_networks",
        );
        $reserved_ifs = get_configured_interface_list(true);
        $pf_reserved_keywords = array_merge($pf_reserved_keywords, $reserved_ifs, $reserved_table_names);

        $object = "alias";
        /* Array of reserved words */
        $reserved = array("port", "pass");

        if (!is_string($name) || strlen($name) >= 32 || preg_match('/(^_*$|^\d*$|[^a-z0-9_])/i', $name)) {
            return sprintf(gettext('The %1$s name must be less than 32 characters long, may not consist of only numbers, may not consist of only underscores, and may only contain the following characters: %2$s'), $object, 'a-z, A-Z, 0-9, _');
        }
        if (in_array($name, $reserved, true)) {
            return sprintf(gettext('The %1$s name must not be either of the reserved words %2$s or %3$s.'), $object, "'port'", "'pass'");
        }
        if (getprotobyname($name)) {
            return sprintf(gettext('The %1$s name must not be an IP protocol name such as TCP, UDP, ICMP etc.'), $object);
        }
        if (getservbyname($name, "tcp") || getservbyname($name, "udp")) {
            return sprintf(gettext('The %1$s name must not be a well-known or registered TCP or UDP port name such as ssh, smtp, pop3, tftp, http, openvpn etc.'), $object);
        }

        /* Check for reserved keyword names */
        foreach ($pf_reserved_keywords as $rk) {
            if (strcasecmp($rk, $name) == 0) {
                return sprintf(gettext("Cannot use a reserved keyword as an alias name: %s"), $rk);
            }
        }

        /*
        * Packages (e.g. tinc) create interface groups, reserve this
        * namespace pkg_ for them.
        * One namespace is shared by Interfaces, Interface Groups and Aliases.
        */
        if (substr($name, 0, 4) == 'pkg_') {
            return gettext("The alias name cannot start with pkg_");
        }

        /* check for name interface description conflicts */
        foreach ($config['interfaces'] as $interface) {
            if (strcasecmp($interface['descr'], $name) == 0) {
                return gettext("An interface description with this name already exists.");
            }
        }

        /* Is the description already used as an interface group name? */
        if (is_array($config['ifgroups']['ifgroupentry'])) {
            foreach ($config['ifgroups']['ifgroupentry'] as $ifgroupentry) {
                if ($ifgroupentry['ifname'] == $name) {
                    return gettext("Sorry, an interface group with this name already exists.");
                }
            }
        }

        // SHELL NOT BE A IP address. for prevent infinite loops make sure the alias name does not equal the value
        if (is_ipaddr($name)) {
            return "name shell not be ip address";
        }
        // CHECK NAME
        foreach ($config["aliases"]["alias"] as $cfgitem) {
            if ($cfgitem["name"] == $name) {
                return  gettext("An alias with this name already exists.");
            }
        }

        return NULL;
    }



    /**
     * network_address_aliases_get()
     * 
     * @return array
     */
    public function network_address_aliases_get()
    {
        global $config;
        fauxApiLogger::debug(__METHOD__);
        return $config["aliases"];
    }


    /**
     * network_address_aliases_create()
     * @param string $name - alias name
     * @param string $type - alias type, supports "network" only
     * @param array $cidr_addresses - alias CIDRAddress {"address": "1.2.3.4/32", "details":"message"}
     * @param string $descr - alias descr, use for UI item.
     * @return array
     */
    public function network_address_aliases_create($name, $type, $cidr_addresses, $descr = "Added by fauxapi")
    {
        global $config;
        fauxApiLogger::debug(__METHOD__, array(
            'name' => $name, 'type' => $type, 'cidr_addresses' => $cidr_addresses, 'descr' => $descr
        ));
        //TODO: change this if we need address_alias type to other.
        if ($type != "network") {
            $error_message = "not supported aliases type for now";
            $error_data = array('type' => $type);
            fauxApiLogger::error($error_message, $error_data);
            throw new \Exception($error_message);
        }
        $error_message = $this->is_valid_network_address_alias_name($name);
        if ($error_message !== NULL) {
            $error_data = array('name' => $name);
            fauxApiLogger::error($error_message, $error_data);
            throw new \Exception($error_message);
        }
        if (!is_array($cidr_addresses) || !$this->is_numeric_array($cidr_addresses) || count($cidr_addresses) == 0) {
            $error_message = "must be a object array";
            $error_data = array('cidr_addresses' => $cidr_addresses);
            fauxApiLogger::error($error_message, $error_data);
            throw new \Exception($error_message);
        }
        $address_cfg = array();
        $details_cfg = array();
        foreach ($cidr_addresses as $addresscfg) {
            $address = $addresscfg["address"];
            if (!is_ipaddr($address) && !is_subnet($address)) {
                $error_message = "must be a address or subnet";
                $error_data = array('address' => $addresscfg);
                fauxApiLogger::error($error_message, $error_data);
                throw new \Exception($error_message);
            }
            $details = $addresscfg["details"];
            $details = preg_replace('/\|\|+/', '|', trim($details, "|"));
            array_push($address_cfg, $address);
            array_push($details_cfg, $details);
        }
        $address_cfg = join(" ", $address_cfg);
        $details_cfg = join("||", $details_cfg);
        # this shell saves to config.
        $pconfig['name'] = $name;
        $pconfig['descr'] = $descr;
        $pconfig['type'] = $type;
        $pconfig['address'] = $address_cfg;
        $pconfig['detail'] = $details_cfg;
        init_config_arr(array('aliases', 'alias'));
        $currsize = count($config['aliases']['alias']);
        $config["aliases"]['alias'][$currsize] = $pconfig;
        $session_item = $_SESSION['Username'];
        $_SESSION['Username'] = "admin";  // workaround to make writeconfig work
        if (write_config(gettext("Edited a firewall alias."))) {
            mark_subsystem_dirty('aliases');
            $_SESSION['Username'] = $session_item;
        } else {
            $_SESSION['Username'] = $session_item;
            $error_message = "must be a address or subnet";
            $error_data = array('address' => $addresscfg);
            fauxApiLogger::error($error_message, $error_data);
            throw new \Exception($error_message);
        }

        return $config["aliases"];
    }

    /**
     * network_address_aliases_get()
     * 
     * @return array
     */
    public function network_address_aliases_update()
    {
        global $config;
        fauxApiLogger::debug(__METHOD__);
        return $config["aliases"];
    }

    /**
     * network_address_aliases_get()
     * 
     * @return array
     */
    public function network_address_aliases_delete()
    {
        global $config;
        fauxApiLogger::debug(__METHOD__);
        return $config["aliases"];
    }
}