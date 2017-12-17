.. _api_tracE_log:

======================
Trace Log
======================

When the trace log feature is enabled on the command line and a script is given snestistics expects the script to be a squirrel script with the following functions:

.. _trace_log_init:

.. c:function:: trace_log_init(replay)

    :param Replay replay: a replay object
    :returns: nothing

This function is used for setup. Breakpoints can be set on the replay objects and global squirrel state can be constructed if the user wants that.

.. trace_log_parameter_printer:

.. c:function:: trace_log_parameter_printer(replay, report)

    :param Replay replay: a replay object
    :param ReportWriter report: a report writer object
    :returns: nothing

This function is called whenever the trace log hits a program counter that it has a breakpoint set for. The trace log system itself will print the name of the function and determine indendation, but this is a chance to do additional printing on some functions that are under investigation.