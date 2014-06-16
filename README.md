HoMM-Map-Filter
===============
This is a tool for filtering Heroes of Might and Magic III maps by following criteria:<br>
1. Minimal difficulty<br>
2. Map size<br>
3. Player count<br>
4. Has dungeon?<br>
5. Is allied?<br>
6. Additional applies: has rumors, timed events, local events, quest guards, grail, dragon utopias, dragon dwellings <br>.

It was designed to fulfill one purpose: filter out all "interersting" maps or me and my gf, for a hot-seat play. "Interesting" aspect is purely subjective, based on availability of rumors, events, timed events, quest guards and obelisks.<br>
Currently, only SoD version is supported, because the map format is more or less consistent within this release. All previous map versions (RoE and AB) are a pure mess, and adding this case handling would be a waste of time (you're welcome, though :)).

Usage
===============
Take the executable from *Release.zip*, along with zlib.dll and place in maps directry of your game. Also, make a *non_matched* empty directory. 
Run the app and input the criteria.
If app crashes, run with verbose mode on (sometimes the maps are corrupted and they crash the app). To test, if the map is corrupted, try to either start a game with it, OR try to unzip it (using 7z or WinRar) -> if it fails, then in is corrupted and you should remove it anyways.

TO-DO
===============
1. Add support for previous versions<br>
2. Make allied check more sophisticated. Currently, if maps has team setting and canBeHuman >=2, then the map is considered allied, altough it can give false-positives. Proper way would be to do a cross-check of teams, so that we have minimum of 1 team, consisting minimum out of 2 canBeHuman players.<br>

Compiling
===============
Make sure you have zlib binaries and adjust paths to it in project settings.


Disclaimer
===============
I am not responsible for anything that might happen to you or your map collection during the execution of this app :)

License
===============
Do what the fuck you want with it :) Just give a credit to this repo or me (artemonster).

Contact
===============
If you want to suggest some feature, send the request at artemonstero (at) gmail com
