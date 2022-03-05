Current Issues:
    message length is extremely susceptible to corruption if the server updates way more frequently
        fixes:
            enforce sending maximum message length, and buffer packets, this leads to latency, because
            we are also trying to enforce that only so many packets are sent a second
    there is no disconnect message so clients remain connected till they timeout and leave corpses
    on timeout, because the game code never removes items from the players object
        fixes:
            implement disconnect message for faster disconnection
            remove players that have disconnected from players map
    TODO: think about encoding

Moving to GUI:
