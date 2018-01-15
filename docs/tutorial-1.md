---
title: Tutorial 1 - Introduction
layout: default
---
In order to do the [Super Famicon Wars (SFW) translation](http://www.romhacking.net/translations/3354) we had to start by understanding how to game worked. Once this was done an equally hard task was to change the game. But understanding came first. In order to understand we needed to reverse engineer! The tool [snestistics](https://github.com/breakin/snestistics) is what we used to solve this task. In order to complete it we had to add a lot of new features but it was worth it! [Here](about) is a detailed history on snestistics with discussion on why it works like it does. In this tutorial series we will simply show what you can do with snestistics and not compare it to other tools and approaches. When we started SFW snestistics was not very mature at all. A lot of tedious tasks we had to do by hand and writing a straightforward tutorial like this would not be possible!

We assume that if you actually want to understand the code presented here in depth you will know some 65816 assembly and have a high-level understanding of the SNES.

The Subject
===========
The game we are going to take a closer look at is the japanese game [Battle Pinball](https://www.youtube.com/watch?v=VKIM2FrK2zY), spin-off game in the [Compati Hero Series](https://en.wikipedia.org/wiki/Compati_Hero_Series).

While not sure we will actually attain anything specific with this game, two possible ideas is to add SaveRAM support for the high score list and translate the pretty minimal amount of Japanese text on display to English. As you can see the intro and the level selector is in Japanese while in-game text is a mix of English and Japanese. When we were doing SFW we worked in secret mode until we *knew* we could do it. This time we just start pulling on some threads in public to see where we end up. No promises!

Interlude
=========
While we own a physical copy of the game and have dumped it ourselves we are not allowed to share the ROM and also not allowed to share the source code for the ROM even though we've dissassemblied it myself. Thus you, the reader, will only see glimps and for the full picture (text really!) you will need to run snestistics on a ROM you've obtained yourself. Furthermore snestistics is constantly being updated/improved so the snippets shown here might not be fully accurate. We will try to update this tutorial series when snestistics is changed so that it is current.

