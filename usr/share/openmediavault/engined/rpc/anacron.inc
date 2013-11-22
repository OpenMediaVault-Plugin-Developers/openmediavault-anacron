<?php

/**
 * Copyright (C)      2013 OpenMediaVault Plugin Developers
 * Copyright (C) 2009-2013 Volker Theile <volker.theile@openmediavault.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/object.inc";
require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/util.inc";
require_once "openmediavault/rpcservice.inc";
require_once "openmediavault/notify.inc";
require_once("openmediavault/system.inc");

class OMVRpcServiceAnacron extends OMVRpcServiceAbstract {

    public function getName() {
        return 'Anacron';
    }

    public function initialize() {
        $this->registerMethod('getTasks');
        $this->registerMethod('getTask');
        $this->registerMethod('setTask');
        $this->registerMethod('deleteTask');
        $this->registerMethod('getCommands');
        $this->registerMethod('doUpload');
    }

    public function getTasks($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       :"object",
            "properties" : {
                "start"     : { "type" : "integer" },
                "limit"     : { '.$GLOBALS['OMV_JSONSCHEMA_COUNTFIELD'].' },
                "sortfield" : { '.$GLOBALS['OMV_JSONSCHEMA_SORTFIELD'].' },
                "sortdir"   : { '.$GLOBALS['OMV_JSONSCHEMA_SORTDIR'].' }
              }
        }');

        // Get the configuration object.
        $xpath = "//services/anacron/task";
        $objects = $xmlConfig->getList($xpath);
        
        if(is_null($objects))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'], 
            $params['sortfield'], $params['sortdir']);
    }

    public function getTask($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Get the configuration object.
        $xpath = sprintf("//services/anacron/task[uuid='%s']", $params['uuid']);
        $object = $xmlConfig->get($xpath);
        
        if(is_null($object))
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED, $xpath);

        return $object;
    }

    public function setTask($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid"       : { '.$GLOBALS['OMV_JSONSCHEMA_UUID_UNDEFINED'].' },
                "period"     : { "type" : "string" },
                "delay"      : { "type" : "integer" },
                "identifier" : { "type" : "string" },
                "command"    : { "type" : "string" }
            }
        }');

        // Prepare the configuration object.
        $object = array(
            "uuid"       => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "period"     => $params['period'],
            "delay"      => $params['delay'],
            "identifier" => $params['identifier'],
            "command"    => $params['command']
        );

        // Set the configuration object.
        $success = FALSE;

        if($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) {
            // Append object to configuration.
            $success = $xmlConfig->set("//services/anacron", array("task" => $object));
        } else {
            // Update existing configuration object.
            $success = $xmlConfig->replace("//services/anacron/task[uuid='{$params['uuid']}']", $object);
        }

        if(FALSE === $success)
            throw new OMVException(OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED);

        // Notify configuration changes.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ?
            OMV_NOTIFY_CREATE : OMV_NOTIFY_MODIFY,
            "org.openmediavault.services.anacron.task", $object);

        // Return the configuration object.
        return $object;
    }

    public function deleteTask($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "uuid" : { '.$GLOBALS['OMV_JSONSCHEMA_UUID'].' }
            }
        }');

        // Delete the configuration object.
        $xpath = sprintf("//services/anacron/task[uuid='%s']", $params['uuid']);
        $object = $this->deleteConfigObjectByPath($xpath,
            "org.openmediavault.services.anacron.task");

        // Return the deleted configuration object.
        return $object;
    }

    public function getCommands($params, $context) {
        global $xmlConfig;

        // Validate the RPC caller context.
        $this->validateMethodContext($context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Get list of files in /var/lib/anacron
        $objects = array_diff(scandir("/var/lib/anacron"), array("..", "."));
        
        $result = array();
        foreach($objects as $row) {
            // Prepare result object.
            $object = array(
                "script" => $row
            );

            $result[] = $object;
        }
        
        return $result;
    }

    function doUpload($params, $context) {
        // Validate the RPC caller context.
        $this->validateMethodContext( $context, array( "role" => OMV_ROLE_ADMINISTRATOR ));

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, '{
            "type"       : "object",
            "properties" : {
                "filename" : { "type" : "string" },
                "filepath" : { "type" : "string" }
            }
        }');

        // Move file to /var/lib/anacron.
        if(!rename($params['filepath'], sprintf("/var/lib/anacron/%s", $params['filename']))) {
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, sprintf(
                "Failed to move script '%s' to '/var/lib/anacron/'.", $params['filename']));
        }
        // Make script executable
        if( !chmod( sprintf("/var/lib/anacron/%s", $params['filename']), 0755) ) {
            throw new OMVException(OMVErrorMsg::E_MISC_FAILURE, sprintf(
                "Failed to make script executable - '%s'", $params['filename']));
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceAnacron());

?>