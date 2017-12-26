---
layout: post
date: 2017-12-28
title: Snestistics Tutorial - Annotations
tags: snes, snestistics
theme: snestistics
landing: drafts
---
Introduction
============
So we have our assembler source. Now starts a cycle of reading assembler, understanding _something_ (small or big) and then to annotate the assembler source so we don't have to remember it all. In snestistics we have a file called a labels file where annotations can be added. Here is a function from the raw assembler source with no annotations:

~~~~~~~~~~
Function A, no annotations
~~~~~~~~~~

We hand-craft the following annotations

~~~~~~~~~~
function ... TODO
~~~~~~~~~~

and regenerate the assembler source. The resulting function in the assembler source now looks like this:

~~~~~~~~~~
Function B, no annotations
~~~~~~~~~~

So that looks nicer! Are there any other benefits to adding annotations? I am glad you asked!

Jumps to annotated function
---------------------------
With name and use-comment.

Indirect jumps
---------------
Make sure at least one is annotated and has use-comment.

Functions
=========
Our current concept is a function. We realize that since the games are not written in a high-level language that have consistent calling conventions this might not be a good fir for all games. This is an area where we are constantly trying to come up with new concepts for and each new game will probably push us to improve here.

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

Closing words
=============
Working iteratively with labels is extremely powerful. The auto-annotator is very fun to play around with and when it works it saved so much work.


DO NOT SHOW DATA ANNOTATIONS, LET PRETEND THEY DON'T EXIST FOR NOW. WE NEED TO RETHINK THEM.
