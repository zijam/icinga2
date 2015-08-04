###API ACTIONS


Action name                            | Parameters                        | Target types
---------------------------------------|-----------------------------------|--------------------------
process-check-result                   | exit_status; plugin_output; check_source; performance_data[]; check_command[]; execution_end; execution_start; schedule_end; schedule_start | Service; Host
reschedule-check                       | {next_check}; {(force_check)} | Service; Host
enable-passive-checks                  | - | Service; Host; ServiceGroup; HostGroup
disable-passive-checks                 | - | Service; Host; ServiceGroup; HostGroup
enable-active-checks                   | - | Host; HostGroup
disable-active-checks                  | - | Host; HostGroup
acknowledge-problem                    | author; comment; {timestamp}; {(sticky)}; {(notify)} | Service; Host
remove-acknowledgement                 | - | Service; Host
add-comment                            | author; comment | Service; Host
remove-comment                         | comment_id | -
remove-all-comments                    | - | Service; Host
enable-notifications                   | - | Service; Host; ServiceGroup; HostGroup
disable-notifications                  | - | Service; Host; ServiceGroup; HostGroup
delay-notifications                    | timestamp | Service;Host
add-downtime                           | start_time; end_time; duration; author; comment; {trigger_id}; {(fixed)} | Service; Host; ServiceGroup; HostGroup
remove-downtime                        | downtime_id | -
