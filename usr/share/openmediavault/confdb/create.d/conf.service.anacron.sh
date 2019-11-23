#!/bin/sh

set -e

. /usr/share/openmediavault/scripts/helper-functions

if ! omv_config_exists "/config/services/anacron"; then
    omv_config_add_node "/config/services" "anacron"
    omv_config_add_node "/config/services/anacron" "tasks"
fi

exit 0
