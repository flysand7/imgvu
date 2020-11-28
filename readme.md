# imgvu

## about
imgvu is a fast reliable open-source cross-platform image viewer written by bumbread. This program is aimed to be fast in all aspects: both performance and the ease of user interface are considered. One of the key features is that there is _no need to use your mouse_ while using this program.

This is a small educational project which I had started for couple reasons.
* I need practice programming and exploring new things, which I can achieve through learning different image formats and ways to parse them. In this way I will be aware of more different compression methods and other algorithms used to store an image and how to implement them. Also by keeping a relatively large project i'm becoming more experienced in things like project management, and how to write good maintainable code.
* I'm fucking tired of windows' image viewer showing black screen for several seconds on start and then turns out that you actually need to restart it, and then it loads faster, but still several seconds.

## general clarification
* *fast* means that a user doesn't get noticeable delay when using the program. There is a particular bug that happens with opengl context loading, which doesn't allow my program to be loaded faster than in 1.5 seconds. Note that you may have that bug as well.
* *reliable* means that if a certain kind of unexpected input is met, the program will be able to handle it smoothly. No crashes. No weird error messages. Sheesh, I'm not even going to _have_ unexpected inputs, I'm making error-free, bug-free program. All bugs get fixed right away. Note, that if you get a bug don't hesitate to report. Post a brief description of the bug and information that might be useful for reproducing/detecting the bug on GitHub issues.
* *cross platform* means that this program is _ready to be ported_ to any other platform if someone writes a platform layer for that platform. I'm always ready to pull platform layer code from any repository (if it supports minimal functionality the program requires).

## supported formats
* [x] ppm, pgm, pbm (both binary and ascii modes)
* [x] bmp (only 24 bit uncompressed RGB data)
* [ ] tga
* [ ] gif
* [ ] jpeg
* [ ] png
* [ ] jpeg2000
* [ ] openexr
* [ ] heif
* [ ] avif
* [ ] flif
* [ ] bpg

## features/key points
* immediate in response
* ~~automatically detects changes in your folder and updates live~~
* tries to minimize its work by keeping a small cache.
* made with love <3

## about the author
* contact email: thebumboni@gmail.com (i'm checking it once half a year)
* discord: bumbread#1977 (i'm checking this literally every day?)