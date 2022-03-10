# What's this for?

I've got a basic Apple II emulator working on Arduino.

(Link to be added at some point).

The trouble is, for anything worthwhile, I want disk emulation. And disks
require accurate time, because Woz was either a genius, or lazy, or crazy (I
think probably the first). So, instead of trying to get CPU cycles counted in
that other simulator, I'm going to try to pare this down to something I can run
on an Arduino device. It seems pretty well optimized, so what the hell, right?
One other potential optimization: I could maybe _compile_ the firmware into
something that could be executed directly. That's kind of crazy, but running ROM
through the interpreter, with labels & stuff, seems not the complicated. I'm
probably underselling it. Oh well...

Nothing trimmed yet: Gotta get it compiling cleanly, then start ripping stuff
out.
