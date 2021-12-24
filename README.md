`tidalrytm` extends the midi capability of tidal, specifically for the analog rytm mkii.

it depends on `oscpack` and `RtMidi`

build with `make`

run by starting tidal with the `BootTidal.hs` provided, and then `./tidalrytm`


from `example.tidal`:

```haskell
setcps(130/60/4) -- also starts midiclock send to the rytm

-- `track` (or `t`) replaces tidal's `sound`

rytm $ track "bd ch"

rytm $ t "ch*16" # n "<0 4 8 12>"

-- new functions for the rytm (default tidal only allows 1 midi CC per msg)

rytm $ t "bt*8" # freq "32 64 78 127" # res 40

-- change machine inside patterns

rytm $ stack [
  t "bt*8" # freq "32 64 78 127" # res 40
  ,
  t "ch*16" # machine "chmetal ch"
             ]

-- conditional functions on a stack of tracks

rytm
  $ every 4 (solo "ch")
  $ sometimes (fixtr "bt" (|+ n 8))
  $ stack [
  t "bt*8" # freq "32 64 78 127" # res 40
  ,
  t "ch*16" # m "chmetal ch"
  ,
  t "oh*4"
             ]

-- switch from lp1 to hp1 filter type for all tracks
-- sweep filter freq for all tracks

rytm
  $ (# ftype "lp1 hp1")
  $ (# freq (irange 30 127 $ slow 2 sine))
  $ stack [
  t "bt*8" # freq "32 64 78 127" # res 40 # fenv "<-30 30>"
  ,
  t "ch*16" # m "chmetal ch"
  ,
  t "oh*4"
  ,
  ("e" ~>) $ t "cp"
             ]

-- more functions in `BootTidal.hs`

hush
```
