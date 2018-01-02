Replay
======
These are the operations that can be performed on an instance of the Replay class.

.. _replay_set_breakpoint:

.. c:function:: replay.set_breakpoint(pc)

    :param integer pc: the program counter to set a break point at
    :returns: nothing

.. _replay_set_breakpoint_range:

.. c:function:: replay.set_breakpoint_range(pc_start, pc_end)

    :param integer pc_start: the first program counter to set a break point at
    :param integer pc_end: the last program counter to set a break point at
    :returns: nothing

.. _replay_read_byte:

.. c:function:: replay.read_byte(address)

    :param integer address: 24-bit address specifying where to read a byte (8-bit)
    :returns: nothing

.. _replay_read_word:

.. c:function:: replay.read_word(address)

    :param integer address: 24-bit address specifying where to read a word (16-bit)
    :returns: nothing

.. _replay_read_long:

.. c:function:: replay.read_word(address)

    :param integer address: 24-bit address specifying where to read a long (24-bit)
    :returns: nothing

.. _replay_pc:

.. c:function:: replay.pc()

    :returns: current program counter

.. _replay_a:

.. c:function:: replay.a()

    :returns: current value of register a (16-bit)

.. _replay_al:

.. c:function:: replay.al()

    :returns: current low byte of register a (8-bit)

.. _replay_ah:

.. c:function:: replay.ah()

    :returns: current high byte of register a (8-bit)

.. _replay_x:

.. c:function:: replay.x()

    :returns: current value of register x (16-bit)

.. _replay_xl:

.. c:function:: replay.xl()

    :returns: current low byte of register x (8-bit)

.. _replay_xh:

.. c:function:: replay.xh()

    :returns: current high byte of register x (8-bit)

.. _replay_y:

.. c:function:: replay.y()

    :returns: current value of register y (16-bit)

.. _replay_yl:

.. c:function:: replay.yl()

    :returns: current low byte of register y (8-bit)

.. _replay_yh:

.. c:function:: replay.yh()

    :returns: current high byte of register y (8-bit)

.. _replay_p:

.. c:function:: replay.p()

    :returns: current value of status register (16-bit)

.. _replay_s:

.. c:function:: replay.s()

    :returns: current value of stack register (16-bit)

.. _replay_dp:

.. c:function:: replay.dp()

    :returns: current value of direct page register (16-bit)

.. _replay_db:

.. c:function:: replay.db()

    :returns: current value of data bank register (8-bit)
