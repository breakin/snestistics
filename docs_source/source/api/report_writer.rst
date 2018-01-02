.. _api_report_writer:

======================
ReportWriter
======================

These are the operations that can be performed on an instance of the ReportWriter class.

.. _report_print:

.. c:function:: report_writer.print(str)

    :param string str: string to print
    :returns: nothing

Will write the string str to the report at the current indentation level. Adds newline automatically.