:set -XOverloadedStrings
:set prompt ""

import Sound.Tidal.Context

import System.IO (hSetEncoding, stdout, utf8)
hSetEncoding stdout utf8

:{
let target = Target {oName = "tidalrytm",
                     oAddress = "127.0.0.1",
                     oPort = 57120,
                     oLatency = 1,
                     oSchedule = Pre MessageStamp,
                     oWindow = Nothing,
                     oHandshake = False,
                     oBusPort = Nothing}

    prestr = "rt"

    pItrack = pI "track"

    track :: Pattern Int -> Pattern ValueMap -> Pattern ValueMap
    track n p = p # pItrack (n |- 1)

    rytmarg :: Int -> Int -> Pattern Double -> ControlPattern
    rytmarg cc i v = pI (prestr ++ show (cc + i - 1)) (fmap round $ v |* 127)

    perf = rytmarg 35
    src  = rytmarg 16
    smpl = rytmarg 24
    fltr = rytmarg 70
    amp 7 = rytmarg 11 0
    amp 8 = rytmarg 8 0
    amp n = rytmarg 78 n
    lfo  = rytmarg 102
    test = rytmarg 0

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
let rytm p = streamReplace stream 1 $ stack p
    hush = streamHush stream
    once = streamOnce stream
    asap = once
    setcps = asap . cps
:}

:set prompt "tidal-rytm> "
:set prompt-cont ""
