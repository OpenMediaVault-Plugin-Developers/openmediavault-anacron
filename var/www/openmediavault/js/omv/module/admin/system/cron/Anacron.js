/**
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @author    OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @copyright Copyright (c) 2009-2013 Volker Theile
 * @copyright Copyright (c) 2013-2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/util/Format.js")
// require("js/omv/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/form/field/UserComboBox.js")
// require("js/omv/window/Execute.js")

/**
 * @class OMV.module.admin.system.cron.Task
 * @derived OMV.workspace.window.Form
 */
Ext.define("OMV.module.admin.system.cron.Task", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject"
    ],

    rpcService   : "anacron",
    rpcGetMethod : "getTask",
    rpcSetMethod : "setTask",
    plugins      : [{
        ptype : "configobject"
    }],

    getFormItems : function() {
        return [{
            xtype: "checkbox",
            name: "enable",
            fieldLabel: _("Enable"),
            checked: true
        },{
            xtype         : "combo",
            name          : "period",
            fieldLabel    : _("Period"),
            queryMode     : "local",
            store         : [
                [ "1", _("daily") ],
                [ "7", _("weekly") ],
                [ "@monthly", _("monthly") ]
            ],
            editable      : false,
            triggerAction : "all",
            value         : ""
        },{
            xtype         : "numberfield",
            name          : "delay",
            fieldLabel    : _("Delay"),
            minValue      : 0,
            maxValue      : 1440,
            allowDecimals : false,
            allowNegative : false,
            allowBlank    : false,
            value         : 0,
            plugins    : [{
                ptype : "fieldinfo",
                text  : _("Delay time in minutes")
            }]
        },{
            xtype      : "textfield",
            name       : "identifier",
            fieldLabel : _("Identifier"),
            allowBlank : false
        },{
            xtype      : "usercombo",
            name       : "username",
            fieldLabel : _("User"),
            value      : "root"
        },{
            xtype         : "combo",
            name          : "command",
            fieldLabel    : _("Command"),
            emptyText     : _("Select a script ..."),
            allowBlank    : false,
            allowNone     : false,
            editable      : true,
            triggerAction : "all",
            displayField  : "script",
            valueField    : "script",
            store         : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "script",
                    fields     : [
                        { name : "script", type : "string" }
                    ]
                }),
                proxy : {
                    type : "rpc",
                    rpcData : {
                        service : "Anacron",
                        method  : "getCommands"
                    },
                    appendSortParams : false
                },
                sorters : [{
                    direction : "ASC",
                    property  : "devicefile"
                }]
            }),
            plugins : [{
                ptype : "fieldinfo",
                text  : _("List of scripts available.  If none listed, upload one or type command.")
            }]
        },{
            xtype      : "checkbox",
            name       : "sendemail",
            fieldLabel : _("Send email"),
            checked    : false,
            boxLabel   : _("Send command output via email"),
            plugins    : [{
                ptype : "fieldinfo",
                text  : _("An email message with the command output (if any produced) is send to the user who performs the job.")
            }]
        },{
            xtype      : "textarea",
            name       : "comment",
            fieldLabel : _("Comment"),
            allowBlank : true,
            vtype      : "comment"
        }];
    }
});

/**
 * @class OMV.module.admin.system.cron.Anacron
 * @derived OMV.workspace.grid.Panel
 */
Ext.define("OMV.module.admin.system.cron.Anacron", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.util.Format",
        "OMV.window.Execute"
    ],
    uses     : [
        "OMV.module.admin.system.cron.Task"
    ],

    hidePagingToolbar : false,
    stateful          : true,
    stateId           : "a982a76d-6804-4632-b31b-8b48c0ea6dde",
    columns           : [{
        xtype     : "booleaniconcolumn",
        text      : _("Enabled"),
        sortable  : true,
        dataIndex : "enable",
        stateId   : "enable",
        align     : "center",
        width     : 80,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        text      : _("Period"),
        sortable  : true,
        dataIndex : "period",
        stateId   : "period",
        renderer  : function(value) {
            switch(value) {
                case '1':
                    content = "daily";
                    break;
                case '7':
                    content = "weekly";
                    break;
                case '@monthly':
                    content = "monthly";
                    break;
            }
            return content;
        }
    },{
        text      : _("Delay"),
        sortable  : true,
        dataIndex : "delay",
        stateId   : "delay"
    },{
        text      : _("Identifier"),
        sortable  : true,
        dataIndex : "identifier",
        stateId   : "identifier"
    },{
        text      : _("User"),
        sortable  : true,
        dataIndex : "username",
        stateId   : "username"
    },{
        text      : _("Command"),
        sortable  : true,
        dataIndex : "command",
        stateId   : "command"
    },{
        text      : _("Comment"),
        sortable  : true,
        dataIndex : "comment",
        stateId   : "comment"
    }],

    initComponent : function() {
        var me = this;
        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoLoad : true,
                model    : OMV.data.Model.createImplicit({
                    idProperty : "uuid",
                    fields     : [
                        { name  : "uuid", type: "string" },
                        { name  : "enable", type: "boolean" },
                        { name  : "period", type: "string" },
                        { name  : "delay", type: "integer" },
                        { name  : "identifier", type: "string" },
                        { name  : "username", type: "string" },
                        { name  : "command", type: "string" },
                        { name  : "sendemail", type: "boolean" },
                        { name  : "comment", type: "string" }
                    ]
                }),
                proxy    : {
                    type    : "rpc",
                    rpcData : {
                        service : "Anacron",
                        method  : "getTasks"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var me = this;
        var items = me.callParent(arguments);

        // Add 'Run' button to top toolbar.
        Ext.Array.insert(items, 2, [{
            id       : me.getId() + "-run",
            xtype    : "button",
            text     : _("Run"),
            icon     : "images/play.png",
            iconCls  : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler  : Ext.Function.bind(me.onRunButton, me, [ me ]),
            scope    : me,
            disabled : true
        }]);

        // Add 'Upload' button to top toolbar
        Ext.Array.insert(items, 4, [{
            id      : me.getId() + "-upload",
            xtype   : "button",
            text    : _("Upload"),
            icon    : "images/upload.png",
            iconCls : Ext.baseCSSPrefix + "btn-icon-16x16",
            handler : Ext.Function.bind(me.onUploadButton, me, [ me ]),
            scope   : me
        }]);
        return items;
    },

    onAddButton : function() {
        var me = this;
        Ext.create("OMV.module.admin.system.cron.Task", {
            title     : _("Add anacron task"),
            uuid      : OMV.UUID_UNDEFINED,
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    onSelectionChange : function(model, records) {
        var me = this;
        me.callParent(arguments);
        // Process additional buttons.
        var tbarRunCtrl = me.queryById(me.getId() + "-run");
        if(records.length <= 0)
            tbarRunCtrl.disable();
        else if(records.length == 1)
            tbarRunCtrl.enable();
        else
            tbarRunCtrl.disable();
    },

    onEditButton : function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.module.admin.system.cron.Task", {
            title     : _("Edit anacron task"),
            uuid      : record.get("uuid"),
            listeners : {
                scope  : me,
                submit : function() {
                    this.doReload();
                }
            }
        }).show();
    },

    doDeletion : function(record) {
        var me = this;
        OMV.Rpc.request({
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Anacron",
                method  : "deleteTask",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    },

    onRunButton : function() {
        var me = this;
        var record = me.getSelected();
        Ext.create("OMV.window.Execute", {
            title      : _("Execute anacron task"),
            rpcService : "Anacron",
            rpcMethod  : "doExecute",
            rpcParams  : {
                uuid : record.get("uuid")
            },
            listeners : {
                scope     : me,
                exception : function(wnd, error) {
                    OMV.MessageBox.error(null, error);
                }
            }
        }).show();
    },

    onUploadButton : function() {
        var me = this;
        Ext.create("OMV.window.Upload", {
            title     : _("Upload anacron script"),
            service   : "Anacron",
            method    : "doUpload",
            listeners : {
                scope   : me,
                success : function(wnd, response) {
                    me.doReload();
                }
            }
        }).show();
    }

});

OMV.WorkspaceManager.registerPanel({
    id        : "anacron",
    path      : "/system/cron",
    text      : _("Anacron"),
    position  : 20,
    className : "OMV.module.admin.system.cron.Anacron"
});
