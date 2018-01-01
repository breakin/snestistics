Auto-annotations
================
There are many small functions that does _something_. Often we don't need to know what they all do, but some of the tools in snestistics such as the trace log wants all program counters that are executed during a trace log session to be in annotated functions. When we did the SFW translation Anders did 1000s of functions manually (mostly since he wanted to help David out and he was procastrinating on the next feature for snestistics). Then one day he said enough and wrote an auto-detector that worked really well for SFW. It found an additional 500 functions and then there was no more manual work to be done.

We've tried it on other games with worse results and this is a feature I imagine will be rewritten a lot. There are two modes in which it can be run. In one mode the auto-labels file is "special". If it is missing on disk it will be re-generated automatically. In the other mode the auto-labels file is simply supplied as a regular labels file once created. This enabled manual editing to fix up "broken" functions.

~~~~~~~
SHOW COMMAND LINES FOR GENERATION AND FOR USAGE (BOTH MODES)
~~~~~~~

See [manual](COMMAND LINE DOCS) for more command line options.

Lets see how it fares on .Battle Pinball_.

TODO:
1. Test the different modes. See how many functions they each get. Add more options.
2. Find buggy cases and show them.
