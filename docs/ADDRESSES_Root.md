# Darkspore.exe Addresses
Inside the game executable, there are the functions that make the game work. In order to make the game functional again, we have to understand what those functions are, and what they do. This is a list of the memory addresses that we already discover the purpose, and is also a reference for future findings. We have given provisory names to those functions and variables in order to make them more understandable from a developer perspective.

All the notes taken here consider only the latest official version of the game (5.3.0.127). This document intention is to map the range of functions by library.

sub_401000
RakNet: from sub_A8D1C0, to sub_ADEF90 or superior.
Blaze: from sub_C2A8D0 or prior, to sub_C46B00 or superior.
OpenSSL: from sub_D2ECE0 or prior, to sub_D3AA00 or superior.
ProtoSSL: from sub_E4BC30 or prior, to sub_E58190 or superior.
sub_FC69E0