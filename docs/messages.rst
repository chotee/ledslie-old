===============
Message formats
===============

Information types
-----------------

* Monotonic events, like next minute starting. Event countdowns.

* Repeating Information like Scheduled events, rocket launches, news headlines.

* Interactive things, such as a spacealert.

What does the layout need to know
---------------------------------

* Who does the selecting of what to show? Is that the layout? What kind of hints does it need?

* What kind of data is it?

    * Time -- Clock informs of time.

    * Countdown -- Reverse clock. Something happens in X time (X can be minutes, days, etc).

    * Info -- Lists of items, show the last N as a batch.

    * Events -- Something that needs to be shown at that moment. What, who, the message. Maybe should take over the
      whole screen.

Format
------

Extendable, JSON based
