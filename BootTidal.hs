:set -XOverloadedStrings
:set prompt ""

import Sound.Tidal.Context
import Text.Printf
import Data.List
import Data.Maybe
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
    fx   = streamReplace stream 2 . (# trackN 13)
    clok = streamReplace stream 3
    hush = streamHush stream
    once = streamOnce stream
    asap = once
    setcps c = do -- sends midi clock on cps change
      asap $ cps c
      clok $ n "0*96" # trackN 65

    patFindIndex :: Int -> [String] -> Pattern String -> Pattern Int
    patFindIndex d ss = fmap (\s -> fromMaybe d $ findIndex (== s) ss)

    clamp :: (Ord a) => a -> a -> a -> a
    clamp mn mx = max mn . min mx

    -- bd = 0, sd = 1, .. cb = 11
    trackMap :: Pattern String -> Pattern Int
    trackMap = (+ 1) . patFindIndex 0 ["bd", "sd", "rs", "cp", "bt", "lt", "mt", "ht", "ch", "oh", "cy", "cb"]

    pItrack = pI "track"

    trackN :: Pattern Int -> ControlPattern
    trackN n = pItrack (n |- 1)

    track :: Pattern String -> ControlPattern
    track = trackN . trackMap

    t :: Pattern String -> ControlPattern
    t = track

    -- note change as a rytm cc
    ncc :: Pattern Note -> ControlPattern
    ncc = rytmcc 0 4 . fmap (round . unNote) . (+ 60)

    -- rytm mapped note
    note :: Pattern Note -> ControlPattern
    note = pN "n" . clamp 12 59 . (+ 36)
    n = note

    -- generic midinote
    midinote :: Pattern Note -> ControlPattern
    midinote = pN "n"

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

    -- useful for midi remapping, e.g.
    -- irange 0 127 $ slow 2 sine
    irange x y = fmap round . range x y

    -- banks of cc's
    perf  = rytmcc 35
    src   = rytmcc 16
    smpl  = rytmcc 24
    fltr  = rytmcc 70
    amp 7 = rytmcc 11 0
    amp 8 = rytmcc 8 0
    amp n = rytmcc 78 n
    lfo   = rytmcc 102

    -- for controlling the dual osc machine
    vtune   = rytmcc 17 1 . (+ 64)
    vdetune = rytmcc 17 4 . (+ 64)
    vwav    = rytmcc 15 7
    vbal    = rytmcc 17 3 . (+ 64)

    -- amp page
    attack  = amp 1
    hold    = amp 2
    decay   = amp 3
    over    = amp 4
    od      = over
    verb    = amp 6
    pan     = rytmcc 11 0 . (+ 64)
    gain    = rytmcc 8 0

    -- filter page
    fatt = fltr 1
    fhol = fltr 2
    fsus = fltr 3
    frel = fltr 4
    freq = fltr 5
    res  = fltr 6
    ftype = fltr 7 . patFindIndex 0 ["lp2", "lp1", "bp", "hp1", "hp2", "bs", "pk"]
    fenv  = fltr 8 . (+ 64)

    -- sample page
    smpltune = smpl 1 . (+ 64)
    smplfine = smpl 2
    crush    = smpl 3
    bank     = smpl 4
    start    = smpl 5
    end      = smpl 6
    -- TODO bools for looping
    loop     = smpl 7
    smplvol  = smpl 8

    -- lfo page
    lfospeed = lfo 1 . (+ 64)
    lfomul'  = lfo 2
    lfodepth = lfo 8 . (+ 64)

    -- easier control of lfo multiplier, takes a pattern of bools (true for mul,
    -- false for div), and a pattern of values (listed below)

    -- n "0*2" # r "bd" # lfomul "t f" "1k 4"
    lfomul :: Pattern Bool -> Pattern String -> ControlPattern
    lfomul b p = lfomul' $ sew b mp (fmap (+ 12) mp)
      where mp = patFindIndex 0 ["1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1k", "2k"] p

    fxdelay = rytmcc 16
    delayt = fxdelay 1
    delaypp :: Pattern Bool -> ControlPattern
    delaypp  = fxdelay 2 . fmap fromEnum
    delayw   = fxdelay 3 . (+ 64)
    delayfb  = fxdelay 4 . fmap (round . (* 0.63))
    delayhpf = fxdelay 5
    delaylpf = fxdelay 6
    delayrev = fxdelay 7
    delayvol = fxdelay 8

    fxverb   = rytmcc 25
    verbpre  = fxverb 0
    verbdec  = fxverb 1
    verbfrq  = fxverb 2
    verbgain = fxverb 3
    verbhpf  = fxverb 4
    verblpf  = fxverb 5
    verbvol  = fxverb 6

    ppMap "pre"  = 0
    ppMap "post" = 1
    ppMap _ = 0

    fxdist    = rytmcc 71
    distort   = fxdist 0
    symmetry  = fxdist 1 . (+ 64)
    delayover = fxdist 2
    delaydist = fxdist 6 . fmap ppMap
    verbdist  = fxdist 7 . fmap ppMap

    ratioMap "1:2" = 0
    ratioMap "1:4" = 1
    ratioMap "1:8" = 2
    ratioMap "max" = 3
    ratioMap _ = 0
    seqMap "off" = 0
    seqMap "lpf" = 1
    seqMap "hpf" = 2
    seqMap "hit" = 3
    seqMap _ = 0
    fxcomp = rytmcc 78
    threshold = fxcomp 1
    -- TODO better mapping for attack & decay
    compattack = fxcomp 2
    compdecay  = fxcomp 3
    makeup     = fxcomp 4
    ratio :: Pattern String -> ControlPattern
    ratio      = fxcomp 5 . fmap ratioMap
    sidechan   = fxcomp 6 . fmap seqMap
    compmix    = fxcomp 7
    compvol    = fxcomp 8

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
