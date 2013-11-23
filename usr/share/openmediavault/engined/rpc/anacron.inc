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
        $this->registerMethod('doExecute');
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

        // Get cron jobs that match the given type.
        $jobs = array();
        foreach($objects as $objectk => $objectv) {
            // Modify result data.
            $objectv['enable'] = boolval($objectv['enable']);
            $objectv['sendemail'] = boolval($objectv['sendemail']);

            $jobs[] = $objectv;
        }

        // Filter the result.
        return $this->applyFilter($jobs, $params['start'], $params['limit'],
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

        // Modify the result object.
        $object['enable'] = boolval($object['enable']);
        $object['sendemail'] = boolval($object['sendemail']);

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
                "enable"     : { "type" : "boolean" },
                "sendemail"  : { "type" : "boolean" },
                "period"     : { "type" : "string" },
                "delay"      : { "type" : "integer" },
                "identifier" : { "type" : "string" },
                "username"   : { "type" : "string" },
                "command"    : { "type" : "string" },
                "comment"    : { "type" : "string" }
            }
        }');

        // Prepare the configuration object.
        $object = array(
            "uuid"       => ($params['uuid'] == $GLOBALS['OMV_UUID_UNDEFINED']) ? OMVUtil::uuid() : $params['uuid'],
            "enable"     => array_boolval($params, 'enable'),
            "sendemail"  => array_boolval($params, 'sendemail'),
            "period"     => $params['period'],
            "delay"      => $params['delay'],
            "identifier" => $params['identifier'],
            "username"   => $params['username'],
            "command"    => $params['command'],
            "comment"    => $params['comment']
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
        $objects1 = array_diff(scandir("/var/lib/anacron"), array("..", "."));
        foreach ($objects1 as &$value)
            $value = "/var/lib/anacron/" . $value;

        $objects2 = array_diff(scandir("/usr/local/bin"), array("..", ".", "installbackports", "fixmodule"));
        foreach ($objects2 as &$value)
            $value = "/usr/local/bin/" . $value;

        $objects = sort(array_merge($objects1, $objects2));

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

        if ( !file_exists("/var/lib/anacron") )
            if ( !mkdir("/var/lib/anacron", 0755) ) {
                throw new OMVException(OMVErrorMsg::E_MISC_FAILURE,
                    "Failed to make script directory '/var/lib/anacron/'.");
            }
        }

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

    public function doExecute($params, $context) {
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

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if($pid > 0) { // Parent process.
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus($bgStatusFilename, "outputfilename", $bgOutputFilename);

            // Get the anacron task object.
            $object = $this->callMethod("get", array("uuid" => $params['uuid']), $context);

            // Execute the cron command line in a shell.
            $cmd = "/bin/su";
            $cmdArgs = array("-c", $object['command'], $object['username']);
            if(FALSE === $this->execve($cmd, $cmdArgs, $bgOutputFilename)) {
                throw new OMVException(OMVErrorMsg::E_EXEC_FAILED,
                    $cmd." ".implode(" ", $cmdArgs), "");
            }
        } catch(Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceAnacron());

?>