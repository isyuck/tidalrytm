:set -XOverloadedStrings
:set prompt ""

import Sound.Tidal.Context
import Text.Printf

import System.IO (hSetEncoding, stdout, utf8)
hSetEncoding stdout utf8

:{
let target = Target {oName = "tidalrytm",
                     oAddress = "127.0.0.1",
                     oPort = 57120,
                     oLatency = 0.5,
                     oSchedule = Pre MessageStamp,
                     oWindow = Nothing,
                     oHandshake = False,
                     oBusPort = Nothing}

    oscplay = OSC "/rytm" Named {requiredArgs = ["track"]}
    oscmap = [(target, [oscplay])]
:}

stream <- startStream defaultConfig oscmap

:{
let rytm = streamReplace stream 1
    clok = streamReplace stream 2
    hush = streamHush stream
    once = streamOnce stream
    asap = once
    setcps c = do -- sends midi clock on cps change
      asap $ cps c
      clok $ n "0*96" # trackN 65

    trackMap :: String -> Int
    trackMap "bd" = 1
    trackMap "sd" = 2
    trackMap "rs" = 3
    trackMap "cp" = 4
    trackMap "bt" = 5
    trackMap "lt" = 6
    trackMap "mt" = 7
    trackMap "ht" = 8
    trackMap "ch" = 9
    trackMap "oh" = 10
    trackMap "cy" = 11
    trackMap "cb" = 12
    trackMap _ = 30

    pItrack = pI "track"

    trackN :: Pattern Int -> ControlPattern
    trackN n = pItrack (n |- 1)

    track :: Pattern String -> ControlPattern
    track p = trackN (fmap trackMap p)

    t :: Pattern String -> ControlPattern
    t = track

    -- mute a track "t", or a list "[t1, t2]"
    mute :: Pattern String -> ControlPattern -> ControlPattern
    mute t = fix (struct "") (track t)

    -- solo a track "t", or a list "[t1, t2]"
    solo :: Pattern String -> ControlPattern -> ControlPattern
    solo t = unfix (struct "") (track t)

    -- mute a track within a cycle
    mutew s n = within s (mute n)

    -- solo a track within a cycle
    solow s n = within s (solo n)

    -- alias for fix but for a track
    fixtr t f = fix f (track t)

    -- a mix of fixtr and pickF
    fixpick t i fs = fixtr t (pickF i fs)

    -- move the track up/down, by n. trmv 2 will change patterns
    -- on track 5 to track 7, track 1 to track 3... etc
    trmv n = (|+ pItrack n)

    -- helper for making control functions
    rytmcc :: Int -> Int -> Pattern Int -> ControlPattern
    rytmcc cc i = pI (printf "cc%03d" (cc + i - 1))

    -- offset range for envelopes, tunes, etc
    rerange :: Pattern Int -> Pattern Int -> Pattern Int
    rerange n pat = (pat |+ n)

    -- useful for midi remapping, e.g.
    -- irange 0 127 $ slow 2 sine
    irange x y = fmap round . range x y

    -- banks of cc's
    perf = rytmcc 35
    src  = rytmcc 16
    smpl = rytmcc 24
    fltr = rytmcc 70
    amp 7 = rytmcc 11 0
    amp 8 = rytmcc 8 0
    amp n = rytmcc 78 n
    lfo  = rytmcc 102

    -- for controlling the dual osc machine
    vtune v = rytmcc 17 1 (v + 64)
    vdetune v = rytmcc 17 4 (v + 64)
    vwav = rytmcc 15 7
    vbal v = rytmcc 17 3 (v + 64)

    -- amp page
    attack = amp 1
    hold = amp 2
    decay = amp 3
    over = amp 4
    od = over
    verb = amp 6
    pan = rytmcc 11 0 . rerange 64
    gain = rytmcc 8 0

    -- filter page
    fatt = fltr 1
    fhol = fltr 2
    fsus = fltr 3
    frel = fltr 4
    freq = fltr 5
    res = fltr 6
    ftype' = fltr 7
    fenv = fltr 8 . rerange 64

    -- allows patterning of filter type, by string
    ftype :: Pattern String -> ControlPattern
    ftype = ftype' . fmap ftypeMap
      where ftypeMap :: String -> Int
            ftypeMap "lp2" = 0
            ftypeMap "lp1" = 1
            ftypeMap "bp" = 2
            ftypeMap "hp1" = 3
            ftypeMap "hp2" = 4
            ftypeMap "bs" = 5
            ftypeMap "pk" = 6

    smpltune = smpl 1 . rerange 64
    smplfine = smpl 2
    stune = smpltune
    sfine = smplfine
    crush = smpl 3
    bank = smpl 4
    begin = smpl 5
    start = begin
    end = smpl 6

    -- TODO bools for looping
    -- loop :: Pattern Bool -> ControlPattern
    loop = smpl 7
    smplvol = smpl 8

    lfospeed = lfo 1 . rerange 64
    lfomul' = lfo 2
    lfodepth = lfo 8 . rerange 64

    -- easier control of lfo multiplier, takes a pattern of bools (true for mul,
    -- false for div), and a pattern of values (listed below)

    -- n "0*2" # r "bd" # lfomul "t f" "1k 4"

    lfomul :: Pattern Bool -> Pattern String -> ControlPattern
    lfomul b p = lfomul' $ sew b mp (fmap (+ 12) mp)
      where mp = fmap lfomulMap p
            lfomulMap :: String -> Int
            lfomulMap "1" = 0
            lfomulMap "2" = 1
            lfomulMap "4" = 2
            lfomulMap "8" = 3
            lfomulMap "16" = 4
            lfomulMap "32" = 5
            lfomulMap "64" = 6
            lfomulMap "128" = 7
            lfomulMap "256" = 8
            lfomulMap "512" = 9
            lfomulMap "1k" = 10
            lfomulMap "2k" = 11

    -- for accessing the cc banks
    -- as labelled on the rytm
    a = 0
    b = 1
    c = 2
    d = 3
    e = 4
    f = 5
    g = 6
    h = 7

    -- replace tidal's `sound` with one that changes
    -- the machine on the rytm
    machine :: Pattern String -> ControlPattern
    machine = rytmcc 15 1 . fmap machineMap
    m = machine
    machineMap :: String -> Int
    machineMap "bdhard" = 0
    machineMap "bd" = 0
    machineMap "bdclassic" = 1
    machineMap "sdhard" = 2
    machineMap "sd" = 2
    machineMap "sdclassic" = 3
    machineMap "rshard" = 4
    machineMap "rsclassic" = 5
    machineMap "rs" = 5
    machineMap "cp" = 6
    machineMap "bt" = 7
    machineMap "xt" = 7
    machineMap "chclassic" = 9
    machineMap "ch" = 9
    machineMap "ohclassic" = 10
    machineMap "oh" = 10
    machineMap "cyclassic" = 11
    machineMap "cy" = 11
    machineMap "cbclassic" = 12
    machineMap "cb" = 12
    machineMap "bdfm" = 13
    machineMap "sdfm" = 14
    machineMap "noise" = 15
    machineMap "impulse" = 16
    machineMap "chmetal" = 17
    machineMap "ohmetal" = 18
    machineMap "cymetal" = 19
    machineMap "cbmetal" = 20
    machineMap "bdplastic" = 21
    machineMap "bdsilky" = 22
    machineMap "sdnatural" = 23
    machineMap "hh" = 24
    machineMap "cyride" = 25
    machineMap "bdsharp" = 26
    machineMap "disabled" = 27
    machineMap "vco" = 28
    machineMap "dualvco" = 28
    machineMap _ = 0
:}

:set prompt "tidalrytm> "
:set prompt-cont ""
