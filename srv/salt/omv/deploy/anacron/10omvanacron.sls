{%- set config = salt['omv_conf.get']('conf.service.anacron') %}
{%- for task in config.tasks.task  | selectattr('enable') %}
{%- set end = ">/dev/null 2>&1" %}
{%- if task.sendemail | to_bool %}
{%- set subject = task.identifier %}
{%- if task.comment | length > 0 %}
{%- set subject = [subject, '::', task.comment] | join (' ') | replace('\n', ' ') %}
{%- endif %}
create_anacron_task_{{ task.uuid }}:
  file.accumulated:
    - filename: "/etc/anacrontab"
    - text: "{{ task.period }}\t{{ task.delay }}\t{{ task.identifier }}\t{{ task.command }} | mail -E -s \"Anacron - {{ subject }}\" -a \"From: Anacron Daemon <{{ task.username }}>\" {{ task.username }}{{ end }}" 
    - require_in:
      - file: append_anacron_entries
{%- else %}
create_anacron_task_{{ task.uuid }}:
  file.accumulated:
    - filename: "/etc/anacrontab"
    - text: "{{ task.period }}\t{{ task.delay }}\t{{ task.identifier }}\t{{ task.command }} {{ end }}"
    - require_in:
      - file: append_anacron_entries
{%- endif %}
{%- endfor %}
