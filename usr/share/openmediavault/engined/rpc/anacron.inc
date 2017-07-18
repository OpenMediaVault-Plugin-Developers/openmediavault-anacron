<?php
/**
 * Copyright (C) 2013-2017 OpenMediaVault Plugin Developers
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

class OMVRpcServiceAnacron extends OMVRpcServiceAbstract {

    public function getName()
    {
        return 'Anacron';
    }

    public function initialize()
    {
        $this->registerMethod('getTaskList');
        $this->registerMethod('getTask');
        $this->registerMethod('setTask');
        $this->registerMethod('deleteTask');
        $this->registerMethod('getCommands');
        $this->registerMethod('doUpload');
        $this->registerMethod('doExecute');
    }

    public function getTaskList($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.getlist");
        // Get configuration data.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->getAssoc("conf.service.anacron.task");
        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function getTask($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc("conf.service.anacron.drive", $params['uuid']);
    }

    public function setTask($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.anacron.settask");
        // Prepare the configuration object.
        $object = new \OMV\Config\ConfigObject("conf.service.anacron.task");
        $db = \OMV\Config\Database::getInstance();
        $object->setAssoc($params);
        // Set the configuration object.
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function deleteTask($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Delete the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.service.anacron.task', $params['uuid']);
        $db->delete($object);
        // Return the deleted configuration object.
        return $object->getAssoc();
    }

    public function getCommands($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);

        $objects = [];

        // Get list of files in /var/lib/anacron
        $objects1 = array_diff(scandir("/var/lib/anacron"), array("..", "."));
        foreach ($objects1 as &$value)
            $value = "/var/lib/anacron/" . $value;

        // Get list of files in /usr/local/bin
        $objects2 = array_diff(
            scandir("/usr/local/bin"),
            [
                "..",
                ".",
                "installbackports",
                "omv-support-libs",
                "omv-support.sh",
                "anacron",
                "upssched-cmd",
                "youtube-dl",
                "fixmodule",
                "create_greyhole_db",
                "installsystemd"
            ]
        );

        foreach ($objects2 as &$value) {
            $value = "/usr/local/bin/" . $value;
        }

        $objects = array_merge($objects1, $objects2);

        $result = [];
        foreach ($objects as $row) {
            // Prepare result object.
            $object = [
                "script" => $row
            ];

            $result[] = $object;
        }

        return $result;
    }

    function doUpload($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.anacron.doupload");

        // Prepare the configuration object.
        $object = new \OMV\Config\ConfigObject("conf.service.anacron.task");
        $db = \OMV\Config\Database::getInstance();
        $object->setAssoc($params);
        // Set the configuration object.
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();

        if ( !file_exists("/var/lib/anacron") ) {
            if ( !mkdir("/var/lib/anacron", 0755) ) {
                throw new \OMV\Exception(
                    "Failed to make script directory '/var/lib/anacron/'."
                );
            }
        }

        // Move file to /var/lib/anacron.
        if ( !rename($params['filepath'],
             sprintf("/var/lib/anacron/%s", $params['filename']))) {
            throw new \OMV\Exception(
                sprintf(
                    "Failed to move script '%s' to '/var/lib/anacron/'.",
                    $params['filename']
                )
            );
        }

        // Make script executable
        if ( !chmod( sprintf("/var/lib/anacron/%s", $params['filename']), 0755) ) {
            throw new \OMV\Exception(
                sprintf(
                    "Failed to make script executable - '%s'",
                    $params['filename']
                )
            );
        }
    }

    public function doExecute($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.common.objectuuid");
        // Create the background process.
        return $this->execBgProc(function($bgStatusFilename, $bgOutputFilename)
          use ($params) {
            // Get the configuration object.
            $db = \OMV\Config\Database::getInstance();
            $object = $db->get("conf.service.anacron.task", $params['uuid']);
            // Execute the cron command line in a shell.
            // Note, we need to set the shell, otherwise the users shell
            // is used instead of the shebang of the command script.
            $cmdArgs = [];
            $cmdArgs[] = "--shell";
            $cmdArgs[] = "--non-interactive";
            $cmdArgs[] = sprintf("--user=%s", $object->get("username"));
            $cmdArgs[] = "--";
            $cmdArgs[] = $object->get("command");
            $cmd = new \OMV\System\Process("sudo", $cmdArgs);
            $cmd->setEnv("SHELL", "/bin/sh");
            $cmd->setRedirect2to1();
            if (0 !== ($exitStatus = $this->exec($cmd->getCommandLine(),
                $output, $bgOutputFilename))) {
                throw new \OMV\ExecException($cmd->getCommandLine(), $output,
                    $exitStatus);
            }
            return $output;
        });
    }
}
