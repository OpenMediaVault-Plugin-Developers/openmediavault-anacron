{% set config = salt['omv_conf.get']('conf.service.anacron') %}
{% for task in config.tasks.task  | selectattr('enable') %}
{%- if task.sendemail | to_bool %}
{% set command2 = '| mail -s "Anacron - {{ task.comment }}" -a "From: Anacron Daemon <{{ task.username }}>" {{ task.username }} >/dev/null 2>&1' %}
{%- endif %}
{{ task.period }}{{'\t'}}{{ task.delay }}{{'\t'}}{{ task.identifier }}{{'\t'}}{{ task.command }}{{ command2 }}
{% endfor %}
