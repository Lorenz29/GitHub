// Number of radio stations 28-11-2021
const int nbrStations = 14;
// Radio Stations - main URL
char *host[nbrStations] = {"icecast.omroep.nl",
                           "icecast.omroep.nl",
                           "icecast.omroep.nl",
                           "icecast.omroep.nl",
                           "icecast.omroep.nl",
                           "21233.live.streamtheworld.com",
                           "22553.live.streamtheworld.com",
                           "icecast-qmusicnl-cdp.triple-it.nl",
                           "22593.live.streamtheworld.com",
                           "20873.live.streamtheworld.com",
                           "25683.live.streamtheworld.com",
                           "22593.live.streamtheworld.com",
                           "rva.ice.infomaniak.ch",
                           "stream.gal.io"};
// Radio Stations - part following main URL
char *path[nbrStations] = {"/radio1-bb-mp3",
                           "/radio2-bb-mp3",
                           "/3fm-bb-mp3",
                           "/radio4-bb-mp3",
                           "/radio5-bb-mp3",
                           "/100PNL_MP3_SC?",
                           "/RADIO538.mp3",
                           "/Qmusic_nl_live_96.mp3",
                           "/RADIO10.mp3",
                           "/VERONICA.mp3",
                           "/SKYRADIO.mp3",
                           "/SLAM_MP3_SC",
                           "/rva-high.mp3",
                           "/arrow"};
// Radio Stations - port number
int   port[nbrStations] = {80,80,80,80,80,80,80,80,80,80,80,80,80,80};
// Radio Stations - info shown on display
char *sname[nbrStations] = {"Radio 1",
                            "Radio 2",
                            "Radio 3",
                            "Radio 4",
                            "Radio 5",
                            "100% NL",
                            "RADIO 538",
                            "QMusic",
                            "Radio 10",
                            "Veronica",
                            "Sky Radio",
                            "Slam",
                            "RVA",
                            "Arrow Classic Rock"};
