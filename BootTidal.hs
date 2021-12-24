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
                     oLatency = 0.75,
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
      clok $ track 65 $ n "0*96"

    pItrack = pI "track"

    (!!!) :: [a] -> Int -> a
    (!!!) xs n = xs !! (n `mod` length xs)

    -- alias for channel
    track :: Pattern Int -> Pattern ValueMap -> Pattern ValueMap
    track n p = p # pItrack (n |- 1)
    tr = track

    -- mute a track "n", or a list "[n1, n2]"
    mute :: Pattern Int -> ControlPattern -> ControlPattern
    mute n = fix (struct "") (pItrack (n - 1))

    -- solo a track "n", or a list "[n1, n2]"
    solo :: Pattern Int -> ControlPattern -> ControlPattern
    solo n = unfix (struct "") (pItrack (n - 1))

    -- mute a track within a cycle
    mutew t n = within t (mute n)

    -- solo a track within a cycle
    solow t n = within t (solo n)

    -- alias for fix but for a track
    fixtr t f = fix f (pItrack (t |- 1))

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

    midirange x y = fmap round . range x y

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
    vtune v = rytmcc 17 (v + 64)
    vdetune v = rytmcc 20 (v + 64)
    vwav = rytmcc 21
    vbal = rytmcc 20 0

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

    -- rytm $ tr 1 $ n "0*2" # lfomul "t f" "1k 4"

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
    sound :: Pattern String -> ControlPattern
    sound = rytmcc 15 1 . fmap machineMap
    s = sound
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

    -- aliases for tracks
    bd = track 1
    sd = track 2
    cp = track 4
    bt = track 5
    lt = track 6
    mt = track 7
    ht = track 8
    ch = track 9
    oh = track 10
    cy = track 11
    cb = track 12

    t1 = track 1
    t2 = track 2
    t3 = track 3
    t4 = track 4
    t5 = track 5
    t6 = track 6
    t7 = track 7
    t8 = track 8
    t9 = track 9
    t10 = track 10
    t11 = track 11
    t12 = track 12
:}

:set prompt "tidal-rytm> "
:set prompt-cont ""
