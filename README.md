tidalrytm enables control of the analog rytm mkii from tidalcycles, directly over midi.

this is a personal tool, but i thought i'd share it here if it's of any use to anyone `:-)`. it (mostly) works, but needs some optimisation and timing improvements.

i made this because of the way tidal and superdirt currently handle midi control change messages, only allowing one per message.

right now, (i think) you can't treat midi cc in the same way control functions are treated, e.g something like:

`fltr5 = # ccn 32 # ccv`
`amp2 = # ccn 10 # ccv`

`d1 $ every 2 (# fltr5 0.8) $ n "0(3,8)" # fltr5 0.2 # amp2 0.8`

is not possbile, because `fltr5` and `amp2` are both aliases for `ccn`.

tidalrytm fixes this, and adds some other useful functions for controlling the rytm easily. examples in `example.tidal`.

it doesn't rely on superdirt or supercollider at all, so all you need is a working tidal install and a rytm.

this is a small cpp project, requiring `oscpack` and `rtmidi`. build with `make`, run with `make run`. make sure to use the included `BootTidal.hs` rather than whatever your editors default is, and to set the tempo of the rytm to the tempo of tidal. e.g. `setcps(120/60/4)` for 120 bpm on the rytm.
