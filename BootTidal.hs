:set -XOverloadedStrings
:set prompt ""

import Sound.Tidal.Context

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

    prestr = "rt"

    pItrack = pI "track"

    track :: Pattern Int -> Pattern ValueMap -> Pattern ValueMap
    track n p = p # pItrack (n |- 1)

    rytmcc :: Int -> Int -> Pattern Double -> ControlPattern
    rytmcc cc i v = pI (prestr ++ show (cc + i - 1)) (fmap round $ v |* 127)
    
    rytmccInt :: Int -> Pattern Int -> ControlPattern
    rytmccInt cc v = pI (prestr ++ show cc) v

    perf = rytmcc 35
    src  = rytmcc 16
    smpl = rytmcc 24
    fltr = rytmcc 70
    amp 7 = rytmcc 11 0
    amp 8 = rytmcc 8 0
    amp n = rytmcc 78 n
    lfo  = rytmcc 102


    oscplay = OSC "/rytm" $ ArgList ([
         ("cycle", Just $ VF 0),
         ("cps", Just $ VF 0),
         ("delta", Just $ VF 0),
         ("sec", Just $ VF 0),
         ("usec", Just $ VF 0),
         ("track", Just $ VI 0),
         ("n", Just $ VF 0)
        ]
      ++ (map (\x -> (prestr ++ show x, Just $ VI (-1))) [1..127]))

    oscmap = [(target, [oscplay])]
:}

stream <- startStream defaultConfig oscmap

:{
let rytm = streamReplace stream 1
    hush = streamHush stream
    once = streamOnce stream
    asap = once
    setcps = asap . cps
:}

:set prompt "tidal-rytm> "
:set prompt-cont ""
