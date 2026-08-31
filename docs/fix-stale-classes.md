# Class creation flags between channels

The class buttons in the character-creation window follow the character-class
configuration of the game server/channel currently being entered. Re-entering
the character-selection screen therefore refreshes the flags; classes enabled
on a previous channel are not carried over.

Administrators should configure `CanGetCreated` and `CreationAllowedFlag` for
each channel's character classes. The server sends the unlock packet after each
character-list response, including when the current channel has no unlocked
classes. The latter is important: an empty flag packet clears the client state
from the previous channel.
